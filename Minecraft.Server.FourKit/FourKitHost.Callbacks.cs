using System.Runtime.InteropServices;

namespace Minecraft.Server.FourKit;

public static partial class FourKitHost
{
    [UnmanagedCallersOnly]
    public static void SetNativeCallbacks(IntPtr damage, IntPtr setHealth, IntPtr teleport, IntPtr setGameMode, IntPtr broadcastMessage, IntPtr setFallDistance, IntPtr getPlayerSnapshot, IntPtr sendMessage, IntPtr setWalkSpeed, IntPtr teleportEntity)
    {
        try
        {
            NativeBridge.SetCallbacks(damage, setHealth, teleport, setGameMode, broadcastMessage, setFallDistance, getPlayerSnapshot, sendMessage, setWalkSpeed, teleportEntity);
            ServerLog.Info("fourkit", "Native callbacks registered.");
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetNativeCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetWorldCallbacks(IntPtr getTileId, IntPtr getTileData, IntPtr setTile, IntPtr setTileData, IntPtr breakBlock, IntPtr getHighestBlockY, IntPtr getWorldInfo, IntPtr setWorldTime, IntPtr setWeather, IntPtr createExplosion, IntPtr strikeLightning, IntPtr setSpawnLocation, IntPtr dropItem)
    {
        try
        {
            NativeBridge.SetWorldCallbacks(getTileId, getTileData, setTile, setTileData, breakBlock, getHighestBlockY, getWorldInfo, setWorldTime, setWeather, createExplosion, strikeLightning, setSpawnLocation, dropItem);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetWorldCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetPlayerCallbacks(IntPtr kickPlayer, IntPtr banPlayer, IntPtr banPlayerIp, IntPtr getPlayerAddress, IntPtr getPlayerLatency)
    {
        try
        {
            NativeBridge.SetPlayerCallbacks(kickPlayer, banPlayer, banPlayerIp, getPlayerAddress, getPlayerLatency);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetPlayerCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetPlayerConnectionCallbacks(IntPtr sendRaw)
    {
        try
        {
            NativeBridge.SetPlayerConnectionCallbacks(sendRaw);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetPlayerConnectionCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetInventoryCallbacks(IntPtr getPlayerInventory, IntPtr setPlayerInventorySlot, IntPtr getContainerContents, IntPtr setContainerSlot, IntPtr getContainerViewerEntityIds, IntPtr closeContainer, IntPtr openVirtualContainer, IntPtr getItemMeta, IntPtr setItemMeta, IntPtr setHeldItemSlot, IntPtr getCarriedItem, IntPtr setCarriedItem, IntPtr getEnderChestContents, IntPtr setEnderChestSlot)
    {
        try
        {
            NativeBridge.SetInventoryCallbacks(getPlayerInventory, setPlayerInventorySlot, getContainerContents, setContainerSlot, getContainerViewerEntityIds, closeContainer, openVirtualContainer, getItemMeta, setItemMeta, setHeldItemSlot, getCarriedItem, setCarriedItem, getEnderChestContents, setEnderChestSlot);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetInventoryCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetEntityCallbacks(IntPtr setSneaking, IntPtr setVelocity, IntPtr setAllowFlight, IntPtr playSound, IntPtr setSleepingIgnored)
    {
        try
        {
            NativeBridge.SetEntityCallbacks(setSneaking, setVelocity, setAllowFlight, playSound, setSleepingIgnored);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetEntityCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetExperienceCallbacks(IntPtr setLevel, IntPtr setExp, IntPtr giveExp, IntPtr giveExpLevels, IntPtr setFoodLevel, IntPtr setSaturation, IntPtr setExhaustion)
    {
        try
        {
            NativeBridge.SetExperienceCallbacks(setLevel, setExp, giveExp, giveExpLevels, setFoodLevel, setSaturation, setExhaustion);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetExperienceCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetParticleCallbacks(IntPtr spawnParticle)
    {
        try
        {
            NativeBridge.SetParticleCallbacks(spawnParticle);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetParticleCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetVehicleCallbacks(IntPtr setPassenger, IntPtr leaveVehicle, IntPtr eject, IntPtr getVehicleId, IntPtr getPassengerId, IntPtr getEntityInfo)
    {
        try
        {
            NativeBridge.SetVehicleCallbacks(setPassenger, leaveVehicle, eject, getVehicleId, getPassengerId, getEntityInfo);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetVehicleCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetChunkCallbacks(IntPtr isChunkLoaded, IntPtr loadChunk, IntPtr unloadChunk, IntPtr getLoadedChunks, IntPtr isChunkInUse, IntPtr getChunkSnapshot, IntPtr unloadChunkRequest, IntPtr regenerateChunk, IntPtr refreshChunk)
    {
        try
        {
            NativeBridge.SetChunkCallbacks(isChunkLoaded, loadChunk, unloadChunk, getLoadedChunks, isChunkInUse, getChunkSnapshot, unloadChunkRequest, regenerateChunk, refreshChunk);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetChunkCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetBlockInfoCallbacks(IntPtr getSkyLight, IntPtr getBlockLight, IntPtr getBiomeId, IntPtr setBiomeId)
    {
        try
        {
            NativeBridge.SetBlockInfoCallbacks(getSkyLight, getBlockLight, getBiomeId, setBiomeId);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetBlockInfoCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetWorldEntityCallbacks(IntPtr getWorldEntities, IntPtr getChunkEntities)
    {
        try
        {
            NativeBridge.SetWorldEntityCallbacks(getWorldEntities, getChunkEntities);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetWorldEntityCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetSubscriptionCallbacks(IntPtr setHandlerMask)
    {
        try
        {
            NativeBridge.SetSubscriptionCallbacks(setHandlerMask);
            // Flush the mask accumulated during plugin onEnable.
            FourKit.ResyncHandlerMask();
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetSubscriptionCallbacks error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void SetServerCallbacks(IntPtr getServerTickCount)
    {
        try
        {
            NativeBridge.SetServerCallbacks(getServerTickCount);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"SetServerCallbacks error: {ex}");
        }
    }
}
