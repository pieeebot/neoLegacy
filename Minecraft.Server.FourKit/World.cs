using System.Runtime.InteropServices;
using Minecraft.Server.FourKit.Chunk;
using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit;

/// <summary>
/// Represents a world, which may contain entities, chunks and blocks.
/// </summary>
public class World
{
    private readonly int _dimensionId;
    private readonly string _name;

    internal World(int dimensionId, string name)
    {
        _dimensionId = dimensionId;
        _name = name;
    }

    /// <summary>
    /// Gets the dimension ID of this world.
    /// </summary>
    /// <returns>Dimension ID of this world.</returns>
    public int getDimensionId() => _dimensionId;

    /// <summary>
    /// Gets the unique name of this world.
    /// </summary>
    /// <returns>Name of this world.</returns>
    public string getName() => _name;

    /// <summary>
    /// Gets the Block at the given coordinates.
    /// </summary>
    /// <param name="x">X-coordinate of the block.</param>
    /// <param name="y">Y-coordinate of the block.</param>
    /// <param name="z">Z-coordinate of the block.</param>
    /// <returns>Block at the given coordinates.</returns>
    public Block.Block getBlockAt(int x, int y, int z)
    {
        return new Block.Block(this, x, y, z);
    }

    /// <summary>
    /// Gets the Block at the given Location.
    /// </summary>
    /// <param name="location">Location of the block.</param>
    /// <returns>Block at the given location.</returns>
    public Block.Block getBlockAt(Location location)
    {
        return getBlockAt(location.getBlockX(), location.getBlockY(), location.getBlockZ());
    }

    /// <summary>
    /// Gets the block type ID at the given coordinates.
    /// </summary>
    /// <param name="x">X-coordinate of the block.</param>
    /// <param name="y">Y-coordinate of the block.</param>
    /// <param name="z">Z-coordinate of the block.</param>
    /// <returns>Type ID of the block.</returns>
    public int getBlockTypeIdAt(int x, int y, int z)
    {
        if (NativeBridge.GetTileId != null)
            return NativeBridge.GetTileId(_dimensionId, x, y, z);
        return 0;
    }

    /// <summary>
    /// Gets the block type ID at the given Location.
    /// </summary>
    /// <param name="location">Location of the block.</param>
    /// <returns>Type ID of the block.</returns>
    public int getBlockTypeIdAt(Location location)
    {
        return getBlockTypeIdAt(location.getBlockX(), location.getBlockY(), location.getBlockZ());
    }

    /// <summary>
    /// Gets the highest non-air coordinate at the given coordinates.
    /// </summary>
    /// <param name="x">X-coordinate.</param>
    /// <param name="z">Z-coordinate.</param>
    /// <returns>The Y-coordinate of the highest non-air block.</returns>
    public int getHighestBlockYAt(int x, int z)
    {
        if (NativeBridge.GetHighestBlockY != null)
            return NativeBridge.GetHighestBlockY(_dimensionId, x, z);
        return 0;
    }

    /// <summary>
    /// Gets the highest non-air coordinate at the given Location.
    /// </summary>
    /// <param name="location">Location to check.</param>
    /// <returns>The Y-coordinate of the highest non-air block.</returns>
    public int getHighestBlockYAt(Location location)
    {
        return getHighestBlockYAt(location.getBlockX(), location.getBlockZ());
    }

    /// <summary>
    /// Gets the highest non-empty block at the given coordinates.
    /// </summary>
    /// <param name="x">X-coordinate.</param>
    /// <param name="z">Z-coordinate.</param>
    /// <returns>Highest non-empty block.</returns>
    public Block.Block getHighestBlockAt(int x, int z)
    {
        int y = getHighestBlockYAt(x, z);
        return getBlockAt(x, y, z);
    }

    /// <summary>
    /// Gets the highest non-empty block at the given Location.
    /// </summary>
    /// <param name="location">Coordinates to get the highest block at.</param>
    /// <returns>Highest non-empty block.</returns>
    public Block.Block getHighestBlockAt(Location location)
    {
        return getHighestBlockAt(location.getBlockX(), location.getBlockZ());
    }

