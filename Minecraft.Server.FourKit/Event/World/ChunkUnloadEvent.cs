namespace Minecraft.Server.FourKit.Event.World;

using Minecraft.Server.FourKit.Chunk;

/// <summary>
/// Called when a chunk is unloaded.
/// </summary>
public class ChunkUnloadEvent : ChunkEvent, Cancellable
{
    private bool _cancel;

    internal ChunkUnloadEvent(Chunk chunk) : base(chunk)
    {
        _cancel = false;
    }

    /// <summary>
    /// Gets the cancellation state of this event. A cancelled event will not
    /// be executed in the server, but will still pass to other plugins.
    /// </summary>
    /// <returns>true if this event is cancelled.</returns>
    public bool isCancelled() => _cancel;

    /// <summary>
    /// Sets the cancellation state of this event. A cancelled event will not
    /// be executed in the server, but will still pass to other plugins.
    /// </summary>
    /// <param name="cancel">true if you wish to cancel this event.</param>
    public void setCancelled(bool cancel)
    {
        _cancel = cancel;
    }
}
