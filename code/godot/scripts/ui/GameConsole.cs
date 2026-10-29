using Godot;
using Godot.Collections;
using System;
using System.Collections.Generic;
using System.Globalization;

public partial class GameConsole : PanelContainer
{
    private const int OutputLimit = 200;
    private const int CommandLimit = 100;
    private LineEdit _input = null!;
    private RichTextLabel _output = null!;
    private RefCounted? _core;
    private long _world;
    private readonly List<string> _lines = new();
    private readonly List<string> _commands = new();
    private readonly System.Collections.Generic.Dictionary<long, CommandInfo> _catalog = new();
    private int _cursor;
    private string _draft = "";

    private sealed record CommandInfo(string Usage, string Description, string SuccessMessage);
    public string LastResult { get; private set; } = "";
    public Dictionary LastExecution { get; private set; } = new();
    public string HistoryText => _output.Text;
    public string InputText => _input.Text;
    internal long World => _world;
    public event Action? CommandExecuted;

    public override void _Ready()
    {
        _input = GetNode<LineEdit>("Margin/Content/Command");
        _output = GetNode<RichTextLabel>("Margin/Content/History");
        _input.TextSubmitted += Submit;
    }

    public void BeginSession(RefCounted core)
    {
        EndSession();
        _core = core;
        _commands.Clear();
        _cursor = 0;
        _draft = "";
        _input.Clear();
        ClearOutput();
        _catalog.Clear();
        using var descriptors = core.Call("get_debug_commands").AsGodotArray();
        foreach (var entry in descriptors)
        {
            using var item = entry.AsGodotDictionary();
            _catalog.Add(item["id"].AsInt64(), new CommandInfo(item["usage"].AsString(),
                Localize(item["description"].AsString()), Localize(item["success_message"].AsString())));
        }
        Append("RA2 控制台  ·  输入 help 查看命令");
    }

    public void BindReadyWorld()
    {
        // Bind once when this map is ready. A later restart must begin a new
        // console session rather than silently retargeting old submissions.
        if (_world == 0 && _core != null) _world = _core.Call("get_debug_command_world").AsInt64();
    }

    public void EndSession()
    {
        SetOpen(false);
        _core = null;
        _world = 0;
    }

    public void SetOpen(bool open)
    {
        Visible = open;
        if (_input == null) return;
        if (open) _input.GrabFocus();
        else if (_input.HasFocus()) _input.ReleaseFocus();
    }

    public bool HandleInput(InputEvent input)
    {
        if (input is InputEventKey key && key.Pressed && !key.Echo &&
            (key.PhysicalKeycode == Key.Quoteleft || key.Keycode == Key.Quoteleft))
        {
            SetOpen(!Visible);
            GetViewport().SetInputAsHandled();
            return true;
        }
        if (!Visible) return false;
        if (input is InputEventKey history && history.Pressed && !history.Echo &&
            (history.Keycode == Key.Up || history.Keycode == Key.Down))
        {
            BrowseHistory(history.Keycode == Key.Up ? -1 : 1);
            GetViewport().SetInputAsHandled();
        }
        else if (input.IsActionPressed("ui_cancel"))
        {
            SetOpen(false);
            GetViewport().SetInputAsHandled();
        }
        // Text entry continues to LineEdit; the parent suppresses map forwarding.
        return true;
    }

    public void Submit(string text)
    {
        if (!Visible || _core == null) return;
        _input.Clear();
        var entered = text.Trim();
        if (entered.Length == 0) return;
        _commands.Add(entered);
        if (_commands.Count > CommandLimit) _commands.RemoveAt(0);
        _cursor = _commands.Count;
        _draft = "";
        if (entered.Equals("clear", StringComparison.OrdinalIgnoreCase))
        {
            ClearOutput();
            return;
        }
        Append("> " + entered);
        if (entered.Equals("life", StringComparison.OrdinalIgnoreCase) || entered.StartsWith("life ", StringComparison.OrdinalIgnoreCase))
        {
            LastResult = "life 已更名为 hp。用法：hp <0..1000000>";
            ReplaceExecution(new Dictionary { ["status"] = "local", ["action"] = "renamed" });
            Append(LastResult);
            return;
        }
        if (entered.Equals("help", StringComparison.OrdinalIgnoreCase))
        {
            var lines = new List<string>();
            foreach (var info in _catalog.Values) lines.Add(info.Usage + "  " + info.Description);
            lines.Add("clear  清空屏幕记录");
            lines.Add("↑/↓  浏览输入历史");
            LastResult = string.Join("\n", lines);
            ReplaceExecution(new Dictionary { ["status"] = "local", ["action"] = "help" });
        }
        else
        {
            ReplaceExecution(_core.Call("execute_debug_command", entered, _world).AsGodotDictionary());
            LastResult = FormatExecution(LastExecution);
            CommandExecuted?.Invoke();
        }
        Append(LastResult);
    }

    private string FormatExecution(Dictionary execution)
    {
        var status = execution["status"].AsString();
        _catalog.TryGetValue(execution["command"].AsInt64(), out var info);
        var count = execution["applied"].AsInt64().ToString(CultureInfo.InvariantCulture);
        var success = (info?.SuccessMessage ?? "已修改 {count} 个目标。").Replace("{count}", count);
        var detail = execution.TryGetValue("message", out var message) ? message.AsString() : "";
        if (detail.Length > 0) success = detail;
        return status switch
        {
            "success" => success,
            "partial_success" => success + $" 另有 {execution["skipped"].AsInt64()} 个目标未完成（{FailureText(execution["first_failure"].AsString())}）。",
            "invalid_command" => "未知命令。输入 help 查看可用命令。",
            "invalid_argument" => detail.Length > 0 ? detail : info != null ? "参数无效。用法：" + info.Usage : "命令过长或格式无效。",
            "map_not_ready" => "地图尚未就绪。",
            "stale_session" => "命令所属地图已失效，请重新进入地图。",
            "no_targets" => "没有可修改的目标（" + FailureText(execution["first_failure"].AsString()) + "）。",
            "resource_failure" => "命令准备失败：内存不足，未修改游戏状态。",
            _ => "命令执行失败，已完成 " + count + " 个目标。"
        };
    }

