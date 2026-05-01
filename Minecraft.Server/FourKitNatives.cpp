#include "FourKitNatives.h"
#include "FourKitBridge.h"
#include "Common/StringUtils.h"
#include "stdafx.h"

#include <atomic>
#include <string>
#include <vector>

#include "../Minecraft.Client/MinecraftServer.h"
#include "../Minecraft.Client/PlayerConnection.h"
#include "../Minecraft.Client/PlayerList.h"
#include "../Minecraft.Client/ServerConnection.h"
#include "../Minecraft.Client/ServerLevel.h"
#include "../Minecraft.Client/ServerPlayer.h"
#include "../Minecraft.Client/ServerPlayerGameMode.h"
#include "../Minecraft.Client/ServerChunkCache.h"
#include "../Minecraft.World/LevelChunk.h"
#include "../Minecraft.World/Biome.h"
#include "../Minecraft.World/LightLayer.h"
#include "../Minecraft.Client/Windows64/Network/WinsockNetLayer.h"
#include "../Minecraft.World/AbstractContainerMenu.h"
#include "../Minecraft.World/AddGlobalEntityPacket.h"
#include "../Minecraft.World/ArrayWithLength.h"
#include "../Minecraft.World/Class.h"
#include "../Minecraft.World/CompoundContainer.h"
#include "../Minecraft.World/Connection.h"
#include "../Minecraft.World/DamageSource.h"
#include "../Minecraft.World/Explosion.h"
#include "../Minecraft.World/ItemEntity.h"
#include "../Minecraft.World/ItemInstance.h"
#include "../Minecraft.World/LevelData.h"
#include "../Minecraft.World/LightningBolt.h"
#include "../Minecraft.World/Player.h"
#include "../Minecraft.World/PlayerAbilitiesPacket.h"
#include "../Minecraft.World/SetCarriedItemPacket.h"
#include "../Minecraft.World/BlockRegionUpdatePacket.h"
#include "../Minecraft.World/SetExperiencePacket.h"
#include "../Minecraft.World/SetHealthPacket.h"
#include "../Minecraft.World/LevelSoundPacket.h"
#include "../Minecraft.World/LevelParticlesPacket.h"
#include "../Minecraft.Client/ParticleType.h"
#include "../Minecraft.World/SetEntityLinkPacket.h"
#include "../Minecraft.World/SimpleContainer.h"
#include "../Minecraft.World/Slot.h"
#include "../Minecraft.World/Tile.h"
#include "../Minecraft.World/net.minecraft.world.entity.player.h"
#include "Access/Access.h"
#include "Common/NetworkUtils.h"
#include "ServerLogManager.h"
#include "../Minecraft.World/ItemInstance.cpp"

namespace
{

std::atomic<uint32_t> g_handlerMask{0};

static shared_ptr<ServerPlayer> FindPlayer(int entityId)
{
    PlayerList *list = MinecraftServer::getPlayerList();
    if (!list)
        return nullptr;
    for (auto &p : list->players)
    {
        if (p && p->entityId == entityId)
            return p;
    }
    return nullptr;
}

static shared_ptr<Entity> FindEntity(int entityId)
{
    MinecraftServer *srv = MinecraftServer::getInstance();
    if (!srv)
        return nullptr;
    const int dims[] = {0, -1, 1};
    for (int dim : dims)
    {
        ServerLevel *level = srv->getLevel(dim);
        if (!level)
            continue;
        shared_ptr<Entity> entity = level->getEntity(entityId);
        if (entity)
            return entity;
    }
    return nullptr;
}

static ServerLevel *GetLevel(int dimId)
{
    MinecraftServer *srv = MinecraftServer::getInstance();
    if (!srv)
        return nullptr;
    return srv->getLevel(dimId);
}

class VirtualContainer : public SimpleContainer
{
    int m_containerType;

  public:
    VirtualContainer(int containerType, const std::wstring &name, int size)
        : SimpleContainer(0, name, !name.empty(), size), m_containerType(containerType)
    {
    }
    virtual int getContainerType() override
    {
        return m_containerType;
    }
};

}

