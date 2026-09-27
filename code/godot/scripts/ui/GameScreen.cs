using Godot;
using Godot.Collections;
using System;
using System.Collections.Generic;
using System.Globalization;

public partial class GameScreen : Control
{
    private Node2D? _map;
    private Label _status = null!;
    private bool _active;
    private string _resourceDirectory = "";
    private string _filename = "";
    public string MapFilename => _filename;
    private string _reportedError = "";
    private TextureRect _surface = null!;
    private RefCounted? _core;
    private PanelContainer _console = null!;
    private LineEdit _command = null!;
    private RichTextLabel _history = null!;
    private readonly List<string> _consoleLines = new();
    private readonly List<string> _commandHistory = new();
    private int _historyCursor;
    private string _historyDraft = "";
    internal string ConsoleLastResult { get; private set; } = "";
    internal RefCounted? Core => _core;

    public override void _Ready()
    {
        _status = GetNode<Label>("Status");
        _surface = GetNode<TextureRect>("WorldSurface");
        _console = GetNode<PanelContainer>("Console");
        _command = GetNode<LineEdit>("Console/Margin/Content/Command");
        _history = GetNode<RichTextLabel>("Console/Margin/Content/History");
        _command.TextSubmitted += RunConsoleCommand;
        SetProcess(false);
    }

    public override void _Input(InputEvent input)
    {
        if (!_active) return;
        if (input is InputEventKey key && key.Pressed && !key.Echo &&
            (key.PhysicalKeycode == Key.Quoteleft || key.Keycode == Key.Quoteleft))
        {
            SetConsoleOpen(!_console.Visible);
            GetViewport().SetInputAsHandled();
            return;
        }
        if (_console.Visible)
        {
            if (input is InputEventKey historyKey && historyKey.Pressed && !historyKey.Echo &&
                (historyKey.Keycode == Key.Up || historyKey.Keycode == Key.Down))
            {
                BrowseCommandHistory(historyKey.Keycode == Key.Up ? -1 : 1);
                GetViewport().SetInputAsHandled();
                return;
            }
            if (input.IsActionPressed("ui_cancel"))
            {
                SetConsoleOpen(false);
                GetViewport().SetInputAsHandled();
            }
            return;
        }
        if (_map == null || !GodotObject.IsInstanceValid(_map)) return;
        using var local = _surface.MakeInputLocal(input);
        using var device = _map.Call("handle_map_input", local, _surface.Size).AsGodotDictionary();
        if (device.TryGetValue("warp_pointer", out var position))
            Input.WarpMouse(_surface.GetGlobalTransformWithCanvas() * position.AsVector2());
        if (device.TryGetValue("consumed", out var consumed) && consumed.AsBool())
            GetViewport().SetInputAsHandled();
    }

    public void ResetMapInput()
    {
        if (_map != null && GodotObject.IsInstanceValid(_map)) _map.Call("reset_map_input");
    }

    public void BeginGame(RefCounted core, string filename, string resourceDirectory = "")
    {
        _core = core;
        SetConsoleOpen(false);
        _command.Clear();
        _commandHistory.Clear();
        _historyCursor = 0;
        _historyDraft = "";
        _consoleLines.Clear();
        _history.Clear();
        ConsoleLastResult = "";
        AppendConsoleLine("RA2 控制台  ·  输入 help 查看命令");
        _resourceDirectory = resourceDirectory;
        if (!ClassDB.ClassExists("RA2MapView"))
        {
            _status.Text = "地图组件不可用，请重新编译原生库。";
            _status.Show();
            return;
        }
        if (_map == null)
        {
            using var instance = ClassDB.Instantiate("RA2MapView");
            _map = instance.AsGodotObject() as Node2D;
            if (_map == null) return;
            _map.Name = "Map";
            GetNode<SubViewport>("WorldViewport").AddChild(_map);
        }
        _filename = filename;
        _reportedError = "";
        _status.Text = "正在加载地图…";
        _status.Show();
        _map.Call("configure", core);
        _map.Call("open_map", filename);
        _active = true;
        SetProcess(true);
    }

    public Dictionary GetMapStatus() => _map != null && GodotObject.IsInstanceValid(_map)
        ? _map.Call("get_render_status").AsGodotDictionary() : new Dictionary();

