using System.Runtime.InteropServices;
using Minecraft.Server.FourKit.Block;
using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Event;
using Minecraft.Server.FourKit.Event.Block;
using Minecraft.Server.FourKit.Event.Entity;
using Minecraft.Server.FourKit.Event.Player;
using Minecraft.Server.FourKit.Event.Inventory;
using Minecraft.Server.FourKit.Inventory;
using Minecraft.Server.FourKit.Net;
using Minecraft.Server.FourKit.Event.World;
using Minecraft.Server.FourKit.Enums;

namespace Minecraft.Server.FourKit;

public static partial class FourKitHost
{
    [UnmanagedCallersOnly]
    public static void FireWorldSave()
    {
        try
        {
            FourKit.FireEvent(new WorldSaveEvent());
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireWorldSave error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerPreLogin(IntPtr namePtr, int nameByteLen, IntPtr ipPtr, int ipByteLen, int port)
    {
        try
        {
            string name = nameByteLen > 0
                ? Marshal.PtrToStringUTF8(namePtr, nameByteLen) ?? string.Empty
                : string.Empty;

            string ipStr = ipByteLen > 0
                ? Marshal.PtrToStringUTF8(ipPtr, ipByteLen) ?? string.Empty
                : string.Empty;

            var evt = new PlayerPreLoginEvent(name, new InetSocketAddress(new InetAddress(ipStr), port));
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerPreLogin error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerLogin(IntPtr namePtr, int nameByteLen, IntPtr ipPtr, int ipByteLen, int port, int type, IntPtr offlineXUIDPtr, IntPtr onlineXUIDPtr)
    {
        try
        {
            string name = nameByteLen > 0
                ? Marshal.PtrToStringUTF8(namePtr, nameByteLen) ?? string.Empty
                : string.Empty;

            string ipStr = ipByteLen > 0
                ? Marshal.PtrToStringUTF8(ipPtr, ipByteLen) ?? string.Empty
                : string.Empty;

            var evt = new PlayerLoginEvent(name, new InetSocketAddress(new InetAddress(ipStr), port), (LoginType)type, unchecked((ulong)Marshal.ReadInt64(onlineXUIDPtr)), unchecked((ulong)Marshal.ReadInt64(offlineXUIDPtr)));
            FourKit.FireEvent(evt);

            if (evt.hasChangedXuidValues())
            {
                Marshal.WriteInt64(offlineXUIDPtr, unchecked((long)evt.getOfflineXuid()));
                Marshal.WriteInt64(onlineXUIDPtr, unchecked((long)evt.getOnlineXuid()));
            }

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerLogin error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static void FirePlayerJoin(int entityId, IntPtr namePtr, int nameByteLen, IntPtr uuidPtr, int uuidByteLen, ulong offlineXUIDPtr, ulong onlineXUIDPtr)
    {
        try
        {
            string name = nameByteLen > 0
                ? Marshal.PtrToStringUTF8(namePtr, nameByteLen) ?? string.Empty
                : string.Empty;

            string uuidStr = uuidByteLen > 0
                ? Marshal.PtrToStringUTF8(uuidPtr, uuidByteLen) ?? string.Empty
                : string.Empty;

            var player = FourKit.TrackPlayer(entityId, name);
            player.SetPlayerUniqueIdInternal(ParseOrHashGuid(uuidStr));
            player.SetPlayerRawOnlineXUIDInternal(onlineXUIDPtr);
            player.SetPlayerRawOfflineXUIDInternal(offlineXUIDPtr);
            SyncPlayerFromNative(player);
            var evt = new PlayerJoinEvent(player);
            FourKit.FireEvent(evt);
            BroadcastNativeMessage(evt.getJoinMessage());
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerJoin error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static void FirePlayerQuit(int entityId)
    {
        try
        {
            var player = FourKit.UntrackPlayer(entityId);
            if (player != null)
            {
                SyncPlayerFromNative(player);
                var evt = new PlayerQuitEvent(player);
                FourKit.FireEvent(evt);
                BroadcastNativeMessage(evt.getQuitMessage());
            }
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerQuit error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerKick(int entityId, int disconnectReason,
        IntPtr reasonPtr, int reasonByteLen,
        IntPtr outBuf, int outBufSize, IntPtr outLenPtr)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                Marshal.WriteInt32(outLenPtr, 0);
                return 0;
            }

            SyncPlayerFromNative(player);

            var reason = Enum.IsDefined(typeof(DisconnectReason), disconnectReason)
                ? (DisconnectReason)disconnectReason
                : DisconnectReason.NONE;

            string defaultLeave = $"{player.getName()} was kicked from the game";
            var evt = new PlayerKickEvent(player, reason, defaultLeave);
            FourKit.FireEvent(evt);

            if (evt.isCancelled())
            {
                Marshal.WriteInt32(outLenPtr, 0);
                return 1;
            }

            string leaveMessage = evt.getLeaveMessage();
            if (!string.IsNullOrEmpty(leaveMessage))
            {
                byte[] utf8Bytes = System.Text.Encoding.UTF8.GetBytes(leaveMessage);
                if (utf8Bytes.Length < outBufSize)
                {
                    Marshal.Copy(utf8Bytes, 0, outBuf, utf8Bytes.Length);
                    Marshal.WriteInt32(outLenPtr, utf8Bytes.Length);
                }
                else
                {
                    Marshal.WriteInt32(outLenPtr, 0);
                }
            }
            else
            {
                Marshal.WriteInt32(outLenPtr, 0);
            }

            return 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerKick error: {ex}");
            Marshal.WriteInt32(outLenPtr, 0);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerMove(int entityId,
        double fromX, double fromY, double fromZ,
        double toX, double toY, double toZ,
        IntPtr outCoords)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                Marshal.Copy(new double[] { toX, toY, toZ }, 0, outCoords, 3);
                return 0;
            }

            SyncPlayerFromNative(player);

            var from = new Location(fromX, fromY, fromZ);
            var to = new Location(toX, toY, toZ);
            var evt = new PlayerMoveEvent(player, from, to);
            FourKit.FireEvent(evt);

            var finalTo = evt.getTo();
            Marshal.Copy(new double[] { finalTo.X, finalTo.Y, finalTo.Z }, 0, outCoords, 3);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerMove error: {ex}");
            Marshal.Copy(new double[] { toX, toY, toZ }, 0, outCoords, 3);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static void UpdatePlayerEntityId(int oldEntityId, int newEntityId)
    {
        try
        {
            FourKit.UpdatePlayerEntityId(oldEntityId, newEntityId);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"UpdatePlayerEntityId error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static int FireStructureGrow(int dimId, int x, int y, int z, int treeType, int wasBonemeal, int entityId)
    {
        try
        {
            Location location = new Location(FourKit.getWorld(dimId), x, y, z);

            StructureGrowEvent evt = new StructureGrowEvent(location, (TreeType)treeType, wasBonemeal == 1, (wasBonemeal == 1 && entityId != -1 ? FourKit.GetPlayerByEntityId(entityId) : null));
            FourKit.FireEvent(evt);

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireStructureGrow error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerChat(int entityId, IntPtr msgPtr, int msgByteLen,
        IntPtr outBuf, int outBufSize, IntPtr outLenPtr)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                Marshal.WriteInt32(outLenPtr, 0);
                return 0;
            }

            SyncPlayerFromNative(player);

            string message = msgByteLen > 0
                ? Marshal.PtrToStringUTF8(msgPtr, msgByteLen) ?? string.Empty
                : string.Empty;

            var evt = new PlayerChatEvent(player, message);
            FourKit.FireEvent(evt);

            if (evt.isCancelled())
            {
                Marshal.WriteInt32(outLenPtr, 0);
                return 1;
            }

            string formatted = JavaFormat(evt.getFormat(), player.getDisplayName(), evt.getMessage());
            byte[] utf8Bytes = System.Text.Encoding.UTF8.GetBytes(formatted);
            if (utf8Bytes.Length < outBufSize)
            {
                Marshal.Copy(utf8Bytes, 0, outBuf, utf8Bytes.Length);
                Marshal.WriteInt32(outLenPtr, utf8Bytes.Length);
            }
            else
            {
                Marshal.WriteInt32(outLenPtr, 0);
            }

            return 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerChat error: {ex}");
            Marshal.WriteInt32(outLenPtr, 0);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireBlockPlace(int entityId, int dimId,
        int placedX, int placedY, int placedZ,
        int againstX, int againstY, int againstZ,
        int itemId, int itemCount, int canBuild)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
                return 0;

            SyncPlayerFromNative(player);

            var world = FourKit.getWorld(dimId);
            var placedBlock = new Block.Block(world, placedX, placedY, placedZ);
            var againstBlock = new Block.Block(world, againstX, againstY, againstZ);

            Material mat = Enum.IsDefined(typeof(Material), itemId) ? (Material)itemId : Material.AIR;
            var itemInHand = new ItemStack(mat, itemCount);

            var evt = new BlockPlaceEvent(placedBlock, againstBlock, itemInHand, player, canBuild != 0);
            FourKit.FireEvent(evt);

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBlockPlace error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireBlockBreak(int entityId, int dimId,
        int x, int y, int z, int tileId, int data, int exp)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
                return exp;

            SyncPlayerFromNative(player);

            var world = FourKit.getWorld(dimId);
            var block = new Block.Block(world, x, y, z);

            var evt = new BlockBreakEvent(block, player, exp);
            FourKit.FireEvent(evt);

            if (evt.isCancelled())
                return -1;

            return evt.getExpToDrop();
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBlockBreak error: {ex}");
            return exp;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireEntityDamage(int entityId, int entityTypeId, int dimId,
        double x, double y, double z, int causeId, double damage, IntPtr outDamage,
        int damagerEntityId, int damagerEntityTypeId,
        double damagerX, double damagerY, double damagerZ)
    {
        try
        {
            Entity.Entity? entity = FourKit.GetPlayerByEntityId(entityId);
            if (entity is Player player)
            {
                SyncPlayerFromNative(player);
            }
            else
            {
                var entityType = Enum.IsDefined(typeof(EntityType), entityTypeId)
                    ? (EntityType)entityTypeId
                    : EntityType.UNKNOWN;
                entity = new LivingEntity(entityId, entityType, dimId, x, y, z);
            }

            var cause = Enum.IsDefined(typeof(EntityDamageEvent.DamageCause), causeId)
                ? (EntityDamageEvent.DamageCause)causeId
                : EntityDamageEvent.DamageCause.CUSTOM;

            EntityDamageByEntityEvent? byEntityEvt = null;
            if (damagerEntityId >= 0)
            {
                Entity.Entity? damager = FourKit.GetPlayerByEntityId(damagerEntityId);
                if (damager is Player damagerPlayer)
                {
                    SyncPlayerFromNative(damagerPlayer);
                }
                else
                {
                    var damagerType = Enum.IsDefined(typeof(EntityType), damagerEntityTypeId)
                        ? (EntityType)damagerEntityTypeId
                        : EntityType.UNKNOWN;
                    damager = new LivingEntity(damagerEntityId, damagerType, dimId, damagerX, damagerY, damagerZ);
                }
                byEntityEvt = new EntityDamageByEntityEvent(damager, entity!, cause, damage);
                FourKit.FireEvent(byEntityEvt);
                damage = byEntityEvt.getDamage();
            }

            var evt = new EntityDamageEvent(entity!, cause, damage);
            if (byEntityEvt != null && byEntityEvt.isCancelled())
                evt.setCancelled(true);
            FourKit.FireEvent(evt);

            Marshal.Copy(new double[] { evt.getDamage() }, 0, outDamage, 1);

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireEntityDamage error: {ex}");
            Marshal.Copy(new double[] { damage }, 0, outDamage, 1);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireSignChange(int entityId, int dimId,
        int x, int y, int z,
        IntPtr line0Ptr, int line0Len,
        IntPtr line1Ptr, int line1Len,
        IntPtr line2Ptr, int line2Len,
        IntPtr line3Ptr, int line3Len,
        IntPtr outBuf, int outBufSize, IntPtr outLensPtr)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                WriteSignOutLens(outLensPtr, [0, 0, 0, 0]);
                return 0;
            }

            SyncPlayerFromNative(player);

            string[] lines =
            [
                line0Len > 0 ? Marshal.PtrToStringUTF8(line0Ptr, line0Len) ?? string.Empty : string.Empty,
                line1Len > 0 ? Marshal.PtrToStringUTF8(line1Ptr, line1Len) ?? string.Empty : string.Empty,
                line2Len > 0 ? Marshal.PtrToStringUTF8(line2Ptr, line2Len) ?? string.Empty : string.Empty,
                line3Len > 0 ? Marshal.PtrToStringUTF8(line3Ptr, line3Len) ?? string.Empty : string.Empty,
            ];

            var world = FourKit.getWorld(dimId);
            var block = new Block.Block(world, x, y, z);
            var evt = new Event.Block.SignChangeEvent(block, player, lines);
            FourKit.FireEvent(evt);

            int offset = 0;
            int[] lens = new int[4];
            for (int i = 0; i < 4; i++)
            {
                byte[] utf8 = System.Text.Encoding.UTF8.GetBytes(evt.getLine(i));
                if (offset + utf8.Length <= outBufSize)
                {
                    Marshal.Copy(utf8, 0, outBuf + offset, utf8.Length);
                    lens[i] = utf8.Length;
                    offset += utf8.Length;
                }
            }
            WriteSignOutLens(outLensPtr, lens);

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireSignChange error: {ex}");
            WriteSignOutLens(outLensPtr, [0, 0, 0, 0]);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireEntityDeath(int entityId, int entityTypeId, int dimId,
        double x, double y, double z, int exp)
    {
        try
        {
            var entityType = Enum.IsDefined(typeof(EntityType), entityTypeId)
                ? (EntityType)entityTypeId
                : EntityType.UNKNOWN;
            var entity = new LivingEntity(entityId, entityType, dimId, x, y, z);

            var drops = new List<ItemStack>();
            var evt = new EntityDeathEvent(entity, drops, exp);
            FourKit.FireEvent(evt);

            return evt.getDroppedExp();
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireEntityDeath error: {ex}");
            return exp;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerDeath(int entityId,
        IntPtr deathMsgPtr, int deathMsgByteLen, int exp,
        IntPtr outMsgBuf, int outMsgBufSize, IntPtr outMsgLenPtr, IntPtr outKeepInventoryPtr,
        IntPtr outNewExpPtr, IntPtr outNewLevelPtr, IntPtr outKeepLevelPtr)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                Marshal.WriteInt32(outMsgLenPtr, 0);
                Marshal.WriteInt32(outKeepInventoryPtr, 0);
                Marshal.WriteInt32(outNewExpPtr, 0);
                Marshal.WriteInt32(outNewLevelPtr, 0);
                Marshal.WriteInt32(outKeepLevelPtr, 0);
                return exp;
            }

            SyncPlayerFromNative(player);

            string deathMessage = deathMsgByteLen > 0
                ? Marshal.PtrToStringUTF8(deathMsgPtr, deathMsgByteLen) ?? string.Empty
                : string.Empty;

            var drops = new List<ItemStack>();

            var playerEvt = new PlayerDeathEvent(player, drops, exp, deathMessage);
            FourKit.FireEvent(playerEvt);

            var entityEvt = new EntityDeathEvent(player, playerEvt.getDrops(), playerEvt.getDroppedExp());
            FourKit.FireEvent(entityEvt);

            int finalExp = entityEvt.getDroppedExp();

            string finalMsg = playerEvt.getDeathMessage();
            byte[] utf8Bytes = System.Text.Encoding.UTF8.GetBytes(finalMsg);
            if (utf8Bytes.Length < outMsgBufSize)
            {
                Marshal.Copy(utf8Bytes, 0, outMsgBuf, utf8Bytes.Length);
                Marshal.WriteInt32(outMsgLenPtr, utf8Bytes.Length);
            }
            else
            {
                Marshal.WriteInt32(outMsgLenPtr, 0);
            }

            Marshal.WriteInt32(outKeepInventoryPtr, playerEvt.getKeepInventory() ? 1 : 0);
            Marshal.WriteInt32(outNewExpPtr, playerEvt.getNewExp());
            Marshal.WriteInt32(outNewLevelPtr, playerEvt.getNewLevel());
            Marshal.WriteInt32(outKeepLevelPtr, playerEvt.getKeepLevel() ? 1 : 0);

            return finalExp;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerDeath error: {ex}");
            Marshal.WriteInt32(outMsgLenPtr, 0);
            Marshal.WriteInt32(outKeepInventoryPtr, 0);
            Marshal.WriteInt32(outNewExpPtr, 0);
            Marshal.WriteInt32(outNewLevelPtr, 0);
            Marshal.WriteInt32(outKeepLevelPtr, 0);
            return exp;
        }
    }

    [UnmanagedCallersOnly]
    public static long FirePlayerDropItem(int entityId, int itemId, int itemCount, int itemAux,
        IntPtr outItemIdPtr, IntPtr outItemCountPtr, IntPtr outItemAuxPtr)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                Marshal.WriteInt32(outItemIdPtr, itemId);
                Marshal.WriteInt32(outItemCountPtr, itemCount);
                Marshal.WriteInt32(outItemAuxPtr, itemAux);
                return 0;
            }

            SyncPlayerFromNative(player);

            Material mat = Enum.IsDefined(typeof(Material), itemId) ? (Material)itemId : Material.AIR;
            var itemStack = new ItemStack(mat, itemCount, (short)itemAux);

            var evt = new PlayerDropItemEvent(player, itemStack);
            FourKit.FireEvent(evt);

            var result = evt.getItemDrop();
            Marshal.WriteInt32(outItemIdPtr, result.getTypeId());
            Marshal.WriteInt32(outItemCountPtr, result.getAmount());
            Marshal.WriteInt32(outItemAuxPtr, result.getDurability());

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerDropItem error: {ex}");
            Marshal.WriteInt32(outItemIdPtr, itemId);
            Marshal.WriteInt32(outItemCountPtr, itemCount);
            Marshal.WriteInt32(outItemAuxPtr, itemAux);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerInteract(int entityId, int action,
        int itemId, int itemCount, int itemAux,
        int clickedX, int clickedY, int clickedZ,
        int blockFace, int dimId,
        IntPtr outUseItemInHandPtr)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                Marshal.WriteInt32(outUseItemInHandPtr, 1);
                return 0;
            }

            SyncPlayerFromNative(player);

            var actionEnum = Enum.IsDefined(typeof(Block.Action), action)
                ? (Block.Action)action
                : Block.Action.RIGHT_CLICK_AIR;

            var faceEnum = Enum.IsDefined(typeof(Block.BlockFace), blockFace)
                ? (Block.BlockFace)blockFace
                : Block.BlockFace.SELF;

            ItemStack? itemStack = null;
            if (itemId > 0)
                itemStack = new ItemStack(itemId, itemCount, (short)itemAux);

            Block.Block? clickedBlock = null;
            bool hasBlock = actionEnum == Block.Action.LEFT_CLICK_BLOCK
                         || actionEnum == Block.Action.RIGHT_CLICK_BLOCK
                         || actionEnum == Block.Action.PHYSICAL;
            if (hasBlock)
            {
                var world = FourKit.getWorld(dimId);
                if (world != null)
                    clickedBlock = new Block.Block(world, clickedX, clickedY, clickedZ);
            }

            var evt = new PlayerInteractEvent(
                player, actionEnum, itemStack, clickedBlock, faceEnum);
            FourKit.FireEvent(evt);

            Marshal.WriteInt32(outUseItemInHandPtr, evt.useItemInHand() ? 1 : 0);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerInteract error: {ex}");
            Marshal.WriteInt32(outUseItemInHandPtr, 1);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerInteractEntity(int playerEntityId,
        int targetEntityId, int targetEntityTypeId,
        int dimId, double targetX, double targetY, double targetZ,
        float targetHealth, float targetMaxHealth, float targetEyeHeight)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(playerEntityId);
            if (player == null)
                return 0;

            SyncPlayerFromNative(player);

            Entity.Entity? target = FourKit.GetPlayerByEntityId(targetEntityId);
            if (target is Player targetPlayer)
            {
                SyncPlayerFromNative(targetPlayer);
            }
            else
            {
                var entityType = Enum.IsDefined(typeof(EntityType), targetEntityTypeId)
                    ? (EntityType)targetEntityTypeId
                    : EntityType.UNKNOWN;
                var living = new LivingEntity(targetEntityId, entityType, dimId, targetX, targetY, targetZ,
                    targetHealth, targetMaxHealth);
                living.SetEyeHeightInternal(targetEyeHeight);
                target = living;
            }

            var evt = new PlayerInteractEntityEvent(player, target);
            FourKit.FireEvent(evt);

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerInteractEntity error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerPickupItem(int playerEntityId,
        int itemEntityId, int dimId, double itemX, double itemY, double itemZ,
        int itemId, int itemCount, int itemAux, int remaining,
        IntPtr outItemIdPtr, IntPtr outItemCountPtr, IntPtr outItemAuxPtr)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(playerEntityId);
            if (player == null)
            {
                Marshal.WriteInt32(outItemIdPtr, itemId);
                Marshal.WriteInt32(outItemCountPtr, itemCount);
                Marshal.WriteInt32(outItemAuxPtr, itemAux);
                // todo: fix
                return 1;
            }

            SyncPlayerFromNative(player);

            var itemStack = new Inventory.ItemStack(itemId, itemCount, (short)itemAux);
            var item = new Entity.Item(itemEntityId, dimId, itemX, itemY, itemZ, itemStack);
            var evt = new PlayerPickupItemEvent(player, item, remaining);
            FourKit.FireEvent(evt);

            var result = evt.getItem().getItemStack();
            Marshal.WriteInt32(outItemIdPtr, result.getTypeId());
            Marshal.WriteInt32(outItemCountPtr, result.getAmount());
            Marshal.WriteInt32(outItemAuxPtr, result.getDurability());

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerPickupItem error: {ex}");
            Marshal.WriteInt32(outItemIdPtr, itemId);
            Marshal.WriteInt32(outItemCountPtr, itemCount);
            Marshal.WriteInt32(outItemAuxPtr, itemAux);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireInventoryOpen(int entityId, int nativeContainerType,
        IntPtr titlePtr, int titleByteLen, int containerSize)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null) return 0;

            SyncPlayerFromNative(player);

            string title = titleByteLen > 0
                ? Marshal.PtrToStringUTF8(titlePtr, titleByteLen) ?? string.Empty
                : string.Empty;

            InventoryType invType = MapNativeContainerType(nativeContainerType);
            Inventory.Inventory topInv = CreateContainerInventory(invType, nativeContainerType, title, containerSize, entityId);
            var bottomInv = player.getInventory();

            var view = new InventoryView(topInv, bottomInv, player, invType);
            var evt = new InventoryOpenEvent(view);
            FourKit.FireEvent(evt);

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireInventoryOpen error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireInventoryClick(int entityId, int slot, int button, int clickType,
        int nativeContainerType, int containerSize, IntPtr titleUtf8Ptr, int titleByteLen)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null) return 0;

            SyncPlayerFromNative(player);

            ClickType click = MapNativeClickType(clickType, button);
            InventoryAction action = DetermineInventoryAction(click, slot);

            InventoryType invType;
            Inventory.Inventory topInv;
            if (nativeContainerType < 0)
            {
                invType = InventoryType.PLAYER;
                topInv = player.getInventory();
            }
            else
            {
                invType = MapNativeContainerType(nativeContainerType);
                int size = containerSize > 0 ? containerSize : invType.getDefaultSize();
                string title = titleByteLen > 0 && titleUtf8Ptr != IntPtr.Zero
                    ? Marshal.PtrToStringUTF8(titleUtf8Ptr, titleByteLen) ?? invType.getDefaultTitle()
                    : invType.getDefaultTitle();
                topInv = CreateContainerInventory(invType, nativeContainerType, title, size, entityId);
            }

            var bottomInv = player.getInventory();
            var view = new InventoryView(topInv, bottomInv, player, invType);

            SlotType slotType = SlotType.CONTAINER;
            if (slot == InventoryView.OUTSIDE)
                slotType = SlotType.OUTSIDE;

            Inventory.Inventory._slotModifiedByPlugin = false;
            int hotbarKey = click == ClickType.NUMBER_KEY ? button : -1;
            var evt = new InventoryClickEvent(view, slotType, slot, click, action, hotbarKey);
            FourKit.FireEvent(evt);

            if (evt.isCancelled()) return 1;
            if (Inventory.Inventory._slotModifiedByPlugin) return 2;
            return 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireInventoryClick error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int HandlePlayerCommand(int entityId, IntPtr cmdUtf8, int cmdByteLen)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
                return 0;

            SyncPlayerFromNative(player);

            string commandLine = cmdByteLen > 0
                ? Marshal.PtrToStringUTF8(cmdUtf8, cmdByteLen) ?? string.Empty
                : string.Empty;

            if (string.IsNullOrEmpty(commandLine))
                return 0;

            return FourKit.DispatchCommand(player, commandLine) ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"HandlePlayerCommand error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireCommandPreprocess(int entityId, IntPtr cmdUtf8, int cmdByteLen, IntPtr outBuf, int outBufSize, IntPtr outLenPtr)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
                return 0;

            SyncPlayerFromNative(player);

            string commandLine = cmdByteLen > 0
                ? Marshal.PtrToStringUTF8(cmdUtf8, cmdByteLen) ?? string.Empty
                : string.Empty;

            if (string.IsNullOrEmpty(commandLine))
                return 0;

            var preEvt = new Event.Player.PlayerCommandPreprocessEvent(player, commandLine);
            FourKit.FireEvent(preEvt);

            if (preEvt.isCancelled())
                return 1;

            string modified = preEvt.getMessage();
            if (modified != commandLine && outBuf != IntPtr.Zero && outBufSize > 0)
            {
                byte[] utf8Bytes = System.Text.Encoding.UTF8.GetBytes(modified);
                if (utf8Bytes.Length < outBufSize)
                {
                    Marshal.Copy(utf8Bytes, 0, outBuf, utf8Bytes.Length);
                    if (outLenPtr != IntPtr.Zero)
                        Marshal.WriteInt32(outLenPtr, utf8Bytes.Length);
                }
            }

            return 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireCommandPreprocess error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int HandleConsoleCommand(IntPtr cmdUtf8, int cmdByteLen)
    {
        try
        {
            string commandLine = cmdByteLen > 0
                ? Marshal.PtrToStringUTF8(cmdUtf8, cmdByteLen) ?? string.Empty
                : string.Empty;

            if (string.IsNullOrEmpty(commandLine))
                return 0;

            string trimmed = commandLine.StartsWith('/') ? commandLine[1..] : commandLine;
            string[] parts = trimmed.Split(' ', StringSplitOptions.RemoveEmptyEntries);
            if (parts.Length == 0)
                return 0;

            if (!FourKit.HasCommand(parts[0]))
                return 0;

            FourKit.DispatchCommand(FourKit.getConsoleSender(), commandLine);
            return 1;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"HandleConsoleCommand error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int GetPluginCommandHelp(IntPtr outBuf, int outBufSize, IntPtr outLenPtr)
    {
        try
        {
            var entries = FourKit.GetRegisteredCommandHelp();
            if (entries.Count == 0)
            {
                if (outLenPtr != IntPtr.Zero)
                    Marshal.WriteInt32(outLenPtr, 0);
                return 0;
            }

            var sb = new System.Text.StringBuilder();
            foreach (var (usage, description) in entries)
            {
                sb.Append(usage);
                sb.Append('\0');
                sb.Append(description);
                sb.Append('\0');
            }

            byte[] utf8 = System.Text.Encoding.UTF8.GetBytes(sb.ToString());
            int copyLen = Math.Min(utf8.Length, outBufSize);
            if (outBuf != IntPtr.Zero && copyLen > 0)
                Marshal.Copy(utf8, 0, outBuf, copyLen);
            if (outLenPtr != IntPtr.Zero)
                Marshal.WriteInt32(outLenPtr, copyLen);

            return entries.Count;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"GetPluginCommandHelp error: {ex}");
            if (outLenPtr != IntPtr.Zero)
                Marshal.WriteInt32(outLenPtr, 0);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerTeleport(int entityId,
        double fromX, double fromY, double fromZ, int fromDimId,
        double toX, double toY, double toZ, int toDimId,
        int cause, IntPtr outCoords)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                Marshal.Copy(new double[] { toX, toY, toZ }, 0, outCoords, 3);
                return 0;
            }

            SyncPlayerFromNative(player);

            var fromWorld = FourKit.getWorld(fromDimId);
            var toWorld = FourKit.getWorld(toDimId);
            var from = new Location(fromWorld, fromX, fromY, fromZ);
            var to = new Location(toWorld, toX, toY, toZ);
            var teleportCause = cause >= 0 && cause <= (int)PlayerTeleportEvent.TeleportCause.UNKNOWN
                ? (PlayerTeleportEvent.TeleportCause)cause
                : PlayerTeleportEvent.TeleportCause.UNKNOWN;

            var evt = new PlayerTeleportEvent(player, from, to, teleportCause);
            FourKit.FireEvent(evt);

            var finalTo = evt.getTo();
            Marshal.Copy(new double[] { finalTo.X, finalTo.Y, finalTo.Z }, 0, outCoords, 3);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerTeleport error: {ex}");
            Marshal.Copy(new double[] { toX, toY, toZ }, 0, outCoords, 3);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePlayerPortal(int entityId,
        double fromX, double fromY, double fromZ, int fromDimId,
        double toX, double toY, double toZ, int toDimId,
        int cause, IntPtr outCoords)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null)
            {
                Marshal.Copy(new double[] { toX, toY, toZ }, 0, outCoords, 3);
                return 0;
            }

            SyncPlayerFromNative(player);

            var fromWorld = FourKit.getWorld(fromDimId);
            var toWorld = FourKit.getWorld(toDimId);
            var from = new Location(fromWorld, fromX, fromY, fromZ);
            var to = new Location(toWorld, toX, toY, toZ);
            var teleportCause = cause >= 0 && cause <= (int)PlayerTeleportEvent.TeleportCause.UNKNOWN
                ? (PlayerTeleportEvent.TeleportCause)cause
                : PlayerTeleportEvent.TeleportCause.UNKNOWN;

            var evt = new PlayerPortalEvent(player, from, to, teleportCause);
            FourKit.FireEvent(evt);

            var finalTo = evt.getTo();
            Marshal.Copy(new double[] { finalTo.X, finalTo.Y, finalTo.Z }, 0, outCoords, 3);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePlayerPortal error: {ex}");
            Marshal.Copy(new double[] { toX, toY, toZ }, 0, outCoords, 3);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireBedEnter(int entityId, int dimId, int bedX, int bedY, int bedZ)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null) return 0;

            SyncPlayerFromNative(player);

            var world = FourKit.getWorld(dimId);
            var bed = new Block.Block(world, bedX, bedY, bedZ);
            var evt = new Event.Player.PlayerBedEnterEvent(player, bed);
            FourKit.FireEvent(evt);

            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBedEnter error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static void FireBedLeave(int entityId, int dimId, int bedX, int bedY, int bedZ)
    {
        try
        {
            var player = FourKit.GetPlayerByEntityId(entityId);
            if (player == null) return;

            SyncPlayerFromNative(player);

            var world = FourKit.getWorld(dimId);
            var bed = new Block.Block(world, bedX, bedY, bedZ);
            var evt = new Event.Player.PlayerBedLeaveEvent(player, bed);
            FourKit.FireEvent(evt);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBedLeave error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static int FireBlockGrow(int dimId, int x, int y, int z, int newTileId, int newTileData)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var block = new Block.Block(world, x, y, z);
            var newState = new Block.BlockState(world, x, y, z, newTileId, newTileData);
            var evt = new Event.Block.BlockGrowEvent(block, newState);
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBlockGrow error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireBlockForm(int dimId, int x, int y, int z, int newTileId, int newTileData)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var block = new Block.Block(world, x, y, z);
            var newState = new Block.BlockState(world, x, y, z, newTileId, newTileData);
            var evt = new Event.Block.BlockFormEvent(block, newState);
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBlockForm error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireBlockBurn(int dimId, int x, int y, int z)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var block = new Block.Block(world, x, y, z);
            var evt = new Event.Block.BlockBurnEvent(block);
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBlockBurn error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireBlockSpread(int dimId, int x, int y, int z, int srcX, int srcY, int srcZ, int newTileId, int newTileData)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var block = new Block.Block(world, x, y, z);
            var source = new Block.Block(world, srcX, srcY, srcZ);
            var newState = new Block.BlockState(world, x, y, z, newTileId, newTileData);
            var evt = new Event.Block.BlockSpreadEvent(block, source, newState);
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBlockSpread error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePistonExtend(int dimId, int x, int y, int z, int direction, int length)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var block = new Block.Block(world, x, y, z);
            var face = Enum.IsDefined(typeof(Block.BlockFace), direction)
                ? (Block.BlockFace)direction
                : Block.BlockFace.SELF;
            var evt = new Event.Block.BlockPistonExtendEvent(block, length, face);
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePistonExtend error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FirePistonRetract(int dimId, int x, int y, int z, int direction)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var block = new Block.Block(world, x, y, z);
            var face = Enum.IsDefined(typeof(Block.BlockFace), direction)
                ? (Block.BlockFace)direction
                : Block.BlockFace.SELF;
            var evt = new Event.Block.BlockPistonRetractEvent(block, face);
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FirePistonRetract error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static int FireBlockFromTo(int dimId, int fromX, int fromY, int fromZ, int toX, int toY, int toZ, int face)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var from = new Block.Block(world, fromX, fromY, fromZ);
            var to = new Block.Block(world, toX, toY, toZ);
            var blockFace = Enum.IsDefined(typeof(Block.BlockFace), face)
                ? (Block.BlockFace)face
                : Block.BlockFace.SELF;
            var evt = new Event.Block.BlockFromToEvent(from, to, blockFace);
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireBlockFromTo error: {ex}");
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static void FireChunkLoad(int dimId, int chunkX, int chunkZ, int isNewChunk)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var chunk = new Chunk.Chunk(world, chunkX, chunkZ);
            var evt = new Event.World.ChunkLoadEvent(chunk, isNewChunk != 0);
            FourKit.FireEvent(evt);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireChunkLoad error: {ex}");
        }
    }

    [UnmanagedCallersOnly]
    public static int FireChunkUnload(int dimId, int chunkX, int chunkZ)
    {
        try
        {
            var world = FourKit.getWorld(dimId);
            var chunk = new Chunk.Chunk(world, chunkX, chunkZ);
            var evt = new Event.World.ChunkUnloadEvent(chunk);
            FourKit.FireEvent(evt);
            return evt.isCancelled() ? 1 : 0;
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"FireChunkUnload error: {ex}");
            return 0;
        }
    }
}