    private double[] GetWorldInfoSnapshot()
    {
        double[] buf = new double[7];
        if (NativeBridge.GetWorldInfo != null)
        {
            var gh = GCHandle.Alloc(buf, GCHandleType.Pinned);
            try
            {
                NativeBridge.GetWorldInfo(_dimensionId, gh.AddrOfPinnedObject());
            }
            finally
            {
                gh.Free();
            }
        }
        return buf;
    }

    /// <summary>
    /// Gets the default spawn Location of this world.
    /// </summary>
    /// <returns>The spawn location of this world.</returns>
    public Location getSpawnLocation()
    {
        double[] info = GetWorldInfoSnapshot();
        return new Location(this, info[0], info[1], info[2], 0f, 0f);
    }

    /// <summary>
    /// Sets the spawn location of the world.
    /// </summary>
    /// <param name="x">X-coordinate.</param>
    /// <param name="y">Y-coordinate.</param>
    /// <param name="z">Z-coordinate.</param>
    /// <returns>True if the spawn was set successfully.</returns>
    public bool setSpawnLocation(int x, int y, int z)
    {
        if (NativeBridge.SetSpawnLocation != null)
            return NativeBridge.SetSpawnLocation(_dimensionId, x, y, z) != 0;
        return false;
    }

    /// <summary>
    /// Gets the Seed for this world.
    /// </summary>
    /// <returns>This world's Seed.</returns>
    public long getSeed()
    {
        double[] info = GetWorldInfoSnapshot();
        return (long)info[3];
    }

    /// <summary>
    /// Gets the relative in-game time of this world.
    /// </summary>
    /// <returns>The current relative time.</returns>
    public long getTime()
    {
        double[] info = GetWorldInfoSnapshot();
        return (long)info[4];
    }

    /// <summary>
    /// Sets the relative in-game time on the server.
    /// </summary>
    /// <param name="time">The new relative time to set the in-game time to.</param>
    public void setTime(long time)
    {
        NativeBridge.SetWorldTime?.Invoke(_dimensionId, time);
    }

    /// <summary>
    /// Sets the in-game time on the server.
    /// </summary>
    /// <param name="time">The new absolute time to set this world to.</param>
    public void setFullTime(long time)
    {
        NativeBridge.SetWorldTime?.Invoke(_dimensionId, time);
    }

    /// <summary>
    /// Set whether there is a storm.
    /// </summary>
    /// <param name="hasStorm">Whether there is rain and snow.</param>
    public void setStorm(bool hasStorm)
    {
        NativeBridge.SetWeather?.Invoke(_dimensionId, hasStorm ? 1 : 0, -1, -1);
    }

    /// <summary>
    /// Set whether it is thundering.
    /// </summary>
    /// <param name="thundering">Whether it is thundering.</param>
    public void setThundering(bool thundering)
    {
        NativeBridge.SetWeather?.Invoke(_dimensionId, -1, thundering ? 1 : 0, -1);
    }

    /// <summary>
    /// Set the thundering duration.
    /// </summary>
    /// <param name="duration">Duration in ticks.</param>
    public void setThunderDuration(int duration)
    {
        NativeBridge.SetWeather?.Invoke(_dimensionId, -1, -1, duration);
    }

    /// <summary>
    /// Get a list of all players in this World.
    /// </summary>
    /// <returns>A list of all Players currently residing in this world.</returns>
    public List<Player> getPlayers()
    {
        var all = FourKit.getOnlinePlayers();
        var result = new List<Player>();
        foreach (var p in all)
        {
            var loc = p.getLocation();
            if (loc?.LocationWorld == this)
                result.Add(p);
        }
        return result;
    }