namespace FourKitBridge
{

void __cdecl NativeSetHandlerMask(uint32_t mask)
{
    g_handlerMask.store(mask, std::memory_order_release);
}

bool HasHandlers(int kind)
{
    if (kind < 0 || kind >= 32) return false;
    return (g_handlerMask.load(std::memory_order_acquire) & (1u << kind)) != 0;
}

int __cdecl NativeGetServerTickCount()
{
    MinecraftServer *srv = MinecraftServer::getInstance();
    return srv ? srv->tickCount : 0;
}

void __cdecl NativeDamagePlayer(int entityId, float amount)
{
    auto player = FindPlayer(entityId);
    if (player)
    {
        player->hurt(DamageSource::genericSource, amount);
        return;
    }
    auto entity = FindEntity(entityId);
    if (entity)
    {
        entity->hurt(DamageSource::genericSource, amount);
    }
}

void __cdecl NativeSetPlayerHealth(int entityId, float health)
{
    auto player = FindPlayer(entityId);
    if (player)
    {
        player->setHealth(health);
    }
}

void __cdecl NativeTeleportPlayer(int entityId, double x, double y, double z)
{
    auto player = FindPlayer(entityId);
    if (player && player->connection)
    {
        double outX, outY, outZ;
        bool cancelled = FirePlayerTeleport(entityId,
                                            player->x, player->y, player->z, player->dimension,
                                            x, y, z, player->dimension,
                                            2 /* PLUGIN */,
                                            &outX, &outY, &outZ);
        if (!cancelled)
        {
            player->connection->teleport(outX, outY, outZ, player->yRot, player->xRot);
        }
    }
}

void __cdecl NativeSetPlayerGameMode(int entityId, int gameMode)
{
    auto player = FindPlayer(entityId);
    if (player && player->gameMode)
    {
        GameType *type = GameType::byId(gameMode);
        if (type)
        {
            player->setGameMode(type);
        }
    }
}

void __cdecl NativeBroadcastMessage(const char *utf8, int len)
{
    if (!utf8 || len <= 0)
        return;
    std::wstring wide = ServerRuntime::StringUtils::Utf8ToWide(utf8);
    if (wide.empty())
        return;
    PlayerList *list = MinecraftServer::getPlayerList();
    if (list)
    {
        list->broadcastAll(std::make_shared<ChatPacket>(wide));
    }
}

void __cdecl NativeSetFallDistance(int entityId, float distance)
{
    auto player = FindPlayer(entityId);
    if (player)
    {
        player->fallDistance = distance;
    }
}

// double[27] = { x, y, z, health, maxHealth, fallDistance, gameMode, walkSpeed, yaw, pitch, dimension, isSleeping, sleepTimer, sneaking, sprinting, onGround, velocityX, velocityY, velocityZ, allowFlight, sleepingIgnored, experienceLevel, experienceProgress, totalExperience, foodLevel, saturation, exhaustion }
void __cdecl NativeGetPlayerSnapshot(int entityId, double *outData)
{
    auto player = FindPlayer(entityId);
    if (!player)
    {
        memset(outData, 0, 27 * sizeof(double));
        outData[3] = 20.0;
        outData[4] = 20.0;
        outData[7] = 0.1;
        outData[24] = 20.0;
        outData[25] = 5.0;
        return;
    }
    outData[0] = player->x;
    outData[1] = player->y;
    outData[2] = player->z;
    outData[3] = (double)player->getHealth();
    outData[4] = (double)player->getMaxHealth();
    outData[5] = (double)player->fallDistance;
    GameType *gm = player->gameMode ? player->gameMode->getGameModeForPlayer() : GameType::SURVIVAL;
    outData[6] = (double)(gm ? gm->getId() : 0);
    outData[7] = (double)player->abilities.getWalkingSpeed();
    outData[8] = (double)player->yRot;
    outData[9] = (double)player->xRot;
    outData[10] = (double)player->dimension;
    outData[11] = player->isSleeping() ? 1.0 : 0.0;
    outData[12] = (double)player->getSleepTimer();
    outData[13] = player->isSneaking() ? 1.0 : 0.0;
    outData[14] = player->isSprinting() ? 1.0 : 0.0;
    outData[15] = player->onGround ? 1.0 : 0.0;
    outData[16] = player->xd;
    outData[17] = player->yd;
    outData[18] = player->zd;
    outData[19] = player->abilities.mayfly ? 1.0 : 0.0;
    outData[20] = player->fk_sleepingIgnored ? 1.0 : 0.0;
    outData[21] = (double)player->experienceLevel;
    outData[22] = (double)player->experienceProgress;
    outData[23] = (double)player->totalExperience;
    FoodData *fd = player->getFoodData();
    outData[24] = fd ? (double)fd->getFoodLevel() : 20.0;
    outData[25] = fd ? (double)fd->getSaturationLevel() : 5.0;
    outData[26] = fd ? (double)fd->getExhaustionLevel() : 0.0;
}

void __cdecl NativeSendMessage(int entityId, const char *utf8, int len)
{
    if (!utf8 || len <= 0)
        return;
    auto player = FindPlayer(entityId);
    if (player && player->connection)
    {
        std::wstring wide = ServerRuntime::StringUtils::Utf8ToWide(utf8);
        if (!wide.empty())
        {
            player->connection->send(std::make_shared<ChatPacket>(wide));
        }
    }
}

void __cdecl NativeSetWalkSpeed(int entityId, float speed)
{
    auto player = FindPlayer(entityId);
    if (player)
    {
        player->abilities.setWalkingSpeed(speed);
        if (player->connection)
        {
            player->connection->send(std::make_shared<PlayerAbilitiesPacket>(&player->abilities));
        }
    }
}

void __cdecl NativeTeleportEntity(int entityId, int dimId, double x, double y, double z)
{
    auto player = FindPlayer(entityId);
    if (player && player->connection)
    {
        double outX = x, outY = y, outZ = z;
        bool cancelled = FirePlayerTeleport(entityId, 
            player->x, player->y, player->z, player->dimension,
            x, y, z, dimId,
            2 /* PLUGIN */,
            &outX, &outY, &outZ
        );

        if (!cancelled)
        {
            if (dimId != player->dimension)
            {
                MinecraftServer::getInstance()->getPlayers()->toggleDimension(player, dimId);
            }
            player->connection->teleport(outX, outY, outZ, player->yRot, player->xRot);
        }
        return;
    }
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return;
    shared_ptr<Entity> entity = level->getEntity(entityId);
    if (entity)
    {
        entity->moveTo(x, y, z, entity->yRot, entity->xRot);
    }
}

int __cdecl NativeGetTileId(int dimId, int x, int y, int z)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;
    return level->getTile(x, y, z);
}

int __cdecl NativeGetTileData(int dimId, int x, int y, int z)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;
    return level->getData(x, y, z);
}

void __cdecl NativeSetTile(int dimId, int x, int y, int z, int tileId, int data, int flags)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return;
    level->setTileAndData(x, y, z, tileId, data, flags);
}

void __cdecl NativeSetTileData(int dimId, int x, int y, int z, int data, int flags)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return;
    level->setData(x, y, z, data, flags);
}

int __cdecl NativeBreakBlock(int dimId, int x, int y, int z)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;
    if (level->getTile(x, y, z) == 0)
        return 0;
    return level->destroyTile(x, y, z, true) ? 1 : 0;
}

int __cdecl NativeGetHighestBlockY(int dimId, int x, int z)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;
    return level->getHeightmap(x, z);
}

// double[7] = { spawnX, spawnY, spawnZ, seed, dayTime, isRaining, isThundering }
void __cdecl NativeGetWorldInfo(int dimId, double *outBuf)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
    {
        memset(outBuf, 0, 7 * sizeof(double));
        return;
    }
    LevelData *ld = level->getLevelData();
    Pos *spawn = level->getSharedSpawnPos();
    outBuf[0] = spawn ? (double)spawn->x : 0.0;
    outBuf[1] = spawn ? (double)spawn->y : 64.0;
    outBuf[2] = spawn ? (double)spawn->z : 0.0;
    outBuf[3] = (double)level->getSeed();
    outBuf[4] = (double)level->getDayTime();
    outBuf[5] = ld && ld->isRaining() ? 1.0 : 0.0;
    outBuf[6] = ld && ld->isThundering() ? 1.0 : 0.0;
}

void __cdecl NativeSetWorldTime(int dimId, int64_t time)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return;
    level->setDayTime(time);
}

void __cdecl NativeSetWeather(int dimId, int storm, int thundering, int thunderDuration)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return;
    LevelData *ld = level->getLevelData();
    if (!ld)
        return;
    if (storm >= 0)
        ld->setRaining(storm != 0);
    if (thundering >= 0)
        ld->setThundering(thundering != 0);
    if (thunderDuration >= 0)
        ld->setThunderTime(thunderDuration);
}

