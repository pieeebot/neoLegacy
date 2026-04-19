using Minecraft.Server.FourKit;
using Minecraft.Server.FourKit.Command;
using Minecraft.Server.FourKit.Enchantments;
using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Event;
using Minecraft.Server.FourKit.Event.World;
using Minecraft.Server.FourKit.Inventory;
using Minecraft.Server.FourKit.Inventory.Meta;
using Minecraft.Server.FourKit.Plugin;

namespace FourKitTestPlugin;

/// <summary>
/// Exercises the new FourKit APIs added by the recent upstream sync:
/// chunk additions, chunk events, ender chest inventory, and the
/// disenchant fix on ItemStack.setDurability.
///
/// Use the <c>/fktest</c> command in-game (or from the console) to run
/// each subtest. Run <c>/fktest help</c> for the full menu.
/// </summary>
public class FourKitTestPlugin : ServerPlugin
{
    public override string name => "FourKitTestPlugin";
    public override string version => "1.0.0";
    public override string author => "LCE-Revelations";

    private static int _chunkLoadCount;
    private static int _chunkUnloadCount;

    private static string? _logPath;
    private static readonly object _logLock = new();

    public override void onEnable()
    {
        _logPath = ResolveLogPath(serverDirectory, dataDirectory);
        Log("FourKitTestPlugin enabled.");
        Log($"Plugin log file: {_logPath}");
        FourKit.addListener(new ChunkEventLogger());

        var cmd = FourKit.getCommand("fktest");
        cmd.setDescription("FourKit API smoke tests.");
        cmd.setUsage("/fktest <help|world|chunks|snapshot|entities|loadchunk|enderchest|disenchant|events>");
        cmd.setExecutor(new TestExecutor());
    }

    public override void onDisable()
    {
        Log("FourKitTestPlugin disabled.");
    }

    internal static int ChunkLoadCount => _chunkLoadCount;
    internal static int ChunkUnloadCount => _chunkUnloadCount;
    internal static void IncChunkLoad() => Interlocked.Increment(ref _chunkLoadCount);
    internal static void IncChunkUnload() => Interlocked.Increment(ref _chunkUnloadCount);

    /// <summary>
    /// Writes a line both to the live server console and to a persistent log
    /// file so test results are recoverable after the server window closes.
    /// Tries server.log first using shared-write mode; falls back to a
    /// sibling fkplugin.log in the server root if the C++ host has the
    /// main log opened exclusively.
    /// </summary>
    internal static void Log(string message)
    {
        string line = $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff}][INFO][fkplugin] {message}";
        Console.WriteLine(line);

        if (_logPath == null) return;
        try
        {
            lock (_logLock)
            {
                using var fs = new FileStream(_logPath, FileMode.Append, FileAccess.Write, FileShare.ReadWrite);
                using var sw = new StreamWriter(fs);
                sw.WriteLine(line);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[fkplugin] WARN: failed to append to {_logPath}: {ex.Message}");
        }
    }

    private static string ResolveLogPath(string serverDir, string dataDir)
    {
        string serverLog = Path.Combine(serverDir, "server.log");
        try
        {
            using var fs = new FileStream(serverLog, FileMode.Append, FileAccess.Write, FileShare.ReadWrite);
            return serverLog;
        }
        catch (IOException)
        {
            string fallback = Path.Combine(serverDir, "fkplugin.log");
            Console.WriteLine($"[fkplugin] server.log is locked; writing to {fallback} instead.");
            return fallback;
        }
        catch (UnauthorizedAccessException)
        {
            string fallback = Path.Combine(dataDir, "fkplugin.log");
            Console.WriteLine($"[fkplugin] server.log not writable; writing to {fallback} instead.");
            return fallback;
        }
    }
}

internal sealed class ChunkEventLogger : Listener
{
    [EventHandler(Priority = EventPriority.Monitor)]
    public void onChunkLoad(ChunkLoadEvent e)
    {
        FourKitTestPlugin.IncChunkLoad();
        var chunk = e.getChunk();
        FourKitTestPlugin.Log($"ChunkLoadEvent dim={chunk.getWorld().getDimensionId()} ({chunk.getX()},{chunk.getZ()}) new={e.isNewChunk()}");
    }

    [EventHandler(Priority = EventPriority.Monitor)]
    public void onChunkUnload(ChunkUnloadEvent e)
    {
        FourKitTestPlugin.IncChunkUnload();
        var chunk = e.getChunk();
        FourKitTestPlugin.Log($"ChunkUnloadEvent dim={chunk.getWorld().getDimensionId()} ({chunk.getX()},{chunk.getZ()})");
    }
}

internal sealed class TestExecutor : CommandExecutor
{
    private static void Reply(CommandSender sender, string message)
    {
        sender.sendMessage(message);
        FourKitTestPlugin.Log($"[/fktest] {message}");
    }

