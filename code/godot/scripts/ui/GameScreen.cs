using Godot;
using Godot.Collections;

public partial class GameScreen : Control
{
    [Signal] public delegate void OptionsRequestedEventHandler();
    [Signal] public delegate void MissionClosedEventHandler();
    private ConfirmationDialog _resultDialog = null!;
    private bool _missionFinished;
    private bool _canContinueMission;
    private bool _transitionLoading;
    private long _missionResultId;
    private string _missionTarget = "";
    private Node2D? _map;
    private Label _status = null!;
    private PanelContainer _performancePanel = null!;
    private Label _performanceText = null!;
    private ulong _nextPerformanceUpdate;
    private bool _active;
    private string _resourceDirectory = "";
    private string _filename = "";
    private bool _skirmish;
    public string MapFilename => _filename;
    private string _reportedError = "";
    private TextureRect _surface = null!;
    private RefCounted? _core;
    public GameConsole Console { get; private set; } = null!;
    internal RefCounted? Core => _core;
    internal bool PerformanceVisible => _performancePanel.IsVisibleInTree();
    internal string PerformanceText => _performanceText.Text;

    public override void _Ready()
    {
        _status = GetNode<Label>("Status");
        _surface = GetNode<TextureRect>("WorldSurface");
        Console = GetNode<GameConsole>("Console");
        _performancePanel = new PanelContainer {
            Name = "PerformanceOverlay", MouseFilter = MouseFilterEnum.Ignore, Visible = false,
            Position = new Vector2(12, 12)
        };
        _performancePanel.AddThemeStyleboxOverride("panel", new StyleBoxFlat {
            BgColor = new Color(0.035f, 0.05f, 0.07f, 0.18f),
            BorderColor = new Color(0.65f, 0.75f, 0.85f, 0.06f),
            BorderWidthLeft = 1, BorderWidthRight = 1, BorderWidthTop = 1, BorderWidthBottom = 1,
            CornerRadiusTopLeft = 4, CornerRadiusTopRight = 4, CornerRadiusBottomLeft = 4, CornerRadiusBottomRight = 4,
            ContentMarginLeft = 10, ContentMarginRight = 10, ContentMarginTop = 7, ContentMarginBottom = 7
        });
        _performanceText = new Label { Name = "Metrics", MouseFilter = MouseFilterEnum.Ignore };
        _performanceText.AddThemeFontSizeOverride("font_size", 12);
        _performanceText.AddThemeColorOverride("font_color", new Color(0.86f, 0.92f, 0.97f, 0.65f));
        _performancePanel.AddChild(_performanceText);
        AddChild(_performancePanel);
        MoveChild(_performancePanel, Console.GetIndex());
        Console.CommandExecuted += RefreshPerformance;
        GetTree().ProcessFrame += ProcessPerformance;
        _resultDialog = new ConfirmationDialog { Name = "MissionResultDialog", Title = "任务结果", Exclusive = true };
        _resultDialog.GetCancelButton().Text = "返回";
        _resultDialog.Confirmed += OnMissionConfirmed;
        _resultDialog.Canceled += () => EmitSignal(SignalName.MissionClosed);
        AddChild(_resultDialog);
        SetProcess(false);
    }