int __cdecl NativeCreateExplosion(int dimId, double x, double y, double z, float power, int setFire, int breakBlocks)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;
    Explosion explosion(level, nullptr, x, y, z, power);
    explosion.fire = (setFire != 0);
    explosion.destroyBlocks = (breakBlocks != 0);
    explosion.explode();
    explosion.finalizeExplosion(true);
    return 1;
}

int __cdecl NativeStrikeLightning(int dimId, double x, double y, double z, int effectOnly)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;

    std::shared_ptr<Entity> lightning = std::shared_ptr<Entity>(new LightningBolt(level, x, y, z));

    if (effectOnly != 0)
    {
        PlayerList *playerList = MinecraftServer::getPlayerList();
        if (playerList == NULL)
            return 0;
        playerList->broadcast(x, y, z, 512.0, dimId, std::shared_ptr<Packet>(new AddGlobalEntityPacket(lightning)));
        level->playSound(x, y, z, eSoundType_AMBIENT_WEATHER_THUNDER, 10000, 0.8f + level->random->nextFloat() * 0.2f);
        level->playSound(x, y, z, eSoundType_RANDOM_EXPLODE, 2, 0.5f + level->random->nextFloat() * 0.2f);
        return 1;
    }

    return level->addGlobalEntity(lightning) ? 1 : 0;
}

int __cdecl NativeSetSpawnLocation(int dimId, int x, int y, int z)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;
    level->setSpawnPos(x, y, z);
    return 1;
}

void __cdecl NativeDropItem(int dimId, double x, double y, double z, int itemId, int count, int auxValue, int naturally)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return;
    if (itemId <= 0 || count <= 0)
        return;

    auto itemInstance = std::make_shared<ItemInstance>(itemId, count, auxValue);
    double spawnX = x, spawnY = y, spawnZ = z;
    if (naturally)
    {
        float s = 0.7f;
        spawnX += level->random->nextFloat() * s + (1 - s) * 0.5;
        spawnY += level->random->nextFloat() * s + (1 - s) * 0.5;
        spawnZ += level->random->nextFloat() * s + (1 - s) * 0.5;
    }

    auto item = std::make_shared<ItemEntity>(level, spawnX, spawnY, spawnZ, itemInstance);
    item->throwTime = 10;
    level->addEntity(item);
}

void __cdecl NativeKickPlayer(int entityId, int reason)
{
    auto player = FindPlayer(entityId);
    if (player && player->connection)
    {
        DisconnectPacket::eDisconnectReason r = static_cast<DisconnectPacket::eDisconnectReason>(reason);
        player->connection->disconnect(r);
    }
}

int __cdecl NativeBanPlayer(int entityId, const char *reasonUtf8, int reasonByteLen)
{
    if (!ServerRuntime::Access::IsInitialized())
        return 0;

    auto player = FindPlayer(entityId);
    if (!player)
        return 0;

    std::vector<PlayerUID> xuids;
    PlayerUID xuid1 = player->getXuid();
    PlayerUID xuid2 = player->getOnlineXuid();
    if (xuid1 != INVALID_XUID)
        xuids.push_back(xuid1);
    if (xuid2 != INVALID_XUID && xuid2 != xuid1)
        xuids.push_back(xuid2);

    if (xuids.empty())
        return 0;

    std::string reason = (reasonUtf8 && reasonByteLen > 0) ? std::string(reasonUtf8, reasonByteLen) : "Banned by plugin.";
    std::string playerName = ServerRuntime::StringUtils::WideToUtf8(player->getName());

    ServerRuntime::Access::BanMetadata metadata = ServerRuntime::Access::BanManager::BuildDefaultMetadata("Plugin");
    metadata.reason = reason;

    for (auto xuid : xuids)
    {
        if (!ServerRuntime::Access::IsPlayerBanned(xuid))
            ServerRuntime::Access::AddPlayerBan(xuid, playerName, metadata);
    }

    if (player->connection)
        player->connection->disconnect(DisconnectPacket::eDisconnect_Banned);

    return 1;
}

int __cdecl NativeBanPlayerIp(int entityId, const char *reasonUtf8, int reasonByteLen)
{
    if (!ServerRuntime::Access::IsInitialized())
        return 0;

    auto player = FindPlayer(entityId);
    if (!player || !player->connection || !player->connection->connection || !player->connection->connection->getSocket())
        return 0;

    unsigned char smallId = player->connection->connection->getSocket()->getSmallId();
    if (smallId == 0)
        return 0;

    std::string playerIp;
    if (!ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(smallId, &playerIp))
        return 0;

    std::string reason = (reasonUtf8 && reasonByteLen > 0) ? std::string(reasonUtf8, reasonByteLen) : "Banned by plugin.";

    ServerRuntime::Access::BanMetadata metadata = ServerRuntime::Access::BanManager::BuildDefaultMetadata("Plugin");
    metadata.reason = reason;

    std::string normalizedIp = ServerRuntime::NetworkUtils::NormalizeIpToken(playerIp);
    if (ServerRuntime::Access::IsIpBanned(normalizedIp))
        return 0;

    if (!ServerRuntime::Access::AddIpBan(normalizedIp, metadata))
        return 0;

    PlayerList *list = MinecraftServer::getPlayerList();
    if (list)
    {
        std::vector<std::shared_ptr<ServerPlayer>> snapshot = list->players;
        for (auto &p : snapshot)
        {
            if (!p || !p->connection || !p->connection->connection || !p->connection->connection->getSocket())
                continue;
            unsigned char sid = p->connection->connection->getSocket()->getSmallId();
            if (sid == 0)
                continue;
            std::string pIp;
            if (!ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(sid, &pIp))
                continue;
            if (ServerRuntime::NetworkUtils::NormalizeIpToken(pIp) == normalizedIp)
            {
                if (p->connection)
                    p->connection->disconnect(DisconnectPacket::eDisconnect_Banned);
            }
        }
    }

    return 1;
}