    private static string FailureText(string reason) => reason switch
    {
        "unsupported_type" => "对象类型不支持",
        "dead_object" => "对象已经死亡",
        "expired_object" => "对象已经失效",
        "unavailable_player" => "当前玩家不可用",
        "exception" => "执行中断",
        "invalid_state" => "对象当前状态不支持",
        "blocked_destination" => "目标位置越界或已被占用",
        "value_out_of_range" => "数值超过当前规则或资金上限",
        _ => "没有适用的选中对象"
    };

    // Presentation-only translations; unknown descriptions/messages retain the
    // core fallback, so adding a game command never requires a host executor.
    private static string Localize(string text) => text switch
    {
        "Set a persistent 1000000 power bonus for the current player." => "当前玩家获得本局固定 1000000 电力加成",
        "Set selected objects, including buildings, to rookie (0), veteran (1) or elite (2)." => "修改选中对象等级，包含建筑：新兵、老兵、精英",
        "Override selected unit health; zero retires units without combat death." => "修改选中单位生命；0 使用调试移除流程",
        "Current player granted a persistent 1000000 power bonus." => "已为当前玩家增加 1000000 电力（本局持续生效）。",
        "Updated the rank of {count} selected objects." => "已修改 {count} 个选中对象的等级。",
        "Updated the health of {count} selected units." => "已修改 {count} 个选中单位的生命值。",
        "Set selected object health, including buildings; zero retires objects." => "设置选中对象生命值，包含建筑；0 使用调试移除流程",
        "Updated the health of {count} selected objects." => "已修改 {count} 个选中对象的生命值。",
        "Toggle shroud/fog display without changing explored cells." => "开关迷雾显示；无参数切换，0 关闭，1 开启",
        "Toggle forced radar availability; 0 restores normal rules, 1 enables radar." => "强制开启雷达地图；无参数切换，0 恢复正常规则，1 强制开启",
        "Add cash to the current player; default 10000." => "给当前玩家加钱，默认 10000",
        "Toggle instant relocation for subsequent player movement clicks." => "开关持续瞬移移动；之后点击移动时立即传送",
        "Set selected combat objects to level 2, HP 1000000, attack 1000000 and mind-control immunity." => "选中对象设为精英，生命和攻击均为 1000000，并免疫心灵控制",
        "Toggle selected objects' extra mind-control immunity; 0 blocks capture, 1 restores original rules." => "设置选中对象的心控免疫；无参数逐个切换，0 禁止心控，1 恢复原版规则；先天免疫保留",
        "Updated extra mind-control immunity for {count} selected objects; original restrictions remain." => "已更新 {count} 个选中对象的额外心控免疫；原版限制保留。",
        "Set live speed: 1..6 timed, 7=120, 8=240, 9=unlimited logic FPS." => "设置本局速度：1–6 原有档位，7/8 为 120/240 逻辑帧，9 不限速",
        "Set absolute original veterancy, including buildings; fractional values are accepted." => "设置选中对象经验，支持小数；1 为老兵，2 为精英，上限按当前规则",
        "Override per-object outgoing damage before victim defenses; reset restores normal damage." => "设置选中对象攻击伤害，目标防御继续生效；reset 恢复正常计算",
        "Selection: 1=default, 2=all non-neutral units, 3=include neutral; no building bandbox." => "选择范围：1 默认，2 包含其他阵营，3 包含中立；不框选建筑",
        "Control: 1=default, 2=all non-neutral objects, 3=include neutral; selection is independent." => "控制范围：1 默认，2 包含其他阵营，3 包含中立；独立于选择模式",
        "Toggle the live performance overlay; 0 hides, 1 shows." => "开关左上角实时性能面板；0 关闭，1 开启",
        "Updated the XP of {count} selected objects." => "已修改 {count} 个选中对象的经验。",
        "Updated the attack of {count} selected objects." => "已修改 {count} 个选中对象的攻击设置。",
        "Applied god attributes to {count} selected objects." => "已为 {count} 个选中对象设置精英、1000000 生命和攻击，并免疫心灵控制。",
        _ => text
    };

    private void ReplaceExecution(Dictionary execution)
    {
        LastExecution.Dispose();
        LastExecution = execution;
    }

    private void ClearOutput()
    {
        _lines.Clear();
        _output.Text = "";
        LastResult = "";
        ReplaceExecution(new Dictionary());
    }

    private void Append(string line)
    {
        _lines.Add(line);
        if (_lines.Count > OutputLimit) _lines.RemoveAt(0);
        _output.Text = string.Join("\n", _lines);
        _output.ScrollToLine(_output.GetLineCount() - 1);
    }

    private void BrowseHistory(int direction)
    {
        if (_commands.Count == 0) return;
        if (direction < 0 && _cursor == _commands.Count) _draft = _input.Text;
        _cursor = Math.Clamp(_cursor + direction, 0, _commands.Count);
        _input.Text = _cursor == _commands.Count ? _draft : _commands[_cursor];
        _input.CaretColumn = _input.Text.Length;
    }

    public override void _ExitTree()
    {
        EndSession();
        LastExecution.Dispose();
    }
}