    public override void _Input(InputEvent input)
    {
        if (!_active || _missionFinished || !IsVisibleInTree()) return;
        var wasOpen = Console.Visible;
        if (Console.HandleInput(input))
        {
            if (wasOpen != Console.Visible) ResetMapInput();
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

    public void BeginGame(RefCounted core, string filename, string resourceDirectory = "", Dictionary? skirmish = null, int campaignDifficulty = -1)
    {
        _core = core;
        _skirmish = skirmish != null;
        _performancePanel.Hide();
        _performanceText.Text = "";
        _nextPerformanceUpdate = 0;
        _missionFinished = false;
        _transitionLoading = false;
        _canContinueMission = false;
        _resultDialog.Hide();
        SetConsoleOpen(false);
        Console.BeginSession(core);
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
        // Release the temporary native reference on the scene thread, not in
        // a later GC finalizer that could otherwise destroy the audio owner.
        using (var coreArgument = Variant.From(core))
        using (var configured = _map.Call("configure", coreArgument)) { }
        if (skirmish != null) _map.Call("open_skirmish", filename, skirmish);
        else if (campaignDifficulty >= 0) _map.Call("open_campaign", filename, campaignDifficulty);
        else _map.Call("open_map", filename);
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
        if (_transitionLoading)
        {
            if (state == "ready" || state == "rendering" || state == "drawn")
            {
                _transitionLoading = false;
                _missionFinished = false;
                _filename = _missionTarget;
                if (_core != null) Console.BeginSession(_core);
            }
            else if (state == "failed" || state == "cancelled")
            {
                _transitionLoading = false;
                _canContinueMission = status.TryGetValue("mission_transition", out var failed) && failed.AsInt32() == 6;
                _resultDialog.Title = "任务加载失败";
                _resultDialog.DialogText = $"无法加载 {_missionTarget}\n{status["error"].AsString()}";
                _resultDialog.GetOkButton().Text = _canContinueMission ? "重试" : "返回";
                _resultDialog.GetCancelButton().Visible = _canContinueMission;
                _resultDialog.PopupCentered(new Vector2I(480, 200));
            }
        }
        if (!_missionFinished && status.TryGetValue("mission_result", out var result) && result.AsInt32() != 0)
        {
            _missionFinished = true;
            SetConsoleOpen(false);
            Console.EndSession();
            ResetMapInput();
            var transition = status.TryGetValue("mission_transition", out var next) ? next.AsInt32() : 0;
            _missionResultId = status.TryGetValue("mission_id", out var id) ? id.AsInt64() : 0;
            _missionTarget = status.TryGetValue("mission_target", out var target) ? target.AsString() : "";
            _canContinueMission = !_skirmish && (transition == 1 || transition == 2);
            _resultDialog.Title = _skirmish ? "对局结果" : "任务结果";
            _resultDialog.DialogText = _skirmish
                ? (result.AsInt32() == 1 ? "你胜利了！" : "你战败了。")
                : (result.AsInt32() == 1 ? "任务完成" : "任务失败，是否重新执行本次任务？");
            if (!_skirmish && transition == 3) _resultDialog.DialogText = "任务完成，战役已结束。";
            if (transition == 4 && status.TryGetValue("mission_error", out var error))
                _resultDialog.DialogText += $"\n无法继续下一关：{error.AsString()}";
            _resultDialog.GetOkButton().Text = _skirmish ? "返回大厅"
                : transition == 1 ? "下一关" : transition == 2 ? "重新执行" : "返回";
            _resultDialog.GetCancelButton().Visible = _canContinueMission;
            _resultDialog.PopupCentered(new Vector2I(360, 160));
        }
        if (status.TryGetValue("special_dialog", out var dialog) && dialog.AsInt32() == 1)
            EmitSignal(SignalName.OptionsRequested);
        if (state == "rendering" || state == "drawn") Console.BindReadyWorld();
        if (_missionFinished || state != "rendering" && state != "drawn") _performancePanel.Hide();
        if (state == "drawn")
        {
            _status.Hide();
        }
        else if (state == "failed" || state == "cancelled")
        {
            var stage = status.TryGetValue("failure_stage", out var failureStage) && failureStage.AsString() == "rendering"
                ? "地图渲染失败" : "地图加载失败";
            _status.Text = $"{stage}：{_filename}\n{status["error"].AsString()}\n资源目录：{_resourceDirectory}\n\n按 Esc 返回{(_skirmish ? "遭遇战大厅" : "主菜单")}";
            if (_reportedError != _status.Text)
            {
                GD.PushError(_status.Text);
                _reportedError = _status.Text;
            }
            _status.Show();
        }
    }

    private void OnMissionConfirmed()
    {
        if (!_canContinueMission) { EmitSignal(SignalName.MissionClosed); return; }
        if (_transitionLoading || _map == null || !GodotObject.IsInstanceValid(_map)) return;
        _canContinueMission = false;
        if (!_map.Call("continue_mission", _missionResultId).AsBool())
        {
            _resultDialog.DialogText = "任务状态已变化，无法继续。";
            _resultDialog.GetOkButton().Text = "返回";
            _resultDialog.GetCancelButton().Hide();
            _resultDialog.CallDeferred("popup_centered", new Vector2I(360, 160));
            return;
        }
        _transitionLoading = true;
        _filename = _missionTarget;
        _reportedError = "";
        _status.Text = $"正在加载任务：{_missionTarget}…";
        _status.Show();
    }

    public void CloseGame()
    {
        _performancePanel?.Hide();
        SetConsoleOpen(false);
        _resultDialog?.Hide();
        _missionFinished = false;
        _transitionLoading = false;
        _canContinueMission = false;
        Console.EndSession();
        _core = null;
        _active = false;
        SetProcess(false);
        if (_map != null && GodotObject.IsInstanceValid(_map)) _map.Call("close_map");
    }

    public override void _ExitTree()
    {
        GetTree().ProcessFrame -= ProcessPerformance;
        CloseGame();
    }

    private void ProcessPerformance()
    {
        // Keep this read-only HUD current while the game input/logic is paused.
        if (_active && IsVisibleInTree() && Time.GetTicksMsec() >= _nextPerformanceUpdate) RefreshPerformance();
    }

    private void RefreshPerformance()
    {
        _nextPerformanceUpdate = Time.GetTicksMsec() + 1000;
        if (_core == null || _missionFinished || Console.World == 0) {
            _performancePanel.Hide();
            return;
        }
        using var snapshot = _core.Call("get_debug_performance", Console.World).AsGodotDictionary();
        _performancePanel.Visible = snapshot.TryGetValue("visible", out var visible) && visible.AsBool();
        if (_performancePanel.Visible) {
            _performanceText.Text = snapshot["message"].AsString();
            if (GetTree().Paused) _performanceText.Text = _performanceText.Text.Replace("PERF · 运行", "PERF · 暂停");
            _performancePanel.ResetSize();
        }
    }

    private void SetConsoleOpen(bool open)
    {
        if (Console == null) return;
        Console.SetOpen(open);
        ResetMapInput();
    }
}
