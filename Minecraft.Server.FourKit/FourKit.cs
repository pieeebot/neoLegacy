namespace Minecraft.Server.FourKit;

using Minecraft.Server.FourKit.Command;
using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Event;
using Minecraft.Server.FourKit.Inventory;
using Minecraft.Server.FourKit.Plugin;

/// <summary>
/// The main entry point for the FourKit plugin API.
/// </summary>
public static class FourKit
{
    private static readonly EventDispatcher _dispatcher;
    private static readonly Dictionary<string, Player> _players = new(StringComparer.OrdinalIgnoreCase);
    private static readonly Dictionary<int, Player> _playersByEntityId = new();
    private static readonly object _playerLock = new();

    // Must match HandlerKind in FourKitNatives.h.
    private enum HandlerKind
    {
        ChunkLoad = 0,
        ChunkUnload = 1,
        PlayerMove = 2,
    }

    private static uint _handlerMask;
    private static readonly object _handlerMaskLock = new();

    static FourKit()
    {
        _dispatcher = new EventDispatcher();
        _dispatcher.OnSubscriptionChanged = OnEventSubscribed;
    }

    private static HandlerKind? MapEventTypeToHandlerKind(Type eventType)
    {
        if (eventType == typeof(Event.World.ChunkLoadEvent)) return HandlerKind.ChunkLoad;
        if (eventType == typeof(Event.World.ChunkUnloadEvent)) return HandlerKind.ChunkUnload;
        if (eventType == typeof(Event.Player.PlayerMoveEvent)) return HandlerKind.PlayerMove;
        return null;
    }

    private static void OnEventSubscribed(Type eventType)
    {
        var kind = MapEventTypeToHandlerKind(eventType);
        if (kind == null) return;

        lock (_handlerMaskLock)
        {
            uint newMask = _handlerMask | (1u << (int)kind.Value);
            if (newMask == _handlerMask) return;
            _handlerMask = newMask;
            NativeBridge.SetHandlerMask?.Invoke(_handlerMask);
        }
    }

    internal static void ResyncHandlerMask()
    {
        lock (_handlerMaskLock)
        {
            NativeBridge.SetHandlerMask?.Invoke(_handlerMask);
        }
    }

    /// <summary>
    /// Gets the current server tick count. Increments once per server tick
    /// (~20 per second under nominal load). Useful for measuring TPS by
    /// sampling the delta against wall clock time.
    /// </summary>
    public static int getServerTick()
    {
        return NativeBridge.GetServerTickCount?.Invoke() ?? 0;
    }

    internal const int MAX_CHAT_LENGTH = 123;

    private static readonly Dictionary<int, World> _worldsByDimId = new();
    private static readonly Dictionary<string, int> _worldNameToDimId = new(StringComparer.OrdinalIgnoreCase)
    {
        // grr
        ["world"] = 0,
        ["world_nether"] = -1,
        ["world_the_end"] = 1,
    };
    private static readonly object _worldLock = new();

    /// <summary>
    /// Gets a world by its name. Supported names: "world" (overworld),
    /// "world_nether" (nether), "world_the_end" (the end).
    /// </summary>
    /// <param name="name">The name of the world to retrieve.</param>
    /// <returns>The world with the given name, or null if none exists.</returns>
    public static World? getWorld(string name)
    {
        if (_worldNameToDimId.TryGetValue(name, out int dimId))
            return getWorld(dimId);
        return null;
    }

    /// <summary>
    /// Gets a world by its dimension ID (0 = overworld, -1 = nether, 1 = the end).
    /// </summary>
    /// <param name="dimId">The dimension ID.</param>
    /// <returns>The world for that dimension, creating it if necessary.</returns>
    public static World getWorld(int dimId)
    {
        lock (_worldLock)
        {
            if (!_worldsByDimId.TryGetValue(dimId, out var world))
            {
                string name = dimId switch
                {
                    0 => "world",
                    -1 => "world_nether",
                    1 => "world_the_end",
                    _ => $"world_dim{dimId}",
                };
                world = new World(dimId, name);
                _worldsByDimId[dimId] = world;
            }
            return world;
        }
    }

    /// <summary>
    /// Registers all the events in the given listener class
    /// </summary>
    public static void addListener(Listener listener)
    {
        _dispatcher.Register(listener);
        //Console.WriteLine($"[FourKit] Registered listener: {listener.GetType().Name}");
    }
    public static Player? getPlayer(string name)
    {
        lock (_playerLock)
        {
            _players.TryGetValue(name, out var p);
            return p;
        }
    }

    public static IReadOnlyList<Player> getOnlinePlayers()
    {
        lock (_playerLock)
        {
            return _players.Values.Where(p => p.IsOnline).ToList().AsReadOnly();
        }
    }


    internal static Player? GetPlayerByEntityId(int entityId)
    {
        lock (_playerLock)
        {
            _playersByEntityId.TryGetValue(entityId, out var p);
            return p;
        }
    }