    public bool onCommand(CommandSender sender, Command command, string label, string[] args)
    {
        if (args.Length == 0)
        {
            Reply(sender,"Usage: /fktest <help|world|chunks|snapshot|entities|loadchunk|enderchest|disenchant|events>");
            return true;
        }

        try
        {
            switch (args[0].ToLowerInvariant())
            {
                case "help": SendHelp(sender); return true;
                case "world": return TestWorld(sender);
                case "chunks": return TestLoadedChunks(sender);
                case "snapshot": return TestChunkSnapshot(sender, args);
                case "entities": return TestChunkEntities(sender, args);
                case "loadchunk": return TestLoadUnloadChunk(sender, args);
                case "enderchest": return TestEnderChest(sender);
                case "disenchant": return TestDisenchant(sender);
                case "events":
                    Reply(sender,$"Chunk loads observed: {FourKitTestPlugin.ChunkLoadCount}");
                    Reply(sender,$"Chunk unloads observed: {FourKitTestPlugin.ChunkUnloadCount}");
                    return true;
                default:
                    Reply(sender,$"Unknown subcommand '{args[0]}'. Try /fktest help");
                    return true;
            }
        }
        catch (Exception ex)
        {
            Reply(sender, $"Test threw: {ex.GetType().Name}: {ex.Message}");
            FourKitTestPlugin.Log($"Exception: {ex}");
            return true;
        }
    }

    private static void SendHelp(CommandSender sender)
    {
        Reply(sender,"FourKit test commands:");
        Reply(sender,"/fktest world       - World/spawn/seed lookup");
        Reply(sender,"/fktest chunks      - List loaded chunks in your world");
        Reply(sender,"/fktest snapshot    - Snapshot of chunk under your feet");
        Reply(sender,"/fktest entities    - Entities in chunk under your feet");
        Reply(sender,"/fktest loadchunk [dx dz] - Load/unload a chunk relative to you");
        Reply(sender,"/fktest enderchest  - Probe ender chest inventory");
        Reply(sender,"/fktest disenchant  - Verify setDurability preserves enchants");
        Reply(sender,"/fktest events      - Show observed chunk-event counters");
    }

    private static Player? RequirePlayer(CommandSender sender)
    {
        if (sender is Player p) return p;
        Reply(sender,"This subcommand must be run by a player.");
        return null;
    }

    private static bool TestWorld(CommandSender sender)
    {
        var player = RequirePlayer(sender);
        if (player == null) return true;

        var loc = player.getLocation();
        var world = loc?.getWorld();
        if (world == null)
        {
            Reply(sender,"Could not resolve player world.");
            return true;
        }

        var spawn = world.getSpawnLocation();
        Reply(sender,$"World name={world.getName()} dim={world.getDimensionId()}");
        Reply(sender,$"Spawn = ({spawn.getBlockX()},{spawn.getBlockY()},{spawn.getBlockZ()}) seed={world.getSeed()} time={world.getTime()}");
        Reply(sender,$"Players in world: {world.getPlayers().Count}");
        return true;
    }

    private static bool TestLoadedChunks(CommandSender sender)
    {
        var player = RequirePlayer(sender);
        if (player == null) return true;

        var world = player.getLocation()?.getWorld();
        if (world == null) { Reply(sender,"No world."); return true; }

        var chunks = world.getLoadedChunks();
        Reply(sender,$"Loaded chunks in {world.getName()}: {chunks.Length}");
        int sample = Math.Min(5, chunks.Length);
        for (int i = 0; i < sample; i++)
        {
            var c = chunks[i];
            bool inUse = world.isChunkInUse(c.getX(), c.getZ());
            Reply(sender,$"  ({c.getX()},{c.getZ()}) loaded={c.isLoaded()} inUse={inUse}");
        }
        return true;
    }

    private static bool TestChunkSnapshot(CommandSender sender, string[] args)
    {
        var player = RequirePlayer(sender);
        if (player == null) return true;

        var loc = player.getLocation();
        var world = loc?.getWorld();
        if (world == null || loc == null) { Reply(sender,"No world."); return true; }

        bool includeBiome = args.Length > 1 && args[1].Equals("biome", StringComparison.OrdinalIgnoreCase);

        var chunk = world.getChunkAt(loc);
        var snap = chunk.getChunkSnapshot(includeBiome, includeBiome);

        int blocksUnderFeet = 0;
        int lx = loc.getBlockX() & 0xF;
        int lz = loc.getBlockZ() & 0xF;
        for (int ly = 0; ly < 128; ly++)
        {
            if (snap.getBlockTypeId(lx, ly, lz) != 0) blocksUnderFeet++;
        }

        Reply(sender,$"Snapshot of chunk ({chunk.getX()},{chunk.getZ()}) in '{snap.getWorldName()}' captured at tick {snap.getCaptureFullTime()}.");
        Reply(sender,$"Non-air column at ({lx},{lz}): {blocksUnderFeet} blocks. Highest = y{snap.getHighestBlockYAt(lx, lz)}.");
        if (includeBiome)
        {
            var biome = snap.getBiome(lx, lz);
            Reply(sender,$"Biome at ({lx},{lz}) = {biome}, temp={snap.getRawBiomeTemperature(lx, lz):0.00}, rain={snap.getRawBiomeRainfall(lx, lz):0.00}");
        }
        return true;
    }