int __cdecl NativeGetPlayerAddress(int entityId, char *outIpBuf, int outIpBufSize, int *outPort)
{
    if (outPort)
        *outPort = 0;
    if (outIpBuf && outIpBufSize > 0)
        outIpBuf[0] = '\0';

    auto player = FindPlayer(entityId);
    if (!player || !player->connection || !player->connection->connection || !player->connection->connection->getSocket())
        return 0;

    unsigned char smallId = player->connection->connection->getSocket()->getSmallId();
    if (smallId == 0)
        return 0;

    std::string playerIp;
    if (!ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(smallId, &playerIp))
    {
        SOCKET sock = WinsockNetLayer::GetSocketForSmallId(smallId);
        if (sock != INVALID_SOCKET)
        {
            sockaddr_in addr;
            int addrLen = sizeof(addr);
            if (getpeername(sock, (sockaddr *)&addr, &addrLen) == 0)
            {
                char ipBuf[64] = {};
                if (inet_ntop(AF_INET, &addr.sin_addr, ipBuf, sizeof(ipBuf)))
                {
                    playerIp = ipBuf;
                    if (outPort)
                        *outPort = (int)ntohs(addr.sin_port);
                }
            }
        }
        if (playerIp.empty())
            return 0;
    }
    else
    {
        SOCKET sock = WinsockNetLayer::GetSocketForSmallId(smallId);
        if (sock != INVALID_SOCKET && outPort)
        {
            sockaddr_in addr;
            int addrLen = sizeof(addr);
            if (getpeername(sock, (sockaddr *)&addr, &addrLen) == 0)
                *outPort = (int)ntohs(addr.sin_port);
        }
    }

    if (outIpBuf && outIpBufSize > 0)
    {
        int copyLen = (int)playerIp.size();
        if (copyLen >= outIpBufSize)
            copyLen = outIpBufSize - 1;
        memcpy(outIpBuf, playerIp.c_str(), copyLen);
        outIpBuf[copyLen] = '\0';
    }

    return 1;
}

int __cdecl NativeGetPlayerLatency(int entityId)
{
    auto player = FindPlayer(entityId);
    if (!player) return -1;

    return player->latency;
}

int __cdecl NativeSendRaw(int entityId, unsigned char *bufferData, int bufferSize)
{
    auto player = FindPlayer(entityId);
    if (!player) return -1;

    if (!player->connection || !player->connection->connection)
        return -1;

    player->connection->connection->send(bufferData, bufferSize);
}

void WriteInventoryItemData(std::shared_ptr<ItemInstance> item, int index, int* outBuffer) {
    if (item) {
        //ItemFlags Key:
        // 0x1 = hasMetadata (has data that needs to be gotten from "ReadMetaFromNative")

        uint8_t itemFlags = 0;
        if (item->getTag() == nullptr) goto doneWithMetadataFlag;
        CompoundTag* itemTag = item->getTag();

        if (itemTag->contains(L"ench")) {
            itemFlags |= 0x1;
            goto doneWithMetadataFlag;
        }
        else { //we just want to check one tag for metadata and return for this flag, not all of them
            CompoundTag* displayTag = itemTag->getCompound(L"display");
            if (displayTag->contains(L"Name") || displayTag->contains(L"Lore")) {
                itemFlags |= 0x1;
                goto doneWithMetadataFlag;
            }
        }


    doneWithMetadataFlag:

        outBuffer[(index * 3) + 0] = item->id;
        outBuffer[(index * 3) + 1] = item->getAuxValue();
        outBuffer[(index * 3) + 2] = (((int)itemFlags << 24) | ((int)item->count << 8));
    }
}

void __cdecl NativeGetPlayerInventory(int entityId, int *outData)
{
    // 9 slots per row, 3 slots in the inventory and the hotbar, 4 armor slots, 1 hand slot
    // (((slotsPerRow * Rows) + ArmorSlots) * AmountOfIntsPerSlot) + hand slot
    // (((9 * 4) + 4) * 3) + 1 = 121
    memset(outData, 0, 121 * sizeof(int));

    auto player = FindPlayer(entityId);
    if (!player || !player->inventory)
        return;

    unsigned int size = player->inventory->getContainerSize();
    if (size > 40)
        size = 40;

    for (unsigned int i = 0; i < size; i++)
    {
        WriteInventoryItemData(player->inventory->getItem(i), i, outData);
    }

    outData[120] = player->inventory->selected;
}

void __cdecl NativeSetPlayerInventorySlot(int entityId, int slot, int itemId, int count, int aux)
{
    auto player = FindPlayer(entityId);
    if (!player || !player->inventory)
        return;

    if (itemId <= 0 || count <= 0)
        player->inventory->setItem(slot, nullptr);
    else
        player->inventory->setItem(slot, std::make_shared<ItemInstance>(itemId, count, aux));
}

void __cdecl NativeGetContainerContents(int entityId, int *outData, int maxSlots)
{
    memset(outData, 0, maxSlots * 3 * sizeof(int));

    auto player = FindPlayer(entityId);
    if (!player || !player->containerMenu || player->containerMenu == player->inventoryMenu)
        return;

    auto *menu = player->containerMenu;
    auto *items = menu->getItems();
    int count = (int)items->size();
    if (count > maxSlots)
        count = maxSlots;

    for (int i = 0; i < count; i++)
    {
        WriteInventoryItemData((*items)[i], i, outData);
    }
    delete items;
}

void __cdecl NativeSetContainerSlot(int entityId, int slot, int itemId, int count, int aux)
{
    auto player = FindPlayer(entityId);
    if (!player || !player->containerMenu || player->containerMenu == player->inventoryMenu)
        return;

    auto *menu = player->containerMenu;
    if (slot < 0 || slot >= (int)menu->slots.size())
        return;

    if (itemId <= 0 || count <= 0)
        menu->setItem(slot, nullptr);
    else
        menu->setItem(slot, std::make_shared<ItemInstance>(itemId, count, aux));

    menu->broadcastChanges();
}

void __cdecl NativeGetContainerViewerEntityIds(int entityId, int *outIds, int maxCount, int *outCount)
{
    *outCount = 0;

    auto player = FindPlayer(entityId);
    if (!player || !player->containerMenu || player->containerMenu == player->inventoryMenu)
        return;

    auto *menu = player->containerMenu;
    if (menu->slots.empty())
        return;

    Container *myContainer = menu->slots[0]->container.get();
    if (!myContainer)
        return;

    CompoundContainer *myCompound = dynamic_cast<CompoundContainer *>(myContainer);
    if (myCompound)
        myContainer = myCompound->getFirstContainer().get();

    PlayerList *list = MinecraftServer::getPlayerList();
    if (!list)
        return;

    int count = 0;
    for (auto &p : list->players)
    {
        if (!p || !p->containerMenu || p->containerMenu == p->inventoryMenu)
            continue;
        if (p->containerMenu->slots.empty())
            continue;
        Container *theirContainer = p->containerMenu->slots[0]->container.get();
        CompoundContainer *theirCompound = dynamic_cast<CompoundContainer *>(theirContainer);
        if (theirCompound)
            theirContainer = theirCompound->getFirstContainer().get();
        if (theirContainer == myContainer && count < maxCount)
            outIds[count++] = p->entityId;
    }
    *outCount = count;
}

