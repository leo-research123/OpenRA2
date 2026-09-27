using Godot;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Linq;

public partial class TestMapWindow : TestRunner
{
    protected override async Task RunTestAsync()
    {
        var renderOnly = HasArgument("--render-only");
        InitializeApp("map-window-test");
        await WaitForBoot();
        Expect(Application.BootReady, "Map test requires actual game resources");
        if (!Application.BootReady) return;
        var directory = CaptureDirectory();
        foreach (var map in new[] { "ALL01UMD.MAP", "SOV02SMD.MAP", "SOV06LMD.MAP", "ALL04DMD.MAP", "ALL05UMD.MAP", "SOV07TMD.MAP" })
        {
            Application.StartMap(map);
            await WaitForMap("drawn");
            if (Failures != 0) return;
            await DrawFrame();
            await DrawFrame();
            using var status = Application.GameScreen.GetMapStatus();
            Expect(status["drawn_cells"].AsInt32() > 10, "Real Cells must issue terrain draws");
            Expect(status["decoded_tiles"].AsInt32() > 1, "Actual TMP resources must populate the cache");
            Expect(status["radar_available"].AsBool() && status["radar_frame_valid"].AsBool(), "Real terrain radar and height-aware frame exist");
            using var image = Application.GetNode<SubViewport>("GameScreen/WorldViewport").GetTexture().GetImage();
            var colors = new HashSet<uint>();
            for (var y = 0; y < image.GetHeight(); y += 7)
                for (var x = 0; x < image.GetWidth(); x += 7) colors.Add(image.GetPixel(x, y).ToRgba32());
            Expect(colors.Count > 32, "GPU output must contain actual varied terrain, not a blank/flat image");
            if (!string.IsNullOrEmpty(directory))
            {
                Expect(image.SavePng(directory.PathJoin(map + ".png")) == Error.Ok, "Map screenshot must save");
                image.Convert(Image.Format.Rgba8);
                var rgba = image.GetData();
                var rgb565 = new byte[rgba.Length / 2];
                for (var i = 0; i < rgba.Length / 4; i++)
                {
                    var pixel = ((rgba[i * 4] >> 3) << 11) | ((rgba[i * 4 + 1] >> 2) << 5) | (rgba[i * 4 + 2] >> 3);
                    rgb565[i * 2] = (byte)pixel;
                    rgb565[i * 2 + 1] = (byte)(pixel >> 8);
                }
                System.IO.File.WriteAllBytes(directory.PathJoin(map + ".gpu.rgb565"), rgb565);

            }
            GD.Print("MAP_RENDER ", map, " ", status, " colors=", colors.Count);
            await Delay(0.05);
            using var unchanged = Application.GameScreen.GetMapStatus();
            Expect(unchanged["decoded_tiles"].AsInt32() == status["decoded_tiles"].AsInt32(), "An unchanged frame must reuse decoded resources");
            if (map == "ALL01UMD.MAP")
            {
                if (!renderOnly) await TestMouseNavigation();
                Expect(Application.DisplayService.ApplySelection(new Vector2I(1920, 1080), (int)DisplayConfig.Mode.Windowed) == Error.Ok, "Resize map output");
                await WaitForMap("drawn", 1920);
                if (!renderOnly) await CheckDrag(new Vector2(70, -30));
                using var resized = Application.GetNode<SubViewport>("GameScreen/WorldViewport").GetTexture().GetImage();
                Expect(resized.GetSize() == new Vector2I(1920, 1080), "GPU target follows selected resolution");
                Expect(Application.DisplayService.ApplySelection(new Vector2I(1280, 720), (int)DisplayConfig.Mode.Windowed) == Error.Ok, "Restore map output");
                await WaitForMap("drawn", 1280);
            }
            await Escape();
            Expect(GetTree().Paused, "Map remains inside the pause flow");
            Application.OnMainMenuPressed();
            await NextFrame();
            using var closed = Application.GameScreen.GetMapStatus();
            Expect(closed["state"].AsString() == "empty", "Return to menu closes the actual world");
        }
        // Closing during loading joins the worker before freeing world/resources.
        Application.StartMap("ALL01UMD.MAP");
        Application.OnMainMenuPressed();
        using (var cancelled = Application.GameScreen.GetMapStatus())
            Expect(cancelled["state"].AsString() == "empty", "Immediate cancellation leaves no partial world");
        Application.StartMap("missing-map.map");
        await WaitForMap("failed");
        Application.OnMainMenuPressed();
    }

    private Vector2 _pointer;
    private MouseButtonMask _buttons;
    private bool _mouseEntered;

    private async Task MovePointer(Vector2 position)
    {
        if (!_mouseEntered) { Root.NotifyMouseEntered(); _mouseEntered = true; }
        using var motion = new InputEventMouseMotion
        {
            Position = position, GlobalPosition = position,
            Relative = position - _pointer, ButtonMask = _buttons
        };
        Input.ParseInputEvent(motion);
        Input.FlushBufferedEvents();
        _pointer = position;
        await NextFrame();
        await NextFrame();
    }

    private async Task RightButton(bool pressed)
    {
        _buttons = pressed ? MouseButtonMask.Right : 0;
        using var click = new InputEventMouseButton
        {
            Position = _pointer, GlobalPosition = _pointer, ButtonIndex = MouseButton.Right,
            Pressed = pressed, ButtonMask = _buttons
        };
        Input.ParseInputEvent(click);
        Input.FlushBufferedEvents();
        await NextFrame();
        await NextFrame();
    }

