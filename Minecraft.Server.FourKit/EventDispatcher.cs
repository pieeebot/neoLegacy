using System.Reflection;
using Minecraft.Server.FourKit.Event;

namespace Minecraft.Server.FourKit;
internal sealed class EventDispatcher
{
    private readonly struct RegisteredHandler : IComparable<RegisteredHandler>
    {
        public readonly Listener Instance;
        public readonly MethodInfo Method;
        public readonly EventPriority Priority;
        public readonly bool IgnoreCancelled;

        public RegisteredHandler(Listener instance, MethodInfo method, EventPriority priority, bool ignoreCancelled)
        {
            Instance = instance;
            Method = method;
            Priority = priority;
            IgnoreCancelled = ignoreCancelled;
        }

        public int CompareTo(RegisteredHandler other) => Priority.CompareTo(other.Priority);
    }

    // Snapshot-on-write: writers swap _handlers atomically; Fire reads it lock-free.
    private volatile Dictionary<Type, RegisteredHandler[]> _handlers = new();
    private readonly object _writeLock = new();

    // Fired when an event type gains its first handler.
    internal Action<Type>? OnSubscriptionChanged;

    public void Register(Listener listener)
    {
        var methods = listener.GetType().GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);

        List<(Type eventType, RegisteredHandler handler)>? pending = null;
        foreach (var method in methods)
        {
            var attr = method.GetCustomAttribute<Event.EventHandlerAttribute>();
            if (attr == null)
                continue;

            var parameters = method.GetParameters();
            if (parameters.Length != 1)
            {
                Console.WriteLine($"[FourKit] Warning: @EventHandler method {method.Name} must have exactly 1 parameter, skipping.");
                continue;
            }

            var eventType = parameters[0].ParameterType;
            if (!typeof(Event.Event).IsAssignableFrom(eventType))
            {
                Console.WriteLine($"[FourKit] Warning: @EventHandler method {method.Name} parameter must extend Event, skipping.");
                continue;
            }

            pending ??= new List<(Type, RegisteredHandler)>();
            pending.Add((eventType, new RegisteredHandler(listener, method, attr.Priority, attr.IgnoreCancelled)));
        }

        if (pending == null) return;

        HashSet<Type> newlySubscribed = new();
        lock (_writeLock)
        {
            var newDict = new Dictionary<Type, RegisteredHandler[]>(_handlers);
            foreach (var (eventType, handler) in pending)
            {
                bool hadAny = newDict.TryGetValue(eventType, out var existing);
                existing ??= Array.Empty<RegisteredHandler>();

                // OrderBy is stable; Array.Sort is not.
                var combined = existing.Append(handler).OrderBy(h => h.Priority).ToArray();
                newDict[eventType] = combined;

                if (!hadAny) newlySubscribed.Add(eventType);
            }
            _handlers = newDict;
        }

        if (OnSubscriptionChanged != null)
        {
            foreach (var t in newlySubscribed)
                OnSubscriptionChanged(t);
        }
    }

    public void Fire(Event.Event evt)
    {
        var snapshot = _handlers;
        if (!snapshot.TryGetValue(evt.GetType(), out var handlers))
            return;

        var cancellable = evt as Cancellable;

        for (int i = 0; i < handlers.Length; i++)
        {
            ref readonly var handler = ref handlers[i];
            if (handler.IgnoreCancelled && cancellable != null && cancellable.isCancelled())
                continue;

            try
            {
                handler.Method.Invoke(handler.Instance, [evt]);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[FourKit] Error in handler {handler.Instance.GetType().Name}.{handler.Method.Name}: {ex.InnerException?.Message ?? ex.Message}");
            }
        }
    }

    internal bool IsSubscribed(Type eventType) => _handlers.ContainsKey(eventType);
}