void __cdecl NativeCloseContainer(int entityId)
{
    auto player = FindPlayer(entityId);
    if (player && player->containerMenu != player->inventoryMenu)
        player->closeContainer();
}

void __cdecl NativeOpenVirtualContainer(int entityId, int nativeType, const char *titleUtf8, int titleByteLen, int slotCount, int *itemsBuf)
{
    auto player = FindPlayer(entityId);
    if (!player)
        return;

    if (player->containerMenu != player->inventoryMenu)
        player->closeContainer();

    std::wstring title = ServerRuntime::StringUtils::Utf8ToWide(std::string(titleUtf8, titleByteLen));
    auto container = std::make_shared<VirtualContainer>(nativeType, title, slotCount);

    for (int i = 0; i < slotCount; i++)
    {
        int id = itemsBuf[i * 3];
        int count = itemsBuf[i * 3 + 1];
        int aux = itemsBuf[i * 3 + 2];
        if (id > 0 && count > 0)
            container->setItem(i, std::make_shared<ItemInstance>(id, count, aux));
    }

    player->openContainer(container);
}
//didnt update this for enchants
// [nameLen:int32][nameUTF8:bytes][loreCount:int32][lore0Len:int32][lore0UTF8:bytes]
int __cdecl NativeGetItemMeta(int entityId, int slot, char *outBuf, int bufSize)
{
    auto player = FindPlayer(entityId);
    if (!player || !player->inventory)
        return 0;

    unsigned int size = player->inventory->getContainerSize();
    if (slot < 0 || slot >= (int)size)
        return 0;

    auto item = player->inventory->getItem(slot);
    if (!item || !item->hasTag())
        return 0;

    CompoundTag *tag = item->getTag();

    bool hasEnchantments = item->isEnchanted();

    CompoundTag *display = (tag && tag->contains(L"display")) ? tag->getCompound(L"display") : nullptr;
    bool hasName = display && display->contains(L"Name");
    bool hasLore = display && display->contains(L"Lore");

    if (!hasName && !hasLore && !hasEnchantments)
        return 0;

    int offset = 0;

    if (hasName)
    {
        std::wstring wname = display->getString(L"Name");
        std::string nameUtf8 = ServerRuntime::StringUtils::WideToUtf8(wname);
        int nameLen = (int)nameUtf8.size();
        if (offset + 4 + nameLen > bufSize) return 0;
        memcpy(outBuf + offset, &nameLen, 4);
        offset += 4;
        memcpy(outBuf + offset, nameUtf8.data(), nameLen);
        offset += nameLen;
    }
    else
    {
        int zero = 0;
        if (offset + 4 > bufSize) return 0;
        memcpy(outBuf + offset, &zero, 4);
        offset += 4;
    }

    if (hasLore)
    {
        ListTag<StringTag> *lore = (ListTag<StringTag> *)display->getList(L"Lore");
        int loreCount = lore->size();
        if (offset + 4 > bufSize) return 0;
        memcpy(outBuf + offset, &loreCount, 4);
        offset += 4;

        for (int i = 0; i < loreCount; i++)
        {
            std::wstring wline = lore->get(i)->data;
            std::string lineUtf8 = ServerRuntime::StringUtils::WideToUtf8(wline);
            int lineLen = (int)lineUtf8.size();
            if (offset + 4 + lineLen > bufSize) return 0;
            memcpy(outBuf + offset, &lineLen, 4);
            offset += 4;
            memcpy(outBuf + offset, lineUtf8.data(), lineLen);
            offset += lineLen;
        }
    }
    else
    {
        int zero = 0;
        if (offset + 4 > bufSize) return 0;
        memcpy(outBuf + offset, &zero, 4);
        offset += 4;
    }

    if (hasEnchantments) {
        ListTag<CompoundTag>* list = item->getEnchantmentTags();
        if (list != nullptr) {
            int listSize = list->size();

            if ((offset + 4 + (listSize * (4 + 4))) > bufSize) return 0;

            memcpy(outBuf + offset, &listSize, 4);
            offset += 4;
            for (int i = 0; i < listSize; i++) {
                int type = list->get(i)->getShort((wchar_t*)ItemInstance::TAG_ENCH_ID);
                int level = list->get(i)->getShort((wchar_t*)ItemInstance::TAG_ENCH_LEVEL);

                memcpy(outBuf + offset, &type, 4);
                offset += 4;

                memcpy(outBuf + offset, &level, 4);
                offset += 4;
            }
        }
    }
    else {
        int zero = 0;
        if (offset + 4 > bufSize) return 0;
        memcpy(outBuf + offset, &zero, 4);
        offset += 4;
    }

    return offset;
}

