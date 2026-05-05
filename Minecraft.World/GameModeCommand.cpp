#include "stdafx.h"
#include "net.minecraft.commands.h"
#include "GameModeCommand.h"
#include "../Minecraft.Client/MinecraftServer.h"
#include "../Minecraft.Client/ServerPlayer.h"
#include "../Minecraft.Client/PlayerList.h"
#include "LevelSettings.h"
#include "net.minecraft.network.packet.h"

EGameCommand GameModeCommand::getId()
{
	return eGameCommand_GameMode;
}

int GameModeCommand::getPermissionLevel()
{
	return LEVEL_GAMEMASTERS;
}

void GameModeCommand::execute(shared_ptr<CommandSender> source, byteArray commandData)
{
	ByteArrayInputStream bais(commandData);
	DataInputStream dis(&bais);
	PlayerUID uid = dis.readPlayerUID();
	int modeId = dis.readInt();
	shared_ptr<ServerPlayer> player = MinecraftServer::getInstance()->getPlayers()->getPlayer(uid);
	if (player != nullptr)
	{
		GameType *newMode = GameType::byId(modeId);
		if (newMode != nullptr)
		{
			player->setGameMode(newMode);
			player->resetLastActionTime();
			source->sendMessage(L"Set " + player->getName() + L"'s game mode to " + newMode->getName());
		}
	}
}

shared_ptr<GameCommandPacket> GameModeCommand::preparePacket(shared_ptr<Player> player, int gameMode)
{
	if (player == nullptr) return nullptr;
	ByteArrayOutputStream baos;
	DataOutputStream dos(&baos);
	dos.writePlayerUID(player->getXuid());
	dos.writeInt(gameMode);
	return std::make_shared<GameCommandPacket>(eGameCommand_GameMode, baos.toByteArray());
}

GameType *GameModeCommand::getModeForString(shared_ptr<CommandSender> source, const wstring &name)
{
	return GameType::byName(name);
}