    /// <summary>
    /// Get a list of all entities in this World.
    /// </summary>
    /// <returns>A list of all Entities currently residing in this world.</returns>
    public List<Entity.Entity> getEntities()
    {
        var result = new List<Entity.Entity>();
        if (NativeBridge.GetWorldEntities == null) return result;

        int count = NativeBridge.GetWorldEntities(_dimensionId, out IntPtr buf);
        if (count <= 0 || buf == IntPtr.Zero) return result;

        try
        {
            int[] data = new int[count * 3];
            Marshal.Copy(buf, data, 0, count * 3);

            for (int i = 0; i < count; i++)
            {
                int entityId = data[i * 3];
                int mappedType = data[i * 3 + 1];
                int isLiving = data[i * 3 + 2];

                var entityType = Enum.IsDefined(typeof(Entity.EntityType), mappedType)
                    ? (Entity.EntityType)mappedType
                    : Entity.EntityType.UNKNOWN;

                if (entityType == Entity.EntityType.PLAYER)
                {
                    var player = FourKit.GetPlayerByEntityId(entityId);
                    if (player != null)
                    {
                        result.Add(player);
                        continue;
                    }
                }

                if (isLiving == 1)
                {
                    result.Add(new Entity.LivingEntity(entityId, entityType, _dimensionId, 0, 0, 0));
                }
                else
                {
                    var entity = new Entity.Entity();
                    entity.SetEntityIdInternal(entityId);
                    entity.SetEntityTypeInternal(entityType);
                    entity.SetDimensionInternal(_dimensionId);
                    result.Add(entity);
                }
            }
        }
        finally
        {
            Marshal.FreeCoTaskMem(buf);
        }

        return result;
    }

    /// <summary>
    /// Get a list of all living entities in this World.
    /// </summary>
    /// <returns>A list of all LivingEntities currently residing in this world.</returns>
    public List<Entity.LivingEntity> getLivingEntities()
    {
        var result = new List<Entity.LivingEntity>();
        if (NativeBridge.GetWorldEntities == null) return result;

        int count = NativeBridge.GetWorldEntities(_dimensionId, out IntPtr buf);
        if (count <= 0 || buf == IntPtr.Zero) return result;

        try
        {
            int[] data = new int[count * 3];
            Marshal.Copy(buf, data, 0, count * 3);

            for (int i = 0; i < count; i++)
            {
                int entityId = data[i * 3];
                int mappedType = data[i * 3 + 1];
                int isLiving = data[i * 3 + 2];

                if (isLiving != 1) continue;

                var entityType = Enum.IsDefined(typeof(Entity.EntityType), mappedType)
                    ? (Entity.EntityType)mappedType
                    : Entity.EntityType.UNKNOWN;

                if (entityType == Entity.EntityType.PLAYER)
                {
                    var player = FourKit.GetPlayerByEntityId(entityId);
                    if (player != null)
                    {
                        result.Add(player);
                        continue;
                    }
                }

                result.Add(new Entity.LivingEntity(entityId, entityType, _dimensionId, 0, 0, 0));
            }
        }
        finally
        {
            Marshal.FreeCoTaskMem(buf);
        }

        return result;
    }

    /// <summary>
    /// Creates explosion at given coordinates with given power.
    /// </summary>
    /// <param name="x">X-coordinate.</param>
    /// <param name="y">Y-coordinate.</param>
    /// <param name="z">Z-coordinate.</param>
    /// <param name="power">The power of explosion, where 4F is TNT.</param>
    /// <returns>false if explosion was canceled, otherwise true.</returns>
    public bool createExplosion(double x, double y, double z, float power)
    {
        return createExplosion(x, y, z, power, false, true);
    }

    /// <summary>
    /// Creates explosion at given coordinates with given power and optionally
    /// setting blocks on fire.
    /// </summary>
    /// <param name="x">X-coordinate.</param>
    /// <param name="y">Y-coordinate.</param>
    /// <param name="z">Z-coordinate.</param>
    /// <param name="power">The power of explosion, where 4F is TNT.</param>
    /// <param name="setFire">Whether or not to set blocks on fire.</param>
    /// <returns>false if explosion was canceled, otherwise true.</returns>
    public bool createExplosion(double x, double y, double z, float power, bool setFire)
    {
        return createExplosion(x, y, z, power, setFire, true);
    }

