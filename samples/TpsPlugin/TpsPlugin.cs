using Minecraft.Server.FourKit;
using Minecraft.Server.FourKit.Command;
using Minecraft.Server.FourKit.Plugin;

namespace TpsPlugin;

/// <summary>
/// Minimal plugin exposing /tps. Samples the server tick counter once per
/// second and reports tick-rate averages over 1s/5s/30s/60s windows.
/// </summary>
public class TpsPlugin : ServerPlugin
{
    public override string name => "TpsPlugin";
    public override string version => "1.0.0";
    public override string author => "LCE-Revelations";

    public override void onEnable()
    {
        TpsProbe.Start();

        var cmd = FourKit.getCommand("tps");
        cmd.setDescription("Show server TPS over 1s/5s/30s/60s windows.");
        cmd.setUsage("/tps");
        cmd.setExecutor(new TpsExecutor());
    }

    public override void onDisable()
    {
        TpsProbe.Stop();
    }
}

internal static class TpsProbe
{
    private static readonly object _lock = new();
    private static readonly LinkedList<(double elapsed, int tick)> _samples = new();
    private static System.Threading.Timer? _timer;
    private static System.Diagnostics.Stopwatch? _sw;

    public static void Start()
    {
        _sw = System.Diagnostics.Stopwatch.StartNew();
        Sample();
        _timer = new System.Threading.Timer(_ => Sample(), null, 1000, 1000);
    }

    public static void Stop()
    {
        _timer?.Dispose();
        _timer = null;
        _sw?.Stop();
        _sw = null;
        lock (_lock) _samples.Clear();
    }

    private static void Sample()
    {
        var sw = _sw;
        if (sw == null) return;
        try
        {
            int tick = FourKit.getServerTick();
            double elapsed = sw.Elapsed.TotalSeconds;
            lock (_lock)
            {
                _samples.AddLast((elapsed, tick));
                // 65s window so the 60s average has full data.
                while (_samples.Count > 66) _samples.RemoveFirst();
            }
        }
        catch
        {
            // Probe is best-effort; never let sampling errors take down the host.
        }
    }

    public static (int samples, double tps1, double tps5, double tps30, double tps60) Read()
    {
        (double elapsed, int tick)[] arr;
        lock (_lock)
        {
            if (_samples.Count < 2) return (_samples.Count, 0, 0, 0, 0);
            arr = _samples.ToArray();
        }

        var last = arr[^1];

        double Window(double seconds)
        {
            for (int j = arr.Length - 2; j >= 0; j--)
            {
                if (last.elapsed - arr[j].elapsed >= seconds)
                {
                    var first = arr[j];
                    double dt = last.elapsed - first.elapsed;
                    return dt > 0 ? (last.tick - first.tick) / dt : 0;
                }
            }
            var oldest = arr[0];
            double dt0 = last.elapsed - oldest.elapsed;
            return dt0 > 0 ? (last.tick - oldest.tick) / dt0 : 0;
        }

        return (arr.Length, Window(1), Window(5), Window(30), Window(60));
    }
}

internal sealed class TpsExecutor : CommandExecutor
{
    public bool onCommand(CommandSender sender, Command command, string label, string[] args)
    {
        var (samples, t1, t5, t30, t60) = TpsProbe.Read();
        if (samples < 2)
        {
            sender.sendMessage($"TPS probe warming up ({samples}/2 samples). Try again in a few seconds.");
            return true;
        }
        int tick = FourKit.getServerTick();
        sender.sendMessage($"TPS  1s={t1:F2}  5s={t5:F2}  30s={t30:F2}  60s={t60:F2}");
        sender.sendMessage($"server tick={tick}  samples={samples}");
        return true;
    }
}