    private static bool TestChunkEntities(CommandSender sender, string[] args)
    {
        var player = RequirePlayer(sender);
        if (player == null) return true;

        var loc = player.getLocation();
        var world = loc?.getWorld();
        if (world == null || loc == null) { Reply(sender,"No world."); return true; }

        var chunk = world.getChunkAt(loc);
        var entities = chunk.getEntities();
        Reply(sender,$"Entities in chunk ({chunk.getX()},{chunk.getZ()}): {entities.Length}");
        int sample = Math.Min(8, entities.Length);
        for (int i = 0; i < sample; i++)
        {
            var ent = entities[i];
            Reply(sender,$"  id={ent.getEntityId()} type={ent.GetType()}");
        }
        return true;
    }

    private static bool TestLoadUnloadChunk(CommandSender sender, string[] args)
    {
        var player = RequirePlayer(sender);
        if (player == null) return true;

        var loc = player.getLocation();
        var world = loc?.getWorld();
        if (world == null || loc == null) { Reply(sender,"No world."); return true; }

        int dx = 0, dz = 0;
        if (args.Length >= 3 && int.TryParse(args[1], out var px) && int.TryParse(args[2], out var pz))
        {
            dx = px; dz = pz;
        }

        int cx = (loc.getBlockX() >> 4) + dx;
        int cz = (loc.getBlockZ() >> 4) + dz;

        bool wasLoaded = world.isChunkLoaded(cx, cz);
        Reply(sender,$"Chunk ({cx},{cz}) loaded? {wasLoaded}");

        if (!wasLoaded)
        {
            bool ok = world.loadChunk(cx, cz, true);
            Reply(sender,$"loadChunk -> {ok}; nowLoaded={world.isChunkLoaded(cx, cz)}");
        }
        else
        {
            bool inUse = world.isChunkInUse(cx, cz);
            if (inUse)
            {
                Reply(sender,$"Chunk in use by a player; requesting unsafe unload would refuse. Trying unloadChunkRequest(safe=true).");
                bool queued = world.unloadChunkRequest(cx, cz, true);
                Reply(sender,$"unloadChunkRequest -> {queued}");
            }
            else
            {
                bool unloaded = world.unloadChunk(cx, cz, true, true);
                Reply(sender,$"unloadChunk -> {unloaded}");
            }
        }
        return true;
    }

    private static bool TestEnderChest(CommandSender sender)
    {
        var player = RequirePlayer(sender);
        if (player == null) return true;

        var ender = player.getEnderChest();
        Reply(sender,$"Ender chest size = {ender.getSize()} type={ender.getType()} name='{ender.getName()}'");

        int filled = 0;
        for (int i = 0; i < ender.getSize(); i++)
        {
            var item = ender.getItem(i);
            if (item != null && item.getAmount() > 0)
            {
                filled++;
                if (filled <= 4)
                {
                    Reply(sender,$"  slot {i}: {item.getType()} x{item.getAmount()} dur={item.getDurability()}");
                }
            }
        }
        Reply(sender,$"Non-empty slots: {filled}");
        return true;
    }

    private static bool TestDisenchant(CommandSender sender)
    {
        var player = RequirePlayer(sender);
        if (player == null) return true;

        var pickaxe = new ItemStack(Material.DIAMOND_PICKAXE, 1, 0);
        var meta = pickaxe.getItemMeta();
        meta.addEnchant(EnchantmentType.DIG_SPEAD, 5, true);
        meta.addEnchant(EnchantmentType.DURABILITY, 3, true);
        pickaxe.setItemMeta(meta);

        int beforeCount = pickaxe.getItemMeta().getEnchants().Count;
        Reply(sender,$"Before setDurability: {beforeCount} enchants.");

        pickaxe.setDurability(123);

        var after = pickaxe.getItemMeta().getEnchants();
        Reply(sender,$"After setDurability(123): {after.Count} enchants, durability={pickaxe.getDurability()}");
        foreach (var kv in after)
        {
            Reply(sender,$"  {kv.Key} lvl {kv.Value}");
        }

        bool ok = after.Count == beforeCount;
        Reply(sender,ok
            ? "PASS: setDurability preserved enchantments."
            : "FAIL: setDurability dropped enchantments.");
        return true;
    }
}
