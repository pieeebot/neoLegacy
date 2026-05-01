namespace Minecraft.Server.FourKit.Block;

/// <summary>
/// Represents a block. This is a live object, and only one Block may exist for
/// any given location in a world.
/// </summary>
public class Block
{
    private readonly World _world;
    private readonly int _x;
    private readonly int _y;
    private readonly int _z;

    internal Block(World world, int x, int y, int z)
    {
        _world = world;
        _x = x;
        _y = y;
        _z = z;
    }

    /// <summary>
    /// Gets the Location of the block.
    /// </summary>
    /// <returns>Location of the block.</returns>
    public Location getLocation()
    {
        return new Location(_world, _x, _y, _z, 0f, 0f);
    }

    /// <summary>
    /// Gets the type of this block.
    /// </summary>
    /// <returns>Block type.</returns>
    public Material getType()
    {
        int id = getTypeId();
        return Enum.IsDefined(typeof(Material), id) ? (Material)id : Material.AIR;
    }

    /// <summary>
    /// Gets the type ID of this block.
    /// </summary>
    /// <returns>Block type ID.</returns>
    public int getTypeId()
    {
        if (NativeBridge.GetTileId != null)
            return NativeBridge.GetTileId(_world.getDimensionId(), _x, _y, _z);
        return 0;
    }

    /// <summary>
    /// Gets the world which contains this Block.
    /// </summary>
    /// <returns>World containing this block.</returns>
    public World getWorld() => _world;

    /// <summary>
    /// Gets the x-coordinate of this block.
    /// </summary>
    /// <returns>X-coordinate.</returns>
    public int getX() => _x;

    /// <summary>
    /// Gets the y-coordinate of this block.
    /// </summary>
    /// <returns>Y-coordinate.</returns>
    public int getY() => _y;

    /// <summary>
    /// Gets the z-coordinate of this block.
    /// </summary>
    /// <returns>Z-coordinate.</returns>
    public int getZ() => _z;

    /// <summary>
    /// Sets the type of this block.
    /// </summary>
    /// <param name="type">Material to change this block to.</param>
    public void setType(Material type)
    {
        setTypeId((int)type);
    }

    /// <summary>
    /// Sets the type ID of this block.
    /// </summary>
    /// <param name="type">Type ID to change this block to.</param>
    /// <returns>Whether the change was successful.</returns>
    public bool setTypeId(int type)
    {
        return setTypeId(type, true);
    }

    /// <summary>
    /// Sets the type ID of this block.
    /// </summary>
    /// <param name="type">Type ID to change this block to.</param>
    /// <param name="applyPhysics">False to cancel physics on the changed block.</param>
    /// <returns>Whether the block was changed.</returns>
    public bool setTypeId(int type, bool applyPhysics)
    {
        int flags = applyPhysics ? 3 : 2;
        NativeBridge.SetTile?.Invoke(_world.getDimensionId(), _x, _y, _z, type, 0, flags);
        return true;
    }

    /// <summary>
    /// Gets the metadata value for this block.
    /// </summary>
    /// <returns>Block specific metadata.</returns>
    public byte getData()
    {
        return (byte)NativeBridge.GetTileData(_world.getDimensionId(), _x, _y, _z);
    }

    /// <summary>
    /// Sets the metadata value for this block.
    /// </summary>
    /// <param name="data">New block specific metadata.</param>
    public void setData(byte data)
    {
        setData(data, true);
    }

    /// <summary>
    /// Sets the metadata for this block.
    /// </summary>
    /// <param name="data">New block specific metadata.</param>
    /// <param name="applyPhysics">False to cancel physics from the changed block.</param>
    public void setData(byte data, bool applyPhysics)
    {
        int flags = applyPhysics ? 3 : 2;
        NativeBridge.SetTileData?.Invoke(_world.getDimensionId(), _x, _y, _z, data, flags);
    }

    /// <summary>
    /// Sets the type ID and data of this block.
    /// </summary>
    /// <param name="type">Type ID to change this block to.</param>
    /// <param name="data">The data value to change this block to.</param>
    /// <returns>Whether the block was changed.</returns>
    public bool setTypeIdAndData(int type, byte data)
    {
        return setTypeIdAndData(type, data, true);
    }

    /// <summary>
    /// Sets the type ID and data of this block.
    /// </summary>
    /// <param name="type">Type ID to change this block to.</param>
    /// <param name="data">The data value to change this block to.</param>
    /// <param name="applyPhysics">False to cancel physics on the changed block.</param>
    /// <returns>Whether the block was changed.</returns>
    public bool setTypeIdAndData(int type, byte data, bool applyPhysics)
    {
        int flags = applyPhysics ? 3 : 2;
        NativeBridge.SetTile?.Invoke(_world.getDimensionId(), _x, _y, _z, type, data, flags);
        return true;
    }

