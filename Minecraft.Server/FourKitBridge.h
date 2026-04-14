#pragma once
#include <string>

// FourKit plugin host bridge.
//
// In the FourKit-enabled server build, MINECRAFT_SERVER_FOURKIT_BUILD is
// defined and the real implementations live in FourKitBridge.cpp /
// FourKitNatives.cpp / FourKitRuntime.cpp / FourKitMappers.cpp.
//
// In the standalone server build (no plugin support), the macro is undefined
// and every entry point becomes an inline no-op stub. This lets gameplay code
// in Minecraft.Server / Minecraft.Client / Minecraft.World call into the
// bridge unconditionally without per-call-site #ifdefs. Cancellable hooks
// return false (do not cancel) so vanilla behaviour is preserved.

namespace FourKitBridge
{
#ifdef MINECRAFT_SERVER_FOURKIT_BUILD
    void Initialize();
    void Shutdown();
    void FireWorldSave();
    bool FirePlayerPreLogin(const std::wstring& name, const std::string& ip, int port);
    bool FirePlayerLogin(const std::wstring& name, const std::string& ip, int port, int type, unsigned long long* onlineXUID, unsigned long long* offlineXUID);
    void FirePlayerJoin(int entityId, const std::wstring& name, const std::wstring& uuid, unsigned long long onlineXUID, unsigned long long offlineXUID);
    bool FirePlayerQuit(int entityId);
    bool FirePlayerKick(int entityId, int disconnectReason,
                        std::wstring &outLeaveMessage);
    bool FirePlayerMove(int entityId,
                        double fromX, double fromY, double fromZ,
                        double toX, double toY, double toZ,
                        double *outToX, double *outToY, double *outToZ);
    bool FireStructureGrow(int dimId, int x, int y, int z, int treeType, bool wasBonemeal = false, int entityId = -1);
    bool FirePlayerChat(int entityId,
                        const std::wstring &message,
                        std::wstring &outFormatted);
    void UpdatePlayerEntityId(int oldEntityId, int newEntityId);

