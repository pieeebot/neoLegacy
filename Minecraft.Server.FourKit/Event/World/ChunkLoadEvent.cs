namespace Minecraft.Server.FourKit.Event.World;

using Minecraft.Server.FourKit.Chunk;

/// <summary>
/// Called when a chunk is loaded.
/// </summary>
public class ChunkLoadEvent : ChunkEvent
{
    private readonly bool _newChunk;

    internal ChunkLoadEvent(Chunk chunk, bool newChunk) : base(chunk)
    {
        _newChunk = newChunk;
    }

    /// <summary>
    /// Gets if this chunk was newly created or not. Note that if this chunk is
    /// new, it will not be populated at this time.
    /// </summary>
    /// <returns>true if the chunk is new, otherwise false.</returns>
    public bool isNewChunk() => _newChunk;
}