    private Vector2I Camera()
    {
        using var status = Application.GameScreen.GetMapStatus();
        return new Vector2I(status["camera_x"].AsInt32(), status["camera_y"].AsInt32());
    }

    private void QueueDrag(Vector2 end)
    {
        using var press = new InputEventMouseButton { Position = _pointer, GlobalPosition = _pointer,
            ButtonIndex = MouseButton.Right, Pressed = true, ButtonMask = MouseButtonMask.Right };
        using var motion = new InputEventMouseMotion { Position = end, GlobalPosition = end,
            Relative = end - _pointer, ButtonMask = MouseButtonMask.Right };
        using var release = new InputEventMouseButton { Position = end, GlobalPosition = end,
            ButtonIndex = MouseButton.Right, Pressed = false, ButtonMask = 0 };
        Input.ParseInputEvent(press);
        Input.ParseInputEvent(motion);
        Input.ParseInputEvent(release);
        Input.FlushBufferedEvents();
        _pointer = end;
    }

    private async Task CheckDrag(Vector2 offset, bool batched = false)
    {
        var surface = Application.GetNode<Control>("GameScreen/WorldSurface");
        var start = surface.GetGlobalTransformWithCanvas() * (surface.Size / 2);
        await MovePointer(start);
        var camera = Camera();
        await DrawFrame();
        var viewport = Application.GetNode<SubViewport>("GameScreen/WorldViewport");
        using var originalImage = viewport.GetTexture().GetImage();
        if (batched) { QueueDrag(start + offset); await NextFrame(); await NextFrame(); }
        else { await RightButton(true); await MovePointer(start + offset); await RightButton(false); }
        await WaitForMap("drawn");
        var moved = Camera();
        Expect(moved == camera + (Vector2I)offset, $"Actual GUI mouse drag must move Tactical by output pixels: {camera} -> {moved}, wanted {offset}");
        await DrawFrame();
        await DrawFrame();
        using var movedImage = viewport.GetTexture().GetImage();
        var differences = 0;
        for (var y = 0; y < movedImage.GetHeight()-32; y += 11)
            for (var x = 0; x < movedImage.GetWidth()-168; x += 11)
            {
                var source = new Vector2I(x, y) + moved - camera;
                if (source.X >= 0 && source.Y >= 0 && source.X < originalImage.GetWidth()-168 && source.Y < originalImage.GetHeight()-32
                    && movedImage.GetPixel(x, y) != originalImage.GetPixel(source.X, source.Y)) differences++;
            }
        Expect(differences == 0, $"Moved GPU terrain must align with its original pixels, mismatches={differences}");
        var capture = CaptureDirectory();
        if (capture.Length > 0) Expect(movedImage.SavePng(capture.PathJoin($"camera-pan-{movedImage.GetWidth()}{(batched ? "-batched" : "")}.png")) == Error.Ok, "Save actual panned terrain");
        if (batched) { QueueDrag(start); await NextFrame(); await NextFrame(); }
        else { await RightButton(true); await MovePointer(start); await RightButton(false); }
        await WaitForMap("drawn");
        Expect(Camera() == camera, "Reverse mouse drag returns to identical Tactical position");
    }

    private async Task TestMouseNavigation()
    {
        var viewport = Application.GetNode<SubViewport>("GameScreen/WorldViewport");
        using var before = viewport.GetTexture().GetImage();
        await CheckDrag(new Vector2(80, -40));
        await CheckDrag(new Vector2(35, -20), true);
        await DrawFrame();
        await DrawFrame();
        using var returned = viewport.GetTexture().GetImage();
        Expect(before.GetData().SequenceEqual(returned.GetData()), "Pan round trip must restore actual GPU pixels");
        var surface = Application.GetNode<Control>("GameScreen/WorldSurface");
        var edge = surface.GetGlobalTransformWithCanvas() * new Vector2(0, surface.Size.Y / 2);
        var original = Camera();
        await MovePointer(edge);
        await Delay(0.1);
        Expect(Camera().X < original.X, "Left map edge scrolls the actual camera");
        await Escape();
        var paused = Camera();
        await Delay(0.1);
        Expect(Camera() == paused, "Paused map cannot keep scrolling");
        await Escape();
        await Delay(0.1);
        Expect(Camera() == paused, "Resuming cannot reuse a stale edge/drag input");
        await MovePointer(edge);
        Root.PropagateNotification((int)NotificationWMWindowFocusOut);
        var unfocused = Camera();
        await MovePointer(edge + Vector2.Down);
        await Delay(0.1);
        Expect(Camera() == unfocused, "Focus loss clears input and ignores subsequent unfocused motion");
        Root.PropagateNotification((int)NotificationWMWindowFocusIn);
        await MovePointer(surface.Size / 2);
        GD.Print("MAP_MOUSE_INPUT camera=", Camera(), " verified drag, edge, pause, focus and pixel round trip");
    }

    private async Task WaitForMap(string expected, int width = 0)
    {
        var deadline = Time.GetTicksMsec() + 30_000;
        while (Time.GetTicksMsec() < deadline)
        {
            using var status = Application.GameScreen.GetMapStatus();
            var state = status.TryGetValue("state", out var value) ? value.AsString() : "empty";
            if (state == expected && (width == 0 || status["viewport_width"].AsInt32() == width)) return;
            if (state == "failed")
            {
                Expect(false, $"Map failed: {status["error"].AsString()}");
                return;
            }
            await Delay(0.01);
        }
        Expect(false, $"Map did not reach {expected}");
    }
}
