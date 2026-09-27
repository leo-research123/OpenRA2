using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;

// Capture real first-map firing through selection/force-attack and the GPU renderer.
public partial class TestGrandCannonWindow : TestRunner
{
    private async Task WaitState(GodotObject target, string method, string wanted)
    {
        ulong deadline = Time.GetTicksMsec() + 60000;
        while (Time.GetTicksMsec() < deadline) {
            Root.GrabFocus();
            using var state = target.Call(method).AsGodotDictionary();
            if (state["state"].AsString() == wanted) return;
            if (state["state"].AsString() == "failed") throw new Exception(state["error"].AsString());
            await NextFrame();
        }
        throw new Exception("Timed out: " + wanted);
    }

    protected override async Task RunTestAsync()
    {
        using var instance = ClassDB.Instantiate("RA2Core");
        using var core = (RefCounted)instance.AsGodotObject();
        string directory = OS.GetCmdlineUserArgs().First(s => s.StartsWith("--game-data="))[12..];
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
        try {
            map.Call("open_map", "ALL01UMD.MAP");
            await WaitState(map, "get_render_status", "drawn");
            Expect(core.Call("grant_map_player_power").AsBool(), "Power first-map defenses");
            long world = 0, id = 0;
            int wx = 0, wy = 0, wz = 0;
            using (var objects = core.Call("get_map_objects").AsGodotArray()) foreach (var entry in objects) {
                using var item = entry.AsGodotDictionary();
                if (item["type_id"].AsString() != "GTGCAN") continue;
                GD.Print("CANNON ", item);
                if (id != 0) continue;
                world = item["world"].AsInt64(); id = item["id"].AsInt64();
                wx = item["world_x"].AsInt32(); wy = item["world_y"].AsInt32(); wz = item["world_z"].AsInt32();
            }
            if (id == 0) throw new Exception("No first-map Grand Cannon");
            for (int pass = 0; pass < 32; ++pass) {
                Root.GrabFocus();
                using var status = core.Call("get_map_status").AsGodotDictionary();
                var bounds = status["map_bounds"].AsRect2I();
                int dx = 30 * (wx - wy) / 256 - status["camera_x"].AsInt32() - bounds.Size.X / 2;
                int dy = 15 * (wx + wy) / 256 - 15 * wz / 104 - status["camera_y"].AsInt32() - bounds.Size.Y / 2;
                if (Math.Abs(dx) < 8 && Math.Abs(dy) < 8) break;
                var start = new Vector2(450, 300);
                var end = start + new Vector2(Math.Clamp(dx * 4, -350, 350), Math.Clamp(dy * 4, -250, 250));
                using var press = new InputEventMouseButton { Position = start, ButtonIndex = MouseButton.Right, Pressed = true };
                using var move = new InputEventMouseMotion { Position = end, ButtonMask = MouseButtonMask.Right };
                using var release = new InputEventMouseButton { Position = end, ButtonIndex = MouseButton.Right, Pressed = false };
                map.Call("handle_map_input", press, (Vector2)viewport.Size);
                map.Call("handle_map_input", move, (Vector2)viewport.Size);
                map.Call("handle_map_input", release, (Vector2)viewport.Size);
                await NextFrame();
            }
            using var camera = core.Call("get_map_status").AsGodotDictionary();
            GD.Print("CANNON_CAMERA ", camera);
            await DrawFrame();
            DirAccess.MakeDirRecursiveAbsolute(CaptureDirectory());
            using (var initial = viewport.GetTexture().GetImage()) initial.SavePng(CaptureDirectory().PathJoin("initial.png"));
            var anchor = new Vector2(30 * (wx - wy) / 256 - camera["camera_x"].AsInt32(),
                15 * (wx + wy) / 256 - 15 * wz / 104 - camera["camera_y"].AsInt32());
            Vector2 picked = Vector2.Zero;
            for (int y = -100; y <= 10 && picked == Vector2.Zero; ++y) for (int x = -65; x <= 65; ++x) {
                var point = anchor + new Vector2(x, y);
                using var motion = new InputEventMouseMotion { Position = point };
                map.Call("handle_map_input", motion, (Vector2)viewport.Size);
                using var status = core.Call("get_map_world_status").AsGodotDictionary();
                if (status["hovered_id"].AsInt64() == id) { picked = point; break; }
            }
            if (picked == Vector2.Zero) throw new Exception("Cannot pick cannon");
            void Click(Vector2 point, bool force) {
                foreach (bool down in new[] { true, false }) {
                    using var click = new InputEventMouseButton { Position = point, ButtonIndex = MouseButton.Left, Pressed = down, CtrlPressed = force };
                    map.Call("handle_map_input", click, (Vector2)viewport.Size);
                }
            }
            Click(picked, false);
            using (var actor = core.Call("get_map_object", world, id).AsGodotDictionary())
                Expect(actor["selected"].AsBool() && actor["powered"].AsBool(), "Cannon selected and powered");
            Click(anchor + new Vector2(-350, 0), true);
            using (var release = new InputEventKey { Keycode = Key.Ctrl, Pressed = false })
                map.Call("handle_map_input", release, (Vector2)viewport.Size);
            map.Call("reset_map_input");
            DirAccess.MakeDirRecursiveAbsolute(CaptureDirectory());
            long previous = -1;
            int captures = 0;
            ulong deadline = Time.GetTicksMsec() + 60000;
            while (captures < 180 && Time.GetTicksMsec() < deadline) {
                Root.GrabFocus(); await NextFrame();
                using var status = core.Call("get_map_status").AsGodotDictionary();
                long frame = status["current_frame"].AsInt64();
                if (frame == previous) continue;
                previous = frame;
                await DrawFrame();
                string name = "left-" + captures.ToString("D3");
                using var image = viewport.GetTexture().GetImage();
                Expect(image.SavePng(CaptureDirectory().PathJoin(name + ".png")) == Error.Ok, "Save " + name);
                System.IO.File.WriteAllText(CaptureDirectory().PathJoin(name + ".json"), Json.Stringify(status));
                ++captures;
            }
            Expect(captures == 180, "Capture 180 simulated frames");
        } finally {
            map.Call("close_map"); screen.QueueFree(); viewport.QueueFree(); await NextFrame();
        }
    }
}
