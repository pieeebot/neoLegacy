namespace Minecraft.Server.FourKit.Block;

/// <summary>
/// Represents a captured state of a block, which will not change
/// automatically.
///
/// <para>Unlike <see cref="Block"/>, which only one object can exist per
/// coordinate, BlockState can exist multiple times for any given Block.
/// Note that another plugin may change the state of the block and you will
/// not know, or they may change the block to another type entirely, causing
/// your BlockState to become invalid.</para>
/// </summary>
public class BlockState
{
    private readonly World _world;
    private readonly int _x;
    private readonly int _y;
    private readonly int _z;
    private int _typeId;
    private int _data;

    internal BlockState(World world, int x, int y, int z, int typeId, int data)
    {
        _world = world;
        _x = x;
        _y = y;
        _z = z;
        _typeId = typeId;
        _data = data;
    }

    /// <summary>
    /// Gets the block represented by this BlockState.
    /// </summary>
    /// <returns>Block that this BlockState represents.</returns>
    public Block getBlock()
    {
        return new Block(_world, _x, _y, _z);
    }

    /// <summary>
    /// Gets the metadata for this block.
    /// </summary>
    /// <returns>Block specific metadata.</returns>
    public int getData() => _data;

    /// <summary>
    /// Sets the metadata for this block.
    /// </summary>
    /// <param name="data">New block specific metadata.</param>
    public void setData(int data)
    {
        _data = data;
    }

    /// <summary>
    /// Gets the type of this block.
    /// </summary>
    /// <returns>Block type.</returns>
    public Material getType()
    {
        return Enum.IsDefined(typeof(Material), _typeId) ? (Material)_typeId : Material.AIR;
    }

    /// <summary>
    /// Gets the type ID of this block.
    /// </summary>
    /// <returns>Block type ID.</returns>
    public int getTypeId() => _typeId;

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
    /// Gets the chunk which contains this block.
    /// </summary>
    /// <returns>Containing Chunk.</returns>
    public Chunk.Chunk getChunk()
    {
        return _world.getChunkAt(_x >> 4, _z >> 4);
    }

    /// <summary>
    /// Gets the location of this block.
    /// </summary>
    /// <returns>Location.</returns>
    public Location getLocation()
    {
        return new Location(_world, _x, _y, _z, 0f, 0f);
    }

    /// <summary>
    /// Stores the location of this block in the provided Location object.
    /// If the provided Location is null this method does nothing and returns
    /// null.
    /// </summary>
    /// <param name="loc">The location object to store in.</param>
    /// <returns>The Location object provided or null.</returns>
    public Location? getLocation(Location? loc)
    {
        if (loc == null) return null;
        loc.X = _x;
        loc.Y = _y;
        loc.Z = _z;
        loc.LocationWorld = _world;
        return loc;
    }

    /// <summary>
    /// Sets the type of this block.
    /// </summary>
    /// <param name="type">Material to change this block to.</param>
    public void setType(Material type)
    {
        _typeId = (int)type;
    }

    /// <summary>
    /// Sets the type ID of this block.
    /// </summary>
    /// <param name="type">Type ID to change this block to.</param>
    /// <returns>Whether the change was accepted.</returns>
    public bool setTypeId(int type)
    {
        _typeId = type;
        return true;
    }

    /// <summary>
    /// Attempts to update the block represented by this state, setting it to
    /// the new values as defined by this state.
    /// <para>This has the same effect as calling <c>update(false)</c>.</para>
    /// </summary>
    /// <returns><c>true</c> if the update was successful, otherwise
    /// <c>false</c>.</returns>
    public bool update()
    {
        return update(false);
    }

    /// <summary>
    /// Attempts to update the block represented by this state, setting it to
    /// the new values as defined by this state.
    /// <para>This has the same effect as calling
    /// <c>update(force, true)</c>.</para>
    /// </summary>
    /// <param name="force"><c>true</c> to forcefully set the state.</param>
    /// <returns><c>true</c> if the update was successful, otherwise
    /// <c>false</c>.</returns>
    public bool update(bool force)
    {
        return update(force, true);
    }

    /// <summary>
    /// Attempts to update the block represented by this state, setting it to
    /// the new values as defined by this state.
    /// <para>Unless <paramref name="force"/> is true, this will not modify the
    /// state of a block if it is no longer the same type as it was when this
    /// state was taken. It will return false in this eventuality.</para>
    /// <para>If <paramref name="force"/> is true, it will set the type of the
    /// block to match the new state, set the state data and then return
    /// true.</para>
    /// <para>If <paramref name="applyPhysics"/> is true, it will trigger a
    /// physics update on surrounding blocks which could cause them to update
    /// or disappear.</para>
    /// </summary>
    /// <param name="force"><c>true</c> to forcefully set the state.</param>
    /// <param name="applyPhysics"><c>false</c> to cancel updating physics on
    /// surrounding blocks.</param>
    /// <returns><c>true</c> if the update was successful, otherwise
    /// <c>false</c>.</returns>
    public bool update(bool force, bool applyPhysics)
    {
        if (NativeBridge.GetTileId == null || NativeBridge.SetTile == null)
            return false;

        int currentType = NativeBridge.GetTileId(_world.getDimensionId(), _x, _y, _z);
        if (!force && currentType != _typeId)
            return false;

        NativeBridge.SetTile(_world.getDimensionId(), _x, _y, _z, _typeId, _data, applyPhysics ? 3 : 2);
        return true;
    }

    /// <summary>
    /// Gets the light level between 0-15.
    /// </summary>
    /// <returns>Light level.</returns>
    public byte getLightLevel()
    {
        return getBlock().getLightLevel();
    }
}