    /// <summary>
    /// Breaks the block and spawns items as if a player had digged it.
    /// </summary>
    /// <returns>true if the block was destroyed.</returns>
    public bool breakNaturally()
    {
        if (NativeBridge.BreakBlock != null)
            return NativeBridge.BreakBlock(_world.getDimensionId(), _x, _y, _z) != 0;
        return false;
    }

    /// <summary>
    /// Gets the block at the given offsets
    /// </summary>
    /// <param name="modX">X offset</param>
    /// <param name="modY">Y offset</param>
    /// <param name="modZ">Z offset</param>
    /// <returns>Block at the given offsets</returns>
    public Block getRelative(int modX, int modY, int modZ)
    {
        return getWorld().getBlockAt(getX() + modX, getY() + modY, getZ() + modZ);
    }

    /// <summary>
    /// Gets the chunk which contains this block.
    /// </summary>
    /// <returns>Containing Chunk.</returns>
    public Chunk.Chunk getChunk()
    {
        return getWorld().getChunkAt(getX() >> 4, getZ() >> 4);
    }

    /// <summary>
    /// Gets the block at the given face
    /// <para>This method is equal to getRelative(face, 1)</para>
    /// </summary>
    /// <param name="face">BlockFace to get relative to</param>
    /// <returns>Block at the given face</returns>
    public Block getRelative(BlockFace face)
    {
        return getRelative(face, 1);
    }

    /// <summary>
    /// Gets the block at the given distance of the given face
    /// <para>For example, the following method places water at 100,102,100; two
    /// blocks above 100,100,100.</para>
    /// <code>
    /// Block block = world.getBlockAt(100, 100, 100);
    /// Block shower = block.getRelative(BlockFace.UP, 2);
    /// shower.setType(Material.WATER);
    /// </code>
    /// </summary>
    /// <param name="face">BlockFace to get relative to</param>
    /// <param name="distance">Distance to get relative to</param>
    /// <returns>Block at the given distance of the given face</returns>
    public Block getRelative(BlockFace face, int distance)
    {
        return getRelative(face.getModX() * distance, face.getModY() * distance, face.getModZ() * distance);
    }

    /// <summary>
    /// Returns the biome that this block resides in.
    /// </summary>
    /// <returns>Biome type containing this block.</returns>
    public Biome getBiome()
    {
        if (NativeBridge.GetBiomeId != null)
            return BiomeHelper.fromId(NativeBridge.GetBiomeId(_world.getDimensionId(), _x, _z));
        return Biome.PLAINS;
    }

    /// <summary>
    /// Sets the biome that this block resides in.
    /// </summary>
    /// <param name="bio">New Biome type for this block.</param>
    public void setBiome(Biome bio)
    {
        NativeBridge.SetBiomeId?.Invoke(_world.getDimensionId(), _x, _z, (int)bio);
    }

    /// <summary>
    /// Gets the humidity of the biome of this block.
    /// </summary>
    /// <returns>Humidity of this block.</returns>
    public double getHumidity()
    {
        return getBiome().getRainfall();
    }

    /// <summary>
    /// Gets the temperature of the biome of this block.
    /// </summary>
    /// <returns>Temperature of this block.</returns>
    public double getTemperature()
    {
        return getBiome().getTemperature();
    }

    /// <summary>
    /// Checks if this block is liquid.
    /// <para>A block is considered liquid when <see cref="getType()"/> returns
    /// <see cref="Material.WATER"/>, <see cref="Material.STATIONARY_WATER"/>,
    /// <see cref="Material.LAVA"/> or <see cref="Material.STATIONARY_LAVA"/>.</para>
    /// </summary>
    /// <returns>true if this block is liquid.</returns>
    public bool isLiquid()
    {
        Material type = getType();
        return type == Material.WATER || type == Material.STATIONARY_WATER ||
               type == Material.LAVA || type == Material.STATIONARY_LAVA;
    }


    /// <summary>
    /// Gets the light level between 0-15.
    /// </summary>
    /// <returns>Light level.</returns>
    public byte getLightLevel()
    {
        int sky = getLightFromSky();
        int block = getLightFromBlocks();
        return (byte)(sky > block ? sky : block);
    }

    /// <summary>
    /// Get the amount of light at this block from the sky.
    /// Any light given from other sources (such as blocks like torches) will be ignored.
    /// </summary>
    /// <returns>Sky light level.</returns>
    public byte getLightFromSky()
    {
        if (NativeBridge.GetSkyLight != null)
            return (byte)NativeBridge.GetSkyLight(_world.getDimensionId(), _x, _y, _z);
        return 0;
    }

    /// <summary>
    /// Get the amount of light at this block from nearby blocks.
    /// Any light given from other sources (such as the sun) will be ignored.
    /// </summary>
    /// <returns>Block light level.</returns>
    public byte getLightFromBlocks()
    {
        if (NativeBridge.GetBlockLight != null)
            return (byte)NativeBridge.GetBlockLight(_world.getDimensionId(), _x, _y, _z);
        return 0;
    }

}
