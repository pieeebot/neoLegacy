#include "stdafx.h"
#include "Options.h"
#include "ServerConnection.h"

#include <memory>
#include "PendingConnection.h"
#include "PlayerConnection.h"
#include "ServerPlayer.h"
#include "../Minecraft.World/net.minecraft.network.h"
#include "../Minecraft.World/Socket.h"
#include "../Minecraft.World/net.minecraft.world.level.h"
#include "MultiPlayerLevel.h"
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
#include "../Minecraft.Server/Security/SecurityConfig.h"
#include "../Minecraft.Server/ServerLogManager.h"
#include "../Minecraft.Server/ServerLogger.h"
#include "../Minecraft.World/System.h"
#endif

ServerConnection::ServerConnection(MinecraftServer *server)
{
	// 4J - added initialiser
	connectionCounter = 0;
	InitializeCriticalSection(&pending_cs);
	InitializeCriticalSection(&players_cs);

	this->server = server;
}

ServerConnection::~ServerConnection()
{
	DeleteCriticalSection(&pending_cs);
	DeleteCriticalSection(&players_cs);
}

// 4J - added to handle incoming connections, to replace thread that original used to have
void ServerConnection::NewIncomingSocket(Socket *socket)
{
	shared_ptr<PendingConnection> unconnectedClient = std::make_shared<PendingConnection>(server, socket, L"Connection #" + std::to_wstring(connectionCounter++));
	handleConnection(unconnectedClient);
}

void ServerConnection::addPlayerConnection(shared_ptr<PlayerConnection> uc)
{
	EnterCriticalSection(&players_cs);
	players.push_back(uc);
	LeaveCriticalSection(&players_cs);
}

void ServerConnection::handleConnection(shared_ptr<PendingConnection> uc)
{
	EnterCriticalSection(&pending_cs);
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	int maxPending = ServerRuntime::Security::GetSettings().maxPendingConnections;
	if (maxPending > 0 && static_cast<int>(pending.size()) >= maxPending)
	{
		LeaveCriticalSection(&pending_cs);
		app.DebugPrintf("SECURITY: Rejecting connection, too many pending (%d/%d)\n",
			static_cast<int>(pending.size()), maxPending);
		uc->disconnect(DisconnectPacket::eDisconnect_ServerFull);
		return;
	}
#endif
	pending.push_back(uc);
	LeaveCriticalSection(&pending_cs);
}

void ServerConnection::stop()
{
	std::vector<shared_ptr<PendingConnection> > pendingSnapshot;
	EnterCriticalSection(&pending_cs);
	pendingSnapshot = pending;
	LeaveCriticalSection(&pending_cs);

	for (unsigned int i = 0; i < pendingSnapshot.size(); i++)
	{
		shared_ptr<PendingConnection> uc = pendingSnapshot[i];
		if (uc != NULL && !uc->done)
		{
			uc->disconnect(DisconnectPacket::eDisconnect_Closed);
		}
	}

	// Snapshot to avoid iterator invalidation if disconnect modifies the vector.
	EnterCriticalSection(&players_cs);
	std::vector<shared_ptr<PlayerConnection> > playerSnapshot = players;
	LeaveCriticalSection(&players_cs);
	for (unsigned int i = 0; i < playerSnapshot.size(); i++)
	{
		shared_ptr<PlayerConnection> player = playerSnapshot[i];
		if (player != NULL && !player->done)
		{
			player->disconnect(DisconnectPacket::eDisconnect_Quitting);
		}
	}
}

