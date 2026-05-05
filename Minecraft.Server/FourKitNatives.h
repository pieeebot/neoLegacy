#pragma once

#include <cstdint>

namespace FourKitBridge
{
    // Must match HandlerKind in FourKit.cs.
    enum HandlerKind : int {
        kHandlerKind_ChunkLoad   = 0,
        kHandlerKind_ChunkUnload = 1,
        kHandlerKind_PlayerMove  = 2,
    };

    void __cdecl NativeSetHandlerMask(uint32_t mask);
    bool HasHandlers(int kind);

    int __cdecl NativeGetServerTickCount();

    // core
    void __cdecl NativeDamagePlayer(int entityId, float amount);
    void __cdecl NativeSetPlayerHealth(int entityId, float health);
    void __cdecl NativeTeleportPlayer(int entityId, double x, double y, double z);
    void __cdecl NativeSetPlayerGameMode(int entityId, int gameMode);
    void __cdecl NativeBroadcastMessage(const char *utf8, int len);
    void __cdecl NativeSetFallDistance(int entityId, float distance);
    void __cdecl NativeGetPlayerSnapshot(int entityId, double *outData);
    void __cdecl NativeSendMessage(int entityId, const char *utf8, int len);
    void __cdecl NativeSetWalkSpeed(int entityId, float speed);
    void __cdecl NativeTeleportEntity(int entityId, int dimId, double x, double y, double z);

    // World
    int __cdecl NativeGetTileId(int dimId, int x, int y, int z);
    int __cdecl NativeGetTileData(int dimId, int x, int y, int z);
    void __cdecl NativeSetTile(int dimId, int x, int y, int z, int tileId, int data, int flags);
    void __cdecl NativeSetTileData(int dimId, int x, int y, int z, int data, int flags);
    int __cdecl NativeBreakBlock(int dimId, int x, int y, int z);
    int __cdecl NativeGetHighestBlockY(int dimId, int x, int z);
    void __cdecl NativeGetWorldInfo(int dimId, double *outBuf);
    void __cdecl NativeSetWorldTime(int dimId, int64_t time);
    void __cdecl NativeSetWeather(int dimId, int storm, int thundering, int thunderDuration);
    int __cdecl NativeCreateExplosion(int dimId, double x, double y, double z, float power, int setFire, int breakBlocks);
    int __cdecl NativeStrikeLightning(int dimId, double x, double y, double z, int effectOnly);
    int __cdecl NativeSetSpawnLocation(int dimId, int x, int y, int z);
    void __cdecl NativeDropItem(int dimId, double x, double y, double z, int itemId, int count, int auxValue, int naturally);

    // plr
    void __cdecl NativeKickPlayer(int entityId, int reason);
    int __cdecl NativeBanPlayer(int entityId, const char *reasonUtf8, int reasonByteLen);
    int __cdecl NativeBanPlayerIp(int entityId, const char *reasonUtf8, int reasonByteLen);
    int __cdecl NativeGetPlayerAddress(int entityId, char* outIpBuf, int outIpBufSize, int* outPort);
    int __cdecl NativeGetPlayerLatency(int entityId);

    //plr connection
    int __cdecl NativeSendRaw(int entityId, unsigned char* dataBuf, int dataBufSize);

    // inv
    void __cdecl NativeGetPlayerInventory(int entityId, int *outData);
    void __cdecl NativeSetPlayerInventorySlot(int entityId, int slot, int itemId, int count, int aux);
    void __cdecl NativeGetContainerContents(int entityId, int *outData, int maxSlots);
    void __cdecl NativeSetContainerSlot(int entityId, int slot, int itemId, int count, int aux);
    void __cdecl NativeGetContainerViewerEntityIds(int entityId, int *outIds, int maxCount, int *outCount);
    void __cdecl NativeCloseContainer(int entityId);
    void __cdecl NativeOpenVirtualContainer(int entityId, int nativeType, const char *titleUtf8, int titleByteLen, int slotCount, int *itemsBuf);
    int __cdecl NativeGetItemMeta(int entityId, int slot, char *outBuf, int bufSize);
    void __cdecl NativeSetItemMeta(int entityId, int slot, const char *inBuf, int bufSize);
    void __cdecl NativeSetHeldItemSlot(int entityId, int slot);

    // carried item (cursor) & ender chest
    void __cdecl NativeGetCarriedItem(int entityId, int *outData);
    void __cdecl NativeSetCarriedItem(int entityId, int itemId, int count, int aux);
    void __cdecl NativeGetEnderChestContents(int entityId, int *outData);
    void __cdecl NativeSetEnderChestSlot(int entityId, int slot, int itemId, int count, int aux);

    // ent
    void __cdecl NativeSetSneaking(int entityId, int sneak);
    void __cdecl NativeSetVelocity(int entityId, double x, double y, double z);
    void __cdecl NativeSetAllowFlight(int entityId, int allowFlight);
    void __cdecl NativePlaySound(int entityId, int soundId, double x, double y, double z, float volume, float pitch);
    void __cdecl NativeSetSleepingIgnored(int entityId, int ignored);

    // x[p&food
    void __cdecl NativeSetLevel(int entityId, int level);
    void __cdecl NativeSetExp(int entityId, float exp);
    void __cdecl NativeGiveExp(int entityId, int amount);
    void __cdecl NativeGiveExpLevels(int entityId, int amount);
    void __cdecl NativeSetFoodLevel(int entityId, int foodLevel);
    void __cdecl NativeSetSaturation(int entityId, float saturation);
    void __cdecl NativeSetExhaustion(int entityId, float exhaustion);

    // particle
    void __cdecl NativeSpawnParticle(int entityId, int particleId, float x, float y, float z, float offsetX, float offsetY, float offsetZ, float speed, int count);

    // vehicle
    int __cdecl NativeSetPassenger(int entityId, int passengerEntityId);
    int __cdecl NativeLeaveVehicle(int entityId);
    int __cdecl NativeEject(int entityId);
    int __cdecl NativeGetVehicleId(int entityId);
    int __cdecl NativeGetPassengerId(int entityId);
    void __cdecl NativeGetEntityInfo(int entityId, double *outData);

    // chunk
    int __cdecl NativeIsChunkLoaded(int dimId, int chunkX, int chunkZ);
    int __cdecl NativeLoadChunk(int dimId, int chunkX, int chunkZ, int generate);
    int __cdecl NativeUnloadChunk(int dimId, int chunkX, int chunkZ, int save, int safe);
    int __cdecl NativeGetLoadedChunks(int dimId, int **coordBuf);
    int __cdecl NativeIsChunkInUse(int dimId, int chunkX, int chunkZ);
    void __cdecl NativeGetChunkSnapshot(int dimId, int chunkX, int chunkZ, int *blockIds, int *blockData, int *maxBlockY);
    int __cdecl NativeUnloadChunkRequest(int dimId, int chunkX, int chunkZ, int safe);
    int __cdecl NativeRegenerateChunk(int dimId, int chunkX, int chunkZ);
    int __cdecl NativeRefreshChunk(int dimId, int chunkX, int chunkZ);

    // world entity bs
    int __cdecl NativeGetWorldEntities(int dimId, int **outBuf);
    int __cdecl NativeGetChunkEntities(int dimId, int chunkX, int chunkZ, int **outBuf);

    // block info (light, biome)
    int __cdecl NativeGetSkyLight(int dimId, int x, int y, int z);
    int __cdecl NativeGetBlockLight(int dimId, int x, int y, int z);
    int __cdecl NativeGetBiomeId(int dimId, int x, int z);
    void __cdecl NativeSetBiomeId(int dimId, int x, int z, int biomeId);
}