    /// <summary>
    /// Creates explosion at given coordinates with given power and optionally
    /// setting blocks on fire or breaking blocks.
    /// </summary>
    /// <param name="x">X-coordinate.</param>
    /// <param name="y">Y-coordinate.</param>
    /// <param name="z">Z-coordinate.</param>
    /// <param name="power">The power of explosion, where 4F is TNT.</param>
    /// <param name="setFire">Whether or not to set blocks on fire.</param>
    /// <param name="breakBlocks">Whether or not to have blocks be destroyed.</param>
    /// <returns>false if explosion was canceled, otherwise true.</returns>
    public bool createExplosion(double x, double y, double z, float power, bool setFire, bool breakBlocks)
    {
        if (NativeBridge.CreateExplosion != null)
            return NativeBridge.CreateExplosion(_dimensionId, x, y, z, power, setFire ? 1 : 0, breakBlocks ? 1 : 0) != 0;
        return false;
    }

    /// <summary>
    /// Creates explosion at given coordinates with given power and optionally
    /// setting blocks on fire or breaking blocks.
    /// </summary>
    /// <param name="loc">Location to blow up.</param>
    /// <param name="power">The power of explosion, where 4F is TNT.</param>
    /// <param name="setFire">Whether or not to set blocks on fire.</param>
    /// <param name="breakBlocks">Whether or not to have blocks be destroyed.</param>
    /// <returns>false if explosion was canceled, otherwise true.</returns>
    public bool createExplosion(Location loc, float power, bool setFire, bool breakBlocks)
    {
        if (NativeBridge.CreateExplosion != null)
            return NativeBridge.CreateExplosion(_dimensionId, loc.X, loc.Y, loc.Z, power, setFire ? 1 : 0, breakBlocks ? 1 : 0) != 0;
        return false;
    }

    /// <summary>
    /// Creates explosion at given coordinates with given power.
    /// </summary>
    /// <param name="loc">Location to blow up.</param>
    /// <param name="power">The power of explosion, where 4F is TNT.</param>
    /// <returns>false if explosion was canceled, otherwise true.</returns>
    public bool createExplosion(Location loc, float power)
    {
        return createExplosion(loc.X, loc.Y, loc.Z, power);
    }

    /// <summary>
    /// Creates explosion at given coordinates with given power and optionally
    /// setting blocks on fire.
    /// </summary>
    /// <param name="loc">Location to blow up.</param>
    /// <param name="power">The power of explosion, where 4F is TNT.</param>
    /// <param name="setFire">Whether or not to set blocks on fire.</param>
    /// <returns>false if explosion was canceled, otherwise true.</returns>
    public bool createExplosion(Location loc, float power, bool setFire)
    {
        return createExplosion(loc.X, loc.Y, loc.Z, power, setFire);
    }

    /// <summary>
    /// Strikes lightning at the given Location.
    /// </summary>
    /// <param name="loc">The location to strike lightning.</param>
    /// <returns>true if lightning was successfully summoned.</returns>
    public bool strikeLightning(Location loc)
    {
        if (NativeBridge.StrikeLightning != null)
            return NativeBridge.StrikeLightning(_dimensionId, loc.X, loc.Y, loc.Z, 0) != 0;
        return false;
    }

    /// <summary>
    /// Strikes lightning at the given Location without doing damage.
    /// </summary>
    /// <param name="loc">The location to strike lightning.</param>
    /// <returns>true if lightning was successfully summoned.</returns>
    public bool strikeLightningEffect(Location loc)
    {
        if (NativeBridge.StrikeLightning != null)
            return NativeBridge.StrikeLightning(_dimensionId, loc.X, loc.Y, loc.Z, 1) != 0;
        return false;
    }

    /// <summary>
    /// Drops an item at the specified Location.
    /// </summary>
    /// <param name="location">Location to drop the item.</param>
    /// <param name="item">ItemStack to drop.</param>
    public void dropItem(Location location, ItemStack item)
    {
        NativeBridge.DropItem?.Invoke(_dimensionId, location.X, location.Y, location.Z, item.getTypeId(), item.getAmount(), item.getDurability(), 0);
    }

