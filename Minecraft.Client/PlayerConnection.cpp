#include "stdafx.h"
#include "PlayerConnection.h"
#include "ServerPlayer.h"
#include "ServerLevel.h"
#include "ServerPlayerGameMode.h"
#include "PlayerList.h"
#include "MinecraftServer.h"
#include "../Minecraft.World/net.minecraft.commands.h"
#include "../Minecraft.World/net.minecraft.network.h"
#include "../Minecraft.World/net.minecraft.world.entity.item.h"
#include "../Minecraft.World/net.minecraft.world.level.h"
#include "../Minecraft.World/net.minecraft.world.level.dimension.h"
#include "../Minecraft.World/net.minecraft.world.item.h"
#include "../Minecraft.World/net.minecraft.world.item.trading.h"
#include "../Minecraft.World/net.minecraft.world.inventory.h"
#include "../Minecraft.World/net.minecraft.world.level.tile.entity.h"
#include "../Minecraft.World/net.minecraft.world.level.saveddata.h"
#include "../Minecraft.World/net.minecraft.world.entity.animal.h"
#include "../Minecraft.World/net.minecraft.network.h"
#include "../Minecraft.World/net.minecraft.world.food.h"
#include "../Minecraft.World/AABB.h"
#include "../Minecraft.World/Pos.h"
#include "../Minecraft.World/SharedConstants.h"
#include "../Minecraft.World/ChatPacket.h"
#include "../Minecraft.World/StringHelpers.h"
#include "../Minecraft.World/Socket.h"
#include "../Minecraft.World/Achievements.h"
#include "../Minecraft.World/net.minecraft.h"
#include "../Minecraft.World/LevelData.h"
#include "../Minecraft.World/Pos.h"
#include "../Minecraft.World/Achievements.h"
#include "EntityTracker.h"
#include "ServerConnection.h"
#include "../Minecraft.World/GenericStats.h"
#include "../Minecraft.World/JavaMath.h"

#include "..\Minecraft.World\ListTag.h"
// 4J Added
#include "../Minecraft.World/net.minecraft.world.item.crafting.h"
#include "Options.h"
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
#include "../Minecraft.Server/ServerLogManager.h"
#include "../Minecraft.Server/Access/Access.h"
#include "../Minecraft.Server/Security/IdentityTokenManager.h"
#include "../Minecraft.Server/Security/SecurityConfig.h"
#include "../Minecraft.Server/Security/ConnectionCipher.h"
#include "../Minecraft.Server/FourKitBridge.h"
extern bool g_Win64DedicatedServer;
#endif

namespace
{
	// Anti-cheat thresholds. Keep server-side checks authoritative even in host mode.
	// Base max squared displacement allowed per move packet before speed flags trigger.
	const double kMoveBaseAllowanceSq = 100.0;
	// Extra squared displacement allowance derived from current server-side velocity.
	const double kMoveVelocityAllowanceScale = 100.0;
	// Max squared distance for interact/attack when the target is visible (normal reach).
	const double kInteractReachSq = 6.0 * 6.0;
	// Stricter max squared distance used when LOS is blocked to reduce wall-hit abuse.
	const double kInteractBlockedReachSq = 3.0 * 3.0;
}

Random PlayerConnection::random;


PlayerConnection::PlayerConnection(MinecraftServer *server, Connection *connection, shared_ptr<ServerPlayer> player)
{
	// 4J - added initialisers
	done = false;
	tickCount = 0;
	aboveGroundTickCount = 0;
	xLastOk = yLastOk = zLastOk = 0;
	synched = true;
	hasDoneFirstTickFourKit = false;
	didTick = false;
	lastKeepAliveId = 0;
	lastKeepAliveTime = 0;
	lastKeepAliveTick = 0;
	chatSpamTickCount = 0;
	dropSpamTickCount = 0;

	this->server = server;
	this->connection = connection;
	connection->setListener(this);
	this->player = player;
	//	player->connection = this;		// 4J - moved out as we can't assign in a ctor
	InitializeCriticalSection(&done_cs);

	m_bCloseOnTick = false;
	m_bWasKicked = false;

	m_friendsOnlyUGC = false;
	m_offlineXUID = INVALID_XUID;
	m_onlineXUID = INVALID_XUID;
	m_bHasClientTickedOnce = false;
	m_logSmallId = 0;
	m_identityVerified = false;
	m_identityChallengeTick = -1;
	m_securityGateOpen = true; // default open; closed when cipher is required

	// Cache the first valid transport smallId because disconnect teardown can clear it before the server logger runs.
	if (this->connection != NULL && this->connection->getSocket() != NULL)
	{
		m_logSmallId = this->connection->getSocket()->getSmallId();
	}

	setShowOnMaps(app.GetGameHostOption(eGameHostOption_Gamertags)!=0?true:false);
}

PlayerConnection::~PlayerConnection()
{
	delete connection;
	DeleteCriticalSection(&done_cs);
}

unsigned char PlayerConnection::getLogSmallId()
{
	// Fall back to the live socket only while the cached value is still empty.
	if (m_logSmallId == 0 && connection != NULL && connection->getSocket() != NULL)
	{
		m_logSmallId = connection->getSocket()->getSmallId();
	}

	return m_logSmallId;
}

void PlayerConnection::tick()
{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
    if (!hasDoneFirstTickFourKit)
    {
		FourKitBridge::FirePlayerJoin(player->entityId, player->name, player->getUUID(), (unsigned long long)player->getOnlineXuid(), (unsigned long long)player->getXuid());
        hasDoneFirstTickFourKit = true;
    }
#endif

	if( done ) return;

	if( m_bCloseOnTick )
	{
		disconnect( DisconnectPacket::eDisconnect_Closed );
		return;
	}

	didTick = false;
	tickCount++;
	connection->tick();
	if(done) return;

	if ((tickCount - lastKeepAliveTick) > 20 * 1)
	{
		lastKeepAliveTick = tickCount;
		lastKeepAliveTime = System::nanoTime() / 1000000;
		lastKeepAliveId = random.nextInt();
		send(std::make_shared<KeepAlivePacket>(lastKeepAliveId));
	}

	if (chatSpamTickCount > 0)
	{
		chatSpamTickCount--;
	}
	if (dropSpamTickCount > 0)
	{
		dropSpamTickCount--;
	}

	// Ensure server-side player tick runs even when no move packet was received this tick.
	// Without this, environmental damage (drowning, fire, lava) is never applied to clients
	// that don't send frequent move packets.
	if (!didTick && player != nullptr)
	{
		player->doTick(false);
	}
}

void PlayerConnection::disconnect(DisconnectPacket::eDisconnectReason reason)
{
	EnterCriticalSection(&done_cs);
	if( done )
	{
		LeaveCriticalSection(&done_cs);
		return;
	}

	std::wstring kickLeaveMessage;
	bool fourKitHandledQuit = false;
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	if (reason != DisconnectPacket::eDisconnect_Closed &&
		reason != DisconnectPacket::eDisconnect_ConnectionCreationFailed &&
		reason != DisconnectPacket::eDisconnect_Quitting)
	{
		if (FourKitBridge::FirePlayerKick(player->entityId, (int)reason, kickLeaveMessage))
		{
			m_bWasKicked.store(false);
			LeaveCriticalSection(&done_cs);
			return;
		}
	}

	ServerRuntime::ServerLogManager::OnPlayerDisconnected(
		getLogSmallId(),
		(player != NULL) ? player->name : std::wstring(),
		reason,
		true);
	fourKitHandledQuit = FourKitBridge::FirePlayerQuit(player->entityId);
#endif
	app.DebugPrintf("PlayerConnection disconect reason: %d\n", reason );
	player->disconnect();

	// Mark done and release the lock BEFORE the heavy PlayerList work.
	// The actual removal, broadcast, and socket teardown are queued for
	// the next tick, which processes them without holding done_cs.
	done = true;
	LeaveCriticalSection(&done_cs);

	server->getPlayers()->queueDisconnect(player, static_cast<int>(reason),
		kickLeaveMessage, getWasKicked(), fourKitHandledQuit);
}

void PlayerConnection::handlePlayerInput(shared_ptr<PlayerInputPacket> packet)
{
	player->setPlayerInput(packet->getXxa(), packet->getYya(), packet->isJumping(), packet->isSneaking());
}