void ServerConnection::tick()
{
	{
		// MGH - changed this so that the the CS lock doesn't cover the tick (was causing a lockup when 2 players tried to join)
		EnterCriticalSection(&pending_cs);
		vector< shared_ptr<PendingConnection> > tempPending = pending;
		LeaveCriticalSection(&pending_cs);

		for (unsigned int i = 0; i < tempPending.size(); i++)
		{
			shared_ptr<PendingConnection> uc = tempPending[i];
	//        try {	// 4J - removed try/catch
				uc->tick();
	//        } catch (Exception e) {
	//            uc.disconnect("Internal server error");
	//            logger.log(Level.WARNING, "Failed to handle packet: " + e, e);
	//        }
			if(uc->connection != nullptr) uc->connection->flush();
		}
	}

	// now remove from the pending list
	EnterCriticalSection(&pending_cs);
	for (unsigned int i = 0; i < pending.size(); i++)
	if (pending[i]->done)
	{
		pending.erase(pending.begin()+i);
		i--;
	}
	LeaveCriticalSection(&pending_cs);

	{
		EnterCriticalSection(&players_cs);
		vector< shared_ptr<PlayerConnection> > tempPlayers = players;
		LeaveCriticalSection(&players_cs);

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		// Substep timing for chunk / tick / flush across all players.
		LARGE_INTEGER scFreq, scA, scB, scC, scD;
		QueryPerformanceFrequency(&scFreq);
		int64_t sc_chunkUs = 0;
		int64_t sc_tickUs  = 0;
		int64_t sc_flushUs = 0;
		int64_t sc_loopT0  = System::currentTimeMillis();
#endif

		// Rotate the per-tick start offset so the chunk-send cap doesn't
		// starve players at the back of the vector.
		static unsigned int s_chunkRotationOffset = 0;
		s_chunkRotationOffset++;
		size_t playerCount = tempPlayers.size();
		size_t startIdx = playerCount > 0 ? (s_chunkRotationOffset % playerCount) : 0;

		for (unsigned int k = 0; k < tempPlayers.size(); k++)
		{
			unsigned int i = (unsigned int)((startIdx + k) % playerCount);
			shared_ptr<PlayerConnection> player = tempPlayers[i];
			shared_ptr<ServerPlayer> serverPlayer = player->getPlayer();
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			QueryPerformanceCounter(&scA);
#endif
			if( serverPlayer )
			{
				serverPlayer->updateFrameTick();
				serverPlayer->doChunkSendingTick(false);
			}
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			QueryPerformanceCounter(&scB);
			sc_chunkUs += (scB.QuadPart - scA.QuadPart) * 1000000 / scFreq.QuadPart;
#endif
			player->tick();
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			QueryPerformanceCounter(&scC);
			sc_tickUs += (scC.QuadPart - scB.QuadPart) * 1000000 / scFreq.QuadPart;
#endif
			if (player->done)
			{
				EnterCriticalSection(&players_cs);
				auto it = find(players.begin(), players.end(), player);
				if (it != players.end()) players.erase(it);
				LeaveCriticalSection(&players_cs);
			}
			else
			{
				player->connection->flush();
			}
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			QueryPerformanceCounter(&scD);
			sc_flushUs += (scD.QuadPart - scC.QuadPart) * 1000000 / scFreq.QuadPart;
#endif
		}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		int64_t sc_loopTotal = System::currentTimeMillis() - sc_loopT0;
		if (sc_loopTotal > 50)
		{
			ServerRuntime::LogInfof("perf",
				"conn playerLoop total=%lldms players=%d chunkUs=%lld tickUs=%lld flushUs=%lld | avg chunk=%lld tick=%lld flush=%lld",
				(long long)sc_loopTotal,
				(int)tempPlayers.size(),
				(long long)sc_chunkUs,
				(long long)sc_tickUs,
				(long long)sc_flushUs,
				tempPlayers.size() > 0 ? (long long)(sc_chunkUs / tempPlayers.size()) : 0LL,
				tempPlayers.size() > 0 ? (long long)(sc_tickUs  / tempPlayers.size()) : 0LL,
				tempPlayers.size() > 0 ? (long long)(sc_flushUs / tempPlayers.size()) : 0LL);
		}
#endif
	}

}

bool ServerConnection::addPendingTextureRequest(const wstring &textureName)
{
    auto it = find(m_pendingTextureRequests.begin(), m_pendingTextureRequests.end(), textureName);
    if( it == m_pendingTextureRequests.end() )
	{
		m_pendingTextureRequests.push_back(textureName);
		return true;
	}

	// 4J Stu - We want to request this texture from everyone, if we have a duplicate it's most likely because the first person we asked for it didn't have it
	// eg They selected a skin then deleted the skin pack. The side effect of this change is that in certain cases we can send a few more requests, and receive
	// a few more responses if people join with the same skin in a short space of time
	return true;
}