    /// <summary>
    /// Drops an item at the specified Location with a random offset.
    /// </summary>
    /// <param name="location">Location to drop the item.</param>
    /// <param name="item">ItemStack to drop.</param>
    public void dropItemNaturally(Location location, ItemStack item)
    {
        NativeBridge.DropItem?.Invoke(_dimensionId, location.X, location.Y, location.Z, item.getTypeId(), item.getAmount(), item.getDurability(), 1);
    }

    /// <summary>
    /// Gets the Chunk at the given coordinates.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <returns>Chunk at the given coordinates.</returns>
    public Chunk.Chunk getChunkAt(int x, int z)
    {
        return new Chunk.Chunk(this, x, z);
    }

    /// <summary>
    /// Gets the Chunk at the given Location.
    /// </summary>
    /// <param name="location">Location of the chunk.</param>
    /// <returns>Chunk at the given location.</returns>
    public Chunk.Chunk getChunkAt(Location location)
    {
        return getChunkAt(location.getBlockX() >> 4, location.getBlockZ() >> 4);
    }

    /// <summary>
    /// Gets the Chunk that contains the given Block.
    /// </summary>
    /// <param name="block">Block to get the containing chunk from.</param>
    /// <returns>The chunk that contains the given block.</returns>
    public Chunk.Chunk getChunkAt(Block.Block block)
    {
        return getChunkAt(block.getX() >> 4, block.getZ() >> 4);
    }

    /// <summary>
    /// Checks if the specified Chunk is loaded.
    /// </summary>
    /// <param name="chunk">The chunk to check.</param>
    /// <returns>true if the chunk is loaded, otherwise false.</returns>
    public bool isChunkLoaded(Chunk.Chunk chunk)
    {
        return isChunkLoaded(chunk.getX(), chunk.getZ());
    }

    /// <summary>
    /// Checks if the Chunk at the specified coordinates is loaded.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <returns>true if the chunk is loaded, otherwise false.</returns>
    public bool isChunkLoaded(int x, int z)
    {
        if (NativeBridge.IsChunkLoaded != null)
            return NativeBridge.IsChunkLoaded(_dimensionId, x, z) != 0;
        return false;
    }

    /// <summary>
    /// Gets an array of all loaded Chunks.
    /// </summary>
    /// <returns>Chunk[] containing all loaded chunks.</returns>
    public Chunk.Chunk[] getLoadedChunks()
    {
        if (NativeBridge.GetLoadedChunks == null)
            return Array.Empty<Chunk.Chunk>();

        int count = NativeBridge.GetLoadedChunks(_dimensionId, out IntPtr buf);
        if (count <= 0 || buf == IntPtr.Zero)
            return Array.Empty<Chunk.Chunk>();

        try
        {
            int[] coords = new int[count * 2];
            Marshal.Copy(buf, coords, 0, count * 2);
            var chunks = new Chunk.Chunk[count];
            for (int i = 0; i < count; i++)
                chunks[i] = new Chunk.Chunk(this, coords[i * 2], coords[i * 2 + 1]);
            return chunks;
        }
        finally
        {
            Marshal.FreeCoTaskMem(buf);
        }
    }

    /// <summary>
    /// Loads the specified Chunk.
    /// </summary>
    /// <param name="chunk">The chunk to load.</param>
    public void loadChunk(Chunk.Chunk chunk)
    {
        loadChunk(chunk.getX(), chunk.getZ());
    }

    /// <summary>
    /// Loads the Chunk at the specified coordinates.
    /// If the chunk does not exist, it will be generated. This method is
    /// analogous to loadChunk(int, int, boolean) where generate is true.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    public void loadChunk(int x, int z)
    {
        loadChunk(x, z, true);
    }

    /// <summary>
    /// Loads the Chunk at the specified coordinates.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <param name="generate">Whether or not to generate a chunk if it doesn't already exist.</param>
    /// <returns>true if the chunk has loaded successfully, otherwise false.</returns>
    public bool loadChunk(int x, int z, bool generate)
    {
        if (NativeBridge.LoadChunk != null)
            return NativeBridge.LoadChunk(_dimensionId, x, z, generate ? 1 : 0) != 0;
        return false;
    }