    int GetTileId(int dimId, int x, int y, int z);
    int GetTileData(int dimId, int x, int y, int z);
    void SetTile(int dimId, int x, int y, int z, int tileId, int data);
    void SetTileData(int dimId, int x, int y, int z, int data);
    int BreakBlock(int dimId, int x, int y, int z);
    int GetHighestBlockY(int dimId, int x, int z);
    bool FireBlockPlace(int entityId, int dimId,
                        int placedX, int placedY, int placedZ,
                        int againstX, int againstY, int againstZ,
                        int itemId, int itemCount, bool canBuild);
    int FireBlockBreak(int entityId, int dimId,
                       int x, int y, int z, int tileId, int data, int exp);
    bool FireEntityDamage(int entityId, int entityTypeId, int dimId,
                          double x, double y, double z, int causeId, double damage,
                          double *outDamage,
                          int damagerEntityId, int damagerEntityTypeId,
                          double damagerX, double damagerY, double damagerZ);
    bool FireSignChange(int entityId, int dimId,
                        int x, int y, int z,
                        const std::wstring &line0, const std::wstring &line1,
                        const std::wstring &line2, const std::wstring &line3,
                        std::wstring outLines[4]);
    int FireEntityDeath(int entityId, int entityTypeId, int dimId,
                        double x, double y, double z, int exp);
    int FirePlayerDeath(int entityId, const std::wstring &deathMessage, int exp,
                        std::wstring &outDeathMessage, int *outKeepInventory,
                        int *outNewExp, int *outNewLevel, int *outKeepLevel);
    int MapEntityType(int nativeType);
    int MapDamageCause(void *source);
    bool FirePlayerDropItem(int entityId, int itemId, int itemCount, int itemAux,
                            int *outItemId, int *outItemCount, int *outItemAux);
    bool FirePlayerInteract(int entityId, int action,
                            int itemId, int itemCount, int itemAux,
                            int clickedX, int clickedY, int clickedZ,
                            int blockFace, int dimId,
                            int *outUseItemInHand);
    bool FirePlayerInteractEntity(int playerEntityId,
                                  int targetEntityId, int targetEntityTypeId,
                                  int dimId, double targetX, double targetY, double targetZ,
                                  float targetHealth, float targetMaxHealth, float targetEyeHeight);
    bool FirePlayerPickupItem(int playerEntityId,
                              int itemEntityId, int dimId, double itemX, double itemY, double itemZ,
                              int itemId, int itemCount, int itemAux, int remaining,
                              int *outItemId, int *outItemCount, int *outItemAux);
    bool FireInventoryOpen(int entityId, int nativeContainerType,
                           const std::wstring &title, int containerSize);
    bool HandlePlayerCommand(int entityId, const std::wstring &commandLine);
    bool HandleConsoleCommand(const std::string &commandLine);
    int GetPluginCommandHelp(char *outBuf, int outBufSize, int *outLen);
    bool FirePlayerTeleport(int entityId,
                            double fromX, double fromY, double fromZ, int fromDimId,
                            double toX, double toY, double toZ, int toDimId,
                            int cause,
                            double *outToX, double *outToY, double *outToZ);
    bool FirePlayerPortal(int entityId,
                          double fromX, double fromY, double fromZ, int fromDimId,
                          double toX, double toY, double toZ, int toDimId,
                          int cause,
                          double *outToX, double *outToY, double *outToZ);
    int FireInventoryClick(int entityId, int slot, int button, int clickType);
    bool FireBedEnter(int entityId, int dimId, int bedX, int bedY, int bedZ);
    void FireBedLeave(int entityId, int dimId, int bedX, int bedY, int bedZ);
    bool FireBlockGrow(int dimId, int x, int y, int z, int newTileId, int newTileData);
    bool FireBlockForm(int dimId, int x, int y, int z, int newTileId, int newTileData);
    bool FireBlockBurn(int dimId, int x, int y, int z);
    bool FireBlockSpread(int dimId, int x, int y, int z, int srcX, int srcY, int srcZ, int newTileId, int newTileData);
    bool FirePistonExtend(int dimId, int x, int y, int z, int direction, int length);
    bool FirePistonRetract(int dimId, int x, int y, int z, int direction);
    bool FireCommandPreprocess(int entityId, const std::wstring &commandLine, std::wstring &outCommand);
    bool FireBlockFromTo(int dimId, int fromX, int fromY, int fromZ, int toX, int toY, int toZ, int face);
    void FireChunkLoad(int dimId, int chunkX, int chunkZ, bool isNewChunk);
    bool FireChunkUnload(int dimId, int chunkX, int chunkZ);
#else
    // Standalone build: every hook is an inline no-op. Cancellable hooks
    // return false so vanilla code paths run unmodified, AND every out
    // parameter is defaulted to its corresponding input value (or to a
    // benign default like 0 / empty string) so callers do not read
    // uninitialized stack memory. Reading uninitialized out-params would
    // pass garbage into anti-cheat / damage / inventory checks and break
    // the vanilla server build.
    inline void Initialize() {}
    inline void Shutdown() {}
    inline void FireWorldSave() {}
    inline bool FirePlayerPreLogin(const std::wstring&, const std::string&, int) { return false; }
    inline bool FirePlayerLogin(const std::wstring&, const std::string&, int, int, unsigned long long*, unsigned long long*) { return false; }
    inline void FirePlayerJoin(int, const std::wstring&, const std::wstring&, unsigned long long, unsigned long long) {}
    inline bool FirePlayerQuit(int) { return false; }
    inline bool FirePlayerKick(int, int, std::wstring &outLeaveMessage) { outLeaveMessage.clear(); return false; }
    inline bool FirePlayerMove(int, double, double, double, double toX, double toY, double toZ, double *outToX, double *outToY, double *outToZ)
    {
        if (outToX) *outToX = toX;
        if (outToY) *outToY = toY;
        if (outToZ) *outToZ = toZ;
        return false;
    }
    inline bool FireStructureGrow(int, int, int, int, int, bool = false, int = -1) { return false; }
    inline bool FirePlayerChat(int, const std::wstring&, std::wstring &outFormatted) { outFormatted.clear(); return false; }
    inline void UpdatePlayerEntityId(int, int) {}

