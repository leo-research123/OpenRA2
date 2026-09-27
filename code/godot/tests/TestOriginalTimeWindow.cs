using Godot;
using System;
using System.Threading.Tasks;
using System.Globalization;

// Integration of the separately instruction-verified core scheduler with
// real map resources and Metal presentation. This is not an EXE scene replay.
public partial class TestOriginalTimeWindow : TestRunner
{
    private int Frame()
    {
        using var status = Application.GameScreen.GetMapStatus();
        return status["current_frame"].AsInt32();
    }

    private async Task WallDelay(ulong milliseconds)
    {
        var end = Time.GetTicksMsec() + milliseconds;
        while (Time.GetTicksMsec() < end) await NextFrame();
    }

    protected override async Task RunTestAsync()
    {
        // Opt-in hardware acceptance gate; ordinary correctness runs do not
        // require a particular machine speed. GameSpeed remains the original 2.
        double minimumFps = 0;
        foreach (var argument in OS.GetCmdlineUserArgs())
            if (argument.StartsWith("--min-logic-fps="))
                minimumFps = double.Parse(argument["--min-logic-fps=".Length..], CultureInfo.InvariantCulture);
        InitializeApp("original-time-test");
        await WaitForBoot();
        Expect(Application.BootReady, "Timing integration requires real game resources");
        if (!Application.BootReady) return;
        foreach (var map in new[] { "ALL01UMD.MAP", "SOV02SMD.MAP", "SOV06LMD.MAP" })
        {
            Application.StartMap(map);
            var deadline = Time.GetTicksMsec() + 30000;
            bool ready = false;
            while (Time.GetTicksMsec() < deadline)
            {
                using var status = Application.GameScreen.GetMapStatus();
                if (status["state"].AsString() == "drawn") { ready = true; break; }
                if (status["state"].AsString() == "failed")
                {
                    Expect(false, status["error"].AsString()); return;
                }
                await NextFrame();
            }
            Expect(ready, "Real map must render through the core main loop");
            if (!ready) return;
            foreach (var scale in new[] { 1.0, 4.0 })
            {
                Engine.TimeScale = scale;
                var before = Frame(); var start = Time.GetTicksMsec();
                var hostBefore = Engine.GetProcessFrames();
                await WallDelay(1000);
                var elapsed = Time.GetTicksMsec() - start; var frames = Frame() - before;
                var logicFps = frames * 1000.0 / elapsed;
                var hostFps = (Engine.GetProcessFrames() - hostBefore) * 1000.0 / elapsed;
                Expect(logicFps >= minimumFps, $"Logic throughput {logicFps:F1} must reach the requested {minimumFps:F1} fps");
                Expect(frames > 0 && frames <= (int)(elapsed / 32) + 2,
                    "Speed 2 must use device milliseconds, independently of Engine.TimeScale");
                using var status = Application.GameScreen.GetMapStatus();
                Expect(status["logic_iterations"].AsInt64() == status["current_frame"].AsInt64(),
                    "One original frame increment per logic iteration");
                GD.Print($"TIME_MAP {map} scale={scale} elapsed_ms={elapsed} frames={frames} logic_fps={logicFps:F1} host_fps={hostFps:F1} resources={status["resource_revision"]}");
            }
            Engine.TimeScale = 1.0;
            await Escape(); var paused = Frame(); await WallDelay(200);
            Expect(Frame() == paused, "Pause stops original frame advancement");
            await Escape(); await WallDelay(100);
            Root.PropagateNotification((int)NotificationWMWindowFocusOut);
            var unfocused = Frame(); await WallDelay(200);
            Expect(Frame() == unfocused, "Single player focus loss stops original logic");
            Root.PropagateNotification((int)NotificationWMWindowFocusIn);
            await NextFrame();
            Expect(Frame() <= unfocused + 1, "Focus recovery has no catch-up burst");
            Application.OnMainMenuPressed(); await NextFrame();
        }
        Engine.TimeScale = 1.0;
    }
}
