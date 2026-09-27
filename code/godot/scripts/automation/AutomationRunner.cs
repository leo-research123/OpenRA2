using Godot;
using System;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Threading.Tasks;

/// <summary>Opt-in, local scripted control of the actual app, including release exports.</summary>
public partial class AutomationRunner : Node
{
    private App _app = null!;
    private string _script = "";
    private string _report = "";
    private readonly JsonArray _steps = new();
    private Vector2 _mousePosition;
    private MouseButtonMask _mouseMask;
    private bool _mouseEntered;
    private string _nextMap = "";

    public static void Attach(App app)
    {
        var script = Argument("--automation=");
        if (script.Length == 0) return;
        app.AddChild(new AutomationRunner
        {
            Name = "Automation", ProcessMode = ProcessModeEnum.Always,
            _app = app, _script = script, _report = Argument("--automation-report=")
        });
    }

    public static string Argument(string prefix)
    {
        var value = "";
        foreach (var argument in OS.GetCmdlineUserArgs())
            if (argument.StartsWith(prefix, StringComparison.Ordinal)) value = argument[prefix.Length..];
        return value;
    }

    public override async void _Ready()
    {
        var passed = false;
        var error = "";
        try
        {
            await Frame(); // App completes _Ready before its controls are used.
            Require(_report.IsAbsolutePath(), "--automation-report requires an absolute path");
            Require(_script.IsAbsolutePath(), "--automation requires an absolute JSON path");
            using var document = JsonDocument.Parse(System.IO.File.ReadAllText(_script));
            foreach (var step in document.RootElement.GetProperty("steps").EnumerateArray())
            {
                var action = Text(step, "action");
                var result = new JsonObject { ["action"] = action, ["passed"] = false };
                _steps.Add(result);
                await Execute(step);
                result["passed"] = true;
                result["snapshot"] = Snapshot();
                GD.Print("AUTOMATION_STEP ", _steps.Count, " ", action);
            }
            passed = true;
        }
        catch (Exception exception) { error = exception.ToString(); }
        try
        {
            if (_report.IsAbsolutePath())
                System.IO.File.WriteAllText(_report, new JsonObject
                {
                    ["passed"] = passed, ["error"] = error, ["steps"] = _steps,
                    ["snapshot"] = Snapshot()
                }.ToJsonString(new JsonSerializerOptions { WriteIndented = true }) + "\n");
        }
        catch (Exception exception) { passed = false; error += exception; }
        GD.Print(passed ? "PASS: AutomationRunner" : "FAIL: AutomationRunner " + error);
        // Use the normal close path before quitting so render-thread cleanup is exercised.
        _app.GameScreen.CloseGame();
        GetTree().Paused = false;
        await Frame();
        await Frame();
        GetTree().Quit(passed ? 0 : 1);
    }