    inline int GetTileId(int, int, int, int) { return 0; }
    inline int GetTileData(int, int, int, int) { return 0; }
    inline void SetTile(int, int, int, int, int, int) {}
    inline void SetTileData(int, int, int, int, int) {}
    inline int BreakBlock(int, int, int, int) { return 0; }
    inline int GetHighestBlockY(int, int, int) { return 0; }
    inline bool FireBlockPlace(int, int, int, int, int, int, int, int, int, int, bool) { return false; }
    inline int FireBlockBreak(int, int, int, int, int, int, int, int exp) { return exp; }
    inline bool FireEntityDamage(int, int, int, double, double, double, int, double damage, double *outDamage, int, int, double, double, double)
    {
        if (outDamage) *outDamage = damage;
        return false;
    }
    inline bool FireSignChange(int, int, int, int, int, const std::wstring &line0, const std::wstring &line1, const std::wstring &line2, const std::wstring &line3, std::wstring outLines[4])
    {
        if (outLines) { outLines[0] = line0; outLines[1] = line1; outLines[2] = line2; outLines[3] = line3; }
        return false;
    }
    inline int FireEntityDeath(int, int, int, double, double, double, int exp) { return exp; }
    inline int FirePlayerDeath(int, const std::wstring&, int exp, std::wstring &outDeathMessage, int *outKeepInventory, int *outNewExp, int *outNewLevel, int *outKeepLevel)
    {
        outDeathMessage.clear();
        if (outKeepInventory) *outKeepInventory = 0;
        if (outNewExp) *outNewExp = 0;
        if (outNewLevel) *outNewLevel = 0;
        if (outKeepLevel) *outKeepLevel = 0;
        return exp;
    }
    inline int MapEntityType(int nativeType) { return nativeType; }
    inline int MapDamageCause(void*) { return 0; }
    inline bool FirePlayerDropItem(int, int itemId, int itemCount, int itemAux, int *outItemId, int *outItemCount, int *outItemAux)
    {
        if (outItemId) *outItemId = itemId;
        if (outItemCount) *outItemCount = itemCount;
        if (outItemAux) *outItemAux = itemAux;
        return false;
    }
    inline bool FirePlayerInteract(int, int, int, int, int, int, int, int, int, int, int *outUseItemInHand)
    {
        if (outUseItemInHand) *outUseItemInHand = 1;
        return false;
    }
    inline bool FirePlayerInteractEntity(int, int, int, int, double, double, double, float, float, float) { return false; }
    inline bool FirePlayerPickupItem(int, int, int, double, double, double, int itemId, int itemCount, int itemAux, int, int *outItemId, int *outItemCount, int *outItemAux)
    {
        if (outItemId) *outItemId = itemId;
        if (outItemCount) *outItemCount = itemCount;
        if (outItemAux) *outItemAux = itemAux;
        return false;
    }
    inline bool FireInventoryOpen(int, int, const std::wstring&, int) { return false; }
    inline bool HandlePlayerCommand(int, const std::wstring&) { return false; }
    inline bool HandleConsoleCommand(const std::string&) { return false; }
    inline int GetPluginCommandHelp(char*, int, int *outLen) { if (outLen) *outLen = 0; return 0; }
    inline bool FirePlayerTeleport(int, double, double, double, int, double toX, double toY, double toZ, int, int, double *outToX, double *outToY, double *outToZ)
    {
        if (outToX) *outToX = toX;
        if (outToY) *outToY = toY;
        if (outToZ) *outToZ = toZ;
        return false;
    }
    inline bool FirePlayerPortal(int, double, double, double, int, double toX, double toY, double toZ, int, int, double *outToX, double *outToY, double *outToZ)
    {
        if (outToX) *outToX = toX;
        if (outToY) *outToY = toY;
        if (outToZ) *outToZ = toZ;
        return false;
    }
    inline int FireInventoryClick(int, int, int, int) { return 0; }
    inline bool FireBedEnter(int, int, int, int, int) { return false; }
    inline void FireBedLeave(int, int, int, int, int) {}
    inline bool FireBlockGrow(int, int, int, int, int, int) { return false; }
    inline bool FireBlockForm(int, int, int, int, int, int) { return false; }
    inline bool FireBlockBurn(int, int, int, int) { return false; }
    inline bool FireBlockSpread(int, int, int, int, int, int, int, int, int) { return false; }
    inline bool FirePistonExtend(int, int, int, int, int, int) { return false; }
    inline bool FirePistonRetract(int, int, int, int, int) { return false; }
    inline bool FireCommandPreprocess(int, const std::wstring &commandLine, std::wstring &outCommand) { outCommand = commandLine; return false; }
    inline bool FireBlockFromTo(int, int, int, int, int, int, int, int) { return false; }
    inline void FireChunkLoad(int, int, int, bool) {}
    inline bool FireChunkUnload(int, int, int) { return false; }
#endif
}