    /// <summary>
    /// Checks if the Chunk at the specified coordinates is loaded and in use
    /// by one or more players.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <returns>true if the chunk is loaded and in use by one or more players, otherwise false.</returns>
    public bool isChunkInUse(int x, int z)
    {
        if (NativeBridge.IsChunkInUse != null)
            return NativeBridge.IsChunkInUse(_dimensionId, x, z) != 0;
        return false;
    }

    /// <summary>
    /// Safely unloads and saves the Chunk at the specified coordinates.
    /// This method is analogous to unloadChunk(int, int, boolean, boolean)
    /// where safe and save is true.
    /// </summary>
    /// <param name="chunk">The chunk to unload.</param>
    /// <returns>true if the chunk has unloaded successfully, otherwise false.</returns>
    public bool unloadChunk(Chunk.Chunk chunk)
    {
        return unloadChunk(chunk.getX(), chunk.getZ());
    }

    /// <summary>
    /// Safely unloads and saves the Chunk at the specified coordinates.
    /// This method is analogous to unloadChunk(int, int, boolean, boolean)
    /// where safe and save is true.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <returns>true if the chunk has unloaded successfully, otherwise false.</returns>
    public bool unloadChunk(int x, int z)
    {
        return unloadChunk(x, z, true, true);
    }

    /// <summary>
    /// Safely unloads and optionally saves the Chunk at the specified coordinates.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <param name="save">Whether or not to save the chunk.</param>
    /// <returns>true if the chunk has unloaded successfully, otherwise false.</returns>
    public bool unloadChunk(int x, int z, bool save)
    {
        return unloadChunk(x, z, save, true);
    }

    /// <summary>
    /// Unloads and optionally saves the Chunk at the specified coordinates.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <param name="save">Controls whether the chunk is saved.</param>
    /// <param name="safe">Controls whether to unload the chunk when players are nearby.</param>
    /// <returns>true if the chunk has unloaded successfully, otherwise false.</returns>
    public bool unloadChunk(int x, int z, bool save, bool safe)
    {
        if (NativeBridge.UnloadChunk != null)
            return NativeBridge.UnloadChunk(_dimensionId, x, z, save ? 1 : 0, safe ? 1 : 0) != 0;
        return false;
    }

    /// <summary>
    /// Safely queues the Chunk at the specified coordinates for unloading.
    /// This method is analogous to unloadChunkRequest(int, int, boolean)
    /// where safe is true.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <returns>true is the queue attempt was successful, otherwise false.</returns>
    public bool unloadChunkRequest(int x, int z)
    {
        return unloadChunkRequest(x, z, true);
    }

    /// <summary>
    /// Queues the Chunk at the specified coordinates for unloading.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <param name="safe">Controls whether to queue the chunk when players are nearby.</param>
    /// <returns>Whether the chunk was actually queued.</returns>
    public bool unloadChunkRequest(int x, int z, bool safe)
    {
        if (NativeBridge.UnloadChunkRequest != null)
            return NativeBridge.UnloadChunkRequest(_dimensionId, x, z, safe ? 1 : 0) != 0;
        return false;
    }

    /// <summary>
    /// Regenerates the Chunk at the specified coordinates.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <returns>Whether the chunk was actually regenerated.</returns>
    public bool regenerateChunk(int x, int z)
    {
        if (NativeBridge.RegenerateChunk != null)
            return NativeBridge.RegenerateChunk(_dimensionId, x, z) != 0;
        return false;
    }

    /// <summary>
    /// Resends the Chunk to all clients.
    /// </summary>
    /// <param name="x">X-coordinate of the chunk.</param>
    /// <param name="z">Z-coordinate of the chunk.</param>
    /// <returns>Whether the chunk was actually refreshed.</returns>
    public bool refreshChunk(int x, int z)
    {
        if (NativeBridge.RefreshChunk != null)
            return NativeBridge.RefreshChunk(_dimensionId, x, z) != 0;
        return false;
    }
}