void PlayerConnection::handleMovePlayer(shared_ptr<MovePlayerPacket> packet)
{
	ServerLevel *level = server->getLevel(player->dimension);

	didTick = true;
	if(synched) m_bHasClientTickedOnce = true;

	if (player->wonGame) return;

	if (!synched)
	{
		double yDiff = packet->y - yLastOk;
		if (packet->x == xLastOk && yDiff * yDiff < 0.01 && packet->z == zLastOk)
		{
			synched = true;
		}
	}

	if (synched)
	{
		if (player->riding != nullptr)
		{

			float yRotT = player->yRot;
			float xRotT = player->xRot;
			player->riding->positionRider();
			double xt = player->x;
			double yt = player->y;
			double zt = player->z;

			if (packet->hasRot)
			{
				yRotT = packet->yRot;
				xRotT = packet->xRot;
			}

			player->onGround = packet->onGround;

			player->doTick(false);
			player->ySlideOffset = 0;
			player->absMoveTo(xt, yt, zt, yRotT, xRotT);
			if (player->riding != nullptr) player->riding->positionRider();
			server->getPlayers()->move(player);

			// player may have been kicked off the mount during the tick, so
			// only copy valid coordinates if the player still is "synched"
			if (synched) {
				xLastOk = player->x;
				yLastOk = player->y;
				zLastOk = player->z;
			}
			static_cast<Level *>(level)->tick(player);

			return;
		}

		if (player->isSleeping())
		{
			player->doTick(false);
			player->absMoveTo(xLastOk, yLastOk, zLastOk, player->yRot, player->xRot);
			static_cast<Level *>(level)->tick(player);
			return;
		}

		double startY = player->y;
		xLastOk = player->x;
		yLastOk = player->y;
		zLastOk = player->z;


		double xt = player->x;
		double yt = player->y;
		double zt = player->z;

		float yRotT = player->yRot;
		float xRotT = player->xRot;
		const float yRotOld = yRotT;
		const float xRotOld = xRotT;

		if (packet->hasPos && packet->y == -999 && packet->yView == -999)
		{
			packet->hasPos = false;
		}

		if (packet->hasPos)
		{
			xt = packet->x;
			yt = packet->y;
			zt = packet->z;
			double yd = packet->yView - packet->y;
			if (!player->isSleeping() && (yd > 1.65 || yd < 0.1))
			{
				disconnect(DisconnectPacket::eDisconnect_IllegalStance);
				//                logger.warning(player->name + " had an illegal stance: " + yd);
				return;
			}
			if (abs(packet->x) > 32000000 || abs(packet->z) > 32000000)
			{
				disconnect(DisconnectPacket::eDisconnect_IllegalPosition);
				return;
			}
		}
		if (packet->hasRot)
		{
			yRotT = packet->yRot;
			xRotT = packet->xRot;
		}

		// 4J Stu Added to stop server player y pos being different than client when flying
		if(player->abilities.mayfly || player->isAllowedToFly() )
		{
			player->abilities.flying = packet->isFlying;
		}
		else player->abilities.flying = false;

		player->doTick(false);
		player->ySlideOffset = 0;
		player->absMoveTo(xLastOk, yLastOk, zLastOk, yRotT, xRotT);

		if (!synched) return;

		double xDist = xt - player->x;
		double yDist = yt - player->y;
		double zDist = zt - player->z;

		double dist = xDist * xDist + yDist * yDist + zDist * zDist;

		// Anti-cheat: reject movement packets that exceed server-authoritative bounds.
		double velocitySq = player->xd * player->xd + player->yd * player->yd + player->zd * player->zd;
		double maxAllowedSq = kMoveBaseAllowanceSq + (velocitySq * kMoveVelocityAllowanceScale);
		if (player->isAllowedToFly() || player->gameMode->isCreative())
		{
			// Creative / flight-allowed players can move farther legitimately per tick.
			maxAllowedSq *= 1.5;
		}
		if (dist > maxAllowedSq)
		{
			disconnect(DisconnectPacket::eDisconnect_MovedTooQuickly);
			return;
		}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		if (xLastOk != xt || yLastOk != yt || zLastOk != zt || yRotT != yRotOld || xRotT != xRotOld)
		{
			double moveToX, moveToY, moveToZ;
			bool cancelled = FourKitBridge::FirePlayerMove(player->entityId,
				xLastOk, yLastOk, zLastOk,
				xt, yt, zt,
				&moveToX, &moveToY, &moveToZ);
			if (cancelled)
			{
				teleport(xLastOk, yLastOk, zLastOk, yRotT, xRotT);
				return;
			}
			if (moveToX != xt || moveToY != yt || moveToZ != zt)
			{
				xt = moveToX;
				yt = moveToY;
				zt = moveToZ;
				xDist = xt - player->x;
				yDist = yt - player->y;
				zDist = zt - player->z;
			}
		}
#endif

		float r = 1 / 16.0f;
		if (player->bb == nullptr) return;
		bool oldOk = level->getCubes(player, player->bb->copy()->shrink(r, r, r))->empty();

		if (player->onGround && !packet->onGround && yDist > 0)
		{
			// assume the player made a jump
			player->causeFoodExhaustion(FoodConstants::EXHAUSTION_JUMP);
		}

		player->move(xDist, yDist, zDist);

		// 4J Stu - It is possible that we are no longer synched (eg By moving into an End Portal), so we should stop any further movement based on this packet
		// Fix for #87764 - Code: Gameplay: Host cannot move and experiences End World Chunks flickering, while in Splitscreen Mode
		// and Fix for #87788 - Code: Gameplay: Client cannot move and experiences End World Chunks flickering, while in Splitscreen Mode
		if (!synched) return;

		player->onGround = packet->onGround;
		// Since server players don't call travel we check food exhaustion
		// here
		player->checkMovementStatistiscs(xDist, yDist, zDist);

		double oyDist = yDist;

		xDist = xt - player->x;
		yDist = yt - player->y;

		// Clamp tiny Y drift noise to reduce false positives.
		if (yDist > -0.5 && yDist < 0.5)
		{
			yDist = 0;
		}
		zDist = zt - player->z;
		dist = xDist * xDist + yDist * yDist + zDist * zDist;
		bool fail = false;
		if (dist > 0.25 * 0.25 && !player->isSleeping() && !player->gameMode->isCreative() && !player->isAllowedToFly())
		{
			fail = true;
			//            logger.warning(player->name + " moved wrongly!");
			//            System.out.println("Got position " + xt + ", " + yt + ", " + zt);
			//            System.out.println("Expected " + player->x + ", " + player->y + ", " + player->z);
#ifndef _CONTENT_PACKAGE
			wprintf(L"%ls moved wrongly!\n",player->name.c_str());
			app.DebugPrintf("Got position %f, %f, %f\n", xt,yt,zt);
			app.DebugPrintf("Expected %f, %f, %f\n", player->x, player->y, player->z);
#endif
		}
		player->absMoveTo(xt, yt, zt, yRotT, xRotT);

		AABB *playerBB = player->bb;
		if (playerBB == nullptr)
		{
			teleport(xLastOk, yLastOk, zLastOk, yRotT, xRotT);
			return;
		}
		bool newOk = level->getCubes(player, playerBB->copy()->shrink(r, r, r))->empty();
		if (oldOk && (fail || !newOk) && !player->isSleeping())
		{
			teleport(xLastOk, yLastOk, zLastOk, yRotT, xRotT);
			return;
		}
		AABB *testBox = playerBB->copy()->grow(r, r, r)->expand(0, -0.55, 0);
		// && server.level.getCubes(player, testBox).size() == 0
		if (!server->isFlightAllowed() && !player->gameMode->isCreative() && !level->containsAnyBlocks(testBox) && !player->isAllowedToFly() )
		{
			if (oyDist >= (-0.5f / 16.0f))
			{
				aboveGroundTickCount++;
				if (aboveGroundTickCount > 80)
				{
					//                    logger.warning(player->name + " was kicked for floating too long!");
#ifndef _CONTENT_PACKAGE
					wprintf(L"%ls was kicked for floating too long!\n", player->name.c_str());
#endif
					disconnect(DisconnectPacket::eDisconnect_NoFlying);
					return;
				}
			}
		}
		else
		{
			aboveGroundTickCount = 0;
		}

		player->onGround = packet->onGround;
		server->getPlayers()->move(player);
		player->doCheckFallDamage(player->y - startY, packet->onGround);
	}
	else if ((tickCount % SharedConstants::TICKS_PER_SECOND) == 0)
	{
		teleport(xLastOk, yLastOk, zLastOk, player->yRot, player->xRot);
	}
}

void PlayerConnection::teleport(double x, double y, double z, float yRot, float xRot, bool sendPacket /*= true*/)
{
	synched = false;
	xLastOk = x;
	yLastOk = y;
	zLastOk = z;
	player->absMoveTo(x, y, z, yRot, xRot);
	// 4J - note that 1.62 is added to the height here as the client connection that receives this will presume it represents y + heightOffset at that end
	// This is different to the way that height is sent back to the server, where it represents the bottom of the player bounding volume
	if(sendPacket) player->connection->send(std::make_shared<MovePlayerPacket::PosRot>(x, y + 1.62f, y, z, yRot, xRot, false, false));
}

void PlayerConnection::handlePlayerAction(shared_ptr<PlayerActionPacket> packet)
{
	ServerLevel *level = server->getLevel(player->dimension);
	player->resetLastActionTime();

	if (packet->action == PlayerActionPacket::DROP_ITEM)
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		{
			shared_ptr<ItemInstance> selected = player->inventory->getSelected();
			if (selected != nullptr && selected->count > 0)
			{
				int outId = selected->id, outCount = 1, outAux = selected->getAuxValue();
				bool cancelled = FourKitBridge::FirePlayerDropItem(
					player->entityId, selected->id, 1, selected->getAuxValue(),
					&outId, &outCount, &outAux);
				if (cancelled)
					return;
				player->inventory->removeItem(player->inventory->selected, 1);
				shared_ptr<ItemInstance> dropItem = (outId == selected->id)
					? selected->copy()
					: std::make_shared<ItemInstance>(outId, outCount, outAux);
				dropItem->count = outCount;
				if (outAux != selected->getAuxValue()) dropItem->setAuxValue(outAux);
				player->drop(dropItem);
				return;
			}
		}
#endif
		player->drop(false);
		return;
	}
	else if (packet->action == PlayerActionPacket::DROP_ALL_ITEMS)
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		{
			shared_ptr<ItemInstance> selected = player->inventory->getSelected();
			if (selected != nullptr && selected->count > 0)
			{
				int outId = selected->id, outCount = selected->count, outAux = selected->getAuxValue();
				bool cancelled = FourKitBridge::FirePlayerDropItem(
					player->entityId, selected->id, selected->count, selected->getAuxValue(),
					&outId, &outCount, &outAux);
				if (cancelled)
					return;
				player->inventory->removeItem(player->inventory->selected, selected->count);
				shared_ptr<ItemInstance> dropItem = (outId == selected->id)
					? selected->copy()
					: std::make_shared<ItemInstance>(outId, outCount, outAux);
				dropItem->count = outCount;
				if (outAux != selected->getAuxValue()) dropItem->setAuxValue(outAux);
				player->drop(dropItem);
				return;
			}
		}
#endif
		player->drop(true);
		return;
	}
	else if (packet->action == PlayerActionPacket::RELEASE_USE_ITEM)
	{
		player->releaseUsingItem();
		return;
	}

	bool shouldVerifyLocation = false;
	if (packet->action == PlayerActionPacket::START_DESTROY_BLOCK) shouldVerifyLocation = true;
	if (packet->action == PlayerActionPacket::ABORT_DESTROY_BLOCK) shouldVerifyLocation = true;
	if (packet->action == PlayerActionPacket::STOP_DESTROY_BLOCK) shouldVerifyLocation = true;

	int x = packet->x;
	int y = packet->y;
	int z = packet->z;
	if (shouldVerifyLocation)
	{
		double xDist = player->x - (x + 0.5);
		// there is a mismatch between the player's camera and the player's
		// position, so add 1.5 blocks
		double yDist = player->y - (y + 0.5) + 1.5;
		double zDist = player->z - (z + 0.5);
		double dist = xDist * xDist + yDist * yDist + zDist * zDist;
		if (dist > 6 * 6)
		{
			return;
		}
		if (y >= server->getMaxBuildHeight())
		{
			return;
		}
	}

	if (packet->action == PlayerActionPacket::START_DESTROY_BLOCK)
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		lastLeftClickTick = tickCount;
		{
			shared_ptr<ItemInstance> heldItem = player->inventory->getSelected();
			int iId = heldItem ? heldItem->id : 0;
			int iCount = heldItem ? heldItem->count : 0;
			int iAux = heldItem ? heldItem->getAuxValue() : 0;
			int useItemInHand = 1;
			bool cancelled = FourKitBridge::FirePlayerInteract(
				player->entityId, 1 /* LEFT_CLICK_BLOCK */,
				iId, iCount, iAux,
				x, y, z, packet->face, player->dimension,
				&useItemInHand);
			if (cancelled)
				return;
		}
#endif
		// Anti-cheat: validate spawn protection on the server for mining start.
		if (!server->isUnderSpawnProtection(level, x, y, z, player)) player->gameMode->startDestroyBlock(x, y, z, packet->face);
		else player->connection->send(std::make_shared<TileUpdatePacket>(x, y, z, level));

	}
	else if (packet->action == PlayerActionPacket::STOP_DESTROY_BLOCK)
	{
		player->gameMode->stopDestroyBlock(x, y, z);
		server->getPlayers()->prioritiseTileChanges(x, y, z, level->dimension->id);	// 4J added - make sure that the update packets for this get prioritised over other general world updates
		if (level->getTile(x, y, z) != 0) player->connection->send(std::make_shared<TileUpdatePacket>(x, y, z, level));
	}
	else if (packet->action == PlayerActionPacket::ABORT_DESTROY_BLOCK)
	{
		player->gameMode->abortDestroyBlock(x, y, z);
		if (level->getTile(x, y, z) != 0) player->connection->send(std::make_shared<TileUpdatePacket>(x, y, z, level));
	}
}

