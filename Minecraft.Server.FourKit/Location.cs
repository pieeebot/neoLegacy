namespace Minecraft.Server.FourKit;

/// <summary>
/// Represents a 3-dimensional position in a world.
/// </summary>
public class Location
{
    internal double X { get; set; }
    internal double Y { get; set; }
    internal double Z { get; set; }
    internal float Yaw { get; set; }
    internal float Pitch { get; set; }
    internal World? LocationWorld { get; set; }

    /// <summary>
    /// Constructs a new Location with the given coordinates and direction.
    /// </summary>
    /// <param name="world">The world in which this location resides.</param>
    /// <param name="x">The x-coordinate.</param>
    /// <param name="y">The y-coordinate.</param>
    /// <param name="z">The z-coordinate.</param>
    /// <param name="yaw">The absolute rotation on the x-plane, in degrees.</param>
    /// <param name="pitch">The absolute rotation on the y-plane, in degrees.</param>
    public Location(World? world, double x, double y, double z, float yaw, float pitch)
    {
        LocationWorld = world;
        X = x;
        Y = y;
        Z = z;
        Yaw = yaw;
        Pitch = pitch;
    }

    /// <summary>
    /// Constructs a new Location with the given coordinates.
    /// </summary>
    /// <param name="world">The world in which this location resides.</param>
    /// <param name="x">The x-coordinate.</param>
    /// <param name="y">The y-coordinate.</param>
    /// <param name="z">The z-coordinate.</param>
    public Location(World? world, double x, double y, double z) : this(world, x, y, z, 0f, 0f) { }

    /// <summary>
    /// Creates a new <see cref="Location"/> with the given coordinates and no world.
    /// </summary>
    /// <param name="x">The x-coordinate.</param>
    /// <param name="y">The y-coordinate.</param>
    /// <param name="z">The z-coordinate.</param>
    public Location(double x, double y, double z) : this(null, x, y, z, 0f, 0f) { }

    // use for internal
    internal Location() : this(null, 0, 0, 0, 0f, 0f) { }

    /// <summary>
    /// Gets the x-coordinate of this location.
    /// </summary>
    /// <returns>The x-coordinate.</returns>
    public double getX() => X;

    /// <summary>
    /// Sets the x-coordinate of this location.
    /// </summary>
    /// <param name="x">The new x-coordinate.</param>
    public void setX(double x) => X = x;

    /// <summary>
    /// Gets the y-coordinate of this location.
    /// </summary>
    /// <returns>The y-coordinate.</returns>
    public double getY() => Y;

    /// <summary>
    /// Sets the y-coordinate of this location.
    /// </summary>
    /// <param name="y">The new y-coordinate.</param>
    public void setY(double y) => Y = y;

    /// <summary>
    /// Gets the z-coordinate of this location.
    /// </summary>
    /// <returns>The z-coordinate.</returns>
    public double getZ() => Z;

    /// <summary>
    /// Sets the z-coordinate of this location.
    /// </summary>
    /// <param name="z">The new z-coordinate.</param>
    public void setZ(double z) => Z = z;

    /// <summary>
    /// Gets the yaw of this location, measured in degrees.
    /// </summary>
    /// <returns>The yaw.</returns>
    public float getYaw() => Yaw;

    /// <summary>
    /// Sets the yaw of this location, measured in degrees.
    /// </summary>
    /// <param name="yaw">The new yaw.</param>
    public void setYaw(float yaw) => Yaw = yaw;

    /// <summary>
    /// Gets the pitch of this location, measured in degrees.
    /// </summary>
    /// <returns>The pitch.</returns>
    public float getPitch() => Pitch;

    /// <summary>
    /// Sets the pitch of this location, measured in degrees.
    /// </summary>
    /// <param name="pitch">The new pitch.</param>
    public void setPitch(float pitch) => Pitch = pitch;

    /// <summary>
    /// Gets the world that this location resides in.
    /// </summary>
    /// <returns>The world, or <c>null</c> if not set.</returns>
    public World? getWorld() => LocationWorld;

    /// <summary>
    /// Sets the world that this location resides in.
    /// </summary>
    /// <param name="world">The new world.</param>
    public void setWorld(World? world) => LocationWorld = world;

    /// <summary>
    /// Gets the floored value of the X component, indicating the block that
    /// this location is contained with.
    /// </summary>
    /// <returns>The block X.</returns>
    public int getBlockX() => (int)Math.Floor(X);

    /// <summary>
    /// Gets the floored value of the Y component, indicating the block that
    /// this location is contained with.
    /// </summary>
    /// <returns>The block Y.</returns>
    public int getBlockY() => (int)Math.Floor(Y);

    /// <summary>
    /// Gets the floored value of the Z component, indicating the block that
    /// this location is contained with.
    /// </summary>
    /// <returns>The block Z.</returns>
    public int getBlockZ() => (int)Math.Floor(Z);

    /// <summary>
    /// Gets the magnitude of the location, defined as sqrt(x^2+y^2+z^2).
    /// </summary>
    /// <returns>The magnitude.</returns>
    public double length() => Math.Sqrt(X * X + Y * Y + Z * Z);

    /// <summary>
    /// Gets the magnitude of the location squared.
    /// </summary>
    /// <returns>The magnitude squared.</returns>
    public double lengthSquared() => X * X + Y * Y + Z * Z;

    /// <summary>
    /// Adds the location by another.
    /// </summary>
    /// <param name="x">The x-coordinate to add.</param>
    /// <param name="y">The y-coordinate to add.</param>
    /// <param name="z">The z-coordinate to add.</param>
    /// <returns>This location, for chaining.</returns>
    public Location add(double x, double y, double z)
    {
        X += x;
        Y += y;
        Z += z;
        return this;
    }

    /// <summary>
    /// Adds the location by another.
    /// </summary>
    /// <param name="vec">The location to add.</param>
    /// <returns>This location, for chaining.</returns>
    public Location add(Location vec)
    {
        X += vec.X;
        Y += vec.Y;
        Z += vec.Z;
        return this;
    }

    public Location clone() => new Location(LocationWorld, X, Y, Z, Yaw, Pitch);

    /// <summary>
    /// Gets the chunk at the represented location.
    /// </summary>
    /// <returns>Chunk at the represented location.</returns>
    public Chunk.Chunk getChunk()
    {
        return getWorld().getChunkAt(getBlockX() >> 4, getBlockZ() >> 4);
    }

    /// <inheritdoc/>
    public override string ToString() => $"Location(world={LocationWorld.getName()}, x={X}, y={Y}, z={Z}, yaw={Yaw}, pitch={Pitch})";
}
