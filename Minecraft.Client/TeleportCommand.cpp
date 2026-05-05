#include "stdafx.h"
#include "MinecraftServer.h"
#include "PlayerList.h"
#include "ServerPlayer.h"
#include "PlayerConnection.h"
#include "../Minecraft.World/net.minecraft.commands.h"
#include "../Minecraft.World/net.minecraft.network.packet.h"
#include "../Minecraft.World/net.minecraft.world.level.h"
#include "../Minecraft.World/net.minecraft.world.level.dimension.h"
#include "TeleportCommand.h"
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
#include "../Minecraft.Server/FourKitBridge.h"
#endif

EGameCommand TeleportCommand::getId()
{
	return eGameCommand_Teleport;
}

void TeleportCommand::execute(shared_ptr<CommandSender> source, byteArray commandData)
{
    ByteArrayInputStream bais(commandData);
    DataInputStream dis(&bais);
    byte flag = dis.readByte();
    PlayerUID subjectID = dis.readPlayerUID();
    PlayerList *players = MinecraftServer::getInstance()->getPlayerList();
    shared_ptr<ServerPlayer> subject = players->getPlayer(subjectID);
    if (subject == nullptr || !subject->isAlive())
        return;

    if (flag == 1)
    {
        float x   = dis.readFloat();
        float y   = dis.readFloat();
        float z   = dis.readFloat();
        byte yRot = dis.readByte();
        byte xRot = dis.readByte();

        subject->ride(nullptr);
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
        {
            double outX, outY, outZ;
            bool cancelled = FourKitBridge::FirePlayerTeleport(subject->entityId,
                subject->x, subject->y, subject->z, subject->dimension,
                x, y, z, subject->dimension,
                1, &outX, &outY, &outZ);
            if (cancelled)
                return;
            subject->connection->teleport(outX, outY, outZ, yRot, xRot);
        }
#else
        subject->connection->teleport(x, y, z, yRot, xRot);
#endif
        logAdminAction(source, ChatPacket::e_ChatCommandTeleportSuccess, subject->getName(), eTYPE_SERVERPLAYER, subject->getName());
    }
    else
    {
        PlayerUID destinationID = dis.readPlayerUID();
        shared_ptr<ServerPlayer> destination = players->getPlayer(destinationID);
        if (destination == nullptr)
            return;

        if (subject->level->dimension->id != destination->level->dimension->id)
            return;

        subject->ride(nullptr);
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
        {
            double outX, outY, outZ;
            bool cancelled = FourKitBridge::FirePlayerTeleport(subject->entityId,
                subject->x, subject->y, subject->z, subject->dimension,
                destination->x, destination->y, destination->z, destination->dimension,
                1, &outX, &outY, &outZ);
            if (cancelled)
                return;
            subject->connection->teleport(outX, outY, outZ, destination->yRot, destination->xRot);
        }
#else
        subject->connection->teleport(destination->x, destination->y, destination->z, destination->yRot, destination->xRot);
#endif
        logAdminAction(source, ChatPacket::e_ChatCommandTeleportSuccess, subject->getName(), eTYPE_SERVERPLAYER, destination->getName());
        if (subject == source)
            destination->sendMessage(subject->getName(), ChatPacket::e_ChatCommandTeleportToMe);
        else
            subject->sendMessage(destination->getName(), ChatPacket::e_ChatCommandTeleportMe);
    }
}

shared_ptr<GameCommandPacket> TeleportCommand::preparePacket(PlayerUID subject, PlayerUID destination)
{
    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);
    dos.writeByte(0);
    dos.writePlayerUID(subject);
    dos.writePlayerUID(destination);
    return std::make_shared<GameCommandPacket>(eGameCommand_Teleport, baos.toByteArray());
}


//neo: added
shared_ptr<GameCommandPacket> TeleportCommand::preparePacket(PlayerUID subject, float x, float y, float z, byte yRot, byte xRot)
{
    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);
    dos.writeByte(1);
    dos.writePlayerUID(subject);
    dos.writeFloat(x);
    dos.writeFloat(y);
    dos.writeFloat(z);
    dos.writeByte(yRot);
    dos.writeByte(xRot);
    return std::make_shared<GameCommandPacket>(eGameCommand_Teleport, baos.toByteArray());
}