void PlayerConnection::handleUseItem(shared_ptr<UseItemPacket> packet)
{
	ServerLevel *level = server->getLevel(player->dimension);
	shared_ptr<ItemInstance> item = player->inventory->getSelected();
	bool informClient = false;
	int x = packet->getX();
	int y = packet->getY();
	int z = packet->getZ();
	int face = packet->getFace();
	player->resetLastActionTime();

	if (packet->getFace() == 255)
	{
		if (item == nullptr) return;
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		{
			int iId = item->id;
			int iCount = item->count;
			int iAux = item->getAuxValue();
			int useItemInHand = 1;
			bool cancelled = FourKitBridge::FirePlayerInteract(
				player->entityId, 2,
				iId, iCount, iAux,
				0, 0, 0, 6, player->dimension,
				&useItemInHand);
			if (cancelled || !useItemInHand)
				return;
		}
#endif
		player->gameMode->useItem(player, level, item);
	}
	else if ((packet->getY() < server->getMaxBuildHeight() - 1) || (packet->getFace() != Facing::UP && packet->getY() < server->getMaxBuildHeight()))
	{
		if (synched && player->distanceToSqr(x + 0.5, y + 0.5, z + 0.5) < 8 * 8)
		{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			{
				int iId = item ? item->id : 0;
				int iCount = item ? item->count : 0;
				int iAux = item ? item->getAuxValue() : 0;
				int useItemInHand = 1;
				bool cancelled = FourKitBridge::FirePlayerInteract(
					player->entityId, 3 /* RIGHT_CLICK_BLOCK */,
					iId, iCount, iAux,
					x, y, z, face, player->dimension,
					&useItemInHand);
				if (cancelled || !useItemInHand)
				{
					informClient = true;
					goto skipUseItemOn;
				}
			}
#endif
			// Anti-cheat: block placement/use must pass server-side spawn protection.
			if (!server->isUnderSpawnProtection(level, x, y, z, player))
			{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
				int placeX = x, placeY = y, placeZ = z;
				bool validFace = true;

				if      (face == 0) placeY--;
				else if (face == 1) placeY++;
				else if (face == 2) placeZ--;
				else if (face == 3) placeZ++;
				else if (face == 4) placeX--;
				else if (face == 5) placeX++;
				else                validFace = false;

				int oldTileId      = validFace ? level->getTile(placeX, placeY, placeZ) : 0;
				int oldTileData    = validFace ? level->getData(placeX, placeY, placeZ) : 0;
				int oldClickedId   = level->getTile(x, y, z);
				int oldClickedData = level->getData(x, y, z);
				int savedItemId    = item ? item->id    : 0;
				int savedItemCount = item ? item->count : 0;
#endif

				player->gameMode->useItemOn(player, level, item, x, y, z, face, packet->getClickX(), packet->getClickY(), packet->getClickZ());

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
				if (validFace)
				{
					int newTileId    = level->getTile(placeX, placeY, placeZ);
					int newClickedId = level->getTile(x, y, z);

					int fireX = placeX, fireY = placeY, fireZ = placeZ;
					int againstX = x, againstY = y, againstZ = z;
					int revertId = oldTileId, revertData = oldTileData;
					bool shouldFire = false;

					if (newTileId != 0 && newTileId != oldTileId)
					{
						shouldFire = true;
					}
					else if (newClickedId != 0 && newClickedId != oldClickedId)
					{
						shouldFire = true;
						fireX = x; fireY = y; fireZ = z;
						revertId = oldClickedId; revertData = oldClickedData;
						againstX = x; againstY = y; againstZ = z;
						if      (face == 0) againstY++;
						else if (face == 1) againstY--;
						else if (face == 2) againstZ++;
						else if (face == 3) againstZ--;
						else if (face == 4) againstX++;
						else if (face == 5) againstX--;
					}

					if (shouldFire)
					{
						bool cancelled = FourKitBridge::FireBlockPlace(
							player->entityId, player->dimension,
							fireX, fireY, fireZ,
							againstX, againstY, againstZ,
							savedItemId, savedItemCount, true);

						if (cancelled)
						{
							level->setTileAndData(fireX, fireY, fireZ, revertId, revertData, Tile::UPDATE_ALL);
							auto &slot = player->inventory->items[player->inventory->selected];
							if (slot != nullptr)
							{
								slot->count = savedItemCount;
							}
							else if (savedItemId > 0 && savedItemId < 256 && Tile::tiles[savedItemId] != nullptr && savedItemCount > 0)
							{
								slot = std::make_shared<ItemInstance>(Tile::tiles[savedItemId], savedItemCount);
							}
							informClient = true;
						}
					}
				}
#endif
			}
		}

skipUseItemOn:
		informClient = true;
	}
	else
	{
		//player->connection->send(shared_ptr<ChatPacket>(new ChatPacket("\u00A77Height limit for building is " + server->maxBuildHeight)));
		informClient = true;
	}

	if (informClient)
	{

		player->connection->send(std::make_shared<TileUpdatePacket>(x, y, z, level));

		if (face == 0) y--;
		if (face == 1) y++;
		if (face == 2) z--;
		if (face == 3) z++;
		if (face == 4) x--;
		if (face == 5) x++;

		// 4J - Fixes an issue where pistons briefly disappear when retracting. The pistons themselves shouldn't have their change from being pistonBase_Id to  pistonMovingPiece_Id
		// directly sent to the client, as this will happen on the client as a result of it actioning (via a tile event) the retraction of the piston locally. However, by putting a switch
		// beside a piston and then performing an action on the side of it facing a piston, the following line of code will send a TileUpdatePacket containing the change to pistonMovingPiece_Id
		// to the client, and this packet is received before the piston retract action happens - when the piston retract then occurs, it doesn't work properly because the piston tile
		// isn't what it is expecting.
		if( level->getTile(x,y,z) != Tile::pistonMovingPiece_Id )
		{
			player->connection->send(std::make_shared<TileUpdatePacket>(x, y, z, level));
		}

	}

	item = player->inventory->getSelected();

	bool forceClientUpdate = false;
	if(item != nullptr && packet->getItem() == nullptr)
	{
		forceClientUpdate = true;
	}
	if (item != nullptr && item->count == 0)
	{
		player->inventory->items[player->inventory->selected] = nullptr;
		item = nullptr;
	}

	if (item == nullptr || item->getUseDuration() == 0)
	{
		player->ignoreSlotUpdateHack = true;
		player->inventory->items[player->inventory->selected] = ItemInstance::clone(player->inventory->items[player->inventory->selected]);
		Slot *s = player->containerMenu->getSlotFor(player->inventory, player->inventory->selected);
		player->containerMenu->broadcastChanges();
		player->ignoreSlotUpdateHack = false;

		if (forceClientUpdate || !ItemInstance::matches(player->inventory->getSelected(), packet->getItem()))
		{
			send(std::make_shared<ContainerSetSlotPacket>(player->containerMenu->containerId, s->index, player->inventory->getSelected()));
		}
	}
}

void PlayerConnection::onDisconnect(DisconnectPacket::eDisconnectReason reason, void *reasonObjects)
{
	EnterCriticalSection(&done_cs);
	if( done )
	{
		LeaveCriticalSection(&done_cs);
		return;
	}
	bool fourKitHandledQuit = false;
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	ServerRuntime::ServerLogManager::OnPlayerDisconnected(
		getLogSmallId(),
		(player != NULL) ? player->name : std::wstring(),
		reason,
		false);
	fourKitHandledQuit = FourKitBridge::FirePlayerQuit(player->entityId);
#endif

	done = true;
	LeaveCriticalSection(&done_cs);

	server->getPlayers()->queueDisconnect(player, static_cast<int>(reason),
		L"", getWasKicked(), fourKitHandledQuit);
}

void PlayerConnection::openSecurityGate()
{
	if (m_securityGateOpen)
		return;

	m_securityGateOpen = true;

	// Flush all buffered packets now that the cipher is active
	for (auto &buffered : m_securityBuffer)
	{
		send(buffered);
	}
	m_securityBuffer.clear();
	m_securityBuffer.shrink_to_fit();
}

void PlayerConnection::onUnhandledPacket(shared_ptr<Packet> packet)
{
	//    logger.warning(getClass() + " wasn't prepared to deal with a " + packet.getClass());
	disconnect(DisconnectPacket::eDisconnect_UnexpectedPacket);
}

void PlayerConnection::send(shared_ptr<Packet> packet)
{
	if( connection->getSocket() != nullptr )
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		// Security gate: when require-secure-client is enabled, buffer ALL outgoing
		// packets until the cipher handshake completes. Only the cipher handshake
		// CustomPayloadPacket (MC|CKey) is sent immediately. Once the cipher activates,
		// openSecurityGate() flushes the buffer. This prevents unsecured/old clients
		// from receiving any game data (PlayerInfoPackets, XUIDs, etc.) before being kicked.
		if (!m_securityGateOpen)
		{
			// Allow cipher handshake packets through immediately
			if (packet->getId() == 250)
			{
				auto cpp = dynamic_pointer_cast<CustomPayloadPacket>(packet);
				if (cpp != nullptr &&
					(cpp->identifier == CustomPayloadPacket::CIPHER_KEY_CHANNEL ||
					 cpp->identifier == CustomPayloadPacket::CIPHER_ACK_CHANNEL ||
					 cpp->identifier == CustomPayloadPacket::CIPHER_ON_CHANNEL))
				{
					// Fall through to send
				}
				else
				{
					m_securityBuffer.push_back(packet);
					return;
				}
			}
			else
			{
				m_securityBuffer.push_back(packet);
				return;
			}
		}
#endif

		if( !server->getPlayers()->canReceiveAllPackets( player ) )
		{
			// Check if we are allowed to send this packet type
			if( !Packet::canSendToAnyClient(packet) )
			{
				//wprintf(L"Not the systems primary player, so not sending them a packet : %ls / %d\n", player->name.c_str(), packet->getId() );
				return;
			}
		}
		connection->send(packet);
	}
}

// 4J Added
void PlayerConnection::queueSend(shared_ptr<Packet> packet)
{
	if( connection->getSocket() != nullptr )
	{
		if( !server->getPlayers()->canReceiveAllPackets( player ) )
		{
			// Check if we are allowed to send this packet type
			if( !Packet::canSendToAnyClient(packet) )
			{
				//wprintf(L"Not the systems primary player, so not queueing them a packet : %ls\n", connection->getSocket()->getPlayer()->GetGamertag() );
				return;
			}
		}
		connection->queueSend(packet);
	}
}

void PlayerConnection::handleSetCarriedItem(shared_ptr<SetCarriedItemPacket> packet)
{
	if (packet->slot < 0 || packet->slot >= Inventory::getSelectionSize())
	{
		//        logger.warning(player.name + " tried to set an invalid carried item");
		return;
	}
	player->inventory->selected = packet->slot;
	player->resetLastActionTime();
}

