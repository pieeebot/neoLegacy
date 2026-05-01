using System.Runtime.InteropServices;

namespace Minecraft.Server.FourKit;

// EAT SHIT AND DIE

internal static class NativeBridge
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeDamageDelegate(int entityId, float amount);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetHealthDelegate(int entityId, float health);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeTeleportDelegate(int entityId, double x, double y, double z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetGameModeDelegate(int entityId, int gameMode);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeBroadcastMessageDelegate(IntPtr utf8, int byteLen);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetFallDistanceDelegate(int entityId, float distance);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetPlayerSnapshotDelegate(int entityId, IntPtr outBuf);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSendMessageDelegate(int entityId, IntPtr utf8, int byteLen);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetWalkSpeedDelegate(int entityId, float speed);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeTeleportEntityDelegate(int entityId, int dimId, double x, double y, double z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetTileIdDelegate(int dimId, int x, int y, int z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetTileDataDelegate(int dimId, int x, int y, int z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetTileDelegate(int dimId, int x, int y, int z, int tileId, int data, int flags);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetTileDataDelegate(int dimId, int x, int y, int z, int data, int flags);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeBreakBlockDelegate(int dimId, int x, int y, int z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetHighestBlockYDelegate(int dimId, int x, int z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetWorldInfoDelegate(int dimId, IntPtr outBuf);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetWorldTimeDelegate(int dimId, long time);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetWeatherDelegate(int dimId, int storm, int thundering, int thunderDuration);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeCreateExplosionDelegate(int dimId, double x, double y, double z, float power, int setFire, int breakBlocks);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeStrikeLightningDelegate(int dimId, double x, double y, double z, int effectOnly);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeSetSpawnLocationDelegate(int dimId, int x, int y, int z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeDropItemDelegate(int dimId, double x, double y, double z, int itemId, int count, int auxValue, int naturally);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeKickPlayerDelegate(int entityId, int reason);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeBanPlayerDelegate(int entityId, IntPtr reasonUtf8, int reasonByteLen);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeBanPlayerIpDelegate(int entityId, IntPtr reasonUtf8, int reasonByteLen);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetPlayerAddressDelegate(int entityId, IntPtr outIpBuf, int outIpBufSize, IntPtr outPort);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetPlayerLatencyDelegate(int entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeSendRawDelegate(int entityId, IntPtr dataBuf, int dataBufSize);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetPlayerInventoryDelegate(int entityId, IntPtr outBuffer);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetPlayerInventorySlotDelegate(int entityId, int slot, int itemId, int count, int aux);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetContainerContentsDelegate(int entityId, IntPtr outBuffer, int maxSlots);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetContainerSlotDelegate(int entityId, int slot, int itemId, int count, int aux);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetContainerViewerEntityIdsDelegate(int entityId, IntPtr outIds, int maxCount, IntPtr outCount);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeCloseContainerDelegate(int entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeOpenVirtualContainerDelegate(int entityId, int nativeType, IntPtr titleUtf8, int titleByteLen, int slotCount, IntPtr itemsBuf);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetItemMetaDelegate(int entityId, int slot, IntPtr outBuf, int bufSize);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetItemMetaDelegate(int entityId, int slot, IntPtr inBuf, int bufSize);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetHeldItemSlotDelegate(int entityId, int slot);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetCarriedItemDelegate(int entityId, IntPtr outData);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetCarriedItemDelegate(int entityId, int itemId, int count, int aux);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetEnderChestContentsDelegate(int entityId, IntPtr outData);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetEnderChestSlotDelegate(int entityId, int slot, int itemId, int count, int aux);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetSneakingDelegate(int entityId, int sneak);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetVelocityDelegate(int entityId, double x, double y, double z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetAllowFlightDelegate(int entityId, int allowFlight);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativePlaySoundDelegate(int entityId, int soundId, double x, double y, double z, float volume, float pitch);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetSleepingIgnoredDelegate(int entityId, int ignored);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetLevelDelegate(int entityId, int level);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetExpDelegate(int entityId, float exp);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGiveExpDelegate(int entityId, int amount);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGiveExpLevelsDelegate(int entityId, int amount);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetFoodLevelDelegate(int entityId, int foodLevel);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetSaturationDelegate(int entityId, float saturation);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetExhaustionDelegate(int entityId, float exhaustion);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSpawnParticleDelegate(int entityId, int particleId, float x, float y, float z, float offsetX, float offsetY, float offsetZ, float speed, int count);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeSetPassengerDelegate(int entityId, int passengerEntityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeLeaveVehicleDelegate(int entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeEjectDelegate(int entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetVehicleIdDelegate(int entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetPassengerIdDelegate(int entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetEntityInfoDelegate(int entityId, IntPtr outBuf);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeIsChunkLoadedDelegate(int dimId, int chunkX, int chunkZ);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeLoadChunkDelegate(int dimId, int chunkX, int chunkZ, int generate);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeUnloadChunkDelegate(int dimId, int chunkX, int chunkZ, int save, int safe);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetLoadedChunksDelegate(int dimId, out IntPtr coordBuf);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeIsChunkInUseDelegate(int dimId, int chunkX, int chunkZ);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeGetChunkSnapshotDelegate(int dimId, int chunkX, int chunkZ, IntPtr blockIds, IntPtr blockData, IntPtr maxBlockY);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeUnloadChunkRequestDelegate(int dimId, int chunkX, int chunkZ, int safe);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeRegenerateChunkDelegate(int dimId, int chunkX, int chunkZ);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeRefreshChunkDelegate(int dimId, int chunkX, int chunkZ);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetWorldEntitiesDelegate(int dimId, out IntPtr outBuf);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetChunkEntitiesDelegate(int dimId, int chunkX, int chunkZ, out IntPtr outBuf);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetSkyLightDelegate(int dimId, int x, int y, int z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetBlockLightDelegate(int dimId, int x, int y, int z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetBiomeIdDelegate(int dimId, int x, int z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetBiomeIdDelegate(int dimId, int x, int z, int biomeId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeSetHandlerMaskDelegate(uint mask);
    internal static NativeSetHandlerMaskDelegate? SetHandlerMask;

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int NativeGetServerTickCountDelegate();
    internal static NativeGetServerTickCountDelegate? GetServerTickCount;

    internal static NativeDamageDelegate? DamagePlayer;
    internal static NativeSetHealthDelegate? SetPlayerHealth;
    internal static NativeTeleportDelegate? TeleportPlayer;
    internal static NativeSetGameModeDelegate? SetPlayerGameMode;
    internal static NativeBroadcastMessageDelegate? BroadcastMessage;
    internal static NativeSetFallDistanceDelegate? SetFallDistance;
    internal static NativeGetPlayerSnapshotDelegate? GetPlayerSnapshot;
    internal static NativeSendMessageDelegate? SendMessage;
    internal static NativeSetWalkSpeedDelegate? SetWalkSpeed;
    internal static NativeTeleportEntityDelegate? TeleportEntity;

    internal static NativeGetTileIdDelegate? GetTileId;
    internal static NativeGetTileDataDelegate? GetTileData;
    internal static NativeSetTileDelegate? SetTile;
    internal static NativeSetTileDataDelegate? SetTileData;
    internal static NativeBreakBlockDelegate? BreakBlock;
    internal static NativeGetHighestBlockYDelegate? GetHighestBlockY;
    internal static NativeGetWorldInfoDelegate? GetWorldInfo;
    internal static NativeSetWorldTimeDelegate? SetWorldTime;
    internal static NativeSetWeatherDelegate? SetWeather;
    internal static NativeCreateExplosionDelegate? CreateExplosion;
    internal static NativeStrikeLightningDelegate? StrikeLightning;
    internal static NativeSetSpawnLocationDelegate? SetSpawnLocation;
    internal static NativeDropItemDelegate? DropItem;
    internal static NativeKickPlayerDelegate? KickPlayer;
    internal static NativeBanPlayerDelegate? BanPlayer;
    internal static NativeBanPlayerIpDelegate? BanPlayerIp;
    internal static NativeGetPlayerAddressDelegate? GetPlayerAddress;
    internal static NativeGetPlayerLatencyDelegate? GetPlayerLatency;
    internal static NativeSendRawDelegate? SendRaw;
    internal static NativeGetPlayerInventoryDelegate? GetPlayerInventory;
    internal static NativeSetPlayerInventorySlotDelegate? SetPlayerInventorySlot;
    internal static NativeGetContainerContentsDelegate? GetContainerContents;
    internal static NativeSetContainerSlotDelegate? SetContainerSlot;
    internal static NativeGetContainerViewerEntityIdsDelegate? GetContainerViewerEntityIds;
    internal static NativeCloseContainerDelegate? CloseContainer;
    internal static NativeOpenVirtualContainerDelegate? OpenVirtualContainer;
    internal static NativeGetItemMetaDelegate? GetItemMeta;
    internal static NativeSetItemMetaDelegate? SetItemMeta;
    internal static NativeSetHeldItemSlotDelegate? SetHeldItemSlot;
    internal static NativeGetCarriedItemDelegate? GetCarriedItem;
    internal static NativeSetCarriedItemDelegate? SetCarriedItem;
    internal static NativeGetEnderChestContentsDelegate? GetEnderChestContents;
    internal static NativeSetEnderChestSlotDelegate? SetEnderChestSlot;
    internal static NativeSetSneakingDelegate? SetSneaking;
    internal static NativeSetVelocityDelegate? SetVelocity;
    internal static NativeSetAllowFlightDelegate? SetAllowFlight;
    internal static NativePlaySoundDelegate? PlaySound;
    internal static NativeSetSleepingIgnoredDelegate? SetSleepingIgnored;
    internal static NativeSetLevelDelegate? SetLevel;
    internal static NativeSetExpDelegate? SetExp;
    internal static NativeGiveExpDelegate? GiveExp;
    internal static NativeGiveExpLevelsDelegate? GiveExpLevels;
    internal static NativeSetFoodLevelDelegate? SetFoodLevel;
    internal static NativeSetSaturationDelegate? SetSaturation;
    internal static NativeSetExhaustionDelegate? SetExhaustion;
    internal static NativeSpawnParticleDelegate? SpawnParticle;
    internal static NativeSetPassengerDelegate? SetPassenger;
    internal static NativeLeaveVehicleDelegate? LeaveVehicle;
    internal static NativeEjectDelegate? Eject;
    internal static NativeGetVehicleIdDelegate? GetVehicleId;
    internal static NativeGetPassengerIdDelegate? GetPassengerId;
    internal static NativeGetEntityInfoDelegate? GetEntityInfo;
    internal static NativeIsChunkLoadedDelegate? IsChunkLoaded;
    internal static NativeLoadChunkDelegate? LoadChunk;
    internal static NativeUnloadChunkDelegate? UnloadChunk;
    internal static NativeGetLoadedChunksDelegate? GetLoadedChunks;
    internal static NativeIsChunkInUseDelegate? IsChunkInUse;
    internal static NativeGetChunkSnapshotDelegate? GetChunkSnapshot;
    internal static NativeUnloadChunkRequestDelegate? UnloadChunkRequest;
    internal static NativeRegenerateChunkDelegate? RegenerateChunk;
    internal static NativeRefreshChunkDelegate? RefreshChunk;
    internal static NativeGetWorldEntitiesDelegate? GetWorldEntities;
    internal static NativeGetChunkEntitiesDelegate? GetChunkEntities;
    internal static NativeGetSkyLightDelegate? GetSkyLight;
    internal static NativeGetBlockLightDelegate? GetBlockLight;
    internal static NativeGetBiomeIdDelegate? GetBiomeId;
    internal static NativeSetBiomeIdDelegate? SetBiomeId;

    internal static void SetCallbacks(IntPtr damage, IntPtr setHealth, IntPtr teleport, IntPtr setGameMode, IntPtr broadcastMessage, IntPtr setFallDistance, IntPtr getPlayerSnapshot, IntPtr sendMessage, IntPtr setWalkSpeed, IntPtr teleportEntity)
    {
        DamagePlayer = Marshal.GetDelegateForFunctionPointer<NativeDamageDelegate>(damage);
        SetPlayerHealth = Marshal.GetDelegateForFunctionPointer<NativeSetHealthDelegate>(setHealth);
        TeleportPlayer = Marshal.GetDelegateForFunctionPointer<NativeTeleportDelegate>(teleport);
        SetPlayerGameMode = Marshal.GetDelegateForFunctionPointer<NativeSetGameModeDelegate>(setGameMode);
        BroadcastMessage = Marshal.GetDelegateForFunctionPointer<NativeBroadcastMessageDelegate>(broadcastMessage);
        SetFallDistance = Marshal.GetDelegateForFunctionPointer<NativeSetFallDistanceDelegate>(setFallDistance);
        GetPlayerSnapshot = Marshal.GetDelegateForFunctionPointer<NativeGetPlayerSnapshotDelegate>(getPlayerSnapshot);
        SendMessage = Marshal.GetDelegateForFunctionPointer<NativeSendMessageDelegate>(sendMessage);
        SetWalkSpeed = Marshal.GetDelegateForFunctionPointer<NativeSetWalkSpeedDelegate>(setWalkSpeed);
        TeleportEntity = Marshal.GetDelegateForFunctionPointer<NativeTeleportEntityDelegate>(teleportEntity);
    }

    internal static void SetWorldCallbacks(IntPtr getTileId, IntPtr getTileData, IntPtr setTile, IntPtr setTileData, IntPtr breakBlock, IntPtr getHighestBlockY, IntPtr getWorldInfo, IntPtr setWorldTime, IntPtr setWeather, IntPtr createExplosion, IntPtr strikeLightning, IntPtr setSpawnLocation, IntPtr dropItem)
    {
        GetTileId = Marshal.GetDelegateForFunctionPointer<NativeGetTileIdDelegate>(getTileId);
        GetTileData = Marshal.GetDelegateForFunctionPointer<NativeGetTileDataDelegate>(getTileData);
        SetTile = Marshal.GetDelegateForFunctionPointer<NativeSetTileDelegate>(setTile);
        SetTileData = Marshal.GetDelegateForFunctionPointer<NativeSetTileDataDelegate>(setTileData);
        BreakBlock = Marshal.GetDelegateForFunctionPointer<NativeBreakBlockDelegate>(breakBlock);
        GetHighestBlockY = Marshal.GetDelegateForFunctionPointer<NativeGetHighestBlockYDelegate>(getHighestBlockY);
        GetWorldInfo = Marshal.GetDelegateForFunctionPointer<NativeGetWorldInfoDelegate>(getWorldInfo);
        SetWorldTime = Marshal.GetDelegateForFunctionPointer<NativeSetWorldTimeDelegate>(setWorldTime);
        SetWeather = Marshal.GetDelegateForFunctionPointer<NativeSetWeatherDelegate>(setWeather);
        CreateExplosion = Marshal.GetDelegateForFunctionPointer<NativeCreateExplosionDelegate>(createExplosion);
        StrikeLightning = Marshal.GetDelegateForFunctionPointer<NativeStrikeLightningDelegate>(strikeLightning);
        SetSpawnLocation = Marshal.GetDelegateForFunctionPointer<NativeSetSpawnLocationDelegate>(setSpawnLocation);
        DropItem = Marshal.GetDelegateForFunctionPointer<NativeDropItemDelegate>(dropItem);
    }

    internal static void SetPlayerCallbacks(IntPtr kickPlayer, IntPtr banPlayer, IntPtr banPlayerIp, IntPtr getPlayerAddress, IntPtr getPlayerLatency)
    {
        KickPlayer = Marshal.GetDelegateForFunctionPointer<NativeKickPlayerDelegate>(kickPlayer);
        BanPlayer = Marshal.GetDelegateForFunctionPointer<NativeBanPlayerDelegate>(banPlayer);
        BanPlayerIp = Marshal.GetDelegateForFunctionPointer<NativeBanPlayerIpDelegate>(banPlayerIp);
        GetPlayerAddress = Marshal.GetDelegateForFunctionPointer<NativeGetPlayerAddressDelegate>(getPlayerAddress);
        GetPlayerLatency = Marshal.GetDelegateForFunctionPointer<NativeGetPlayerLatencyDelegate>(getPlayerLatency);
    }

    internal static void SetPlayerConnectionCallbacks(IntPtr sendRaw)
    {
        SendRaw = Marshal.GetDelegateForFunctionPointer<NativeSendRawDelegate>(sendRaw);
    }

    internal static void SetInventoryCallbacks(IntPtr getPlayerInventory, IntPtr setPlayerInventorySlot, IntPtr getContainerContents, IntPtr setContainerSlot, IntPtr getContainerViewerEntityIds, IntPtr closeContainer, IntPtr openVirtualContainer, IntPtr getItemMeta, IntPtr setItemMeta, IntPtr setHeldItemSlot, IntPtr getCarriedItem, IntPtr setCarriedItem, IntPtr getEnderChestContents, IntPtr setEnderChestSlot)
    {
        GetPlayerInventory = Marshal.GetDelegateForFunctionPointer<NativeGetPlayerInventoryDelegate>(getPlayerInventory);
        SetPlayerInventorySlot = Marshal.GetDelegateForFunctionPointer<NativeSetPlayerInventorySlotDelegate>(setPlayerInventorySlot);
        GetContainerContents = Marshal.GetDelegateForFunctionPointer<NativeGetContainerContentsDelegate>(getContainerContents);
        SetContainerSlot = Marshal.GetDelegateForFunctionPointer<NativeSetContainerSlotDelegate>(setContainerSlot);
        GetContainerViewerEntityIds = Marshal.GetDelegateForFunctionPointer<NativeGetContainerViewerEntityIdsDelegate>(getContainerViewerEntityIds);
        CloseContainer = Marshal.GetDelegateForFunctionPointer<NativeCloseContainerDelegate>(closeContainer);
        OpenVirtualContainer = Marshal.GetDelegateForFunctionPointer<NativeOpenVirtualContainerDelegate>(openVirtualContainer);
        GetItemMeta = Marshal.GetDelegateForFunctionPointer<NativeGetItemMetaDelegate>(getItemMeta);
        SetItemMeta = Marshal.GetDelegateForFunctionPointer<NativeSetItemMetaDelegate>(setItemMeta);
        SetHeldItemSlot = Marshal.GetDelegateForFunctionPointer<NativeSetHeldItemSlotDelegate>(setHeldItemSlot);
        GetCarriedItem = Marshal.GetDelegateForFunctionPointer<NativeGetCarriedItemDelegate>(getCarriedItem);
        SetCarriedItem = Marshal.GetDelegateForFunctionPointer<NativeSetCarriedItemDelegate>(setCarriedItem);
        GetEnderChestContents = Marshal.GetDelegateForFunctionPointer<NativeGetEnderChestContentsDelegate>(getEnderChestContents);
        SetEnderChestSlot = Marshal.GetDelegateForFunctionPointer<NativeSetEnderChestSlotDelegate>(setEnderChestSlot);
    }

    internal static void SetEntityCallbacks(IntPtr setSneaking, IntPtr setVelocity, IntPtr setAllowFlight, IntPtr playSound, IntPtr setSleepingIgnored)
    {
        SetSneaking = Marshal.GetDelegateForFunctionPointer<NativeSetSneakingDelegate>(setSneaking);
        SetVelocity = Marshal.GetDelegateForFunctionPointer<NativeSetVelocityDelegate>(setVelocity);
        SetAllowFlight = Marshal.GetDelegateForFunctionPointer<NativeSetAllowFlightDelegate>(setAllowFlight);
        PlaySound = Marshal.GetDelegateForFunctionPointer<NativePlaySoundDelegate>(playSound);
        SetSleepingIgnored = Marshal.GetDelegateForFunctionPointer<NativeSetSleepingIgnoredDelegate>(setSleepingIgnored);
    }

    internal static void SetExperienceCallbacks(IntPtr setLevel, IntPtr setExp, IntPtr giveExp, IntPtr giveExpLevels, IntPtr setFoodLevel, IntPtr setSaturation, IntPtr setExhaustion)
    {
        SetLevel = Marshal.GetDelegateForFunctionPointer<NativeSetLevelDelegate>(setLevel);
        SetExp = Marshal.GetDelegateForFunctionPointer<NativeSetExpDelegate>(setExp);
        GiveExp = Marshal.GetDelegateForFunctionPointer<NativeGiveExpDelegate>(giveExp);
        GiveExpLevels = Marshal.GetDelegateForFunctionPointer<NativeGiveExpLevelsDelegate>(giveExpLevels);
        SetFoodLevel = Marshal.GetDelegateForFunctionPointer<NativeSetFoodLevelDelegate>(setFoodLevel);
        SetSaturation = Marshal.GetDelegateForFunctionPointer<NativeSetSaturationDelegate>(setSaturation);
        SetExhaustion = Marshal.GetDelegateForFunctionPointer<NativeSetExhaustionDelegate>(setExhaustion);
    }

    internal static void SetParticleCallbacks(IntPtr spawnParticle)
    {
        SpawnParticle = Marshal.GetDelegateForFunctionPointer<NativeSpawnParticleDelegate>(spawnParticle);
    }

    internal static void SetVehicleCallbacks(IntPtr setPassenger, IntPtr leaveVehicle, IntPtr eject, IntPtr getVehicleId, IntPtr getPassengerId, IntPtr getEntityInfo)
    {
        SetPassenger = Marshal.GetDelegateForFunctionPointer<NativeSetPassengerDelegate>(setPassenger);
        LeaveVehicle = Marshal.GetDelegateForFunctionPointer<NativeLeaveVehicleDelegate>(leaveVehicle);
        Eject = Marshal.GetDelegateForFunctionPointer<NativeEjectDelegate>(eject);
        GetVehicleId = Marshal.GetDelegateForFunctionPointer<NativeGetVehicleIdDelegate>(getVehicleId);
        GetPassengerId = Marshal.GetDelegateForFunctionPointer<NativeGetPassengerIdDelegate>(getPassengerId);
        GetEntityInfo = Marshal.GetDelegateForFunctionPointer<NativeGetEntityInfoDelegate>(getEntityInfo);
    }

    internal static void SetChunkCallbacks(IntPtr isChunkLoaded, IntPtr loadChunk, IntPtr unloadChunk, IntPtr getLoadedChunks, IntPtr isChunkInUse, IntPtr getChunkSnapshot, IntPtr unloadChunkRequest, IntPtr regenerateChunk, IntPtr refreshChunk)
    {
        IsChunkLoaded = Marshal.GetDelegateForFunctionPointer<NativeIsChunkLoadedDelegate>(isChunkLoaded);
        LoadChunk = Marshal.GetDelegateForFunctionPointer<NativeLoadChunkDelegate>(loadChunk);
        UnloadChunk = Marshal.GetDelegateForFunctionPointer<NativeUnloadChunkDelegate>(unloadChunk);
        GetLoadedChunks = Marshal.GetDelegateForFunctionPointer<NativeGetLoadedChunksDelegate>(getLoadedChunks);
        IsChunkInUse = Marshal.GetDelegateForFunctionPointer<NativeIsChunkInUseDelegate>(isChunkInUse);
        GetChunkSnapshot = Marshal.GetDelegateForFunctionPointer<NativeGetChunkSnapshotDelegate>(getChunkSnapshot);
        UnloadChunkRequest = Marshal.GetDelegateForFunctionPointer<NativeUnloadChunkRequestDelegate>(unloadChunkRequest);
        RegenerateChunk = Marshal.GetDelegateForFunctionPointer<NativeRegenerateChunkDelegate>(regenerateChunk);
        RefreshChunk = Marshal.GetDelegateForFunctionPointer<NativeRefreshChunkDelegate>(refreshChunk);
    }

    internal static void SetWorldEntityCallbacks(IntPtr getWorldEntities, IntPtr getChunkEntities)
    {
        GetWorldEntities = Marshal.GetDelegateForFunctionPointer<NativeGetWorldEntitiesDelegate>(getWorldEntities);
        GetChunkEntities = Marshal.GetDelegateForFunctionPointer<NativeGetChunkEntitiesDelegate>(getChunkEntities);
    }

    internal static void SetBlockInfoCallbacks(IntPtr getSkyLight, IntPtr getBlockLight, IntPtr getBiomeId, IntPtr setBiomeId)
    {
        GetSkyLight = Marshal.GetDelegateForFunctionPointer<NativeGetSkyLightDelegate>(getSkyLight);
        GetBlockLight = Marshal.GetDelegateForFunctionPointer<NativeGetBlockLightDelegate>(getBlockLight);
        GetBiomeId = Marshal.GetDelegateForFunctionPointer<NativeGetBiomeIdDelegate>(getBiomeId);
        SetBiomeId = Marshal.GetDelegateForFunctionPointer<NativeSetBiomeIdDelegate>(setBiomeId);
    }

    internal static void SetSubscriptionCallbacks(IntPtr setHandlerMask)
    {
        SetHandlerMask = Marshal.GetDelegateForFunctionPointer<NativeSetHandlerMaskDelegate>(setHandlerMask);
    }

    internal static void SetServerCallbacks(IntPtr getServerTickCount)
    {
        GetServerTickCount = Marshal.GetDelegateForFunctionPointer<NativeGetServerTickCountDelegate>(getServerTickCount);
    }
}
