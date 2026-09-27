using Godot;
using System;
using System.Threading.Tasks;

// CLI integration probe for lighting residency while the camera moves.
// Inject focus/input into the test node: no desktop automation is required.
public partial class TestLightingBatchWindow : TestRunner
{
    private Node2D _map = null!;
    private async Task Tick()
    {
        _map.Notification((int)NotificationWMWindowFocusIn);
        await NextFrame();
        using var status = Application.GameScreen.GetMapStatus();
        if (status["state"].AsString() == "failed") throw new InvalidOperationException(status["error"].AsString());
    }
    private void Input(InputEvent input, Vector2 size)
    {
        using (input) _map.Call("handle_map_input", input, size);
    }
    protected override async Task RunTestAsync()
    {
        InitializeApp("lighting-batch-test");
        await WaitForBoot();
        Expect(Application.BootReady, "Lighting residency test needs game resources");
        if (!Application.BootReady) return;
        Application.StartMap("ALL01UMD.MAP");
        _map = Application.GetNode<Node2D>("GameScreen/WorldViewport/Map");
        var deadline = Time.GetTicksMsec() + 30000;
        while (true)
        {
            await Tick();
            using var status = Application.GameScreen.GetMapStatus();
            if (status["state"].AsString() == "drawn") break;
            if (Time.GetTicksMsec() > deadline) throw new TimeoutException("Map did not render");
        }
        using var loaded = Application.GameScreen.GetMapStatus();
        var size = new Vector2(loaded["viewport_width"].AsInt32(), loaded["viewport_height"].AsInt32());
        var center = size / 2;
        int initialX = loaded["camera_x"].AsInt32(), initialY = loaded["camera_y"].AsInt32();
        int warmImages = 0;
        for (int pass = 0; pass < 2; ++pass)
        {
            ulong begin = Time.GetTicksUsec();
            for (int n = 0; n < 40; ++n)
            {
                // Original DragScroll starts only beyond the 8-pixel threshold.
                var end = center + (n % 2 == 0 ? new Vector2(12, -10) : new Vector2(-12, 10));
                Input(new InputEventMouseButton { Position = center, ButtonIndex = MouseButton.Right,
                    Pressed = true, ButtonMask = MouseButtonMask.Right }, size);
                Input(new InputEventMouseMotion { Position = end, Relative = end - center, ButtonMask = MouseButtonMask.Right }, size);
                Input(new InputEventMouseButton { Position = end, ButtonIndex = MouseButton.Right, Pressed = false }, size);
                await Tick();
                await Tick();
                using var moved = Application.GameScreen.GetMapStatus();
                if (n == 0) Expect(moved["camera_x"].AsInt32() != initialX, "Camera must move during residency measurement");
            }
            using var current = Application.GameScreen.GetMapStatus();
            int images = current["decoded_lighting"].AsInt32();
            Expect(images > 0, "Lighting SHP frames must populate the immutable cache");
            if (pass == 1) Expect(images == warmImages, "Repeated pan must reuse decoded lighting frames");
            warmImages = images;
            Expect(current["camera_x"].AsInt32() == initialX && current["camera_y"].AsInt32() == initialY,
                "Pan sequence must return to the starting camera");
            GD.Print($"LIGHTING_PAN pass={pass} moves=40 host_frames=80 elapsed_us={Time.GetTicksUsec()-begin} decoded_lighting={images} packets={current["packets"]}");
        }
    }
}