void PlayerConnection::handleChat(shared_ptr<ChatPacket> packet)
{
	if (packet->m_stringArgs.empty()) return;
	wstring message = trimString(packet->m_stringArgs[0]);
	if (message.length() > SharedConstants::maxChatLength)
	{
		disconnect(DisconnectPacket::eDisconnect_None); // or a specific reason
		return;
	}
	// Optional: validate characters (acceptableLetters)
	if (message.length() > 0 && message[0] == L'/')
	{
		handleCommand(message);
		return;
	}
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	{
		std::wstring formatted;
        if (!FourKitBridge::FirePlayerChat(player->entityId, message, formatted))
		{
			if (formatted.empty())
				formatted = L"<" + player->name + L"> " + message;
			server->getPlayers()->broadcastAll(std::make_shared<ChatPacket>(formatted));
		}
	}
#else
	wstring formatted = L"<" + player->name + L"> " + message;
	server->getPlayers()->broadcastAll(shared_ptr<ChatPacket>(new ChatPacket(app.FormatChatMessage(formatted, false))));
#endif
	chatSpamTickCount += SharedConstants::TICKS_PER_SECOND;
	if (chatSpamTickCount > SharedConstants::TICKS_PER_SECOND * 10)
	{
		disconnect(DisconnectPacket::eDisconnect_None); // spam
	}
}

void PlayerConnection::handleCommand(const wstring& message)
{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	std::wstring commandLine = message;
	if (FourKitBridge::FireCommandPreprocess(player->entityId, commandLine, commandLine))
		return;
	if (FourKitBridge::HandlePlayerCommand(player->entityId, commandLine))
		return;
#endif
	// 4J - TODO
#if 0
	server.getCommandDispatcher().performCommand(player, message);
#endif
}

void PlayerConnection::handleAnimate(shared_ptr<AnimatePacket> packet)
{
	player->resetLastActionTime();
	if (packet->action == AnimatePacket::SWING)
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		if (lastLeftClickTick != tickCount)
		{
			int useItemInHand = 1;
			auto item = player->inventory->getSelected();
			int iId = item ? item->id : 0;
			int iCount = item ? item->count : 0;
			int iAux = item ? item->getAuxValue() : 0;
			bool cancelled = FourKitBridge::FirePlayerInteract(
				player->entityId, 0,
				iId, iCount, iAux,
				0, 0, 0, 6, player->dimension,
				&useItemInHand);
			if (cancelled)
				return;
		}
#endif
		player->swing();
	}
}

void PlayerConnection::handlePlayerCommand(shared_ptr<PlayerCommandPacket> packet)
{
	player->resetLastActionTime();
	if (packet->action == PlayerCommandPacket::START_SNEAKING)
	{
		player->setSneaking(true);
	}
	else if (packet->action == PlayerCommandPacket::STOP_SNEAKING)
	{
		player->setSneaking(false);
	}
	else if (packet->action == PlayerCommandPacket::START_SPRINTING)
	{
		player->setSprinting(true);
	}
	else if (packet->action == PlayerCommandPacket::STOP_SPRINTING)
	{
		player->setSprinting(false);
	}
	else if (packet->action == PlayerCommandPacket::STOP_SLEEPING)
	{
		player->stopSleepInBed(false, true, true);
		synched = false;
	}
	else if (packet->action == PlayerCommandPacket::RIDING_JUMP)
	{
		// currently only supported by horses...
		if ( (player->riding != nullptr) && player->riding->GetType() == eTYPE_HORSE)
		{
			dynamic_pointer_cast<EntityHorse>(player->riding)->onPlayerJump(packet->data);
		}
	}
	else if (packet->action == PlayerCommandPacket::OPEN_INVENTORY)
	{
		// also only supported by horses...
		if ( (player->riding != nullptr) && player->riding->instanceof(eTYPE_HORSE) )
		{
			dynamic_pointer_cast<EntityHorse>(player->riding)->openInventory(player);
		}
	}
	else if (packet->action == PlayerCommandPacket::START_IDLEANIM)
	{
		player->setIsIdle(true);
	}
	else if (packet->action == PlayerCommandPacket::STOP_IDLEANIM)
	{
		player->setIsIdle(false);
	}
}

void PlayerConnection::setShowOnMaps(bool bVal)
{
	player->setShowOnMaps(bVal);
}

void PlayerConnection::handleDisconnect(shared_ptr<DisconnectPacket> packet)
{
	// 4J Stu - Need to remove the player from the receiving list before their socket is NULLed so that we can find another player on their system
	server->getPlayers()->removePlayerFromReceiving( player );
	connection->close(DisconnectPacket::eDisconnect_Quitting);
}

int PlayerConnection::countDelayedPackets()
{
	return connection->countDelayedPackets();
}

void PlayerConnection::info(const wstring& string)
{
	// 4J-PB - removed, since it needs to be localised in the language the client is in
	//send( shared_ptr<ChatPacket>( new ChatPacket(L"�7" + string) ) );
}

void PlayerConnection::warn(const wstring& string)
{
	// 4J-PB - removed, since it needs to be localised in the language the client is in
	//send( shared_ptr<ChatPacket>( new ChatPacket(L"�9" + string) ) );
}

wstring PlayerConnection::getConsoleName()
{
	return player->getName();
}

void PlayerConnection::handleInteract(shared_ptr<InteractPacket> packet)
{
	ServerLevel *level = server->getLevel(player->dimension);
	shared_ptr<Entity> target = level->getEntity(packet->target);
	player->resetLastActionTime();

	// Fix for #8218 - Gameplay: Attacking zombies from a different level often results in no hits being registered
	// 4J Stu - If the client says that we hit something, then agree with it. The canSee can fail here as it checks
	// a ray from head->head, but we may actually be looking at a different part of the entity that can be seen
	// even though the ray is blocked.
	if (target != nullptr)
	{
		// Anti-cheat: enforce reach and LOS on the server to reject forged hits.
		bool canSeeTarget = player->canSee(target);
		double maxDistSq = canSeeTarget ? kInteractReachSq : kInteractBlockedReachSq;
		if (player->distanceToSqr(target) > maxDistSq)
		{
			return;
		}

		if (packet->action == InteractPacket::INTERACT)
		{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			{
				int mappedType = FourKitBridge::MapEntityType((int)target->GetType());
				float targetHealth = 0, targetMaxHealth = 0, targetEyeHeight = 0;
				auto living = dynamic_pointer_cast<LivingEntity>(target);
				if (living)
				{
					targetHealth = living->getHealth();
					targetMaxHealth = living->getMaxHealth();
					targetEyeHeight = living->getHeadHeight();
				}
				if (FourKitBridge::FirePlayerInteractEntity(
					player->entityId, target->entityId, mappedType,
					player->dimension, target->x, target->y, target->z,
					targetHealth, targetMaxHealth, targetEyeHeight))
					return;
			}
#endif
			player->interact(target);
		}
		else if (packet->action == InteractPacket::ATTACK)
		{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			lastLeftClickTick = tickCount;
#endif
			if ((target->GetType() == eTYPE_ITEMENTITY) || (target->GetType() == eTYPE_EXPERIENCEORB) || (target->GetType() == eTYPE_ARROW) || target == player)
			{
					return;
			}
			player->attack(target);
		}
	}

}

bool PlayerConnection::canHandleAsyncPackets()
{
	return true;
}

void PlayerConnection::handleTexture(shared_ptr<TexturePacket> packet)
{
	// Both PlayerConnection and ClientConnection should handle this mostly the same way

	if(packet->dwBytes==0)
	{
		// Request for texture
#ifndef _CONTENT_PACKAGE
		wprintf(L"Server received request for custom texture %ls\n",packet->textureName.c_str());
#endif
		PBYTE pbData=nullptr;
		DWORD dwBytes=0;
		app.GetMemFileDetails(packet->textureName,&pbData,&dwBytes);

		if(dwBytes!=0)
		{
			send(std::make_shared<TexturePacket>(packet->textureName, pbData, dwBytes));
		}
		else
		{
			m_texturesRequested.push_back( packet->textureName );
		}
	}
	else
	{
		// Response with texture data
#ifndef _CONTENT_PACKAGE
		wprintf(L"Server received custom texture %ls\n",packet->textureName.c_str());
#endif
		app.AddMemoryTextureFile(packet->textureName,packet->pbData,packet->dwBytes);
		server->connection->handleTextureReceived(packet->textureName);
	}
}

void PlayerConnection::handleTextureAndGeometry(shared_ptr<TextureAndGeometryPacket> packet)
{
	// Both PlayerConnection and ClientConnection should handle this mostly the same way

	if(packet->dwTextureBytes==0)
	{
		// Request for texture and geometry
#ifndef _CONTENT_PACKAGE
		wprintf(L"Server received request for custom texture %ls\n",packet->textureName.c_str());
#endif
		PBYTE pbData=nullptr;
		DWORD dwTextureBytes=0;
		app.GetMemFileDetails(packet->textureName,&pbData,&dwTextureBytes);
		DLCSkinFile *pDLCSkinFile = app.m_dlcManager.getSkinFile(packet->textureName);

		if(dwTextureBytes!=0)
		{

			if(pDLCSkinFile)
			{
				if(pDLCSkinFile->getAdditionalBoxesCount()!=0)
				{
					send(std::make_shared<TextureAndGeometryPacket>(packet->textureName, pbData, dwTextureBytes, pDLCSkinFile));
				}
				else
				{
					send(std::make_shared<TextureAndGeometryPacket>(packet->textureName, pbData, dwTextureBytes));
				}
			}
			else
			{
				// we don't have the dlc skin, so retrieve the data from the app store
				vector<SKIN_BOX *> *pvSkinBoxes = app.GetAdditionalSkinBoxes(packet->dwSkinID);
				unsigned int uiAnimOverrideBitmask= app.GetAnimOverrideBitmask(packet->dwSkinID);

				send(std::make_shared<TextureAndGeometryPacket>(packet->textureName, pbData, dwTextureBytes, pvSkinBoxes, uiAnimOverrideBitmask));
			}
		}
		else
		{
			m_texturesRequested.push_back( packet->textureName );
		}
	}
	else
	{
		// Response with texture and geometry data
#ifndef _CONTENT_PACKAGE
		wprintf(L"Server received custom texture %ls and geometry\n",packet->textureName.c_str());
#endif
		app.AddMemoryTextureFile(packet->textureName,packet->pbData,packet->dwTextureBytes);

		// add the geometry to the app list
		if(packet->dwBoxC!=0)
		{
#ifndef _CONTENT_PACKAGE
			wprintf(L"Adding skin boxes for skin id %X, box count %d\n",packet->dwSkinID,packet->dwBoxC);
#endif
			app.SetAdditionalSkinBoxes(packet->dwSkinID,packet->BoxDataA,packet->dwBoxC);
		}
		// Add the anim override
		app.SetAnimOverrideBitmask(packet->dwSkinID,packet->uiAnimOverrideBitmask);

		player->setCustomSkin(packet->dwSkinID);

		server->connection->handleTextureAndGeometryReceived(packet->textureName);
	}
}

void PlayerConnection::handleTextureReceived(const wstring &textureName)
{
	// This sends the server received texture out to any other players waiting for the data
    auto it = find(m_texturesRequested.begin(), m_texturesRequested.end(), textureName);
    if( it != m_texturesRequested.end() )
	{
		PBYTE pbData=nullptr;
		DWORD dwBytes=0;
		app.GetMemFileDetails(textureName,&pbData,&dwBytes);

		if(dwBytes!=0)
		{
			send(std::make_shared<TexturePacket>(textureName, pbData, dwBytes));
			m_texturesRequested.erase(it);
		}
	}
}

