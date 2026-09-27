using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;

// Exercise production packet generation as new scenery enters the viewport.
// Loading a map or rendering only its starting camera misses these failures.
public partial class TestMapPanWindow : TestRunner
{
    private async Task WaitState(GodotObject target, string method, string wanted)
    {
        var deadline = Time.GetTicksMsec() + 60000;
        while (Time.GetTicksMsec() < deadline)
        {
            using var status = target.Call(method).AsGodotDictionary();
            if (status["state"].AsString() == "failed") throw new Exception(status.ToString());
            if (status["state"].AsString() == wanted) return;
            Root.GrabFocus();
            await NextFrame();
        }
        throw new Exception($"Timed out waiting for {wanted}");
    }

    protected override async Task RunTestAsync()
    {
        var args = OS.GetCmdlineUserArgs();
        var directory = args.First(s => s.StartsWith("--game-data="))[12..];
        var filename = args.FirstOrDefault(s => s.StartsWith("--map="))?[6..] ?? "ALL02UMD.MAP";
        Root.Size = new Vector2I(1024, 768);
        using var instance = ClassDB.Instantiate("RA2Core");
        using var core = (RefCounted)instance.AsGodotObject();
        core.Call("begin_resource_loading", directory);
        await WaitState(core, "get_resource_progress", "complete");
        var viewport = new SubViewport { Size = Root.Size, RenderTargetUpdateMode = SubViewport.UpdateMode.Always };
        Root.AddChild(viewport);
        Root.AddChild(new TextureRect { Texture = viewport.GetTexture(), Size = Root.Size });
        using var mapInstance = ClassDB.Instantiate("RA2MapView");
        var map = (Node2D)mapInstance.AsGodotObject();
        viewport.AddChild(map);
        map.Call("configure", core);
        async Task Pan(int x, int y)
        {
            Root.GrabFocus();
            var start = new Vector2(400, 300);
            using var press = new InputEventMouseButton { Position = start, ButtonIndex = MouseButton.Right, Pressed = true };
            using var move = new InputEventMouseMotion { Position = start + new Vector2(x, y), ButtonMask = MouseButtonMask.Right };
            using var release = new InputEventMouseButton { Position = move.Position, ButtonIndex = MouseButton.Right, Pressed = false };
            map.Call("handle_map_input", press, (Vector2)viewport.Size);
            map.Call("handle_map_input", move, (Vector2)viewport.Size);
            map.Call("handle_map_input", release, (Vector2)viewport.Size);
            map.Call("reset_map_input");
            await NextFrame(); await NextFrame();
            await WaitState(map, "get_render_status", "drawn");
            using var hover = new InputEventMouseMotion { Position = start };
            map.Call("handle_map_input", hover, (Vector2)viewport.Size);
            await NextFrame(); await NextFrame();
            await WaitState(map, "get_render_status", "drawn");
        }
        try
        {
            map.Call("open_map", filename);
            await WaitState(map, "get_render_status", "drawn");
            // First reproduce left/right scrolling from the mission's start.
            foreach (int direction in new[] { -1, 1, -1 })
            {
                for (int step = 0; step < 24; ++step) await Pan(direction * 400, 0);
                GD.Print("PAN horizontal: ", core.Call("get_map_status"));
            }
            // Then cover the rest of the map with overlapping camera windows.
            for (int step = 0; step < 24; ++step) await Pan(0, -300);
            for (int row = 0; row < 24; ++row)
            {
                for (int column = 0; column < 24; ++column) await Pan((row % 2 == 0 ? 1 : -1) * 400, 0);
                await Pan(0, 300);
                GD.Print("PAN row ", row, ": ", core.Call("get_map_status"));
            }
            if (!string.IsNullOrEmpty(CaptureDirectory()))
            {
                DirAccess.MakeDirRecursiveAbsolute(CaptureDirectory());
                using var image = viewport.GetTexture().GetImage();
                Expect(image.SavePng(CaptureDirectory().PathJoin("map-pan.png")) == Error.Ok, "Save map pan capture");
            }
        }
        finally { map.Call("close_map"); map.QueueFree(); viewport.QueueFree(); await NextFrame(); }
    }
}