void ServerConnection::handleTextureReceived(const wstring &textureName)
{
    auto it = find(m_pendingTextureRequests.begin(), m_pendingTextureRequests.end(), textureName);
    if( it != m_pendingTextureRequests.end() )
	{
		m_pendingTextureRequests.erase(it);
	}
	EnterCriticalSection(&players_cs);
	vector< shared_ptr<PlayerConnection> > tempPlayers = players;
	LeaveCriticalSection(&players_cs);
	for (auto& player : tempPlayers)
	{
        if (!player->done)
		{
			player->handleTextureReceived(textureName);
        }
    }
}

void ServerConnection::handleTextureAndGeometryReceived(const wstring &textureName)
{
    auto it = find(m_pendingTextureRequests.begin(), m_pendingTextureRequests.end(), textureName);
    if( it != m_pendingTextureRequests.end() )
	{
		m_pendingTextureRequests.erase(it);
	}
	EnterCriticalSection(&players_cs);
	vector< shared_ptr<PlayerConnection> > tempPlayers = players;
	LeaveCriticalSection(&players_cs);
	for (auto& player : tempPlayers)
	{
		if (!player->done)
		{
			player->handleTextureAndGeometryReceived(textureName);
		}
	}
}

void ServerConnection::handleServerSettingsChanged(shared_ptr<ServerSettingsChangedPacket> packet)
{
	Minecraft *pMinecraft = Minecraft::GetInstance();

	if(packet->action==ServerSettingsChangedPacket::HOST_DIFFICULTY)
	{
		for(unsigned int i = 0; i < pMinecraft->levels.length; ++i)
		{
			if( pMinecraft->levels[i] != nullptr )
			{
				app.DebugPrintf("ClientConnection::handleServerSettingsChanged - Difficulty = %d",packet->data);
				pMinecraft->levels[i]->difficulty = packet->data;
			}
		}
	}
// 	else if(packet->action==ServerSettingsChangedPacket::HOST_IN_GAME_SETTINGS)// options
// 	{
// 		app.SetGameHostOption(eGameHostOption_All,packet->m_serverSettings)
// 	}
// 	else
// 	{
// 		unsigned char ucData=(unsigned char)packet->data;
// 		if(ucData&1)
// 		{
// 			// hide gamertags
// 			pMinecraft->options->SetGamertagSetting(true);
// 		}
// 		else
// 		{
// 			pMinecraft->options->SetGamertagSetting(false);
// 		}
//
// 		for (unsigned int i = 0; i < players.size(); i++)
// 		{
// 			shared_ptr<PlayerConnection> playerconnection = players[i];
// 			playerconnection->setShowOnMaps(pMinecraft->options->GetGamertagSetting());
// 		}
// 	}
}

vector< shared_ptr<PlayerConnection> >  * ServerConnection::getPlayers()
{
	return &players;
}

vector< shared_ptr<PlayerConnection> > ServerConnection::getPlayersSnapshot()
{
	EnterCriticalSection(&players_cs);
	vector< shared_ptr<PlayerConnection> > snapshot = players;
	LeaveCriticalSection(&players_cs);
	return snapshot;
}

void ServerConnection::sortPlayersByChunkPriority()
{
	EnterCriticalSection(&players_cs);
	if( players.size() )
	{
		vector< shared_ptr<PlayerConnection> > playersOrig = players;
		players.clear();

		do
		{
			int longestTime = 0;
			auto playerConnectionBest = playersOrig.begin();
			for( auto it = playersOrig.begin(); it != playersOrig.end(); it++)
			{
				int thisTime = 0;
				INetworkPlayer *np = (*it)->getNetworkPlayer();
				if( np )
				{
					thisTime = np->GetTimeSinceLastChunkPacket_ms();
				}

				if( thisTime > longestTime )
				{
					playerConnectionBest = it;
					longestTime = thisTime;
				}
			}
			players.push_back(*playerConnectionBest);
			playersOrig.erase(playerConnectionBest);
		} while ( playersOrig.size() > 0 );
	}
	LeaveCriticalSection(&players_cs);
}