void PlayerConnection::handleTextureAndGeometryReceived(const wstring &textureName)
{
	// This sends the server received texture out to any other players waiting for the data
    auto it = find(m_texturesRequested.begin(), m_texturesRequested.end(), textureName);
    if( it != m_texturesRequested.end() )
	{
		PBYTE pbData=nullptr;
		DWORD dwTextureBytes=0;
		app.GetMemFileDetails(textureName,&pbData,&dwTextureBytes);
		DLCSkinFile *pDLCSkinFile=app.m_dlcManager.getSkinFile(textureName);

		if(dwTextureBytes!=0)
		{
			if(pDLCSkinFile && (pDLCSkinFile->getAdditionalBoxesCount()!=0))
			{
				send(std::make_shared<TextureAndGeometryPacket>(textureName, pbData, dwTextureBytes, pDLCSkinFile));
			}
			else
			{
				// get the data from the app
				DWORD dwSkinID = app.getSkinIdFromPath(textureName);
				vector<SKIN_BOX *> *pvSkinBoxes = app.GetAdditionalSkinBoxes(dwSkinID);
				unsigned int uiAnimOverrideBitmask= app.GetAnimOverrideBitmask(dwSkinID);

				send(std::make_shared<TextureAndGeometryPacket>(textureName, pbData, dwTextureBytes, pvSkinBoxes, uiAnimOverrideBitmask));
			}
			m_texturesRequested.erase(it);
		}
	}
}

void PlayerConnection::handleTextureChange(shared_ptr<TextureChangePacket> packet)
{
	switch(packet->action)
	{
	case TextureChangePacket::e_TextureChange_Skin:
		player->setCustomSkin( app.getSkinIdFromPath( packet->path ) );
#ifndef _CONTENT_PACKAGE
		wprintf(L"Skin for server player %ls has changed to %ls (%d)\n", player->name.c_str(), player->customTextureUrl.c_str(), player->getPlayerDefaultSkin() );
#endif
		break;
	case TextureChangePacket::e_TextureChange_Cape:
		player->setCustomCape( Player::getCapeIdFromPath( packet->path ) );
		//player->customTextureUrl2 = packet->path;
#ifndef _CONTENT_PACKAGE
		wprintf(L"Cape for server player %ls has changed to %ls\n", player->name.c_str(), player->customTextureUrl2.c_str() );
#endif
		break;
	}
	if(!packet->path.empty() && packet->path.substr(0,3).compare(L"def") != 0 && !app.IsFileInMemoryTextures(packet->path))
	{
        if (server->connection->addPendingTextureRequest(packet->path))
        {
#ifndef _CONTENT_PACKAGE
            wprintf(L"Sending texture packet to get custom skin %ls from player %ls\n", packet->path.c_str(), player->name.c_str());
#endif
            send(std::make_shared<TexturePacket>(
                packet->path,
                nullptr,
                static_cast<DWORD>(0)
                ));

        }
    }
	else if(!packet->path.empty() && app.IsFileInMemoryTextures(packet->path))
	{
		// Update the ref count on the memory texture data
		app.AddMemoryTextureFile(packet->path,nullptr,0);
	}
	server->getPlayers()->broadcastAll(std::make_shared<TextureChangePacket>(player, packet->action, packet->path), player->dimension );
}

void PlayerConnection::handleTextureAndGeometryChange(shared_ptr<TextureAndGeometryChangePacket> packet)
{

	player->setCustomSkin( app.getSkinIdFromPath( packet->path ) );
#ifndef _CONTENT_PACKAGE
	wprintf(L"PlayerConnection::handleTextureAndGeometryChange - Skin for server player %ls has changed to %ls (%d)\n", player->name.c_str(), player->customTextureUrl.c_str(), player->getPlayerDefaultSkin() );
#endif


	if(!packet->path.empty() && packet->path.substr(0,3).compare(L"def") != 0 && !app.IsFileInMemoryTextures(packet->path))
	{
		if(	server->connection->addPendingTextureRequest(packet->path))
		{
#ifndef _CONTENT_PACKAGE
			wprintf(L"Sending texture packet to get custom skin %ls from player %ls\n",packet->path.c_str(), player->name.c_str());
#endif
            send(std::make_shared<TextureAndGeometryPacket>(
                packet->path,
                nullptr,
                static_cast<DWORD>(0)));
		}
	}
	else if(!packet->path.empty() && app.IsFileInMemoryTextures(packet->path))
	{
		// Update the ref count on the memory texture data
		app.AddMemoryTextureFile(packet->path,nullptr,0);

		player->setCustomSkin(packet->dwSkinID);

		// If we already have the texture, then we already have the model parts too
		//app.SetAdditionalSkinBoxes(packet->dwSkinID,)
		//DebugBreak();
	}
	server->getPlayers()->broadcastAll(std::make_shared<TextureAndGeometryChangePacket>(player, packet->path), player->dimension );
}

void PlayerConnection::handleServerSettingsChanged(shared_ptr<ServerSettingsChangedPacket> packet)
{
	if(packet->action==ServerSettingsChangedPacket::HOST_IN_GAME_SETTINGS)
	{
		INetworkPlayer *networkPlayer = getNetworkPlayer();
		bool isHost = (networkPlayer != nullptr && networkPlayer->IsHost());
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		// On dedicated servers, only the host can change server settings.
		// Moderators (OPs) should not be able to alter game rules.
		if (!isHost)
		{
			app.DebugPrintf("SECURITY: Non-host player %ls attempted to change server settings\n",
				player->getName().c_str());
			return;
		}
#endif
		if( isHost || player->isModerator())
		{
			app.SetGameHostOption(eGameHostOption_FireSpreads, app.GetGameHostOption(packet->data,eGameHostOption_FireSpreads));
			app.SetGameHostOption(eGameHostOption_TNT, app.GetGameHostOption(packet->data,eGameHostOption_TNT));
			app.SetGameHostOption(eGameHostOption_MobGriefing, app.GetGameHostOption(packet->data, eGameHostOption_MobGriefing));
			app.SetGameHostOption(eGameHostOption_KeepInventory, app.GetGameHostOption(packet->data, eGameHostOption_KeepInventory));
			app.SetGameHostOption(eGameHostOption_DoMobSpawning, app.GetGameHostOption(packet->data, eGameHostOption_DoMobSpawning));
			app.SetGameHostOption(eGameHostOption_DoMobLoot, app.GetGameHostOption(packet->data, eGameHostOption_DoMobLoot));
			app.SetGameHostOption(eGameHostOption_DoTileDrops, app.GetGameHostOption(packet->data, eGameHostOption_DoTileDrops));
			app.SetGameHostOption(eGameHostOption_DoDaylightCycle, app.GetGameHostOption(packet->data, eGameHostOption_DoDaylightCycle));
			app.SetGameHostOption(eGameHostOption_NaturalRegeneration, app.GetGameHostOption(packet->data, eGameHostOption_NaturalRegeneration));

			server->getPlayers()->broadcastAll(std::make_shared<ServerSettingsChangedPacket>(ServerSettingsChangedPacket::HOST_IN_GAME_SETTINGS, app.GetGameHostOption(eGameHostOption_All)));

			// Update the QoS data
			g_NetworkManager.UpdateAndSetGameSessionData();
		}
	}
}

void PlayerConnection::handleKickPlayer(shared_ptr<KickPlayerPacket> packet)
{
	INetworkPlayer *networkPlayer = getNetworkPlayer();
	bool isHost = (networkPlayer != nullptr && networkPlayer->IsHost());
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Live ops.json check for non-host players
	if (!isHost)
	{
		PlayerUID kickerXuid = m_offlineXUID;
		if (kickerXuid == INVALID_XUID) kickerXuid = m_onlineXUID;
		if (!ServerRuntime::Access::IsPlayerOp(kickerXuid))
		{
			app.DebugPrintf("SECURITY: Non-OP player %ls attempted to kick\n", player->getName().c_str());
			{
				INetworkPlayer *npLog = getNetworkPlayer();
				if (npLog != nullptr)
					ServerRuntime::ServerLogManager::OnUnauthorizedCommand(npLog->GetSmallId(), player->getName(), "kick");
			}
			return;
		}
	}
#endif
	if( isHost || player->isModerator())
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		// On dedicated servers, non-host moderators cannot kick other moderators or the host.
		if (!isHost)
		{
			for (auto &checkingPlayer : server->getPlayers()->players)
			{
				if (checkingPlayer != nullptr &&
					checkingPlayer->connection->getNetworkPlayer() != nullptr &&
					checkingPlayer->connection->getNetworkPlayer()->GetSmallId() == packet->m_networkSmallId)
				{
					if (checkingPlayer->isModerator() ||
						checkingPlayer->connection->getNetworkPlayer()->IsHost())
					{
						app.DebugPrintf("SECURITY: Moderator %ls tried to kick host/moderator %ls\n",
							player->getName().c_str(), checkingPlayer->getName().c_str());
						return;
					}
					break;
				}
			}
		}
		app.DebugPrintf("CMD: Player %ls kicked player with smallId=%d\n",
			player->getName().c_str(), packet->m_networkSmallId);
#endif
		server->getPlayers()->kickPlayerByShortId(packet->m_networkSmallId);
	}
}

void PlayerConnection::handleGameCommand(shared_ptr<GameCommandPacket> packet)
{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	INetworkPlayer *networkPlayer = getNetworkPlayer();
	bool isHost = (networkPlayer != nullptr && networkPlayer->IsHost());
	if (!isHost)
	{
		// Live ops.json check - in-memory isModerator() can be stale if ops.json was edited mid-session
		PlayerUID cmdXuid = m_offlineXUID;
		if (cmdXuid == INVALID_XUID) cmdXuid = m_onlineXUID;
		if (!ServerRuntime::Access::IsPlayerOp(cmdXuid))
		{
			app.DebugPrintf("SECURITY: Non-OP player %ls attempted server command id=%d\n",
				player->getName().c_str(), static_cast<int>(packet->command));
			{
				INetworkPlayer *npLog = getNetworkPlayer();
				if (npLog != nullptr)
					ServerRuntime::ServerLogManager::OnUnauthorizedCommand(npLog->GetSmallId(), player->getName(), "game-command");
			}
			return;
		}
	}
	app.DebugPrintf("CMD: Player %ls (OP=%d, Host=%d) executed command id=%d\n",
		player->getName().c_str(), player->isModerator() ? 1 : 0, isHost ? 1 : 0,
		static_cast<int>(packet->command));
#endif
	MinecraftServer::getInstance()->getCommandDispatcher()->performCommand(player, packet->command, packet->data);
}

void PlayerConnection::handleClientCommand(shared_ptr<ClientCommandPacket> packet)
{
	player->resetLastActionTime();
	if (packet->action == ClientCommandPacket::PERFORM_RESPAWN)
	{
		if (player->wonGame)
		{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			int oldEntityId = player->entityId;
#endif
			player = server->getPlayers()->respawn(player, player->m_enteredEndExitPortal?0:player->dimension, true);
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			FourKitBridge::UpdatePlayerEntityId(oldEntityId, player->entityId);
#endif
		}
		else if (player->level->getLevelData()->isHardcore())
		{
			// Hardcore mode — server rejects respawn. Ban and disconnect are already
			// handled in ServerPlayer::die() via banPlayerForHardcoreDeath().
			return;
		}
		else
		{
			if (player->getHealth() > 0) return;
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			int oldEntityId = player->entityId;
#endif
			player = server->getPlayers()->respawn(player, 0, false);
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			FourKitBridge::UpdatePlayerEntityId(oldEntityId, player->entityId);
#endif
		}
	}
}

