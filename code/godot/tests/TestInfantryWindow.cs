using Godot;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;

/// <summary>Real GI resources through the native test1 scene and GPU readback.</summary>
public partial class TestInfantryWindow : TestRunner
{
    private static Godot.Collections.Dictionary State(Node soldier) =>
        soldier.Call("get_display_state").AsGodotDictionary();
    private static Godot.Collections.Dictionary Instance(Node soldier, int index) =>
        soldier.Call("get_instance_state", index).AsGodotDictionary();

    private readonly record struct WalkerState(Vector2 Position, Vector2 Velocity, int Phase, int Frame);

    private WalkerState[] ReadCrowd(Node soldier)
    {
        var display = State(soldier);
        int count = display["count"].AsInt32();
        var result = new WalkerState[count];
        var bounds = new Rect2(Vector2.Zero, display["viewport_size"].AsVector2()).Grow(0.001f);
        var directions = new HashSet<int>();
        var phases = new HashSet<int>();
        int outside = 0, scaled = 0, wrongFacing = 0;
        for (int i = 0; i < count; ++i)
        {
            using var state = Instance(soldier, i);
            var velocity = state["velocity"].AsVector2();
            var direction = state["direction"].AsInt32();
            var phase = state["phase"].AsInt32();
            result[i] = new(state["position"].AsVector2(), velocity, phase, state["frame"].AsInt32());
            directions.Add(direction); phases.Add(phase);
            if (!bounds.Encloses(state["draw_rect"].AsRect2())) ++outside;
            if (state["draw_rect"].AsRect2().Size != state["frame_size"].AsVector2()) ++scaled;
            int expected = ((int)Math.Round(Math.Atan2(-velocity.X, -velocity.Y) * 4 / Math.PI,
                MidpointRounding.AwayFromZero) + 8) % 8;
            if (expected != direction || result[i].Frame != 8 + direction * 6 + phase) ++wrongFacing;
        }
        Expect(outside == 0 && scaled == 0, "Every GI remains in the viewport at its original SHP frame size");
        Expect(wrongFacing == 0, "Every GI's frame follows its velocity and animation phase");
        Expect(directions.Count == 8 && phases.Count == 6, "Random crowd includes all facings and independent animation phases");
        return result;
    }

    private async Task<Image> ReadPixels(string capture = "")
    {
        await DrawFrame();
        var image = Root.GetTexture().GetImage();
        if (capture.Length > 0 && CaptureDirectory().Length > 0)
            Expect(image.SavePng(CaptureDirectory().PathJoin(capture)) == Error.Ok, "Save infantry capture");
        return image;
    }

    private int CheckSprite(Image image, Godot.Collections.Dictionary state)
    {
        var expected = state["draw_rect"].AsRect2().Grow(1);
        var background = image.GetPixel(0, 0);
        int count = 0, outside = 0;
        var colors = new HashSet<Color>();
        for (int y = 0; y < image.GetHeight(); ++y)
        for (int x = 0; x < image.GetWidth(); ++x)
        {
            var color = image.GetPixel(x, y);
            if (color.IsEqualApprox(background)) continue;
            ++count; colors.Add(color);
            if (!expected.HasPoint(new Vector2(x + 0.5f, y + 0.5f))) ++outside;
        }
        Expect(count > 55 && count < 900,
            $"One visible infantry sprite: pixels={count}, image={image.GetSize()}, rect={expected}");
        Expect(outside == 0, "Pixels follow SHP frame offset and submitted position; no old drawing remains");
        Expect(count < expected.Size.X * expected.Size.Y * 0.8f, "Index zero remains transparent");
        Expect(colors.Count > 15, "Indexed atlas uses the unit palette");
        Expect(state["draw_rect"].AsRect2().Size == state["frame_size"].AsVector2(), "1x uses the original cropped frame size");
        return count;
    }

    private async Task KeyPress(Key key)
    {
        using var press = new InputEventKey { Keycode = key, Pressed = true };
        Input.ParseInputEvent(press);
        await NextFrame();
        using var release = new InputEventKey { Keycode = key, Pressed = false };
        Input.ParseInputEvent(release);
        await NextFrame();
    }