    internal static Entity.Entity? GetEntityByEntityId(int entityId)
    {
        var player = GetPlayerByEntityId(entityId);
        if (player != null) return player;

        if (NativeBridge.GetEntityInfo == null) return null;

        IntPtr buf = System.Runtime.InteropServices.Marshal.AllocHGlobal(5 * sizeof(double));
        try
        {
            NativeBridge.GetEntityInfo(entityId, buf);
            double[] data = new double[5];
            System.Runtime.InteropServices.Marshal.Copy(buf, data, 0, 5);

            int typeId = (int)data[0];
            if (typeId < 0) return null;

            var entityType = Enum.IsDefined(typeof(Entity.EntityType), typeId)
                ? (Entity.EntityType)typeId
                : Entity.EntityType.UNKNOWN;
            int dimId = (int)data[4];

            var entity = new Entity.Entity();
            entity.SetEntityIdInternal(entityId);
            entity.SetEntityTypeInternal(entityType);
            entity.SetDimensionInternal(dimId);
            entity.SetLocation(new Location(getWorld(dimId), data[1], data[2], data[3]));
            return entity;
        }
        finally
        {
            System.Runtime.InteropServices.Marshal.FreeHGlobal(buf);
        }
    }

    internal static Player TrackPlayer(int entityId, string name)
    {
        lock (_playerLock)
        {
            if (_playersByEntityId.TryGetValue(entityId, out var existing))
            {
                var oldName = existing.getName();
                existing.SetNameInternal(name);
                existing.IsOnline = true;

                if (!string.Equals(oldName, name, StringComparison.OrdinalIgnoreCase))
                {
                    _players.Remove(oldName);
                    _players[name] = existing;
                }
                return existing;
            }

            var player = new Player(entityId, name);
            _players[name] = player;
            _playersByEntityId[entityId] = player;
            return player;
        }
    }

    internal static Player? UntrackPlayer(int entityId)
    {
        lock (_playerLock)
        {
            if (_playersByEntityId.TryGetValue(entityId, out var player))
            {
                player.IsOnline = false;
                _playersByEntityId.Remove(entityId);
                _players.Remove(player.getName());
                return player;
            }
            return null;
        }
    }

    internal static void UpdatePlayerEntityId(int oldEntityId, int newEntityId)
    {
        lock (_playerLock)
        {
            if (_playersByEntityId.TryGetValue(oldEntityId, out var player))
            {
                _playersByEntityId.Remove(oldEntityId);
                player.SetEntityIdInternal(newEntityId);
                _playersByEntityId[newEntityId] = player;
            }
        }
    }

    internal static void FireEvent(Event.Event evt)
    {
        _dispatcher.Fire(evt);
    }

    private static readonly Dictionary<string, PluginCommand> _commands = new(StringComparer.OrdinalIgnoreCase);
    private static readonly object _commandLock = new();

    /// <summary>
    /// Gets a <see cref="PluginCommand"/> with the given name, creating it
    /// if it does not already exist. The returned command can be configured
    /// with <see cref="PluginCommand.setExecutor"/>,
    /// <see cref="Command.setDescription"/>, etc.
    /// </summary>
    /// <param name="name">Name of the command.</param>
    /// <returns>The command for that name.</returns>
    public static PluginCommand getCommand(string name)
    {
        lock (_commandLock)
        {
            if (!_commands.TryGetValue(name, out var cmd))
            {
                cmd = new PluginCommand(name);
                _commands[name] = cmd;
            }
            return cmd;
        }
    }

    internal static bool DispatchCommand(CommandSender sender, string commandLine)
    {
        string trimmed = commandLine.StartsWith('/') ? commandLine[1..] : commandLine;
        if (string.IsNullOrEmpty(trimmed)) return false;

        string[] parts = trimmed.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        string label = parts[0];
        string[] args = parts.Length > 1 ? parts[1..] : [];

        PluginCommand? cmd;
        lock (_commandLock)
        {
            if (!_commands.TryGetValue(label, out cmd))
            {
                foreach (var entry in _commands.Values)
                {
                    if (entry.getAliases().Exists(a => string.Equals(a, label, StringComparison.OrdinalIgnoreCase)))
                    {
                        cmd = entry;
                        break;
                    }
                }
            }
        }
        if (cmd == null || cmd.getExecutor() == null) return false;

        try
        {
            return cmd.execute(sender, label, args);
        }
        catch (Exception ex)
        {
            ServerLog.Error("fourkit", $"Error executing command '/{label}': {ex}");
            sender.sendMessage($"An internal error occurred while executing /{label}.");
            return false;
        }
    }

    internal static ConsoleCommandSender getConsoleSender() => ConsoleCommandSender.Instance;

