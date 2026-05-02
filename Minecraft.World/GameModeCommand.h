#pragma once

#include "Command.h"
class GameType;
class GameCommandPacket;

class GameModeCommand : public Command
{
public:
	virtual EGameCommand getId();
	int getPermissionLevel();
	virtual void execute(shared_ptr<CommandSender> source, byteArray commandData);
	static shared_ptr<GameCommandPacket> preparePacket(shared_ptr<Player> player, int gameMode);

protected:
	GameType *getModeForString(shared_ptr<CommandSender> source, const wstring &name);
};