#include "FourKitBridge.h"
#include "FourKitNatives.h"
#include "FourKitRuntime.h"
#include "Common/StringUtils.h"
#include "ServerLogger.h"
#include "stdafx.h"

#include <string>
#include <unordered_map>
#include <windows.h>

using ServerRuntime::LogDebugf;
using ServerRuntime::LogError;
using ServerRuntime::LogInfo;

namespace FourKitBridge
{

typedef void(__stdcall *fn_initialize)();
typedef void(__stdcall *fn_shutdown)();
typedef void(__stdcall *fn_fire_world_save)();
typedef int(__stdcall *fn_fire_player_prelogin)(const char* nameUtf8, int nameByteLen, const char* ipUtf8, int ipByteLen, int port);
typedef int(__stdcall *fn_fire_player_login)(const char* nameUtf8, int nameByteLen, const char* ipUtf8, int ipByteLen, int port, int type, unsigned long long* offlineXUID, unsigned long long* onlineXUID);
typedef void(__stdcall *fn_fire_player_join)(int entityId, const char *nameUtf8, int nameByteLen, const char *uuidUtf8, int uuidByteLen, unsigned long long offlineXUID, unsigned long long onlineXUID);
typedef void(__stdcall *fn_fire_player_quit)(int entityId);
typedef int(__stdcall *fn_fire_player_kick)(int entityId, int disconnectReason,
                                            const char *reasonUtf8, int reasonByteLen,
                                            char *outBuf, int outBufSize, int *outLen);
typedef int(__stdcall *fn_fire_player_move)(int entityId,
                                            double fromX, double fromY, double fromZ,
                                            double toX, double toY, double toZ,
                                            double *outCoords);
typedef void(__stdcall *fn_set_native_callbacks)(void *damage, void *setHealth, void *teleport, void *setGameMode, void *broadcastMessage, void *setFallDistance, void *getPlayerSnapshot, void *sendMessage, void *setWalkSpeed, void *teleportEntity);
typedef void(__stdcall *fn_set_world_callbacks)(void *getTileId, void *getTileData, void *setTile, void *setTileData, void *breakBlock, void *getHighestBlockY, void *getWorldInfo, void *setWorldTime, void *setWeather, void *createExplosion, void *strikeLightning, void *setSpawnLocation, void *dropItem);
typedef void(__stdcall *fn_update_entity_id)(int oldEntityId, int newEntityId);
typedef int(__stdcall *fn_fire_structure_grow)(int dimId, int x, int y, int z, int species, int wasBonemeal, int entityId);
typedef int(__stdcall *fn_fire_player_chat)(int entityId, const char *msgUtf8, int msgByteLen, char *outBuf, int outBufSize, int *outLen);
typedef int(__stdcall *fn_fire_block_place)(int entityId, int dimId,
                                            int placedX, int placedY, int placedZ,
                                            int againstX, int againstY, int againstZ,
                                            int itemId, int itemCount, int canBuild);
typedef int(__stdcall *fn_fire_block_break)(int entityId, int dimId,
                                            int x, int y, int z, int tileId, int data, int exp);
typedef int(__stdcall *fn_fire_entity_damage)(int entityId, int entityTypeId, int dimId,
                                              double x, double y, double z, int causeId, double damage, double *outDamage,
                                              int damagerEntityId, int damagerEntityTypeId, double damagerX, double damagerY, double damagerZ);
typedef int(__stdcall *fn_fire_sign_change)(int entityId, int dimId,
                                            int x, int y, int z,
                                            const char *line0, int line0Len,
                                            const char *line1, int line1Len,
                                            const char *line2, int line2Len,
                                            const char *line3, int line3Len,
                                            char *outBuf, int outBufSize, int *outLens);
typedef int(__stdcall *fn_fire_entity_death)(int entityId, int entityTypeId, int dimId,
                                             double x, double y, double z, int exp);
typedef int(__stdcall *fn_fire_player_death)(int entityId,
                                             const char *deathMsgUtf8, int deathMsgByteLen, int exp,
                                             char *outMsgBuf, int outMsgBufSize, int *outMsgLen, int *outKeepInventory,
                                             int *outNewExp, int *outNewLevel, int *outKeepLevel);
typedef void(__stdcall *fn_set_player_callbacks)(void *kickPlayer, void *banPlayer, void *banPlayerIp, void *getPlayerAddress, void *getPlayerLatency);
typedef void(__stdcall *fn_set_player_connection_callbacks)(void *sendRaw);
typedef long long(__stdcall *fn_fire_player_drop_item)(int entityId,
                                                       int itemId, int itemCount, int itemAux,
                                                       int *outItemId, int *outItemCount, int *outItemAux);
typedef void(__stdcall *fn_set_inventory_callbacks)(void *getPlayerInventory, void *setPlayerInventorySlot, void *getContainerContents, void *setContainerSlot, void *getContainerViewerEntityIds, void *closeContainer, void *openVirtualContainer, void *getItemMeta, void *setItemMeta, void *setHeldItemSlot, void *getCarriedItem, void *setCarriedItem, void *getEnderChestContents, void *setEnderChestSlot);
typedef int(__stdcall *fn_fire_player_interact)(int entityId, int action,
                                                int itemId, int itemCount, int itemAux,
                                                int clickedX, int clickedY, int clickedZ,
                                                int blockFace, int dimId,
                                                int *outUseItemInHand);
typedef int(__stdcall *fn_fire_player_interact_entity)(int playerEntityId,
                                                       int targetEntityId, int targetEntityTypeId,
                                                       int dimId, double targetX, double targetY, double targetZ,
                                                       float targetHealth, float targetMaxHealth, float targetEyeHeight);
typedef int(__stdcall *fn_fire_player_pickup_item)(int playerEntityId,
                                                   int itemEntityId, int dimId, double itemX, double itemY, double itemZ,
                                                   int itemId, int itemCount, int itemAux, int remaining,
                                                   int *outItemId, int *outItemCount, int *outItemAux);
typedef int(__stdcall *fn_fire_inventory_open)(int entityId, int nativeContainerType,
                                               const char *titleUtf8, int titleByteLen, int containerSize);
typedef int(__stdcall *fn_handle_player_command)(int entityId,
                                                 const char *cmdUtf8, int cmdByteLen);
typedef int(__stdcall *fn_handle_console_command)(const char *cmdUtf8, int cmdByteLen);
typedef int(__stdcall *fn_get_plugin_command_help)(char *outBuf, int outBufSize, int *outLen);
typedef int(__stdcall *fn_fire_player_teleport)(int entityId,
                                                double fromX, double fromY, double fromZ, int fromDimId,
                                                double toX, double toY, double toZ, int toDimId,
                                                int cause, double *outCoords);
typedef int(__stdcall *fn_fire_inventory_click)(int entityId,
                                                 int slot, int button, int clickType, int nativeContainerType, int containerSize,
                                                 const char *titleUtf8, int titleByteLen);
typedef int(__stdcall *fn_fire_bed_enter)(int entityId, int dimId, int bedX, int bedY, int bedZ);
typedef void(__stdcall *fn_fire_bed_leave)(int entityId, int dimId, int bedX, int bedY, int bedZ);
typedef void(__stdcall *fn_set_entity_callbacks)(void *setSneaking, void *setVelocity, void *setAllowFlight, void *playSound, void *setSleepingIgnored);
typedef void(__stdcall *fn_set_experience_callbacks)(void *setLevel, void *setExp, void *giveExp, void *giveExpLevels, void *setFoodLevel, void *setSaturation, void *setExhaustion);
typedef void(__stdcall *fn_set_particle_callbacks)(void *spawnParticle);
typedef void(__stdcall *fn_set_vehicle_callbacks)(void *setPassenger, void *leaveVehicle, void *eject, void *getVehicleId, void *getPassengerId, void *getEntityInfo);
typedef int(__stdcall *fn_fire_block_grow)(int dimId, int x, int y, int z, int newTileId, int newTileData);
typedef int(__stdcall *fn_fire_block_form)(int dimId, int x, int y, int z, int newTileId, int newTileData);
typedef int(__stdcall *fn_fire_block_burn)(int dimId, int x, int y, int z);
typedef int(__stdcall *fn_fire_block_spread)(int dimId, int x, int y, int z, int srcX, int srcY, int srcZ, int newTileId, int newTileData);
typedef int(__stdcall *fn_fire_piston_extend)(int dimId, int x, int y, int z, int direction, int length);
typedef int(__stdcall *fn_fire_piston_retract)(int dimId, int x, int y, int z, int direction);
typedef int(__stdcall *fn_fire_command_preprocess)(int entityId, const char *cmdUtf8, int cmdByteLen, char *outBuf, int outBufSize, int *outLen);
typedef int(__stdcall *fn_fire_block_from_to)(int dimId, int fromX, int fromY, int fromZ, int toX, int toY, int toZ, int face);
typedef void(__stdcall *fn_set_chunk_callbacks)(void *isChunkLoaded, void *loadChunk, void *unloadChunk, void *getLoadedChunks, void *isChunkInUse, void *getChunkSnapshot, void *unloadChunkRequest, void *regenerateChunk, void *refreshChunk);
typedef void(__stdcall *fn_set_block_info_callbacks)(void *getSkyLight, void *getBlockLight, void *getBiomeId, void *setBiomeId);
typedef void(__stdcall *fn_set_world_entity_callbacks)(void *getWorldEntities, void *getChunkEntities);
typedef void(__stdcall *fn_set_subscription_callbacks)(void *setHandlerMask);
typedef void(__stdcall *fn_set_server_callbacks)(void *getServerTickCount);
typedef void(__stdcall *fn_fire_chunk_load)(int dimId, int chunkX, int chunkZ, int isNewChunk);
typedef int(__stdcall *fn_fire_chunk_unload)(int dimId, int chunkX, int chunkZ);

struct OpenContainerInfo
{
    int type;
    int size;
    std::wstring title;
};
static std::unordered_map<int, OpenContainerInfo> s_openContainerInfo;

static fn_initialize s_managedInit = nullptr;
static fn_shutdown s_managedShutdown = nullptr;
static fn_fire_world_save s_managedFireWorldSave = nullptr;
static fn_fire_player_prelogin s_managedFirePreLogin = nullptr;
static fn_fire_player_login s_managedFireLogin = nullptr;
static fn_fire_player_join s_managedFireJoin = nullptr;
static fn_update_entity_id s_managedUpdateEntityId = nullptr;
static fn_fire_player_quit s_managedFireQuit = nullptr;
static fn_fire_player_kick s_managedFireKick = nullptr;
static fn_fire_player_move s_managedFireMove = nullptr;
static fn_set_native_callbacks s_managedSetCallbacks = nullptr;
static fn_set_world_callbacks s_managedSetWorldCallbacks = nullptr;
static fn_fire_structure_grow s_managedFireStructureGrow = nullptr;
static fn_fire_player_chat s_managedFireChat = nullptr;
static fn_fire_block_place s_managedFireBlockPlace = nullptr;
static fn_fire_block_break s_managedFireBlockBreak = nullptr;
static fn_fire_entity_damage s_managedFireEntityDamage = nullptr;
static fn_fire_sign_change s_managedFireSignChange = nullptr;
static fn_fire_entity_death s_managedFireEntityDeath = nullptr;
static fn_fire_player_death s_managedFirePlayerDeath = nullptr;
static fn_set_player_callbacks s_managedSetPlayerCallbacks = nullptr;
static fn_set_player_connection_callbacks s_managedSetPlayerConnectionCallbacks = nullptr;
static fn_fire_player_drop_item s_managedFirePlayerDropItem = nullptr;
static fn_set_inventory_callbacks s_managedSetInventoryCallbacks = nullptr;
static fn_fire_player_interact s_managedFirePlayerInteract = nullptr;
static fn_fire_player_interact_entity s_managedFirePlayerInteractEntity = nullptr;
static fn_fire_player_pickup_item s_managedFirePlayerPickupItem = nullptr;
static fn_fire_inventory_open s_managedFireInventoryOpen = nullptr;
static fn_handle_player_command s_managedHandlePlayerCommand = nullptr;
static fn_handle_console_command s_managedHandleConsoleCommand = nullptr;
static fn_get_plugin_command_help s_managedGetPluginCommandHelp = nullptr;
static fn_fire_player_teleport s_managedFirePlayerTeleport = nullptr;
static fn_fire_player_teleport s_managedFirePlayerPortal = nullptr;
static fn_fire_inventory_click s_managedFireInventoryClick = nullptr;
static fn_fire_bed_enter s_managedFireBedEnter = nullptr;
static fn_fire_bed_leave s_managedFireBedLeave = nullptr;
static fn_set_entity_callbacks s_managedSetEntityCallbacks = nullptr;
static fn_set_experience_callbacks s_managedSetExperienceCallbacks = nullptr;
static fn_set_particle_callbacks s_managedSetParticleCallbacks = nullptr;
static fn_set_vehicle_callbacks s_managedSetVehicleCallbacks = nullptr;
static fn_fire_block_grow s_managedFireBlockGrow = nullptr;
static fn_fire_block_form s_managedFireBlockForm = nullptr;
static fn_fire_block_burn s_managedFireBlockBurn = nullptr;
static fn_fire_block_spread s_managedFireBlockSpread = nullptr;
static fn_fire_piston_extend s_managedFirePistonExtend = nullptr;
static fn_fire_piston_retract s_managedFirePistonRetract = nullptr;
static fn_fire_command_preprocess s_managedFireCommandPreprocess = nullptr;
static fn_fire_block_from_to s_managedFireBlockFromTo = nullptr;
static fn_set_chunk_callbacks s_managedSetChunkCallbacks = nullptr;
static fn_set_block_info_callbacks s_managedSetBlockInfoCallbacks = nullptr;
static fn_set_world_entity_callbacks s_managedSetWorldEntityCallbacks = nullptr;
static fn_set_subscription_callbacks s_managedSetSubscriptionCallbacks = nullptr;
static fn_set_server_callbacks s_managedSetServerCallbacks = nullptr;
static fn_fire_chunk_load s_managedFireChunkLoad = nullptr;
static fn_fire_chunk_unload s_managedFireChunkUnload = nullptr;

static bool s_initialized = false;

void Initialize()
{
    LogInfo("fourkit", "FourKit initializing...");

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    std::wstring exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
    {
        exeDir = exeDir.substr(0, lastSlash);
    }

    // FourKit's self-contained .NET runtime + managed assembly live in a "runtime/"
    // subfolder to keep the server's root directory tidy. See FourKitRuntime.cpp's
    // LoadHostfxr() for the matching subfolder lookup. The runtimeconfig.json
    // sits next to the DLL but is consumed implicitly by hostfxr's command-line
    // init API rather than passed in directly.
    std::wstring assemblyPath = exeDir + L"\\runtime\\Minecraft.Server.FourKit.dll";

    load_assembly_fn loadAssembly = nullptr;
    if (!LoadManagedRuntime(assemblyPath.c_str(), exePath, &loadAssembly))
    {
        return;
    }

    const wchar_t *typeName = L"Minecraft.Server.FourKit.FourKitHost, Minecraft.Server.FourKit";
    const wchar_t *asmPath = assemblyPath.c_str();

    struct { const wchar_t *name; void **target; } entries[] = {
        {L"Initialize", (void **)&s_managedInit},
        {L"Shutdown", (void **)&s_managedShutdown},
        {L"FireWorldSave", (void **)&s_managedFireWorldSave},
        {L"FirePlayerPreLogin", (void **)&s_managedFirePreLogin},
        {L"FirePlayerLogin", (void **)&s_managedFireLogin},
        {L"FirePlayerJoin", (void **)&s_managedFireJoin},
        {L"FirePlayerQuit", (void **)&s_managedFireQuit},
        {L"FirePlayerKick", (void **)&s_managedFireKick},
        {L"FirePlayerMove", (void **)&s_managedFireMove},
        {L"SetNativeCallbacks", (void **)&s_managedSetCallbacks},
        {L"SetWorldCallbacks", (void **)&s_managedSetWorldCallbacks},
        {L"UpdatePlayerEntityId", (void **)&s_managedUpdateEntityId},
        {L"FirePlayerChat", (void**)&s_managedFireChat},
        {L"FireStructureGrow", (void **)&s_managedFireStructureGrow},
        {L"FireBlockPlace", (void **)&s_managedFireBlockPlace},
        {L"FireBlockBreak", (void **)&s_managedFireBlockBreak},
        {L"FireEntityDamage", (void **)&s_managedFireEntityDamage},
        {L"FireSignChange", (void **)&s_managedFireSignChange},
        {L"FireEntityDeath", (void **)&s_managedFireEntityDeath},
        {L"FirePlayerDeath", (void **)&s_managedFirePlayerDeath},
        {L"SetPlayerCallbacks", (void**)&s_managedSetPlayerCallbacks},
        {L"SetPlayerConnectionCallbacks", (void **)&s_managedSetPlayerConnectionCallbacks},
        {L"FirePlayerDropItem", (void **)&s_managedFirePlayerDropItem},
        {L"SetInventoryCallbacks", (void **)&s_managedSetInventoryCallbacks},
        {L"FirePlayerInteract", (void **)&s_managedFirePlayerInteract},
        {L"FirePlayerInteractEntity", (void **)&s_managedFirePlayerInteractEntity},
        {L"FirePlayerPickupItem", (void **)&s_managedFirePlayerPickupItem},
        {L"FireInventoryOpen", (void **)&s_managedFireInventoryOpen},
        {L"HandlePlayerCommand", (void **)&s_managedHandlePlayerCommand},
        {L"HandleConsoleCommand", (void **)&s_managedHandleConsoleCommand},
        {L"GetPluginCommandHelp", (void **)&s_managedGetPluginCommandHelp},
        {L"FirePlayerTeleport", (void **)&s_managedFirePlayerTeleport},
        {L"FirePlayerPortal", (void **)&s_managedFirePlayerPortal},
        {L"FireInventoryClick", (void **)&s_managedFireInventoryClick},
        {L"FireBedEnter", (void **)&s_managedFireBedEnter},
        {L"FireBedLeave", (void **)&s_managedFireBedLeave},
        {L"SetEntityCallbacks", (void **)&s_managedSetEntityCallbacks},
        {L"SetExperienceCallbacks", (void **)&s_managedSetExperienceCallbacks},
        {L"SetParticleCallbacks", (void **)&s_managedSetParticleCallbacks},
        {L"SetVehicleCallbacks", (void **)&s_managedSetVehicleCallbacks},
        {L"FireBlockGrow", (void **)&s_managedFireBlockGrow},
        {L"FireBlockForm", (void **)&s_managedFireBlockForm},
        {L"FireBlockBurn", (void **)&s_managedFireBlockBurn},
        {L"FireBlockSpread", (void **)&s_managedFireBlockSpread},
        {L"FirePistonExtend", (void **)&s_managedFirePistonExtend},
        {L"FirePistonRetract", (void **)&s_managedFirePistonRetract},
        {L"FireCommandPreprocess", (void **)&s_managedFireCommandPreprocess},
        {L"FireBlockFromTo", (void **)&s_managedFireBlockFromTo},
        {L"SetChunkCallbacks", (void **)&s_managedSetChunkCallbacks},
        {L"SetBlockInfoCallbacks", (void **)&s_managedSetBlockInfoCallbacks},
        {L"SetWorldEntityCallbacks", (void **)&s_managedSetWorldEntityCallbacks},
        {L"SetSubscriptionCallbacks", (void **)&s_managedSetSubscriptionCallbacks},
        {L"SetServerCallbacks", (void **)&s_managedSetServerCallbacks},
        {L"FireChunkLoad", (void **)&s_managedFireChunkLoad},
        {L"FireChunkUnload", (void **)&s_managedFireChunkUnload},
    };

    bool ok = true;
    for (const auto &e : entries)
    {
        ok = ok && GetManagedEntryPoint(loadAssembly, asmPath, typeName, e.name, e.target);
    }

    if (!ok)
    {
        LogError("fourkit", "Not all managed entry points resolved. FourKit will be disabled.");
        return;
    }

    s_initialized = true;

    s_managedInit();

    s_managedSetCallbacks(
        (void *)&NativeDamagePlayer,
        (void *)&NativeSetPlayerHealth,
        (void *)&NativeTeleportPlayer,
        (void *)&NativeSetPlayerGameMode,
        (void *)&NativeBroadcastMessage,
        (void *)&NativeSetFallDistance,
        (void *)&NativeGetPlayerSnapshot,
        (void *)&NativeSendMessage,
        (void *)&NativeSetWalkSpeed,
        (void *)&NativeTeleportEntity);

    s_managedSetWorldCallbacks(
        (void *)&NativeGetTileId,
        (void *)&NativeGetTileData,
        (void *)&NativeSetTile,
        (void *)&NativeSetTileData,
        (void *)&NativeBreakBlock,
        (void *)&NativeGetHighestBlockY,
        (void *)&NativeGetWorldInfo,
        (void *)&NativeSetWorldTime,
        (void *)&NativeSetWeather,
        (void *)&NativeCreateExplosion,
        (void *)&NativeStrikeLightning,
        (void *)&NativeSetSpawnLocation,
        (void *)&NativeDropItem);

    s_managedSetPlayerCallbacks(
        (void *)&NativeKickPlayer,
        (void *)&NativeBanPlayer,
        (void *)&NativeBanPlayerIp,
        (void *)&NativeGetPlayerAddress,
        (void *)&NativeGetPlayerLatency);

    s_managedSetPlayerConnectionCallbacks(
        (void*)&NativeSendRaw);

    s_managedSetInventoryCallbacks(
        (void *)&NativeGetPlayerInventory,
        (void *)&NativeSetPlayerInventorySlot,
        (void *)&NativeGetContainerContents,
        (void *)&NativeSetContainerSlot,
        (void *)&NativeGetContainerViewerEntityIds,
        (void *)&NativeCloseContainer,
        (void *)&NativeOpenVirtualContainer,
        (void *)&NativeGetItemMeta,
        (void *)&NativeSetItemMeta,
        (void *)&NativeSetHeldItemSlot,
        (void *)&NativeGetCarriedItem,
        (void *)&NativeSetCarriedItem,
        (void *)&NativeGetEnderChestContents,
        (void *)&NativeSetEnderChestSlot);

    s_managedSetEntityCallbacks(
        (void *)&NativeSetSneaking,
        (void *)&NativeSetVelocity,
        (void *)&NativeSetAllowFlight,
        (void *)&NativePlaySound,
        (void *)&NativeSetSleepingIgnored);

    s_managedSetExperienceCallbacks(
        (void *)&NativeSetLevel,
        (void *)&NativeSetExp,
        (void *)&NativeGiveExp,
        (void *)&NativeGiveExpLevels,
        (void *)&NativeSetFoodLevel,
        (void *)&NativeSetSaturation,
        (void *)&NativeSetExhaustion);

    s_managedSetParticleCallbacks(
        (void *)&NativeSpawnParticle);

    s_managedSetVehicleCallbacks(
        (void *)&NativeSetPassenger,
        (void *)&NativeLeaveVehicle,
        (void *)&NativeEject,
        (void *)&NativeGetVehicleId,
        (void *)&NativeGetPassengerId,
        (void *)&NativeGetEntityInfo);

    s_managedSetChunkCallbacks(
        (void *)&NativeIsChunkLoaded,
        (void *)&NativeLoadChunk,
        (void *)&NativeUnloadChunk,
        (void *)&NativeGetLoadedChunks,
        (void *)&NativeIsChunkInUse,
        (void *)&NativeGetChunkSnapshot,
        (void *)&NativeUnloadChunkRequest,
        (void *)&NativeRegenerateChunk,
        (void *)&NativeRefreshChunk);

    s_managedSetBlockInfoCallbacks(
        (void *)&NativeGetSkyLight,
        (void *)&NativeGetBlockLight,
        (void *)&NativeGetBiomeId,
        (void *)&NativeSetBiomeId);

    s_managedSetWorldEntityCallbacks(
        (void *)&NativeGetWorldEntities,
        (void *)&NativeGetChunkEntities);

    s_managedSetSubscriptionCallbacks(
        (void *)&NativeSetHandlerMask);

    s_managedSetServerCallbacks(
        (void *)&NativeGetServerTickCount);

    LogInfo("fourkit", "FourKit initialized successfully.");
}

void Shutdown()
{
    if (!s_initialized)
    {
        return;
    }

    LogInfo("fourkit", "FourKit shutting down...");
    s_managedShutdown();
    s_initialized = false;
    LogInfo("fourkit", "FourKit shut down.");
}

void FireWorldSave()
{
    if (!s_initialized || !s_managedFireWorldSave)
    {
        return;
    }

    s_managedFireWorldSave();

    LogDebugf("fourkit", "Fired WorldSave");
}

bool FirePlayerPreLogin(const std::wstring& name, const std::string& ip, int port)
{
    if (!s_initialized || !s_managedFirePreLogin)
    {
        return true;
    }

    std::string nameUtf8 = ServerRuntime::StringUtils::WideToUtf8(name);
    int canceled = s_managedFirePreLogin(
        nameUtf8.empty() ? "" : nameUtf8.data(), (int)nameUtf8.size(),
        ip.empty() ? "" : ip.data(), (int)ip.size(),
        port);

    LogDebugf("fourkit", "Fired PlayerPreLogin: %s", nameUtf8.data());

    return canceled != 0;
}

bool FirePlayerLogin(const std::wstring& name, const std::string& ip, int port, int type, unsigned long long* onlineXUID, unsigned long long* offlineXUID)
{
    if (!s_initialized || !s_managedFireLogin)
    {
        return true;
    }
    
    std::string nameUtf8 = ServerRuntime::StringUtils::WideToUtf8(name);

    int canceled = s_managedFireLogin(
        nameUtf8.empty() ? "" : nameUtf8.data(), (int)nameUtf8.size(), 
        ip.empty() ? "" : ip.data(), (int)ip.size(), 
        port, type, offlineXUID, onlineXUID);

    LogDebugf("fourkit", "Fired PlayerLogin: %s", nameUtf8.data());

    return canceled != 0;
}

void FirePlayerJoin(int entityId, const std::wstring &name, const std::wstring &uuid, unsigned long long onlineXUID, unsigned long long offlineXUID)
{
    if (!s_initialized || !s_managedFireJoin)
    {
        return;
    }

    std::string nameUtf8 = ServerRuntime::StringUtils::WideToUtf8(name);
    std::string uuidUtf8 = ServerRuntime::StringUtils::WideToUtf8(uuid);
    s_managedFireJoin(entityId,
                      nameUtf8.empty() ? "" : nameUtf8.data(), (int)nameUtf8.size(),
                      uuidUtf8.empty() ? "" : uuidUtf8.data(), (int)uuidUtf8.size(),
                      offlineXUID, onlineXUID);
    LogDebugf("fourkit", "Fired PlayerJoin: entityId=%d", entityId);
}

bool FirePlayerQuit(int entityId)
{
    s_openContainerInfo.erase(entityId);

    if (!s_initialized || !s_managedFireQuit)
    {
        return false;
    }

    s_managedFireQuit(entityId);
    LogDebugf("fourkit", "Fired PlayerQuit: entityId=%d", entityId);
    return true;
}

bool FirePlayerKick(int entityId, int disconnectReason,
                    std::wstring &outLeaveMessage)
{
    if (!s_initialized || !s_managedFireKick)
    {
        return false;
    }

    std::string reasonUtf8 = std::to_string(disconnectReason);

    const int kBufSize = 2048;
    char outBuf[kBufSize] = {};
    int outLen = 0;

    int cancelled = s_managedFireKick(entityId, disconnectReason,
                                      reasonUtf8.empty() ? "" : reasonUtf8.data(), (int)reasonUtf8.size(),
                                      outBuf, kBufSize, &outLen);

    if (outLen > 0 && outLen < kBufSize)
    {
        std::string resultUtf8(outBuf, outLen);
        outLeaveMessage = ServerRuntime::StringUtils::Utf8ToWide(resultUtf8);
    }

    LogDebugf("fourkit", "Fired PlayerKick: entityId=%d cancelled=%d", entityId, cancelled);
    return cancelled != 0;
}

void UpdatePlayerEntityId(int oldEntityId, int newEntityId)
{
    auto it = s_openContainerInfo.find(oldEntityId);
    if (it != s_openContainerInfo.end())
    {
        s_openContainerInfo[newEntityId] = it->second;
        s_openContainerInfo.erase(it);
    }

    if (!s_initialized || !s_managedUpdateEntityId)
    {
        return;
    }
    s_managedUpdateEntityId(oldEntityId, newEntityId);
    LogDebugf("fourkit", "UpdatePlayerEntityId: %d -> %d", oldEntityId, newEntityId);
}

bool FirePlayerMove(int entityId,
                    double fromX, double fromY, double fromZ,
                    double toX, double toY, double toZ,
                    double *outToX, double *outToY, double *outToZ)
{
    // Caller reads outTo* unconditionally; init on every early-return.
    if (!s_initialized || !s_managedFireMove || !HasHandlers(kHandlerKind_PlayerMove))
    {
        *outToX = toX;
        *outToY = toY;
        *outToZ = toZ;
        return false;
    }

    double outCoords[3] = {toX, toY, toZ};
    int cancelled = s_managedFireMove(entityId,
                                      fromX, fromY, fromZ,
                                      toX, toY, toZ,
                                      outCoords);

    *outToX = outCoords[0];
    *outToY = outCoords[1];
    *outToZ = outCoords[2];
    return cancelled != 0;
}

bool FireStructureGrow(int dimId, int x, int y, int z, int treeType, bool wasBonemeal, int entityId)
{
    if (!s_initialized || !s_managedFireStructureGrow)
    {
        return false;
    }

    int cancelled = s_managedFireStructureGrow(dimId, x, y, z, treeType, wasBonemeal ? 1 : 0, entityId);

    if (cancelled != 0)
    {
        return true;
    }

    return false;
}

bool FirePlayerChat(int entityId,
                    const std::wstring &message,
                    std::wstring &outFormatted)
{
    if (!s_initialized || !s_managedFireChat)
    {
        return false;
    }

    std::string msgUtf8 = ServerRuntime::StringUtils::WideToUtf8(message);

    const int kBufSize = 2048;
    char outBuf[kBufSize] = {};
    int outLen = 0;

    int cancelled = s_managedFireChat(entityId,
                                      msgUtf8.empty() ? "" : msgUtf8.data(), (int)msgUtf8.size(),
                                      outBuf, kBufSize, &outLen);

    if (cancelled != 0)
    {
        return true;
    }

    if (outLen > 0 && outLen < kBufSize)
    {
        std::string resultUtf8(outBuf, outLen);
        outFormatted = ServerRuntime::StringUtils::Utf8ToWide(resultUtf8);
    }

    return false;
}

bool FireBlockPlace(int entityId, int dimId,
                    int placedX, int placedY, int placedZ,
                    int againstX, int againstY, int againstZ,
                    int itemId, int itemCount, bool canBuild)
{
    if (!s_initialized || !s_managedFireBlockPlace)
    {
        return false;
    }

    int cancelled = s_managedFireBlockPlace(entityId, dimId,
                                            placedX, placedY, placedZ,
                                            againstX, againstY, againstZ,
                                            itemId, itemCount, canBuild ? 1 : 0);
    return cancelled != 0;
}

int FireBlockBreak(int entityId, int dimId,
                   int x, int y, int z, int tileId, int data, int exp)
{
    if (!s_initialized || !s_managedFireBlockBreak)
    {
        return exp;
    }

    return s_managedFireBlockBreak(entityId, dimId, x, y, z, tileId, data, exp);
}

bool FireEntityDamage(int entityId, int entityTypeId, int dimId,
                      double x, double y, double z, int causeId, double damage,
                      double *outDamage,
                      int damagerEntityId, int damagerEntityTypeId,
                      double damagerX, double damagerY, double damagerZ)
{
    if (!s_initialized || !s_managedFireEntityDamage)
    {
        *outDamage = damage;
        return false;
    }

    double outDmg = damage;
    int cancelled = s_managedFireEntityDamage(entityId, entityTypeId, dimId,
                                              x, y, z, causeId, damage, &outDmg,
                                              damagerEntityId, damagerEntityTypeId, damagerX, damagerY, damagerZ);
    *outDamage = outDmg;
    return cancelled != 0;
}

bool FireSignChange(int entityId, int dimId,
                    int x, int y, int z,
                    const std::wstring &line0, const std::wstring &line1,
                    const std::wstring &line2, const std::wstring &line3,
                    std::wstring outLines[4])
{
    if (!s_initialized || !s_managedFireSignChange)
    {
        return false;
    }

    std::string l0 = ServerRuntime::StringUtils::WideToUtf8(line0);
    std::string l1 = ServerRuntime::StringUtils::WideToUtf8(line1);
    std::string l2 = ServerRuntime::StringUtils::WideToUtf8(line2);
    std::string l3 = ServerRuntime::StringUtils::WideToUtf8(line3);

    const int kBufSize = 512;
    char outBuf[kBufSize] = {};
    int outLens[4] = {};

    int cancelled = s_managedFireSignChange(entityId, dimId,
                                            x, y, z,
                                            l0.empty() ? "" : l0.data(), (int)l0.size(),
                                            l1.empty() ? "" : l1.data(), (int)l1.size(),
                                            l2.empty() ? "" : l2.data(), (int)l2.size(),
                                            l3.empty() ? "" : l3.data(), (int)l3.size(),
                                            outBuf, kBufSize, outLens);

    int offset = 0;
    for (int i = 0; i < 4; i++)
    {
        if (outLens[i] > 0 && offset + outLens[i] <= kBufSize)
        {
            std::string s(outBuf + offset, outLens[i]);
            outLines[i] = ServerRuntime::StringUtils::Utf8ToWide(s);
        }
        else
        {
            outLines[i] = (i == 0 ? line0 : (i == 1 ? line1 : (i == 2 ? line2 : line3)));
        }
        offset += outLens[i];
    }

    return cancelled != 0;
}

int FireEntityDeath(int entityId, int entityTypeId, int dimId,
                    double x, double y, double z, int exp)
{
    if (!s_initialized || !s_managedFireEntityDeath)
    {
        return exp;
    }

    return s_managedFireEntityDeath(entityId, entityTypeId, dimId, x, y, z, exp);
}

int FirePlayerDeath(int entityId, const std::wstring &deathMessage, int exp,
                    std::wstring &outDeathMessage, int *outKeepInventory,
                    int *outNewExp, int *outNewLevel, int *outKeepLevel)
{
    if (!s_initialized || !s_managedFirePlayerDeath)
    {
        outDeathMessage = deathMessage;
        *outKeepInventory = 0;
        *outNewExp = 0;
        *outNewLevel = 0;
        *outKeepLevel = 0;
        return exp;
    }

    std::string msgUtf8 = ServerRuntime::StringUtils::WideToUtf8(deathMessage);

    const int kBufSize = 2048;
    char outMsgBuf[kBufSize] = {};
    int outMsgLen = 0;
    int keepInv = 0;
    int newExp = 0, newLevel = 0, keepLevel = 0;

    int outExp = s_managedFirePlayerDeath(entityId,
                                          msgUtf8.empty() ? "" : msgUtf8.data(), (int)msgUtf8.size(), exp,
                                          outMsgBuf, kBufSize, &outMsgLen, &keepInv,
                                          &newExp, &newLevel, &keepLevel);

    if (outMsgLen > 0 && outMsgLen < kBufSize)
    {
        std::string resultUtf8(outMsgBuf, outMsgLen);
        outDeathMessage = ServerRuntime::StringUtils::Utf8ToWide(resultUtf8);
    }
    else
    {
        outDeathMessage = deathMessage;
    }

    *outKeepInventory = keepInv;
    *outNewExp = newExp;
    *outNewLevel = newLevel;
    *outKeepLevel = keepLevel;
    return outExp;
}

bool FirePlayerDropItem(int entityId, int itemId, int itemCount, int itemAux,
                        int *outItemId, int *outItemCount, int *outItemAux)
{
    *outItemId = itemId;
    *outItemCount = itemCount;
    *outItemAux = itemAux;

    if (!s_initialized || !s_managedFirePlayerDropItem)
    {
        return false;
    }

    int newId = itemId, newCount = itemCount, newAux = itemAux;
    long long result = s_managedFirePlayerDropItem(entityId, itemId, itemCount, itemAux,
                                                   &newId, &newCount, &newAux);

    *outItemId = newId;
    *outItemCount = newCount;
    *outItemAux = newAux;

    return result != 0;
}

bool FirePlayerInteract(int entityId, int action,
                        int itemId, int itemCount, int itemAux,
                        int clickedX, int clickedY, int clickedZ,
                        int blockFace, int dimId,
                        int *outUseItemInHand)
{
    *outUseItemInHand = 1;

    if (!s_initialized || !s_managedFirePlayerInteract)
    {
        return false;
    }

    int useItem = 1;
    int result = s_managedFirePlayerInteract(entityId, action,
                                             itemId, itemCount, itemAux,
                                             clickedX, clickedY, clickedZ,
                                             blockFace, dimId,
                                             &useItem);

    *outUseItemInHand = useItem;
    return result != 0;
}

bool FirePlayerInteractEntity(int playerEntityId,
                              int targetEntityId, int targetEntityTypeId,
                              int dimId, double targetX, double targetY, double targetZ,
                              float targetHealth, float targetMaxHealth, float targetEyeHeight)
{
    if (!s_initialized || !s_managedFirePlayerInteractEntity)
    {
        return false;
    }

    int result = s_managedFirePlayerInteractEntity(playerEntityId,
                                                   targetEntityId, targetEntityTypeId,
                                                   dimId, targetX, targetY, targetZ,
                                                   targetHealth, targetMaxHealth, targetEyeHeight);

    return result != 0;
}

bool FirePlayerPickupItem(int playerEntityId,
                          int itemEntityId, int dimId, double itemX, double itemY, double itemZ,
                          int itemId, int itemCount, int itemAux, int remaining,
                          int *outItemId, int *outItemCount, int *outItemAux)
{
    if (!s_initialized || !s_managedFirePlayerPickupItem)
    {
        *outItemId = itemId;
        *outItemCount = itemCount;
        *outItemAux = itemAux;
        return false;
    }

    *outItemId = itemId;
    *outItemCount = itemCount;
    *outItemAux = itemAux;

    int result = s_managedFirePlayerPickupItem(playerEntityId,
                                               itemEntityId, dimId, itemX, itemY, itemZ,
                                               itemId, itemCount, itemAux, remaining,
                                               outItemId, outItemCount, outItemAux);

    return result != 0;
}

bool FireInventoryOpen(int entityId, int nativeContainerType,
                       const std::wstring &title, int containerSize)
{
    if (!s_initialized || !s_managedFireInventoryOpen)
    {
        return false;
    }

    s_openContainerInfo[entityId] = {nativeContainerType, containerSize, title};

    std::string titleUtf8 = ServerRuntime::StringUtils::WideToUtf8(title);

    int cancelled = s_managedFireInventoryOpen(entityId, nativeContainerType,
                                               titleUtf8.empty() ? "" : titleUtf8.data(), (int)titleUtf8.size(),
                                               containerSize);

    return cancelled != 0;
}

bool HandlePlayerCommand(int entityId, const std::wstring &commandLine)
{
    if (!s_initialized || !s_managedHandlePlayerCommand)
    {
        return false;
    }

    std::string cmdUtf8 = ServerRuntime::StringUtils::WideToUtf8(commandLine);

    int handled = s_managedHandlePlayerCommand(entityId,
                                               cmdUtf8.empty() ? "" : cmdUtf8.data(), (int)cmdUtf8.size());

    return handled != 0;
}

bool HandleConsoleCommand(const std::string &commandLine)
{
    if (!s_initialized || !s_managedHandleConsoleCommand)
    {
        return false;
    }

    int handled = s_managedHandleConsoleCommand(
        commandLine.empty() ? "" : commandLine.data(), (int)commandLine.size());

    return handled != 0;
}

int GetPluginCommandHelp(char *outBuf, int outBufSize, int *outLen)
{
    if (!s_initialized || !s_managedGetPluginCommandHelp)
    {
        return 0;
    }

    return s_managedGetPluginCommandHelp(outBuf, outBufSize, outLen);
}

static bool FireSpatialEvent(fn_fire_player_teleport managedFn,
                             int entityId,
                             double fromX, double fromY, double fromZ, int fromDimId,
                             double toX, double toY, double toZ, int toDimId,
                             int cause,
                             double *outToX, double *outToY, double *outToZ)
{
    if (!s_initialized || !managedFn)
    {
        return false;
    }

    double outCoords[3] = {toX, toY, toZ};
    int cancelled = managedFn(entityId,
                              fromX, fromY, fromZ, fromDimId,
                              toX, toY, toZ, toDimId,
                              cause, outCoords);

    *outToX = outCoords[0];
    *outToY = outCoords[1];
    *outToZ = outCoords[2];
    return cancelled != 0;
}

bool FirePlayerTeleport(int entityId,
                        double fromX, double fromY, double fromZ, int fromDimId,
                        double toX, double toY, double toZ, int toDimId,
                        int cause,
                        double *outToX, double *outToY, double *outToZ)
{
    return FireSpatialEvent(s_managedFirePlayerTeleport, entityId,
                            fromX, fromY, fromZ, fromDimId,
                            toX, toY, toZ, toDimId,
                            cause, outToX, outToY, outToZ);
}

bool FirePlayerPortal(int entityId,
                      double fromX, double fromY, double fromZ, int fromDimId,
                      double toX, double toY, double toZ, int toDimId,
                      int cause,
                      double *outToX, double *outToY, double *outToZ)
{
    return FireSpatialEvent(s_managedFirePlayerPortal, entityId,
                            fromX, fromY, fromZ, fromDimId,
                            toX, toY, toZ, toDimId,
                            cause, outToX, outToY, outToZ);
}

int FireInventoryClick(int entityId, int slot, int button, int clickType)
{
    if (!s_initialized || !s_managedFireInventoryClick)
    {
        return 0;
    }

    int nativeContainerType = -1;
    int containerSize = 0;
    std::string titleUtf8;
    auto it = s_openContainerInfo.find(entityId);
    if (it != s_openContainerInfo.end())
    {
        nativeContainerType = it->second.type;
        containerSize = it->second.size;
        if (!it->second.title.empty())
        {
            titleUtf8 = ServerRuntime::StringUtils::WideToUtf8(it->second.title);
        }
    }

    return s_managedFireInventoryClick(entityId, slot, button, clickType, nativeContainerType, containerSize,
                                       titleUtf8.empty() ? nullptr : titleUtf8.data(), (int)titleUtf8.size());
}

bool FireBedEnter(int entityId, int dimId, int bedX, int bedY, int bedZ)
{
    if (!s_initialized || !s_managedFireBedEnter)
        return false;
    return s_managedFireBedEnter(entityId, dimId, bedX, bedY, bedZ) != 0;
}

void FireBedLeave(int entityId, int dimId, int bedX, int bedY, int bedZ)
{
    if (!s_initialized || !s_managedFireBedLeave)
        return;
    s_managedFireBedLeave(entityId, dimId, bedX, bedY, bedZ);
}

bool FireBlockGrow(int dimId, int x, int y, int z, int newTileId, int newTileData)
{
    if (!s_initialized || !s_managedFireBlockGrow)
        return false;
    return s_managedFireBlockGrow(dimId, x, y, z, newTileId, newTileData) != 0;
}

bool FireBlockForm(int dimId, int x, int y, int z, int newTileId, int newTileData)
{
    if (!s_initialized || !s_managedFireBlockForm)
        return false;
    return s_managedFireBlockForm(dimId, x, y, z, newTileId, newTileData) != 0;
}

bool FireBlockBurn(int dimId, int x, int y, int z)
{
    if (!s_initialized || !s_managedFireBlockBurn)
        return false;
    return s_managedFireBlockBurn(dimId, x, y, z) != 0;
}

bool FireBlockSpread(int dimId, int x, int y, int z, int srcX, int srcY, int srcZ, int newTileId, int newTileData)
{
    if (!s_initialized || !s_managedFireBlockSpread)
        return false;
    return s_managedFireBlockSpread(dimId, x, y, z, srcX, srcY, srcZ, newTileId, newTileData) != 0;
}

bool FirePistonExtend(int dimId, int x, int y, int z, int direction, int length)
{
    if (!s_initialized || !s_managedFirePistonExtend)
        return false;
    return s_managedFirePistonExtend(dimId, x, y, z, direction, length) != 0;
}

bool FirePistonRetract(int dimId, int x, int y, int z, int direction)
{
    if (!s_initialized || !s_managedFirePistonRetract)
        return false;
    return s_managedFirePistonRetract(dimId, x, y, z, direction) != 0;
}

bool FireCommandPreprocess(int entityId, const std::wstring &commandLine, std::wstring &outCommand)
{
    if (!s_initialized || !s_managedFireCommandPreprocess)
    {
        return false;
    }

    std::string cmdUtf8 = ServerRuntime::StringUtils::WideToUtf8(commandLine);

    const int kBufSize = 2048;
    char outBuf[kBufSize] = {};
    int outLen = 0;

    int cancelled = s_managedFireCommandPreprocess(entityId,
                                                    cmdUtf8.empty() ? "" : cmdUtf8.data(), (int)cmdUtf8.size(),
                                                    outBuf, kBufSize, &outLen);

    if (cancelled != 0)
    {
        return true;
    }

    if (outLen > 0 && outLen < kBufSize)
    {
        std::string resultUtf8(outBuf, outLen);
        outCommand = ServerRuntime::StringUtils::Utf8ToWide(resultUtf8);
    }
    else
    {
        outCommand = commandLine;
    }

    return false;
}

bool FireBlockFromTo(int dimId, int fromX, int fromY, int fromZ, int toX, int toY, int toZ, int face)
{
    if (!s_initialized || !s_managedFireBlockFromTo)
        return false;
    return s_managedFireBlockFromTo(dimId, fromX, fromY, fromZ, toX, toY, toZ, face) != 0;
}

void FireChunkLoad(int dimId, int chunkX, int chunkZ, bool isNewChunk)
{
    if (!s_initialized || !s_managedFireChunkLoad)
        return;
    if (!HasHandlers(kHandlerKind_ChunkLoad))
        return;
    s_managedFireChunkLoad(dimId, chunkX, chunkZ, isNewChunk ? 1 : 0);
}

bool FireChunkUnload(int dimId, int chunkX, int chunkZ)
{
    if (!s_initialized || !s_managedFireChunkUnload)
        return false;
    if (!HasHandlers(kHandlerKind_ChunkUnload))
        return false;
    return s_managedFireChunkUnload(dimId, chunkX, chunkZ) != 0;
}
} // namespace FourKitBridge
