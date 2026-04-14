namespace Minecraft.Server.FourKit.Event.World;

using Minecraft.Server.FourKit.Chunk;

/// <summary>
/// Represents a Chunk related event.
/// </summary>
public abstract class ChunkEvent : WorldEvent
{
    protected Chunk chunk;

    protected ChunkEvent(Chunk chunk) : base(chunk.getWorld())
    {
        this.chunk = chunk;
    }

    /// <summary>
    /// Gets the chunk being loaded/unloaded.
    /// </summary>
    /// <returns>Chunk that triggered this event.</returns>
    public Chunk getChunk() => chunk;
}