    public override void _Process(double delta)
    {
        if (!_active) return;
        using var status = GetMapStatus();
        var state = status.TryGetValue("state", out var value) ? value.AsString() : "empty";
        if (state == "drawn") _status.Hide();
        else if (state == "failed" || state == "cancelled")
        {
            var stage = status.TryGetValue("failure_stage", out var failureStage) && failureStage.AsString() == "rendering"
                ? "地图渲染失败" : "地图加载失败";
            _status.Text = $"{stage}：{_filename}\n{status["error"].AsString()}\n资源目录：{_resourceDirectory}\n\n按 Esc 打开菜单，可返回主菜单";
            if (_reportedError != _status.Text)
            {
                GD.PushError(_status.Text);
                _reportedError = _status.Text;
            }
            _status.Show();
        }
    }

    public void CloseGame()
    {
        SetConsoleOpen(false);
        _core = null;
        _active = false;
        SetProcess(false);
        if (_map != null && GodotObject.IsInstanceValid(_map)) _map.Call("close_map");
    }

    public override void _ExitTree() => CloseGame();

    private void SetConsoleOpen(bool open)
    {
        if (_console == null) return;
        _console.Visible = open;
        ResetMapInput();
        if (open)
        {
            _command.GrabFocus();
        }
        else if (_command.HasFocus()) _command.ReleaseFocus();
    }

    private void RunConsoleCommand(string text)
    {
        _command.Clear();
        var entered = text.Trim();
        if (entered.Length == 0) return;
        _commandHistory.Add(entered);
        if (_commandHistory.Count > 100) _commandHistory.RemoveAt(0);
        _historyCursor = _commandHistory.Count;
        _historyDraft = "";
        if (entered.Equals("clear", StringComparison.OrdinalIgnoreCase))
        {
            _consoleLines.Clear();
            _history.Clear();
            ConsoleLastResult = "";
            return;
        }
        AppendConsoleLine("> " + entered);
        ConsoleLastResult = ExecuteConsoleCommand(entered);
        AppendConsoleLine(ConsoleLastResult);
    }

    private string ExecuteConsoleCommand(string text)
    {
        var parts = text.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        var command = parts[0].ToLowerInvariant();
        if (command == "help" && parts.Length == 1)
            return "power          当前玩家增加 1000000 电力\n" +
                   "level 0/1/2    修改选中单位等级\n" +
                   "life 数字       修改选中单位生命值\n" +
                   "clear           清空屏幕记录\n" +
                   "↑/↓             浏览输入历史";
        if (command == "power" && parts.Length == 1)
            return _core != null && _core.Call("grant_map_player_power").AsBool()
                ? "已为当前玩家增加 1000000 电力（本局持续生效）。" : "地图或当前玩家尚未就绪。";
        if (command != "level" && command != "life")
            return "未知命令。输入 help 查看可用命令。";
        if (parts.Length != 2 || !int.TryParse(parts[1], NumberStyles.Integer,
                CultureInfo.InvariantCulture, out var value) ||
            (command == "level" ? value < 0 || value > 2 : value < 0 || value > 1000000))
            return command == "level" ? "等级请输入 0、1 或 2（新兵、老兵、精英）。" : "生命请输入 0 到 1000000 之间的整数。";
        if (_core == null) return "地图尚未就绪。";
        int changed = 0;
        using var objects = _core.Call("get_map_objects").AsGodotArray();
        foreach (var entry in objects)
        {
            using var item = entry.AsGodotDictionary();
            if (!item["selected"].AsBool()) continue;
            var kind = item["kind"].AsString();
            if (kind != "infantry" && kind != "unit" && kind != "aircraft") continue;
            if (_core.Call(command == "level" ? "set_map_object_level" : "set_map_unit_cheat_health",
                item["world"].AsInt64(), item["id"].AsInt64(), value).AsBool()) changed++;
        }
        return changed > 0
            ? $"已修改 {changed} 个选中单位的{(command == "level" ? "等级" : "生命值")}。"
            : "没有可修改的选中单位。";
    }

    private void AppendConsoleLine(string line)
    {
        _consoleLines.Add(line);
        if (_consoleLines.Count > 200) _consoleLines.RemoveAt(0);
        _history.Text = string.Join("\n", _consoleLines);
        _history.ScrollToLine(_history.GetLineCount() - 1);
    }

    private void BrowseCommandHistory(int direction)
    {
        if (_commandHistory.Count == 0) return;
        if (direction < 0 && _historyCursor == _commandHistory.Count)
            _historyDraft = _command.Text;
        _historyCursor = Math.Clamp(_historyCursor + direction, 0, _commandHistory.Count);
        _command.Text = _historyCursor == _commandHistory.Count
            ? _historyDraft : _commandHistory[_historyCursor];
        _command.CaretColumn = _command.Text.Length;
    }
}
