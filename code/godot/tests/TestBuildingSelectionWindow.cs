using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;
using Dictionary = Godot.Collections.Dictionary;

// Real resources and production GPU path. Native instruction fixtures verify
// exact pip/line geometry; this scene checks selection, health colors and output.
public partial class TestBuildingSelectionWindow : TestRunner
{
    private async Task WaitState(GodotObject target, string method, string wanted)
    {
        var deadline = Time.GetTicksMsec() + 30000;
        while (Time.GetTicksMsec() < deadline)
        {
            using var status = target.Call(method).AsGodotDictionary();
            var state = status["state"].AsString();
            if (state == wanted) return;
            if (state == "failed") throw new Exception(status["error"].AsString());
            await NextFrame();
        }
        throw new Exception($"Timed out waiting for {wanted}");
    }
    protected override async Task RunTestAsync()
    {
        using var instance = ClassDB.Instantiate("RA2Core");
        using var core = (RefCounted)instance.AsGodotObject();
        var directory = OS.GetCmdlineUserArgs().First(s => s.StartsWith("--game-data="))[12..];
        core.Call("begin_resource_loading", directory);
        await WaitState(core, "get_resource_progress", "complete");
        var viewport = new SubViewport { Size = new Vector2I(1280, 720), RenderTargetUpdateMode = SubViewport.UpdateMode.Always };
        Root.AddChild(viewport);
        var screen = new TextureRect { Texture = viewport.GetTexture(), Size = new Vector2(1280, 720) };
        Root.AddChild(screen);
        using var mapInstance = ClassDB.Instantiate("RA2MapView");
        var map = (Node2D)mapInstance.AsGodotObject();
        viewport.AddChild(map);
        map.Call("configure", core);
        try
        {
            foreach (var filename in new[] { "ALL01UMD.MAP", "SOV02SMD.MAP", "SOV06LMD.MAP" })
            {
                map.Call("open_map", filename);
                await WaitState(map, "get_render_status", "drawn");
                // This mission starts with its base partly below the viewport.
                // Pan through ordinary right-drag input before choosing a target.
                if (filename == "SOV02SMD.MAP")
                {
                    using var press = new InputEventMouseButton { Position = new Vector2(500, 300), ButtonIndex = MouseButton.Right, Pressed = true };
                    using var move = new InputEventMouseMotion { Position = new Vector2(350, 520), ButtonMask = MouseButtonMask.Right };
                    using var release = new InputEventMouseButton { Position = new Vector2(350, 520), ButtonIndex = MouseButton.Right, Pressed = false };
                    map.Call("handle_map_input", press, (Vector2)viewport.Size);
                    map.Call("handle_map_input", move, (Vector2)viewport.Size);
                    map.Call("handle_map_input", release, (Vector2)viewport.Size);
                    await Delay(0.15);
                }
                using var status = core.Call("get_map_status").AsGodotDictionary();
                var bounds = status["map_bounds"].AsRect2I();
                long world = 0, id = 0;
                // Search visible building pixels through the ordinary device
                // input route. No test-only selection setter or object model.
                using var objects = core.Call("get_map_objects").AsGodotArray();
                foreach (var value in objects)
                {
                    using var item = value.AsGodotDictionary();
                    if (item["kind"].AsString() != "building" || !item["alive"].AsBool()) continue;
                    int x = item["world_x"].AsInt32(), y = item["world_y"].AsInt32(), z = item["world_z"].AsInt32();
                    int sx = 30 * (x - y) / 256 - status["camera_x"].AsInt32() + bounds.Position.X;
                    int sy = 15 * (x + y) / 256 - (int)(z * BitConverter.Int64BitsToDouble(0x3FC25E5374344960) + (z >= 728 ? 1 : 0) + 0.5)
                        - status["camera_y"].AsInt32() + bounds.Position.Y;
                    foreach (int dy in new[] { -20, -40, 0, 20 })
                    {
                        var point = new Vector2I(sx, sy + dy);
                        if (!bounds.Grow(-30).HasPoint(point)) continue;
                        using var motion = new InputEventMouseMotion { Position = point };
                        map.Call("handle_map_input", motion, (Vector2)viewport.Size);
                        foreach (var pressed in new[] { true, false })
                        {
                            using var click = new InputEventMouseButton { Position = point, ButtonIndex = MouseButton.Left, Pressed = pressed };
                            map.Call("handle_map_input", click, (Vector2)viewport.Size);
                        }
                        using var current = core.Call("get_map_objects").AsGodotArray();
                        foreach (var entry in current)
                        {
                            using var candidate = entry.AsGodotDictionary();
                            if (candidate["kind"].AsString() == "building" && candidate["selected"].AsBool())
                            { world = candidate["world"].AsInt64(); id = candidate["id"].AsInt64(); break; }
                        }
                        if (id != 0) break;
                    }
                    if (id != 0) break;
                }
                Expect(id != 0, $"{filename}: select a visible building through mouse input");
                if (id == 0) return;
                using var selected = core.Call("get_map_object", world, id).AsGodotDictionary();
                int maximum = selected["max_health"].AsInt32();
                byte[]? previous = null;
                foreach (var percent in new[] { 100, 50, 25, 1 })
                {
                    Expect(core.Call("set_map_object_health", world, id, Math.Max(1, maximum * percent / 100)).AsBool(), "set native health");
                    await Delay(0.15); await DrawFrame();
                    using var rendered = map.Call("get_render_status").AsGodotDictionary();
                    Expect(rendered["state"].AsString() == "drawn", "selected building renders through production backend");
                    using var image = viewport.GetTexture().GetImage();
                    var pixels = image.GetData();
                    if (previous != null) Expect(!pixels.SequenceEqual(previous), "changed health must change visible output");
                    previous = pixels;
                    if (CaptureDirectory().Length > 0)
                        Expect(image.SavePng(CaptureDirectory().PathJoin($"{filename}-{percent}.png")) == Error.Ok, "save building capture");
                    GD.Print($"BUILDING_SELECTION {filename} {selected["type_id"]} health={percent}%");
                }
                map.Call("close_map");
            }
        }
        finally
        {
            map.Call("close_map"); screen.QueueFree(); viewport.QueueFree(); await NextFrame();
        }
    }
}