    private async Task Execute(JsonElement step)
    {
        switch (Text(step, "action"))
        {
            case "wait_boot":
                await WaitUntil(() => _app.BootReady || _app.BootError.Visible, step);
                Require(_app.BootReady, _app.BootError.Text);
                break;
            case "wait_boot_error":
                await WaitUntil(() => _app.BootError.Visible, step);
                Require(!_app.BootReady && _app.StartButton.Disabled, "Boot error must block Start");
                break;
            case "choose_resources":
                Require(_app.GetNode<Button>("ResourceDirectoryButton").IsVisibleInTree(), "Resource selection is unavailable");
                _app.GetNode<FileDialog>("ResourceDirectoryDialog").EmitSignal(FileDialog.SignalName.DirSelected, Text(step, "directory"));
                break;
            case "press":
                var button = _app.GetNode<Button>(Text(step, "node"));
                Require(button.IsVisibleInTree() && !button.Disabled, "Button is hidden or disabled");
                if (button == _app.StartButton && _nextMap.Length > 0)
                {
                    _app.StartMap(_nextMap);
                    _nextMap = "";
                }
                else button.EmitSignal(BaseButton.SignalName.Pressed);
                await Frame();
                break;
            case "set_map":
                Require(_app.CurrentScreen != App.Screen.Game, "Close the current game before changing maps");
                _nextMap = Text(step, "map");
                break;
            case "wait_map_list":
                Require(_app.CurrentScreen == App.Screen.MapSelection, "Map selection is not open");
                await WaitUntil(() => !_app.MapSelectionScreen.Scanning, step);
                break;
            case "select_map":
                Require(_app.CurrentScreen == App.Screen.MapSelection, "Map selection is not open");
                Require(_app.MapSelectionScreen.SelectMap(Text(step, "map")), "Requested map is not in the list");
                break;
            case "filter_maps":
                Require(_app.CurrentScreen == App.Screen.MapSelection, "Map selection is not open");
                var mapType = _app.MapSelectionScreen.MapType;
                Require(!mapType.Disabled, "Map categories are unavailable while scanning or after a scan failure");
                var category = Enum.Parse<MapSelectionScreen.Category>(Text(step, "category"));
                Require(Enum.IsDefined(category), "Unknown map category");
                mapType.Select((int)category);
                mapType.EmitSignal(OptionButton.SignalName.ItemSelected, (long)category);
                break;
            case "wait_map":
                var expected = Text(step, "state");
                await WaitUntil(() =>
                {
                    using var status = _app.GameScreen.GetMapStatus();
                    var state = status.TryGetValue("state", out var value) ? value.AsString() : "empty";
                    Require(state != "failed" || expected == "failed", status.TryGetValue("error", out var failure) ? failure.AsString() : "Map failed");
                    return state == expected &&
                        (!step.TryGetProperty("width", out var width) || status["viewport_width"].AsInt32() == width.GetInt32()) &&
                        (!step.TryGetProperty("height", out var height) || status["viewport_height"].AsInt32() == height.GetInt32());
                }, step);
                break;
            case "assert":
                JsonNode? actual = Snapshot();
                foreach (var part in Text(step, "field").Split('.')) actual = actual?[part];
                var wanted = JsonNode.Parse(step.GetProperty("equals").GetRawText());
                Require(JsonNode.DeepEquals(actual, wanted), $"Assertion {Text(step, "field")}: expected {wanted}, got {actual}");
                break;
            case "key":
                var key = Enum.Parse<Key>(Text(step, "key"), true);
                using (var press = new InputEventKey { Keycode = key, Pressed = true }) Input.ParseInputEvent(press);
                await Frame();
                using (var release = new InputEventKey { Keycode = key, Pressed = false }) Input.ParseInputEvent(release);
                await Frame();
                break;
            case "console_command":
                var console = _app.GetNode<PanelContainer>("GameScreen/Console");
                Require(console.IsVisibleInTree(), "Console is closed");
                _app.GetNode<LineEdit>("GameScreen/Console/Margin/Content/Command")
                    .EmitSignal(LineEdit.SignalName.TextSubmitted, Text(step, "text"));
                await Frame();
                break;
            case "assert_selected":
                var core = _app.GameScreen.Core;
                Require(core != null, "Game core is unavailable");
                var matched = 0;
                using (var selectedObjects = core!.Call("get_map_objects").AsGodotArray())
                    foreach (var entry in selectedObjects)
                    {
                        using var item = entry.AsGodotDictionary();
                        if (!item["selected"].AsBool()) continue;
                        var kind = item["kind"].AsString();
                        if (kind != "infantry" && kind != "unit" && kind != "aircraft") continue;
                        if (step.TryGetProperty("health", out var health))
                            Require(item["health"].AsInt32() == health.GetInt32(), "Selected unit health does not match");
                        if (step.TryGetProperty("level", out var level))
                            Require(item["level"].AsInt32() == level.GetInt32(), "Selected unit level does not match");
                        matched++;
                    }
                Require(matched > 0, "No unit is selected");
                break;
            case "select_unit":
                await SelectVisibleUnit();
                break;
            case "mouse_move":
                var position = new Vector2(step.GetProperty("x").GetSingle(), step.GetProperty("y").GetSingle());
                if (step.TryGetProperty("node", out var targetNode))
                {
                    var control = _app.GetNode<Control>(targetNode.GetString()!);
                    Require(control.IsVisibleInTree(), "Mouse target is hidden");
                    if (step.TryGetProperty("normalized", out var normalized) && normalized.GetBoolean()) position *= control.Size;
                    position = control.GetGlobalTransformWithCanvas() * position;
                }
                if (!_mouseEntered) { GetTree().Root.NotifyMouseEntered(); _mouseEntered = true; }
                using (var motion = new InputEventMouseMotion
                {
                    Position = position, GlobalPosition = position,
                    Relative = position - _mousePosition, ButtonMask = _mouseMask
                }) Input.ParseInputEvent(motion);
                Input.FlushBufferedEvents();
                _mousePosition = position;
                await Frame();
                await Frame();
                break;
            case "mouse_button":
                var mouseButton = Enum.Parse<MouseButton>(Text(step, "button"), true);
                Require(mouseButton >= MouseButton.Left && mouseButton <= MouseButton.Middle, "Use Left, Right or Middle");
                var pressed = step.GetProperty("pressed").GetBoolean();
                var mask = (MouseButtonMask)(1 << ((int)mouseButton - 1));
                _mouseMask = pressed ? _mouseMask | mask : _mouseMask & ~mask;
                if (!_mouseEntered) { GetTree().Root.NotifyMouseEntered(); _mouseEntered = true; }
                using (var click = new InputEventMouseButton
                {
                    Position = _mousePosition, GlobalPosition = _mousePosition, ButtonIndex = mouseButton,
                    Pressed = pressed, ButtonMask = _mouseMask
                }) Input.ParseInputEvent(click);
                Input.FlushBufferedEvents();
                await Frame();
                await Frame();
                break;
            case "resize":
                Require(_app.DisplayService.ApplySelection(new Vector2I(step.GetProperty("width").GetInt32(),
                    step.GetProperty("height").GetInt32()), (int)DisplayConfig.Mode.Windowed) == Error.Ok, "Resize failed");
                await Frame();
                break;
            case "frames":
                var count = step.GetProperty("count").GetInt32();
                Require(count >= 0 && count <= 600, "frames count must be 0..600");
                for (var i = 0; i < count; i++) await Frame();
                break;
            case "capture":
                Require(DisplayServer.GetName() != "headless", "Capture requires a graphical renderer");
                await ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);
                await ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);
                var viewport = step.TryGetProperty("node", out var node)
                    ? _app.GetNode<Viewport>(node.GetString()!) : GetTree().Root;
                using (var image = viewport.GetTexture().GetImage())
                {
                    if (step.TryGetProperty("min_colors", out var min))
                    {
                        var colors = new System.Collections.Generic.HashSet<uint>();
                        for (var y = 0; y < image.GetHeight(); y += 7)
                            for (var x = 0; x < image.GetWidth(); x += 7) colors.Add(image.GetPixel(x, y).ToRgba32());
                        Require(colors.Count >= min.GetInt32(), $"Only {colors.Count} colors in rendered image");
                    }
                    Require(image.SavePng(Text(step, "path")) == Error.Ok, "Screenshot save failed");
                }
                break;
            default: throw new InvalidOperationException("Unknown automation action: " + Text(step, "action"));
        }
    }

    private JsonObject Snapshot()
    {
        using var status = _app.GameScreen.GetMapStatus();
        return new JsonObject
        {
            ["boot_ready"] = _app.BootReady, ["screen"] = _app.CurrentScreen.ToString(),
            ["paused"] = GetTree().Paused, ["boot_error"] = _app.BootError.Text,
            ["window_focused"] = GetTree().Root.HasFocus(),
            ["resource_directory"] = _app.ActiveResourceDirectory,
            ["map_list_scanning"] = _app.MapSelectionScreen.Scanning,
            ["map_list_count"] = _app.MapSelectionScreen.Maps.ItemCount,
            ["map_category"] = ((MapSelectionScreen.Category)_app.MapSelectionScreen.MapType.Selected).ToString(),
            ["selected_map"] = _app.MapSelectionScreen.SelectedMap,
            ["map_file"] = _app.GameScreen.MapFilename,
            ["console_visible"] = _app.GetNode<PanelContainer>("GameScreen/Console").IsVisibleInTree(),
            ["console_output"] = _app.GameScreen.ConsoleLastResult,
            ["console_history"] = _app.GetNode<RichTextLabel>("GameScreen/Console/Margin/Content/History").Text,
            ["console_input"] = _app.GetNode<LineEdit>("GameScreen/Console/Margin/Content/Command").Text,
            ["resources"] = JsonNode.Parse(Godot.Json.Stringify(_app.ResourceProgress)),
            ["map"] = JsonNode.Parse(Godot.Json.Stringify(status))
        };
    }

    private async Task SelectVisibleUnit()
    {
        var core = _app.GameScreen.Core;
        Require(core != null, "Game core is unavailable");
        var map = _app.GetNode<Node2D>("GameScreen/WorldViewport/Map");
        var viewport = _app.GetNode<SubViewport>("GameScreen/WorldViewport");
        using var status = core!.Call("get_map_status").AsGodotDictionary();
        var bounds = status["map_bounds"].AsRect2I();
        var candidates = new System.Collections.Generic.List<(long World, long Id, Vector2 Anchor)>();
        using (var objects = core.Call("get_map_objects").AsGodotArray())
            foreach (var entry in objects)
            {
                using var item = entry.AsGodotDictionary();
                if (!item["alive"].AsBool() || item["kind"].AsString() != "unit") continue;
                var wx = item["world_x"].AsInt32();
                var wy = item["world_y"].AsInt32();
                var wz = item["world_z"].AsInt32();
                var anchor = new Vector2(30 * (wx - wy) / 256 - status["camera_x"].AsInt32(),
                    15 * (wx + wy) / 256 - 15 * wz / 104 - status["camera_y"].AsInt32());
                if (bounds.HasPoint((Vector2I)anchor))
                    candidates.Add((item["world"].AsInt64(), item["id"].AsInt64(), anchor));
            }
        foreach (var candidate in candidates)
            for (var dy = -36; dy <= 16; dy += 4)
                for (var dx = -36; dx <= 36; dx += 4)
                {
                    var point = candidate.Anchor + new Vector2(dx, dy);
                    if (!bounds.HasPoint((Vector2I)point)) continue;
                    using var motion = new InputEventMouseMotion { Position = point };
                    map.Call("handle_map_input", motion, (Vector2)viewport.Size);
                    using var world = core.Call("get_map_world_status").AsGodotDictionary();
                    if (world["hovered_id"].AsInt64() != candidate.Id ||
                        world["hovered_world"].AsInt64() != candidate.World) continue;
                    foreach (var down in new[] { true, false })
                    {
                        using var click = new InputEventMouseButton
                        { Position = point, ButtonIndex = MouseButton.Left, Pressed = down };
                        map.Call("handle_map_input", click, (Vector2)viewport.Size);
                    }
                    await Frame();
                    using var selected = core.Call("get_map_object", candidate.World, candidate.Id).AsGodotDictionary();
                    if (selected.Count > 0 && selected["selected"].AsBool()) return;
                }
        throw new InvalidOperationException("Could not select a visible unit");
    }

    private async Task WaitUntil(Func<bool> condition, JsonElement step)
    {
        var timeout = step.TryGetProperty("timeout_seconds", out var value) ? value.GetDouble() : 30;
        Require(timeout > 0 && timeout <= 120, "Timeout must be 0..120 seconds");
        var deadline = Time.GetTicksMsec() + (ulong)(timeout * 1000);
        while (!condition())
        {
            Require(Time.GetTicksMsec() < deadline, "Timed out: " + step);
            await Frame();
        }
    }

    private async Task Frame() => await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
    private static string Text(JsonElement step, string field) => step.GetProperty(field).GetString()!;
    private static void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}