void PlayerConnection::handleRespawn(shared_ptr<RespawnPacket> packet)
{
}

void PlayerConnection::handleContainerClose(shared_ptr<ContainerClosePacket> packet)
{
	player->doCloseContainer();
}

#ifndef _CONTENT_PACKAGE
void PlayerConnection::handleContainerSetSlot(shared_ptr<ContainerSetSlotPacket> packet)
{
	if(player->gameMode->isSurvival()){ // Still allow creative players to change slots manually with packets(?) -- might want this different.
		server->warn(L"Player " + player->getName() + L" just tried to set a slot in a container in survival mode");
		return;
	}
	if (packet->containerId == AbstractContainerMenu::CONTAINER_ID_CARRIED )
	{
		player->inventory->setCarried(packet->item);
	}
	else
	{
		if (packet->containerId == AbstractContainerMenu::CONTAINER_ID_INVENTORY && packet->slot >= 36 && packet->slot < 36 + 9)
		{
			shared_ptr<ItemInstance> lastItem = player->inventoryMenu->getSlot(packet->slot)->getItem();
			if (packet->item != nullptr)
			{
				if (lastItem == nullptr || lastItem->count < packet->item->count)
				{
					packet->item->popTime = Inventory::POP_TIME_DURATION;
				}
			}
			player->inventoryMenu->setItem(packet->slot, packet->item);
			player->ignoreSlotUpdateHack = true;
			player->containerMenu->broadcastChanges();
			player->broadcastCarriedItem();
			player->ignoreSlotUpdateHack = false;
		}
		else if (packet->containerId == player->containerMenu->containerId)
		{
			player->containerMenu->setItem(packet->slot, packet->item);
			player->ignoreSlotUpdateHack = true;
			player->containerMenu->broadcastChanges();
			player->broadcastCarriedItem();
			player->ignoreSlotUpdateHack = false;
		}
	}
}
#endif

void PlayerConnection::handleContainerClick(shared_ptr<ContainerClickPacket> packet)
{
	player->resetLastActionTime();
	if (player->containerMenu->containerId == packet->containerId && player->containerMenu->isSynched(player))
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		int fourKitClickResult = FourKitBridge::FireInventoryClick(player->entityId, packet->slotNum, packet->buttonNum, packet->clickType);
		if (fourKitClickResult == 1)
		{
			expectedAcks[player->containerMenu->containerId] = packet->uid;
			player->connection->send(std::make_shared<ContainerAckPacket>(packet->containerId, packet->uid, false));
			player->containerMenu->setSynched(player, false);
			vector<shared_ptr<ItemInstance>> items;
			for (unsigned int i = 0; i < player->containerMenu->slots.size(); i++)
				items.push_back(player->containerMenu->slots.at(i)->getItem());
			player->refreshContainer(player->containerMenu, &items);
			return;
		}
#endif
		shared_ptr<ItemInstance> clicked = player->containerMenu->clicked(packet->slotNum, packet->buttonNum, packet->clickType, player);

		if (ItemInstance::matches(packet->item, clicked))
		{
			// Yep, you sure did click what you claimed to click!
			player->connection->send(std::make_shared<ContainerAckPacket>(packet->containerId, packet->uid, true));
			player->ignoreSlotUpdateHack = true;
			player->containerMenu->broadcastChanges();
			player->broadcastCarriedItem();
			player->ignoreSlotUpdateHack = false;
		}
		else
		{
			// No, you clicked the wrong thing!
			expectedAcks[player->containerMenu->containerId] = packet->uid;
			player->connection->send(std::make_shared<ContainerAckPacket>(packet->containerId, packet->uid, false));
			player->containerMenu->setSynched(player, false);

			vector<shared_ptr<ItemInstance> > items;
			for (unsigned int i = 0; i < player->containerMenu->slots.size(); i++)
			{
				items.push_back(player->containerMenu->slots.at(i)->getItem());
			}
			player->refreshContainer(player->containerMenu, &items);

			//                player.containerMenu.broadcastChanges();
		}
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		if (fourKitClickResult == 2)
		{
			vector<shared_ptr<ItemInstance>> refreshItems;
			for (unsigned int i = 0; i < player->containerMenu->slots.size(); i++)
				refreshItems.push_back(player->containerMenu->slots.at(i)->getItem());
			player->refreshContainer(player->containerMenu, &refreshItems);
		}
#endif
	}

}

void PlayerConnection::handleContainerButtonClick(shared_ptr<ContainerButtonClickPacket> packet)
{
	player->resetLastActionTime();
	if (player->containerMenu->containerId == packet->containerId && player->containerMenu->isSynched(player))
	{
		player->containerMenu->clickMenuButton(player, packet->buttonId);
		player->containerMenu->broadcastChanges();
	}
}

void PlayerConnection::handleSetCreativeModeSlot(shared_ptr<SetCreativeModeSlotPacket> packet)
{
	if (player->gameMode->isCreative())
	{
		bool drop = packet->slotNum < 0;
		shared_ptr<ItemInstance> item = packet->item;

		if(item != nullptr && item->id == Item::map_Id)
		{
			int mapScale = 3;
#ifdef _LARGE_WORLDS
			int scale = MapItemSavedData::MAP_SIZE * 2 * (1 << mapScale);
			int centreXC = static_cast<int>(Math::round(player->x / scale) * scale);
			int centreZC = static_cast<int>(Math::round(player->z / scale) * scale);
#else
			// 4J-PB - for Xbox maps, we'll centre them on the origin of the world, since we can fit the whole world in our map
			int centreXC = 0;
			int centreZC = 0;
#endif
			item->setAuxValue( player->level->getAuxValueForMap(player->getXuid(), player->dimension, centreXC, centreZC, mapScale) );

			shared_ptr<MapItemSavedData> data = MapItem::getSavedData(item->getAuxValue(), player->level);
			// 4J Stu - We only have one map per player per dimension, so don't reset the one that they have
			// when a new one is created
			wchar_t buf[64];
			swprintf(buf,64,L"map_%d", item->getAuxValue());
			std::wstring id = wstring(buf);
			if( data == nullptr )
			{
				data = std::make_shared<MapItemSavedData>(id);
			}
			player->level->setSavedData(id, (shared_ptr<SavedData> ) data);

			data->scale = mapScale;
			// 4J-PB - for Xbox maps, we'll centre them on the origin of the world, since we can fit the whole world in our map
			data->x = centreXC;
			data->z = centreZC;
			data->dimension = static_cast<byte>(player->level->dimension->id);
			data->setDirty();
		}

		bool validSlot = (packet->slotNum >= InventoryMenu::CRAFT_SLOT_START && packet->slotNum < (InventoryMenu::USE_ROW_SLOT_START + Inventory::getSelectionSize()));
		bool validItem = item == nullptr || (item->id < Item::items.length && item->id >= 0 && Item::items[item->id] != nullptr);
		bool validData = item == nullptr || (item->getAuxValue() >= 0 && item->count > 0 && item->count <= 64);

		if (validSlot && validItem && validData)
		{
			if (item == nullptr)
			{
				player->inventoryMenu->setItem(packet->slotNum, nullptr);
			}
			else
			{
				player->inventoryMenu->setItem(packet->slotNum, item );
			}
			player->inventoryMenu->setSynched(player, true);
			//                player.slotChanged(player.inventoryMenu, packet.slotNum, player.inventoryMenu.getSlot(packet.slotNum).getItem());
		}
		else if (drop && validItem && validData)
		{
			if (dropSpamTickCount < SharedConstants::TICKS_PER_SECOND * 10)
			{
				dropSpamTickCount += SharedConstants::TICKS_PER_SECOND;
				// drop item
				shared_ptr<ItemEntity> dropped = player->drop(item);
				if (dropped != nullptr)
				{
					dropped->setShortLifeTime();
				}
			}
		}

		if( item != nullptr && item->id == Item::map_Id )
		{
			// 4J Stu - Maps need to have their aux value update, so the client should always be assumed to be wrong
			// This is how the Java works, as the client also incorrectly predicts the auxvalue of the mapItem
			vector<shared_ptr<ItemInstance> > items;
			for (unsigned int i = 0; i < player->inventoryMenu->slots.size(); i++)
			{
				items.push_back(player->inventoryMenu->slots.at(i)->getItem());
			}
			player->refreshContainer(player->inventoryMenu, &items);
		}
	}
}

void PlayerConnection::handleContainerAck(shared_ptr<ContainerAckPacket> packet)
{
    auto it = expectedAcks.find(player->containerMenu->containerId);

    if (it != expectedAcks.end() && packet->uid == it->second && player->containerMenu->containerId == packet->containerId && !player->containerMenu->isSynched(player))
	{
		player->containerMenu->setSynched(player, true);
	}
}

void PlayerConnection::handleSignUpdate(shared_ptr<SignUpdatePacket> packet)
{
	player->resetLastActionTime();
	app.DebugPrintf("PlayerConnection::handleSignUpdate\n");

	ServerLevel *level = server->getLevel(player->dimension);
	if (level->hasChunkAt(packet->x, packet->y, packet->z))
	{
		shared_ptr<TileEntity> te = level->getTileEntity(packet->x, packet->y, packet->z);

		if (dynamic_pointer_cast<SignTileEntity>(te) != nullptr)
		{
			shared_ptr<SignTileEntity> ste = dynamic_pointer_cast<SignTileEntity>(te);
			if (!ste->isEditable() || ste->getPlayerWhoMayEdit() != player)
			{
				server->warn(L"Player " + player->getName() + L" just tried to change non-editable sign");
				return;
			}
		}

		// 4J-JEV: Changed to allow characters to display as a [].
		if (dynamic_pointer_cast<SignTileEntity>(te) != nullptr)
		{
			int x = packet->x;
			int y = packet->y;
			int z = packet->z;
			shared_ptr<SignTileEntity> ste = dynamic_pointer_cast<SignTileEntity>(te);

			wstring lines[4];
			for (int i = 0; i < 4; i++)
			{
				lines[i] = packet->lines[i].substr(0,15);
			}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			{
				std::wstring outLines[4];
				bool cancelled = FourKitBridge::FireSignChange(
					player->entityId, player->dimension,
					x, y, z,
					lines[0], lines[1], lines[2], lines[3],
					outLines);
				if (cancelled)
					return;
				for (int i = 0; i < 4; i++)
					lines[i] = outLines[i];
			}
#endif

			for (int i = 0; i < 4; i++)
			{
				ste->SetMessage( i, lines[i] );
			}
			ste->SetVerified(false);
			ste->setChanged();
			level->sendTileUpdated(x, y, z);
		}
	}

}

void PlayerConnection::handleKeepAlive(shared_ptr<KeepAlivePacket> packet)
{
	if (packet->id == lastKeepAliveId)
	{
		int time = static_cast<int>(System::nanoTime() / 1000000 - lastKeepAliveTime);
		player->latency = (player->latency * 3 + time) / 4;
	}
}