void __cdecl NativeSetItemMeta(int entityId, int slot, const char *inBuf, int bufSize)
{
    auto player = FindPlayer(entityId);
    if (!player || !player->inventory)
        return;

    unsigned int size = player->inventory->getContainerSize();
    if (slot < 0 || slot >= (int)size)
        return;

    auto item = player->inventory->getItem(slot);
    if (!item)
        return;

    if (inBuf == nullptr || bufSize <= 0)
    {
        item->resetHoverName();
        if (item->hasTag())
        {
            CompoundTag *tag = item->getTag();
            if (tag && tag->contains(L"display"))
            {
                CompoundTag *display = tag->getCompound(L"display");
                display->remove(L"Lore");
                if (display->isEmpty())
                {
                    tag->remove(L"display");
                    if (tag->isEmpty())
                        item->setTag(nullptr);
                }
            }

            if (tag && tag->contains(L"ench"))
            {
                tag->remove(L"ench");
            }
        }
        return;
    }

    int offset = 0;

    if (offset + 4 > bufSize) return;
    int nameLen = 0;
    memcpy(&nameLen, inBuf + offset, 4);
    offset += 4;

    if (nameLen > 0)
    {
        if (offset + nameLen > bufSize) return;
        std::string nameUtf8(inBuf + offset, nameLen);
        offset += nameLen;
        std::wstring wname = ServerRuntime::StringUtils::Utf8ToWide(nameUtf8);
        item->setHoverName(wname);
    }
    else
    {
        item->resetHoverName();
    }

    if (offset + 4 > bufSize) return;
    int loreCount = 0;
    memcpy(&loreCount, inBuf + offset, 4);
    offset += 4;

    if (loreCount > 0)
    {
        if (!item->hasTag()) item->setTag(new CompoundTag());
        CompoundTag *tag = item->getTag();
        if (!tag->contains(L"display")) tag->putCompound(L"display", new CompoundTag());
        CompoundTag *display = tag->getCompound(L"display");

        auto *loreList = new ListTag<StringTag>(L"Lore");
        for (int i = 0; i < loreCount; i++)
        {
            if (offset + 4 > bufSize) break;
            int lineLen = 0;
            memcpy(&lineLen, inBuf + offset, 4);
            offset += 4;

            std::wstring wline;
            if (lineLen > 0 && offset + lineLen <= bufSize)
            {
                std::string lineUtf8(inBuf + offset, lineLen);
                offset += lineLen;
                wline = ServerRuntime::StringUtils::Utf8ToWide(lineUtf8);
            }
            loreList->add(new StringTag(L"", wline));
        }
        display->put(L"Lore", loreList);
    }
    else
    {
        if (item->hasTag())
        {
            CompoundTag *tag = item->getTag();
            if (tag && tag->contains(L"display"))
                tag->getCompound(L"display")->remove(L"Lore");
        }
    }

    if (offset + 4 > bufSize) return;
    int enchantCount = 0;
    memcpy(&enchantCount, inBuf + offset, 4);
    offset += 4;

    if (enchantCount > 0)
    {
        if (!item->hasTag()) item->setTag(new CompoundTag());
        CompoundTag* tag = item->getTag();
        if (!tag->contains(L"ench")) tag->put(L"ench", new ListTag<CompoundTag>(L"ench"));
        ListTag<CompoundTag>* enchantments = static_cast<ListTag<CompoundTag> *>(tag->get(L"ench"));

        for (int i = 0; i < enchantCount; i++) {
            if (offset + (4 + 4) > bufSize) break;

            int type = 0;
            memcpy(&type, inBuf + offset, 4);
            offset += 4;

            int level = 0;
            memcpy(&level, inBuf + offset, 4);
            offset += 4;

            CompoundTag* ench = new CompoundTag();
            ench->putShort((wchar_t*)ItemInstance::TAG_ENCH_ID, static_cast<short>(type));
            ench->putShort((wchar_t*)ItemInstance::TAG_ENCH_LEVEL, static_cast<byte>(level));
            enchantments->add(ench);
        }
    }
    else
    {
        if (item->hasTag())
        {
            CompoundTag* tag = item->getTag();
            if (tag && tag->contains(L"ench"))
            {
                tag->remove(L"ench");
            }
        }
    }
}

void __cdecl NativeSetHeldItemSlot(int entityId, int slot)
{
    auto player = FindPlayer(entityId);
    if (!player || !player->inventory) return;
    if (slot < 0 || slot >= Inventory::getSelectionSize()) return;
    player->inventory->selected = slot;
    if (player->connection)
        player->connection->queueSend(std::make_shared<SetCarriedItemPacket>(slot));
}

void __cdecl NativeGetCarriedItem(int entityId, int *outData)
{
    outData[0] = 0;
    outData[1] = 0;
    outData[2] = 0;
    auto player = FindPlayer(entityId);
    if (!player || !player->inventory)
        return;
    auto item = player->inventory->getCarried();
    if (item)
    {
        outData[0] = item->id;
        outData[1] = item->getAuxValue();
        outData[2] = (int)item->count;
    }
}

void __cdecl NativeSetCarriedItem(int entityId, int itemId, int count, int aux)
{
    auto player = FindPlayer(entityId);
    if (!player || !player->inventory)
        return;
    if (itemId <= 0 || count <= 0)
        player->inventory->setCarried(nullptr);
    else
        player->inventory->setCarried(std::make_shared<ItemInstance>(itemId, count, aux));
}

void __cdecl NativeGetEnderChestContents(int entityId, int *outData)
{
    memset(outData, 0, 27 * 3 * sizeof(int));
    auto player = FindPlayer(entityId);
    if (!player)
        return;
    auto ec = player->getEnderChestInventory();
    if (!ec)
        return;
    unsigned int size = ec->getContainerSize();
    if (size > 27)
        size = 27;
    for (unsigned int i = 0; i < size; i++)
    {
        WriteInventoryItemData(ec->getItem(i), i, outData);
    }
}

void __cdecl NativeSetEnderChestSlot(int entityId, int slot, int itemId, int count, int aux)
{
    auto player = FindPlayer(entityId);
    if (!player)
        return;
    auto ec = player->getEnderChestInventory();
    if (!ec)
        return;
    if (slot < 0 || slot >= (int)ec->getContainerSize())
        return;
    if (itemId <= 0 || count <= 0)
        ec->setItem(slot, nullptr);
    else
        ec->setItem(slot, std::make_shared<ItemInstance>(itemId, count, aux));
}

void __cdecl NativeSetSneaking(int entityId, int sneak)
{
    auto player = FindPlayer(entityId);
    if (player)
        player->setSneaking(sneak != 0);
}

void __cdecl NativeSetVelocity(int entityId, double x, double y, double z)
{
    auto player = FindPlayer(entityId);
    if (player)
    {
        player->xd = x;
        player->yd = y;
        player->zd = z;
        player->hurtMarked = true;
        return;
    }
    auto entity = FindEntity(entityId);
    if (entity)
    {
        entity->xd = x;
        entity->yd = y;
        entity->zd = z;
        entity->hurtMarked = true;
    }
}

void __cdecl NativeSetAllowFlight(int entityId, int allowFlight)
{
    auto player = FindPlayer(entityId);
    if (player)
    {
        player->abilities.mayfly = (allowFlight != 0);
        if (!player->abilities.mayfly)
            player->abilities.flying = false;
        if (player->connection)
            player->connection->send(std::make_shared<PlayerAbilitiesPacket>(&player->abilities));
    }
}

void __cdecl NativePlaySound(int entityId, int soundId, double x, double y, double z, float volume, float pitch)
{
    auto player = FindPlayer(entityId);
    if (player && player->connection)
        player->connection->send(std::make_shared<LevelSoundPacket>(soundId, x, y, z, volume, pitch));
}

void __cdecl NativeSetSleepingIgnored(int entityId, int ignored)
{
    auto player = FindPlayer(entityId);
    if (player)
        player->fk_sleepingIgnored = (ignored != 0);
}

void __cdecl NativeSetLevel(int entityId, int level)
{
    auto player = FindPlayer(entityId);
    if (!player) return;
    player->experienceLevel = level;
    if (player->connection)
        player->connection->send(std::make_shared<SetExperiencePacket>(player->experienceProgress, player->totalExperience, player->experienceLevel));
}

void __cdecl NativeSetExp(int entityId, float exp)
{
    auto player = FindPlayer(entityId);
    if (!player) return;
    player->experienceProgress = exp;
    if (player->connection)
        player->connection->send(std::make_shared<SetExperiencePacket>(player->experienceProgress, player->totalExperience, player->experienceLevel));
}