    internal static bool HasCommand(string label)
    {
        lock (_commandLock)
        {
            if (_commands.TryGetValue(label, out var cmd) && cmd.getExecutor() != null)
                return true;
            foreach (var entry in _commands.Values)
            {
                if (entry.getExecutor() != null &&
                    entry.getAliases().Exists(a => string.Equals(a, label, StringComparison.OrdinalIgnoreCase)))
                    return true;
            }
        }
        return false;
    }

    internal static List<(string usage, string description)> GetRegisteredCommandHelp()
    {
        var result = new List<(string, string)>();
        lock (_commandLock)
        {
            foreach (var cmd in _commands.Values)
            {
                if (cmd.getExecutor() != null)
                    result.Add((cmd.getUsage(), cmd.getDescription()));
            }
        }
        return result;
    }

    /// <summary>
    /// Broadcasts a message to all online players.
    /// </summary>
    /// <param name="message">The message to broadcast.</param>
    public static void broadcastMessage(string message)
    {
        if (string.IsNullOrEmpty(message) || NativeBridge.BroadcastMessage == null)
            return;
        if (message.Length > MAX_CHAT_LENGTH)
            message = message[..MAX_CHAT_LENGTH];
        IntPtr ptr = System.Runtime.InteropServices.Marshal.StringToCoTaskMemUTF8(message);
        try
        {
            NativeBridge.BroadcastMessage(ptr, System.Text.Encoding.UTF8.GetByteCount(message));
        }
        finally
        {
            System.Runtime.InteropServices.Marshal.FreeCoTaskMem(ptr);
        }
    }

    /// <summary>
    /// Creates a new <see cref="Inventory.Inventory"/> with the specified size.
    /// The inventory will be of type <see cref="InventoryType.CHEST"/> with
    /// the default title.
    /// </summary>
    /// <param name="size">The size of the inventory (must be a multiple of 9).</param>
    /// <returns>A new Inventory.</returns>
    public static Inventory.Inventory createInventory(int size)
    {
        return new Inventory.Inventory("Chest", InventoryType.CHEST, size);
    }

    /// <summary>
    /// Creates a new <see cref="Inventory.Inventory"/> with the specified size
    /// and custom title.
    /// </summary>
    /// <param name="size">The size of the inventory (must be a multiple of 9).</param>
    /// <param name="title">The title that will be shown to players.</param>
    /// <returns>A new Inventory.</returns>
    public static Inventory.Inventory createInventory(int size, string title)
    {
        return new Inventory.Inventory(title, InventoryType.CHEST, size);
    }

    /// <summary>
    /// Creates a new <see cref="Inventory.Inventory"/> of the specified
    /// <see cref="InventoryType"/> with the default title and size.
    /// </summary>
    /// <param name="type">The type of inventory to create.</param>
    /// <returns>A new Inventory.</returns>
    public static Inventory.Inventory createInventory(InventoryType type)
    {
        return new Inventory.Inventory(type.getDefaultTitle(), type, type.getDefaultSize());
    }

    /// <summary>
    /// Creates a new <see cref="Inventory.Inventory"/> of the specified
    /// <see cref="InventoryType"/> with a custom title.
    /// </summary>
    /// <param name="type">The type of inventory to create.</param>
    /// <param name="title">The title that will be shown to players.</param>
    /// <returns>A new Inventory.</returns>
    public static Inventory.Inventory createInventory(InventoryType type, string title)
    {
        return new Inventory.Inventory(title, type, type.getDefaultSize());
    }

    /// <summary>
    /// Checks if the given plugin is loaded and returns it when applicable.
    /// <para>
    /// Please note that the name of the plugin is case-sensitive.
    /// </para>
    /// </summary>
    /// <param name="name">Name of the plugin to check.</param>
    /// <returns>Plugin if it exists, otherwise null</returns>
    public static ServerPlugin? getPlugin(string name) {
        var loadedPlugins = FourKitHost.getLoadedPlugins().Where(x => x.name == name);

	if (loadedPlugins.Count() > 1) ServerLog.Warn("fourkit", $"More than one instance of a(n) '{name}' plugin.");
	return loadedPlugins.FirstOrDefault();
    }

    /// <summary>
    /// Gets a list of all currently loaded plugins.
    /// </summary>
    /// <returns>The array of plugins.</returns>
    public static ServerPlugin[] getPlugins() => FourKitHost.getLoadedPlugins().ToArray(); // returns an array for better compatibility for bukkit->fourkit

    /// <summary>
    /// Enables the specified plugin.
    /// </summary>
    /// <param name="plugin">Plugin to enable.</param>
    public static void enablePlugin(ServerPlugin plugin) => FourKitHost.s_loader?.EnablePlugin(plugin);

    /// <summary>
    /// Disables the specified plugin.
    /// </summary>
    /// <param name="plugin">Plugin to disable.</param>
    public static void disablePlugin(ServerPlugin plugin) => FourKitHost.s_loader?.DisablePlugin(plugin);
}