void PlayerConnection::handlePlayerInfo(shared_ptr<PlayerInfoPacket> packet)
{
	INetworkPlayer *networkPlayer = getNetworkPlayer();
	bool isHost = (networkPlayer != nullptr && networkPlayer->IsHost());
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Live ops.json check for non-host players
	if (!isHost)
	{
		PlayerUID infoXuid = m_offlineXUID;
		if (infoXuid == INVALID_XUID) infoXuid = m_onlineXUID;
		if (!ServerRuntime::Access::IsPlayerOp(infoXuid))
		{
			return;
		}
	}
#endif
	if( isHost || player->isModerator() )
	{
		shared_ptr<ServerPlayer> serverPlayer;
		// Find the player being edited
		for(auto& checkingPlayer : server->getPlayers()->players)
		{
			if(checkingPlayer->connection->getNetworkPlayer() != nullptr && checkingPlayer->connection->getNetworkPlayer()->GetSmallId() == packet->m_networkSmallId)
			{
				serverPlayer = checkingPlayer;
				break;
			}
		}

		if(serverPlayer != nullptr)
		{
			unsigned int origPrivs = serverPlayer->getAllPlayerGamePrivileges();

			bool trustPlayers = app.GetGameHostOption(eGameHostOption_TrustPlayers) != 0;
			bool cheats = app.GetGameHostOption(eGameHostOption_CheatsEnabled) != 0;
			if(serverPlayer == player)
			{
				GameType *gameType = Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CreativeMode) ? GameType::CREATIVE : GameType::SURVIVAL;
				gameType = LevelSettings::validateGameType(gameType->getId());
				if (serverPlayer->gameMode->getGameModeForPlayer() != gameType)
				{
#ifndef _CONTENT_PACKAGE
					wprintf(L"Setting %ls to game mode %d\n", serverPlayer->name.c_str(), gameType->getId());
#endif
					serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CreativeMode,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CreativeMode) );
					serverPlayer->gameMode->setGameModeForPlayer(gameType);
					serverPlayer->connection->send(std::make_shared<GameEventPacket>(GameEventPacket::CHANGE_GAME_MODE, gameType->getId()));
				}
				else
				{
#ifndef _CONTENT_PACKAGE
					wprintf(L"%ls already has game mode %d\n", serverPlayer->name.c_str(), gameType);
#endif
				}
				if(cheats)
				{
					// Editing self
					bool canBeInvisible = Player::getPlayerGamePrivilege(origPrivs, Player::ePlayerGamePrivilege_CanToggleInvisible) != 0;
					if(canBeInvisible)serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_Invisible,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_Invisible) );
					if(canBeInvisible)serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_Invulnerable,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_Invulnerable) );

					bool inCreativeMode = Player::getPlayerGamePrivilege(origPrivs,Player::ePlayerGamePrivilege_CreativeMode) != 0;
					if(!inCreativeMode)
					{
						bool canFly = Player::getPlayerGamePrivilege(origPrivs,Player::ePlayerGamePrivilege_CanToggleFly);
						bool canChangeHunger = Player::getPlayerGamePrivilege(origPrivs,Player::ePlayerGamePrivilege_CanToggleClassicHunger);

						if(canFly)serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CanFly,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CanFly) );
						if(canChangeHunger)serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_ClassicHunger,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_ClassicHunger) );
					}
				}
			}
			else
			{
				// Editing someone else
				if(!trustPlayers && !serverPlayer->connection->getNetworkPlayer()->IsHost())
				{
					serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CannotMine,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CannotMine) );
					serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CannotBuild,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CannotBuild) );
					serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CannotAttackPlayers,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CannotAttackPlayers) );
					serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CannotAttackAnimals,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CannotAttackAnimals) );
					serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CanUseDoorsAndSwitches,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CanUseDoorsAndSwitches) );
					serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CanUseContainers,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CanUseContainers) );
				}

				if(networkPlayer->IsHost())
				{
					if(cheats)
					{
						serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CanToggleInvisible,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CanToggleInvisible) );
						serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CanToggleFly,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CanToggleFly) );
						serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CanToggleClassicHunger,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CanToggleClassicHunger) );
						serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_CanTeleport,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_CanTeleport) );
					}
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
					// On dedicated servers, OP can only be granted/revoked if the target is in ops.json.
					// This prevents runtime OP escalation via crafted PlayerInfoPackets.
					bool wantsOp = Player::getPlayerGamePrivilege(packet->m_playerPrivileges, Player::ePlayerGamePrivilege_Op) != 0;
					PlayerUID targetXuid = serverPlayer->connection->m_offlineXUID;
					if (targetXuid == INVALID_XUID) targetXuid = serverPlayer->connection->m_onlineXUID;
					if (wantsOp && !ServerRuntime::Access::IsPlayerOp(targetXuid))
					{
						app.DebugPrintf("SECURITY: Host tried to OP player %ls who is not in ops.json\n",
							serverPlayer->getName().c_str());
					}
					else
					{
						serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_Op, wantsOp ? 1u : 0u);
					}
#else
					serverPlayer->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_Op,Player::getPlayerGamePrivilege(packet->m_playerPrivileges,Player::ePlayerGamePrivilege_Op) );
#endif
				}
			}

			server->getPlayers()->broadcastAll(std::make_shared<PlayerInfoPacket>(serverPlayer));
		}
	}
}

bool PlayerConnection::isServerPacketListener()
{
	return true;
}

void PlayerConnection::handlePlayerAbilities(shared_ptr<PlayerAbilitiesPacket> playerAbilitiesPacket)
{
	player->abilities.flying = playerAbilitiesPacket->isFlying() && player->abilities.mayfly;
}

//void handleChatAutoComplete(ChatAutoCompletePacket packet) {
//	StringBuilder result = new StringBuilder();

//	for (String candidate : server.getAutoCompletions(player, packet.getMessage())) {
//		if (result.length() > 0) result.append("\0");

//		result.append(candidate);
//	}

//	player.connection.send(new ChatAutoCompletePacket(result.toString()));
//}

//void handleClientInformation(shared_ptr<ClientInformationPacket> packet)
//{
//	player->updateOptions(packet);
//}

void PlayerConnection::handleCustomPayload(shared_ptr<CustomPayloadPacket> customPayloadPacket)
{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Identity token response from client
	if (CustomPayloadPacket::IDENTITY_TOKEN_RESPONSE.compare(customPayloadPacket->identifier) == 0)
	{
		PlayerUID xuid = m_offlineXUID;
		if (xuid == INVALID_XUID) xuid = m_onlineXUID;

		bool tokenValid = false;
		if (customPayloadPacket->length == ServerRuntime::Security::IdentityTokenManager::TOKEN_SIZE &&
			customPayloadPacket->data.length == ServerRuntime::Security::IdentityTokenManager::TOKEN_SIZE &&
			customPayloadPacket->data.data != nullptr)
		{
			tokenValid = ServerRuntime::Security::GetIdentityTokenManager().VerifyToken(xuid, customPayloadPacket->data.data);
		}

		if (tokenValid)
		{
			m_identityVerified = true;
			app.DebugPrintf("SECURITY: Identity token verified for player %ls\n", player->getName().c_str());
			INetworkPlayer *npLog = getNetworkPlayer();
			if (npLog != nullptr)
				ServerRuntime::ServerLogManager::OnIdentityTokenVerified(npLog->GetSmallId());
		}
		else
		{
			app.DebugPrintf("SECURITY: Identity token MISMATCH for player %ls - will disconnect\n", player->getName().c_str());
			app.DebugPrintf("SECURITY: If this player lost their token, use: revoketoken %ls\n", player->getName().c_str());
			INetworkPlayer *npLog = getNetworkPlayer();
			if (npLog != nullptr)
				ServerRuntime::ServerLogManager::OnIdentityTokenMismatch(npLog->GetSmallId(), player->getName());
			// Defer disconnect to avoid re-entrancy issues during packet dispatch
			setWasKicked();
			closeOnTick();
		}
		return;
	}
#endif

	if (CustomPayloadPacket::CUSTOM_BOOK_PACKET.compare(customPayloadPacket->identifier) == 0)
	{
		ByteArrayInputStream bais(customPayloadPacket->data);
		DataInputStream input(&bais);
		shared_ptr<ItemInstance> sentItem = Packet::readItem(&input);

		if (sentItem->tag == nullptr)
		{
			throw new IOException(L"Invalid book tag!");
		}

		// make sure the sent item is the currently carried item
		shared_ptr<ItemInstance> carried = player->inventory->getSelected();
		if (sentItem != nullptr && sentItem->id == Item::writingBook_Id && sentItem->id == carried->id)
		{
			player->inventory->setItem(player->inventory->selected, sentItem);
		}
	}
	else if (CustomPayloadPacket::CUSTOM_BOOK_SIGN_PACKET.compare(customPayloadPacket->identifier) == 0)
	{
		ByteArrayInputStream bais(customPayloadPacket->data);
		DataInputStream input(&bais);
		shared_ptr<ItemInstance> sentItem = Packet::readItem(&input);

		if (sentItem->tag == nullptr)
		{
			throw new IOException(L"Invalid book tag!");
		}

		// make sure the sent item is the currently carried item
		shared_ptr<ItemInstance> carried = player->inventory->getSelected();

		if (sentItem != nullptr && sentItem->id == Item::writingBook_Id && sentItem->id == carried->id)
		{
			sentItem->setHoverName(sentItem->tag->getString(L"title"));
			sentItem->id = 387;
			player->inventory->setItem(player->inventory->selected, sentItem);
		}
	}
	else if (CustomPayloadPacket::TRADER_SELECTION_PACKET.compare(customPayloadPacket->identifier) == 0)
	{
		ByteArrayInputStream bais(customPayloadPacket->data);
		DataInputStream input(&bais);
		int selection = input.readInt();

		AbstractContainerMenu *menu = player->containerMenu;
		if (dynamic_cast<MerchantMenu *>(menu))
		{
			static_cast<MerchantMenu *>(menu)->setSelectionHint(selection);
		}
	}
	else if (CustomPayloadPacket::SET_ADVENTURE_COMMAND_PACKET.compare(customPayloadPacket->identifier) == 0)
	{
		if (!server->isCommandBlockEnabled())
		{
			app.DebugPrintf("Command blocks not enabled");
			//player->sendMessage(ChatMessageComponent.forTranslation("advMode.notEnabled"));
		}
		else if (player->hasPermission(eGameCommand_Effect) && player->abilities.instabuild)
		{
			ByteArrayInputStream bais(customPayloadPacket->data);
			DataInputStream input(&bais);
			int x = input.readInt();
			int y = input.readInt();
			int z = input.readInt();
			wstring command = Packet::readUtf(&input, 256);

			shared_ptr<TileEntity> tileEntity = player->level->getTileEntity(x, y, z);
			shared_ptr<CommandBlockEntity> cbe = dynamic_pointer_cast<CommandBlockEntity>(tileEntity);
			if (tileEntity != nullptr && cbe != nullptr)
			{
				cbe->setCommand(command);
				player->level->sendTileUpdated(x, y, z);
				//player->sendMessage(ChatMessageComponent.forTranslation("advMode.setCommand.success", command));
			}
		}
		else
		{
			//player.sendMessage(ChatMessageComponent.forTranslation("advMode.notAllowed"));
		}
	}
	else if (CustomPayloadPacket::SET_BEACON_PACKET.compare(customPayloadPacket->identifier) == 0)
	{
	    if (dynamic_cast<BeaconMenu *>(player->containerMenu) != nullptr)
	    {
	        ByteArrayInputStream bais(customPayloadPacket->data);
	        DataInputStream input(&bais);
	        int primary = input.readInt();
	        int secondary = input.readInt();

	        BeaconMenu *beaconMenu = static_cast<BeaconMenu *>(player->containerMenu);
	        Slot *slot = beaconMenu->getSlot(0);

	        if (slot != nullptr && slot->hasItem())
	        {
	            shared_ptr<BeaconTileEntity> beacon = beaconMenu->getBeacon();

	            int prevPrimary = beacon->getPrimaryPower();
	            int prevSecondary = beacon->getSecondaryPower();

	            beacon->setPrimaryPower(primary);
	            beacon->setSecondaryPower(secondary);

	            // Only consume the payment item if the powers actually changed
	            if (beacon->getPrimaryPower() != prevPrimary ||
                    beacon->getSecondaryPower() != prevSecondary)
	            {
	                slot->remove(1);
	            }

	            beacon->setChanged();
	        }
	    }
	}
	else if (CustomPayloadPacket::SET_ITEM_NAME_PACKET.compare(customPayloadPacket->identifier) == 0)
	{
	    AnvilMenu *menu = dynamic_cast<AnvilMenu *>(player->containerMenu);
	    if (menu)
	    {
	        if (customPayloadPacket->data.data == nullptr || customPayloadPacket->data.length < 1)
	        {
	            menu->setItemName(L"");
	        }
	        else
	        {
	            ByteArrayInputStream bais(customPayloadPacket->data);
	            DataInputStream dis(&bais);
	            wstring name = dis.readUTF();

	            if (name.length() <= 30)
	            {
	                menu->setItemName(name);
	            }
	        }
	    }
	}
}