void __cdecl NativeGiveExp(int entityId, int amount)
{
    auto player = FindPlayer(entityId);
    if (!player) return;
    player->increaseXp(amount);
    if (player->connection)
        player->connection->send(std::make_shared<SetExperiencePacket>(player->experienceProgress, player->totalExperience, player->experienceLevel));
}

void __cdecl NativeGiveExpLevels(int entityId, int amount)
{
    auto player = FindPlayer(entityId);
    if (!player) return;
    player->giveExperienceLevels(amount);
    if (player->connection)
        player->connection->send(std::make_shared<SetExperiencePacket>(player->experienceProgress, player->totalExperience, player->experienceLevel));
}

void __cdecl NativeSetFoodLevel(int entityId, int foodLevel)
{
    auto player = FindPlayer(entityId);
    if (!player) return;
    FoodData *fd = player->getFoodData();
    if (!fd) return;
    fd->setFoodLevel(foodLevel);
    if (player->connection)
        player->connection->send(std::make_shared<SetHealthPacket>(player->getHealth(), fd->getFoodLevel(), fd->getSaturationLevel(), eTelemetryChallenges_Unknown));
}

void __cdecl NativeSetSaturation(int entityId, float saturation)
{
    auto player = FindPlayer(entityId);
    if (!player) return;
    FoodData *fd = player->getFoodData();
    if (!fd) return;
    fd->setSaturation(saturation);
    if (player->connection)
        player->connection->send(std::make_shared<SetHealthPacket>(player->getHealth(), fd->getFoodLevel(), fd->getSaturationLevel(), eTelemetryChallenges_Unknown));
}

void __cdecl NativeSetExhaustion(int entityId, float exhaustion)
{
    auto player = FindPlayer(entityId);
    if (!player) return;
    FoodData *fd = player->getFoodData();
    if (!fd) return;
    fd->setExhaustion(exhaustion);
}

void __cdecl NativeSpawnParticle(int entityId, int particleId, float x, float y, float z, float offsetX, float offsetY, float offsetZ, float speed, int count)
{
    auto player = FindPlayer(entityId);
    if (!player || !player->connection) return;
    const ParticleType* type = ParticleType::byId(particleId);
    if (!type) type = ParticleType::getDefault();
    arrayWithLength<int> noParams = { nullptr, 0 };
    player->connection->send(std::make_shared<LevelParticlesPacket>(type, false, x, y, z, offsetX, offsetY, offsetZ, speed, count, noParams));
}

int __cdecl NativeSetPassenger(int entityId, int passengerEntityId)
{
    auto entity = FindEntity(entityId);
    auto passenger = FindEntity(passengerEntityId);
    if (!entity || !passenger) return 0;
    passenger->ride(entity);
    PlayerList *list = MinecraftServer::getPlayerList();
    if (list)
        list->broadcastAll(std::make_shared<SetEntityLinkPacket>(SetEntityLinkPacket::RIDING, passenger, entity), entity->dimension);
    return 1;
}

int __cdecl NativeLeaveVehicle(int entityId)
{
    auto entity = FindEntity(entityId);
    if (!entity || !entity->riding) return 0;
    int dim = entity->riding->dimension;
    entity->ride(nullptr);
    PlayerList *list = MinecraftServer::getPlayerList();
    if (list)
        list->broadcastAll(std::make_shared<SetEntityLinkPacket>(SetEntityLinkPacket::RIDING, entity, nullptr), dim);
    return 1;
}

int __cdecl NativeEject(int entityId)
{
    auto entity = FindEntity(entityId);
    if (!entity) return 0;
    auto riderPtr = entity->rider.lock();
    if (!riderPtr) return 0;
    riderPtr->ride(nullptr);
    PlayerList *list = MinecraftServer::getPlayerList();
    if (list)
        list->broadcastAll(std::make_shared<SetEntityLinkPacket>(SetEntityLinkPacket::RIDING, riderPtr, nullptr), entity->dimension);
    return 1;
}

int __cdecl NativeGetVehicleId(int entityId)
{
    auto entity = FindEntity(entityId);
    if (!entity || !entity->riding) return -1;
    return entity->riding->entityId;
}

int __cdecl NativeGetPassengerId(int entityId)
{
    auto entity = FindEntity(entityId);
    if (!entity) return -1;
    auto riderPtr = entity->rider.lock();
    if (!riderPtr) return -1;
    return riderPtr->entityId;
}

void __cdecl NativeGetEntityInfo(int entityId, double *outData)
{
    outData[0] = -1;
    outData[1] = 0;
    outData[2] = 0;
    outData[3] = 0;
    outData[4] = 0;
    auto entity = FindEntity(entityId);
    if (!entity) return;
    outData[0] = (double)MapEntityType((int)entity->GetType());
    outData[1] = entity->x;
    outData[2] = entity->y;
    outData[3] = entity->z;
    outData[4] = (double)entity->dimension;
}

int __cdecl NativeGetWorldEntities(int dimId, int **outBuf)
{
    *outBuf = nullptr;
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;

    EnterCriticalSection(&level->m_entitiesCS);
    int total = (int)level->entities.size();
    int *buf = (int *)CoTaskMemAlloc(total * 3 * sizeof(int));
    int count = 0;
    if (buf)
    {
        for (auto &entity : level->entities)
        {
            if (!entity)
                continue;
            int idx = count * 3;
            buf[idx] = entity->entityId;
            buf[idx + 1] = MapEntityType((int)entity->GetType());
            buf[idx + 2] = entity->instanceof(eTYPE_LIVINGENTITY) ? 1 : 0;
            count++;
        }
    }
    LeaveCriticalSection(&level->m_entitiesCS);
    *outBuf = buf;
    return count;
}

int __cdecl NativeGetChunkEntities(int dimId, int chunkX, int chunkZ, int **outBuf)
{
    *outBuf = nullptr;
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;

    EnterCriticalSection(&level->m_entitiesCS);
    int total = (int)level->entities.size();
    int *buf = (int *)CoTaskMemAlloc(total * 3 * sizeof(int));
    int count = 0;
    if (buf)
    {
        for (auto &entity : level->entities)
        {
            if (!entity)
                continue;
            int ecx = Mth::floor(entity->x / 16.0);
            int ecz = Mth::floor(entity->z / 16.0);
            if (ecx != chunkX || ecz != chunkZ)
                continue;
            int idx = count * 3;
            buf[idx] = entity->entityId;
            buf[idx + 1] = MapEntityType((int)entity->GetType());
            buf[idx + 2] = entity->instanceof(eTYPE_LIVINGENTITY) ? 1 : 0;
            count++;
        }
    }
    LeaveCriticalSection(&level->m_entitiesCS);
    *outBuf = buf;
    return count;
}