    protected override async Task RunTestAsync()
    {
        if (DisplayServer.GetName() == "headless")
            throw new InvalidOperationException("TestInfantryWindow requires a real rendering window.");
        Root.ContentScaleMode = Window.ContentScaleModeEnum.Disabled;
        Root.Size = new Vector2I(800, 600);
        await DrawFrame();
        using var scene = GD.Load<PackedScene>("res://scenes/test1.tscn");
        if (HasArgument("--expect-resource-error"))
        {
            var missing = scene.Instantiate<Node2D>();
            Root.AddChild(missing);
            Expect(!State(missing)["ready"].AsBool() && State(missing)["error"].AsString().Length > 0,
                "Missing resources report an error without starting playback");
            using var failure = await ReadPixels("test1-missing-resources.png");
            missing.QueueFree();
            await NextFrame();
            return;
        }
        for (int run = 0; run < 2; ++run)
        {
            var originalTitle = Root.Title;
            var originalMode = Root.ContentScaleMode;
            var originalScale = Root.ContentScaleSize;
            var originalFps = Engine.MaxFps;
            var originalVsync = DisplayServer.WindowGetVsyncMode(Root.GetWindowId());
            var soldier = scene.Instantiate<Node2D>();
            soldier.Set("instance_count", 1);
            soldier.Set("show_info", false);
            soldier.Set("resize_window", false);
            soldier.Set("unlimited_fps", false);
            Root.AddChild(soldier);
            soldier.SetProcess(false);
            var start = State(soldier);
            if (!start["ready"].AsBool()) throw new InvalidOperationException(start["error"].AsString());
            Expect(start["scale"].AsSingle() == 1 && start["submitted"].AsInt32() == 1,
                "Isolated pixel test uses one unscaled GI");
            Expect(start["walk_start"].AsInt32() == 8 && start["walk_count"].AsInt32() == 6 &&
                start["walk_stride"].AsInt32() == 6 && start["atlas_frames"].AsInt32() == 48,
                "GI Walk sequence comes from art INI: eight directions, six phases each");
            using var first = await ReadPixels(run == 0 ? "test1-start.png" : "");
            int startCount = CheckSprite(first, start);
            // GI's red uniform is above its feet. This also catches vertically
            // inverted QuadMesh UVs, which a symmetric ball cannot reveal.
            int redCount = 0;
            double redY = 0;
            for (int y = 0; y < first.GetHeight(); ++y)
            for (int x = 0; x < first.GetWidth(); ++x)
            {
                var color = first.GetPixel(x, y);
                if (color.R > 0.2f && color.R > color.G * 2 && color.R > color.B * 2)
                { ++redCount; redY += y + 0.5; }
            }
            Expect(redCount > 3 && redY / redCount < start["draw_rect"].AsRect2().GetCenter().Y,
                "GI's uniform renders above its feet, with the SHP row order preserved");
            var phases = new HashSet<int>();
            var counts = new HashSet<int> { startCount };
            for (int phase = 1; phase <= 6; ++phase)
            {
                soldier.Call("advance", 0.125);
                var state = State(soldier);
                phases.Add(state["frame"].AsInt32());
                Expect(state["phase"].AsInt32() == phase % 6, "Eight animation ticks per second");
                using var pixels = await ReadPixels(run == 0 && phase == 3 ? "test1-walk.png" : "");
                counts.Add(CheckSprite(pixels, state));
            }
            var moved = State(soldier);
            Expect(phases.Count == 6 && counts.Count > 1, "Six walk frames change the rendered silhouette");
            Expect(moved["position"].AsVector2().DistanceTo(start["position"].AsVector2()) > 40,
                "Walking animation accompanies independent movement");
            await KeyPress(Key.Space);
            soldier.Call("advance", 0.25);
            var paused = State(soldier);
            Expect(paused["paused"].AsBool() && paused["position"].AsVector2() == moved["position"].AsVector2() &&
                paused["frame"].AsInt32() == moved["frame"].AsInt32(), "Space freezes position and animation");
            await KeyPress(Key.R);
            Expect(!State(soldier)["paused"].AsBool() &&
                State(soldier)["position"].AsVector2() == start["position"].AsVector2(), "R resets playback");
            using var reset = await ReadPixels();
            Expect(first.GetData().AsSpan().SequenceEqual(reset.GetData()), "Reset returns to identical pixels");
            soldier.Call("advance", double.NaN);
            soldier.Call("advance", -1.0);
            Expect(State(soldier)["position"].AsVector2() == start["position"].AsVector2(), "Invalid deltas are ignored");
            var envelope = start["frame_envelope"].AsRect2();
            var velocity = start["velocity"].AsVector2();
            double right = Root.Size.X - envelope.End.X;
            double hitWall = (right - start["position"].AsVector2().X) / velocity.X;
            soldier.Call("advance", hitWall + 0.02);
            var reflected = State(soldier);
            Expect(reflected["velocity"].AsVector2().X < 0 &&
                Math.Abs(reflected["position"].AsVector2().X - (right - velocity.X * 0.02)) < 0.01,
                "Right-edge reflection preserves overshoot, like test-ball");
            using var bounced = await ReadPixels(run == 0 ? "test1-bounce.png" : "");
            CheckSprite(bounced, reflected);
            soldier.Call("advance", 1_000_000.0);
            using var longStep = await ReadPixels();
            CheckSprite(longStep, State(soldier));
            Root.Size = new Vector2I(420, 340);
            await DrawFrame();
            soldier.Call("advance", 0);
            soldier.QueueRedraw();
            using var resized = await ReadPixels(run == 0 ? "test1-resized.png" : "");
            CheckSprite(resized, State(soldier));
            Expect(new Rect2(Vector2.Zero, Root.Size).Encloses(State(soldier)["draw_rect"].AsRect2()),
                "Resize keeps the sprite inside the viewport");
            soldier.QueueFree();
            await NextFrame();
            using var cleared = await ReadPixels();
            var bg = cleared.GetPixel(0, 0);
            int remaining = 0;
            for (int y = 0; y < cleared.GetHeight(); ++y)
            for (int x = 0; x < cleared.GetWidth(); ++x)
                if (!cleared.GetPixel(x, y).IsEqualApprox(bg)) ++remaining;
            Expect(remaining == 0, "Exit releases the native CanvasItem and textures");
            Expect(Root.Title == originalTitle && Root.ContentScaleMode == originalMode &&
                Root.ContentScaleSize == originalScale && Engine.MaxFps == originalFps &&
                DisplayServer.WindowGetVsyncMode(Root.GetWindowId()) == originalVsync,
                "Exit restores window scaling and frame scheduling");
            Root.Size = new Vector2I(800, 600);
            await DrawFrame();
        }
        Root.Size = new Vector2I(1920, 1080);
        await DrawFrame();
        var crowd = scene.Instantiate<Node2D>(); // exercise the scene's real 10,000 default
        crowd.Set("show_info", false);
        crowd.Set("resize_window", false);
        crowd.Set("unlimited_fps", false);
        Root.AddChild(crowd);
        crowd.SetProcess(false);
        Expect(State(crowd)["count"].AsInt32() == 10_000 && State(crowd)["submitted"].AsInt32() == 10_000 &&
            State(crowd)["scale"].AsSingle() == 1 && State(crowd)["atlas_frames"].AsInt32() == 48,
            "Default test1 submits all ten thousand GI at 1x through a shared 48-frame atlas");
        var initial = ReadCrowd(crowd);
        var positions = new HashSet<Vector2>();
        var velocities = new HashSet<Vector2>();
        foreach (var walker in initial) { positions.Add(walker.Position); velocities.Add(walker.Velocity); }
        Expect(positions.Count > 9900 && velocities.Count > 9900, "Independent random starting positions and velocities");
        using var before = await ReadPixels("test1-10000-start.png");
        int coverage = 0;
        var background = before.GetPixel(0, 0);
        for (int y = 0; y < before.GetHeight(); ++y)
        for (int x = 0; x < before.GetWidth(); ++x)
            if (!before.GetPixel(x, y).IsEqualApprox(background)) ++coverage;
        Expect(coverage > 500_000, "Actual pixels contain the full crowd, not just a reported count");
        crowd.Call("advance", 0.25);
        var movedCrowd = ReadCrowd(crowd);
        int moving = 0, animating = 0;
        for (int i = 0; i < initial.Length; ++i)
        {
            if (movedCrowd[i].Position.DistanceTo(initial[i].Position) > 0.001f) ++moving;
            if (movedCrowd[i].Phase == (initial[i].Phase + 2) % 6) ++animating;
        }
        Expect(moving == 10_000 && animating == 10_000, "All ten thousand instances move and advance their own walking animation");
        using var after = await ReadPixels("test1-10000-moving.png");
        int changed = 0;
        for (int y = 0; y < before.GetHeight(); y += 4)
        for (int x = 0; x < before.GetWidth(); x += 4)
            if (!before.GetPixel(x, y).IsEqualApprox(after.GetPixel(x, y))) ++changed;
        Expect(changed > 25_000, "Movement and animation change pixels across the GPU-rendered crowd");
        await KeyPress(Key.Space);
        crowd.Call("advance", 0.25);
        using var pausedCrowd = await ReadPixels();
        Expect(after.GetData().AsSpan().SequenceEqual(pausedCrowd.GetData()), "Pause freezes the entire crowd");
        await KeyPress(Key.R);
        var repeated = ReadCrowd(crowd);
        Expect(initial.AsSpan().SequenceEqual(repeated), "Fixed seed resets every instance reproducibly");
        Root.Size = new Vector2I(640, 480);
        await DrawFrame();
        crowd.Call("advance", 0.1);
        ReadCrowd(crowd);
        Expect(State(crowd)["submitted"].AsInt32() == 10_000, "Resize retains all ten thousand instances");
        crowd.QueueFree();
        await NextFrame();

        // Normal automatic playback, with the same window and FPS HUD as the user sees.
        var overview = scene.Instantiate<Node2D>();
        Root.AddChild(overview);
        var liveStart = State(overview);
        await Delay(1.2);
        overview.SetProcess(false);
        var liveEnd = State(overview);
        Expect(liveEnd["submission_count"].AsInt64() > liveStart["submission_count"].AsInt64() + 2 &&
            liveEnd["position"].AsVector2() != liveStart["position"].AsVector2(), "Godot _process drives live crowd movement");
        GD.Print($"test1 live sample: {liveEnd["submitted"]} GI, {liveEnd["fps"]} FPS, " +
            $"update {liveEnd["update_ms"]} ms, submit {liveEnd["submit_ms"]} ms (short Debug sample)");
        using var screenshot = await ReadPixels("test1-10000-overview.png");
        overview.QueueFree();
        await NextFrame();
    }
}