bool PlayerConnection::isDisconnected()
{
	return done;
}

// 4J Added

void PlayerConnection::handleDebugOptions(shared_ptr<DebugOptionsPacket> packet)
{
#ifdef _DEBUG
    // Player player = dynamic_pointer_cast<Player>( player->shared_from_this() );
    player->SetDebugOptions(packet->m_uiVal);
#endif
}

void PlayerConnection::handleCraftItem(shared_ptr<CraftItemPacket> packet)
{
	int iRecipe = packet->recipe;

	if(iRecipe == -1)
		return;

	int recipeCount = (int)Recipes::getInstance()->getRecipies()->size();
	if(iRecipe < 0 || iRecipe >= recipeCount)
		return;

	Recipy::INGREDIENTS_REQUIRED *pRecipeIngredientsRequired=Recipes::getInstance()->getRecipeIngredientsArray();
	shared_ptr<ItemInstance> pTempItemInst=pRecipeIngredientsRequired[iRecipe].pRecipy->assemble(nullptr);

	if(app.DebugSettingsOn() && (player->GetDebugOptions()&(1L<<eDebugSetting_CraftAnything)))
	{
		pTempItemInst->onCraftedBy(player->level, dynamic_pointer_cast<Player>( player->shared_from_this() ), pTempItemInst->count );
		if(player->inventory->add(pTempItemInst)==false )
		{
			// no room in inventory, so throw it down
			player->drop(pTempItemInst);
		}
	}
	else if (pTempItemInst->id == Item::fireworksCharge_Id || pTempItemInst->id == Item::fireworks_Id)
	{
		CraftingMenu *menu = static_cast<CraftingMenu *>(player->containerMenu);
		player->openFireworks(menu->getX(), menu->getY(), menu->getZ() );
	}
	else
	{


		Recipy::INGREDIENTS_REQUIRED &req = pRecipeIngredientsRequired[iRecipe];
		if (req.iType == RECIPE_TYPE_3x3 && dynamic_cast<CraftingMenu *>(player->containerMenu) == nullptr)
		{
			server->warn(L"Player " + player->getName() + L" tried to craft a 3x3 recipe without a crafting bench");
			return;
		}
		for (int i = 0; i < req.iIngC; i++){
			int need = req.iIngValA[i];
			int have = player->inventory->countResource(req.iIngIDA[i], req.iIngAuxValA[i]);
			if (have < need){
				server->warn(L"Player " + player->getName() + L" just tried to craft item " + to_wstring(pTempItemInst->id) + L" with insufficient ingredients");
				return;
			}
		}

		pTempItemInst->onCraftedBy(player->level, dynamic_pointer_cast<Player>( player->shared_from_this() ), pTempItemInst->count );

		// and remove those resources from your inventory
		for(int i=0;i<pRecipeIngredientsRequired[iRecipe].iIngC;i++)
		{
			for(int j=0;j<pRecipeIngredientsRequired[iRecipe].iIngValA[i];j++)
			{
				shared_ptr<ItemInstance> ingItemInst = nullptr;
				// do we need to remove a specific aux value?
				if(pRecipeIngredientsRequired[iRecipe].iIngAuxValA[i]!=Recipes::ANY_AUX_VALUE)
				{
					ingItemInst = player->inventory->getResourceItem( pRecipeIngredientsRequired[iRecipe].iIngIDA[i],pRecipeIngredientsRequired[iRecipe].iIngAuxValA[i] );
					player->inventory->removeResource(pRecipeIngredientsRequired[iRecipe].iIngIDA[i],pRecipeIngredientsRequired[iRecipe].iIngAuxValA[i]);
				}
				else
				{
					ingItemInst = player->inventory->getResourceItem( pRecipeIngredientsRequired[iRecipe].iIngIDA[i] );
					player->inventory->removeResource(pRecipeIngredientsRequired[iRecipe].iIngIDA[i]);
				}

				// 4J Stu - Fix for #13097 - Bug: Milk Buckets are removed when crafting Cake
				if (ingItemInst != nullptr)
				{
					if (ingItemInst->getItem()->hasCraftingRemainingItem())
					{
						// replace item with remaining result
						player->inventory->add(std::make_shared<ItemInstance>(ingItemInst->getItem()->getCraftingRemainingItem()));
					}

				}
			}
		}

		// 4J Stu - Fix for #13119 - We should add the item after we remove the ingredients
		if(player->inventory->add(pTempItemInst)==false )
		{
			// no room in inventory, so throw it down
			player->drop(pTempItemInst);
		}

		if( pTempItemInst->id == Item::map_Id )
		{
			// 4J Stu - Maps need to have their aux value update, so the client should always be assumed to be wrong
			// This is how the Java works, as the client also incorrectly predicts the auxvalue of the mapItem
			vector<shared_ptr<ItemInstance> > items;
			for (unsigned int i = 0; i < player->containerMenu->slots.size(); i++)
			{
				items.push_back(player->containerMenu->slots.at(i)->getItem());
			}
			player->refreshContainer(player->containerMenu, &items);
		}
		else
		{
			// Do same hack as PlayerConnection::handleContainerClick does - do our broadcast of changes just now, but with a hack so it just thinks it has sent
			// things but hasn't really. This will stop the client getting a message back confirming the current inventory items, which might then arrive
			// after another local change has been made on the client and be stale.
			player->ignoreSlotUpdateHack = true;
			player->containerMenu->broadcastChanges();
			player->broadcastCarriedItem();
			player->ignoreSlotUpdateHack = false;
		}
	}

	// handle achievements
	switch(pTempItemInst->id )
	{
	case Tile::workBench_Id:		player->awardStat(GenericStats::buildWorkbench(),		GenericStats::param_buildWorkbench());		break;
	case Item::pickAxe_wood_Id:		player->awardStat(GenericStats::buildPickaxe(),			GenericStats::param_buildPickaxe());		break;
	case Tile::furnace_Id:			player->awardStat(GenericStats::buildFurnace(),			GenericStats::param_buildFurnace());		break;
	case Item::hoe_wood_Id:			player->awardStat(GenericStats::buildHoe(),				GenericStats::param_buildHoe());			break;
	case Item::bread_Id:			player->awardStat(GenericStats::makeBread(),			GenericStats::param_makeBread());			break;
	case Item::cake_Id:				player->awardStat(GenericStats::bakeCake(),				GenericStats::param_bakeCake());			break;
	case Item::pickAxe_stone_Id:	player->awardStat(GenericStats::buildBetterPickaxe(),	GenericStats::param_buildBetterPickaxe());	break;
	case Item::sword_wood_Id:		player->awardStat(GenericStats::buildSword(),			GenericStats::param_buildSword());			break;
	case Tile::dispenser_Id:		player->awardStat(GenericStats::dispenseWithThis(),		GenericStats::param_dispenseWithThis());	break;
	case Tile::enchantTable_Id:		player->awardStat(GenericStats::enchantments(),			GenericStats::param_enchantments());		break;
	case Tile::bookshelf_Id:		player->awardStat(GenericStats::bookcase(),				GenericStats::param_bookcase());			break;
	}
	//}
	// ELSE The server thinks the client was wrong...
}


void PlayerConnection::handleTradeItem(shared_ptr<TradeItemPacket> packet)
{
	if (player->containerMenu->containerId == packet->containerId)
	{
		MerchantMenu *menu = static_cast<MerchantMenu *>(player->containerMenu);

		MerchantRecipeList *offers = menu->getMerchant()->getOffers(player);

		if(offers)
		{
			int selectedShopItem = packet->offer;
			if( selectedShopItem < offers->size() )
			{
				MerchantRecipe *activeRecipe = offers->at(selectedShopItem);
				if(!activeRecipe->isDeprecated())
				{
					// Do we have the ingredients?
					shared_ptr<ItemInstance> buyAItem = activeRecipe->getBuyAItem();
					shared_ptr<ItemInstance> buyBItem = activeRecipe->getBuyBItem();

					int buyAMatches = player->inventory->countMatches(buyAItem);
					int buyBMatches = player->inventory->countMatches(buyBItem);
					if( (buyAItem != nullptr && buyAMatches >= buyAItem->count) && (buyBItem == nullptr || buyBMatches >= buyBItem->count) )
					{
						menu->getMerchant()->notifyTrade(activeRecipe);

						// Remove the items we are purchasing with
						player->inventory->removeResources(buyAItem);
						player->inventory->removeResources(buyBItem);

						// Add the item we have purchased
						shared_ptr<ItemInstance> result = activeRecipe->getSellItem()->copy();

						// 4J JEV - Award itemsBought stat.
						player->awardStat(
							GenericStats::itemsBought(result->getItem()->id),
							GenericStats::param_itemsBought(
							result->getItem()->id,
							result->getAuxValue(),
							result->GetCount()
							)
							);

						if (!player->inventory->add(result))
						{
							player->drop(result);
						}
					}
				}
			}
		}
	}
}

INetworkPlayer *PlayerConnection::getNetworkPlayer()
{
	if( connection != nullptr && connection->getSocket() != nullptr) return connection->getSocket()->getPlayer();
	else return nullptr;
}

bool PlayerConnection::isLocal()
{
	if( connection->getSocket() == nullptr )
	{
		return false;
	}
	else
	{
		bool isLocal = connection->getSocket()->isLocal();
		return connection->getSocket()->isLocal();
	}
}

bool PlayerConnection::isGuest()
{
	if( connection->getSocket() == nullptr )
	{
		return false;
	}
	else
	{
		INetworkPlayer *networkPlayer = connection->getSocket()->getPlayer();
		bool isGuest = false;
		if(networkPlayer != nullptr)
		{
			isGuest = networkPlayer->IsGuest() == TRUE;
		}
		return isGuest;
	}
}