int __cdecl NativeIsChunkLoaded(int dimId, int chunkX, int chunkZ)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level || !level->cache)
        return 0;
    return level->cache->hasChunk(chunkX, chunkZ) ? 1 : 0;
}

int __cdecl NativeLoadChunk(int dimId, int chunkX, int chunkZ, int generate)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level || !level->cache)
        return 0;
    LevelChunk *chunk = level->cache->create(chunkX, chunkZ);
    return (chunk != nullptr) ? 1 : 0;
}

int __cdecl NativeUnloadChunk(int dimId, int chunkX, int chunkZ, int save, int safe)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level || !level->cache)
        return 0;
    if (safe)
    {
        if (!level->cache->hasChunk(chunkX, chunkZ))
            return 0;
        LevelChunk *chunk = level->cache->getChunk(chunkX, chunkZ);
        if (chunk && chunk->containsPlayer())
            return 0;
    }
    level->cache->drop(chunkX, chunkZ);
    return 1;
}

int __cdecl NativeGetLoadedChunks(int dimId, int **coordBuf)
{
    // wow gay
    *coordBuf = nullptr;
    ServerLevel *level = GetLevel(dimId);
    if (!level || !level->cache)
        return 0;

    std::vector<LevelChunk *> *list = level->cache->getLoadedChunkList();

    if (!list)
        return 0;



    int total = (int)list->size();
    int *buf = (int *)CoTaskMemAlloc(total * 2 * sizeof(int));
    int count = 0;

    if (buf)
    {
        for (auto *chunk : *list)
        {
            if (chunk)
            {
                buf[count * 2] = chunk->x;
                buf[count * 2 + 1] = chunk->z;
                count++;
            }
        }
    }

    *coordBuf = buf;
    return count;
}

int __cdecl NativeIsChunkInUse(int dimId, int chunkX, int chunkZ)
{
    PlayerList *list = MinecraftServer::getPlayerList();
    if (!list)
        return 0;
    for (auto &p : list->players)
    {
        if (p && p->dimension == dimId)
        {
            int px = (int)floor(p->x) >> 4;
            int pz = (int)floor(p->z) >> 4;
            if (px == chunkX && pz == chunkZ)
                return 1;
        }
    }
    return 0;
}

void __cdecl NativeGetChunkSnapshot(int dimId, int chunkX, int chunkZ, int *blockIds, int *blockData, int *maxBlockY)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level || !level->cache)
    {
        memset(blockIds, 0, 16 * 128 * 16 * sizeof(int));
        memset(blockData, 0, 16 * 128 * 16 * sizeof(int));
        memset(maxBlockY, 0, 16 * 16 * sizeof(int));
        return;
    }
    if (!level->cache->hasChunk(chunkX, chunkZ))
    {
        memset(blockIds, 0, 16 * 128 * 16 * sizeof(int));
        memset(blockData, 0, 16 * 128 * 16 * sizeof(int));
        memset(maxBlockY, 0, 16 * 16 * sizeof(int));
        return;
    }
    LevelChunk *chunk = level->cache->getChunk(chunkX, chunkZ);
    if (!chunk)
    {
        memset(blockIds, 0, 16 * 128 * 16 * sizeof(int));
        memset(blockData, 0, 16 * 128 * 16 * sizeof(int));
        memset(maxBlockY, 0, 16 * 16 * sizeof(int));
        return;
    }
    for (int lx = 0; lx < 16; lx++)
    {
        for (int lz = 0; lz < 16; lz++)
        {
            int highest = 0;
            for (int ly = 0; ly < 128; ly++)
            {
                int idx = (lx * 128 * 16) + (ly * 16) + lz;
                blockIds[idx] = chunk->getTile(lx, ly, lz);
                blockData[idx] = chunk->getData(lx, ly, lz);
                if (blockIds[idx] != 0)
                    highest = ly;
            }
            maxBlockY[lx * 16 + lz] = highest;
        }
    }
}

int __cdecl NativeUnloadChunkRequest(int dimId, int chunkX, int chunkZ, int safe)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level || !level->cache)
        return 0;
    if (safe)
    {
        if (!level->cache->hasChunk(chunkX, chunkZ))
            return 0;
        LevelChunk *chunk = level->cache->getChunk(chunkX, chunkZ);
        if (chunk && chunk->containsPlayer())
            return 0;
    }
    level->cache->drop(chunkX, chunkZ);
    return 1;
}

int __cdecl NativeRegenerateChunk(int dimId, int chunkX, int chunkZ)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level || !level->cache)
        return 0;
    level->cache->regenerateChunk(chunkX, chunkZ);
    return 1;
}

int __cdecl NativeRefreshChunk(int dimId, int chunkX, int chunkZ)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;

    PlayerList *list = MinecraftServer::getPlayerList();
    if (!list)
        return 0;

    auto packet = std::make_shared<BlockRegionUpdatePacket>(chunkX * 16, 0, chunkZ * 16, 16, Level::maxBuildHeight, 16, level);
    for (auto &p : list->players)
    {
        if (!p || p->dimension != dimId || !p->connection || p->connection->isLocal())
            continue;
        p->connection->send(packet);
    }
    return 1;
}

int __cdecl NativeGetSkyLight(int dimId, int x, int y, int z)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;
    return level->getBrightness(LightLayer::Sky, x, y, z);
}

int __cdecl NativeGetBlockLight(int dimId, int x, int y, int z)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 0;
    return level->getBrightness(LightLayer::Block, x, y, z);
}

int __cdecl NativeGetBiomeId(int dimId, int x, int z)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return 1;
    Biome *biome = level->getBiome(x, z);
    return biome ? biome->id : 1;
}

void __cdecl NativeSetBiomeId(int dimId, int x, int z, int biomeId)
{
    ServerLevel *level = GetLevel(dimId);
    if (!level)
        return;
    LevelChunk *chunk = level->getChunk(x >> 4, z >> 4);
    if (!chunk)
        return;
    byteArray biomes = chunk->getBiomes();
    if (biomes.data == nullptr)
        return;
    int lx = x & 0xf;
    int lz = z & 0xf;
    biomes.data[(lz << 4) | lx] = static_cast<unsigned char>(biomeId & 0xff);
}

} // namespace FourKitBridge