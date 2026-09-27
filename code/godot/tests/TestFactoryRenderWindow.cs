using Godot;
using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Threading.Tasks;

// Capture the production GPU renderer at a camera read from an original frame.
// This prepares evidence, not a claim that the two images already match.
public partial class TestFactoryRenderWindow : TestRunner
{
    private static IntPtr LoadCoreLibrary()
    {
        // Use the binary configured for this project, including the main
        // project's ../../out path and capture snapshots' res://bin path.
        using var extension = new ConfigFile();
        if (extension.Load("res://ra2_core.gdextension") != Error.Ok)
            throw new Exception("Cannot read the project's native extension path");
        string path = extension.GetValue("libraries", "macos.debug.arm64").AsString();
        if (!path.StartsWith("res://") && !System.IO.Path.IsPathRooted(path)) path = "res://" + path;
        return NativeLibrary.Load(ProjectSettings.GlobalizePath(path));
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void RandomizerConstructor(IntPtr instance, uint seed);

    // Test-only access to the existing core globals. Seed before map loading so
    // the real CellClass algorithm generates its own table; never inject a table.
    private static void SetTerrainRandomSeed(uint seed)
    {
        var library = LoadCoreLibrary();
        try
        {
            var initialized = Marshal.ReadIntPtr(NativeLibrary.GetExport(library, "_ZN9CellClass27TileVariantTableInitializedE"));
            if (Marshal.ReadByte(initialized) != 0) throw new Exception("Tile table already generated before seed replay");
            var random = Marshal.ReadIntPtr(NativeLibrary.GetExport(library, "_ZN10Randomizer6GlobalE"));
            var construct = Marshal.GetDelegateForFunctionPointer<RandomizerConstructor>(NativeLibrary.GetExport(library, "_ZN10RandomizerC1Ej"));
            construct(random, seed);
        }
        finally { NativeLibrary.Free(library); }
    }

    private static int[] ReadTerrainVariants()
    {
        var library = LoadCoreLibrary();
        try
        {
            var table = Marshal.ReadIntPtr(NativeLibrary.GetExport(library, "_ZN9CellClass16TileVariantTableE"));
            var result = new int[64];
            Marshal.Copy(table, result, 0, result.Length);
            return result;
        }
        finally { NativeLibrary.Free(library); }
    }

    // Diagnostic only: replay the original run's generated tile-variant table
    // after the replica's first draw. This preserves the replica's RNG calls
    // and isolates terrain input from palette and object rendering.
    private static void ApplyCapturedTileVariants(string path)
    {
        using var source = JsonDocument.Parse(System.IO.File.ReadAllText(path));
        var variants = source.RootElement.GetProperty("original_table");
        if (variants.GetArrayLength() != 64) throw new Exception("Expected 64 captured tile variants");
        var library = LoadCoreLibrary();
        try
        {
            var reference = NativeLibrary.GetExport(library, "_ZN9CellClass16TileVariantTableE");
            var table = Marshal.ReadIntPtr(reference);
            for (int i = 0; i < 64; ++i) Marshal.WriteInt32(table, i * sizeof(int), variants[i].GetInt32());
            var initialized = NativeLibrary.GetExport(library, "_ZN9CellClass27TileVariantTableInitializedE");
            Marshal.WriteByte(Marshal.ReadIntPtr(initialized), 1);
        }
        finally { NativeLibrary.Free(library); }
    }

    private async Task WaitState(GodotObject target, string method, string wanted)
    {
        var deadline = Time.GetTicksMsec() + 60000;
        ulong lastLog = 0;
        while (Time.GetTicksMsec() < deadline)
        {
            if (!Root.HasFocus()) Root.GrabFocus();
            using var status = target.Call(method).AsGodotDictionary();
            if (status["state"].AsString() == wanted) return;
            if (status["state"].AsString() == "failed") throw new Exception(status["error"].AsString());
            if (Time.GetTicksMsec() - lastLog > 5000) { GD.Print("CAPTURE_WAIT ", status); lastLog = Time.GetTicksMsec(); }
            await NextFrame();
        }
        throw new Exception("Timed out: " + wanted);
    }

    protected override async Task RunTestAsync()
    {
        var args = OS.GetCmdlineUserArgs();
        var referencePath = args.First(s => s.StartsWith("--reference-json="))[17..];
        using var reference = JsonDocument.Parse(System.IO.File.ReadAllText(referencePath));
        var source = reference.RootElement;
        var seedArgument = args.FirstOrDefault(s => s.StartsWith("--random-seed="));
        var capturedTiles = args.FirstOrDefault(s => s.StartsWith("--tile-variants="));
        if (seedArgument != null && capturedTiles != null) throw new Exception("Choose seed generation or table injection, not both");
        uint terrainSeed = 0;
        if (seedArgument != null)
        {
            var value = seedArgument[14..];
            terrainSeed = value.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                ? Convert.ToUInt32(value[2..], 16) : Convert.ToUInt32(value);
        }
        int width = source.GetProperty("window")[2].GetInt32();
        int height = source.GetProperty("window")[3].GetInt32();
        int targetX = source.GetProperty("camera")[0].GetInt32();
        int targetY = source.GetProperty("camera")[1].GetInt32();
        Root.Size = new Vector2I(width, height);
        Root.GrabFocus();
        using var instance = ClassDB.Instantiate("RA2Core");
        using var core = (RefCounted)instance.AsGodotObject();
        core.Call("begin_resource_loading", args.First(s => s.StartsWith("--game-data="))[12..]);
        await WaitState(core, "get_resource_progress", "complete");
        var viewport = new SubViewport { Size = Root.Size, RenderTargetUpdateMode = SubViewport.UpdateMode.Always };
        Root.AddChild(viewport);
        Root.AddChild(new TextureRect { Texture = viewport.GetTexture(), Size = Root.Size });
        using var mapInstance = ClassDB.Instantiate("RA2MapView");
        var map = (Node2D)mapInstance.AsGodotObject();
        viewport.AddChild(map);
        map.Call("configure", core);
        try
        {
            if (seedArgument != null) SetTerrainRandomSeed(terrainSeed);
            map.Call("open_map", source.GetProperty("map").GetString()!);
            Root.GrabFocus();
            await WaitState(map, "get_render_status", "drawn");
            if (capturedTiles != null) ApplyCapturedTileVariants(capturedTiles[16..]);
            if (seedArgument != null)
            {
                var expected = source.GetProperty("tile_variant_table").EnumerateArray().Select(v => v.GetInt32()).ToArray();
                Expect(ReadTerrainVariants().SequenceEqual(expected), "Seed-generated tile table equals all 64 original entries");
            }
            double gainX = 0.25, gainY = 0.25;
            for (int pass = 0; pass < 80; ++pass)
            {
                using var status = core.Call("get_map_status").AsGodotDictionary();
                int dx = targetX - status["camera_x"].AsInt32();
                int dy = targetY - status["camera_y"].AsInt32();
                if (dx == 0 && dy == 0) break;
                int moveX = (int)Math.Clamp(Math.Round(dx / gainX), -300, 300);
                int moveY = (int)Math.Clamp(Math.Round(dy / gainY), -240, 240);
                if (Math.Abs(moveX) <= 8 && Math.Abs(moveY) <= 8) { moveX += 20; moveY += 20; }
                var start = new Vector2(400, 300);
                var end = start + new Vector2(moveX, moveY);
                using var press = new InputEventMouseButton { Position = start, ButtonIndex = MouseButton.Right, Pressed = true };
                using var move = new InputEventMouseMotion { Position = end, ButtonMask = MouseButtonMask.Right };
                using var release = new InputEventMouseButton { Position = end, ButtonIndex = MouseButton.Right, Pressed = false };
                map.Call("handle_map_input", press, (Vector2)viewport.Size);
                map.Call("handle_map_input", move, (Vector2)viewport.Size);
                map.Call("handle_map_input", release, (Vector2)viewport.Size);
                map.Call("reset_map_input");
                await NextFrame();
                using var after = core.Call("get_map_status").AsGodotDictionary();
                int actualX = after["camera_x"].AsInt32() - status["camera_x"].AsInt32();
                int actualY = after["camera_y"].AsInt32() - status["camera_y"].AsInt32();
                if (moveX != 0 && actualX != 0) gainX = (double)actualX / moveX;
                if (moveY != 0 && actualY != 0) gainY = (double)actualY / moveY;
            }
            await DrawFrame(); await DrawFrame();
            DirAccess.MakeDirRecursiveAbsolute(CaptureDirectory());
            int captureCount = 8;
            var countArg = args.FirstOrDefault(s => s.StartsWith("--capture-count="));
            if (countArg != null) captureCount = int.Parse(countArg[16..]);
            if (captureCount < 1 || captureCount > 8) throw new Exception("Capture count out of range");
            long lastTick = -12;
            for (int i = 0; i < captureCount; ++i)
            {
                var deadline = Time.GetTicksMsec() + 30000;
                while (true)
                {
                    if (!Root.HasFocus()) Root.GrabFocus();
                    using var progress = core.Call("get_map_status").AsGodotDictionary();
                    if (progress["current_frame"].AsInt64() >= lastTick + 12) break;
                    if (Time.GetTicksMsec() >= deadline) throw new Exception("Simulation did not advance between captures");
                    await NextFrame();
                }
                await DrawFrame();
                using var status = core.Call("get_map_status").AsGodotDictionary();
                lastTick = status["current_frame"].AsInt64();
                Expect(status["camera_x"].AsInt32() == targetX && status["camera_y"].AsInt32() == targetY, "Exact original camera alignment: " + status);
                using var objects = core.Call("get_map_objects").AsGodotArray();
                using var record = new Godot.Collections.Dictionary { ["status"] = status, ["objects"] = objects, ["reference"] = referencePath };
                record["tile_variant_table"] = ReadTerrainVariants();
                record["terrain_input"] = capturedTiles != null ? "injected table" : seedArgument != null ? $"seed 0x{terrainSeed:X8}" : "default global RNG";
                System.IO.File.WriteAllText(CaptureDirectory().PathJoin($"replica-{i:D2}.json"), Godot.Json.Stringify(record, "  "));
                using var shot = viewport.GetTexture().GetImage();
                Expect(shot.SavePng(CaptureDirectory().PathJoin($"replica-{i:D2}.png")) == Error.Ok, "Capture saved");
            }
        }
        finally { map.Call("close_map"); map.QueueFree(); viewport.QueueFree(); await NextFrame(); }
    }
}
