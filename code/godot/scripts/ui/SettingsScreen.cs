using Godot;
using System.Collections.Generic;
using System;

/// <summary>Edits a draft; applying persists it, returning discards unapplied changes.</summary>
public partial class SettingsScreen : Control
{
    [Signal] public delegate void BackRequestedEventHandler();

    private DisplayService _display = null!;
    private AudioConfig _audio = null!;
    private RefCounted _core = null!;
    private string _audioConfigPath = "";
    private GameConfig _game = null!;
    private string _gameConfigPath = "";
    public string HotkeyConfigPath { get; set; } = "user://keyboard.cfg";
    private Godot.Collections.Dictionary _hotkeyDraft = new();
    private Godot.Collections.Array _hotkeyRows = new();
    private readonly Dictionary<string, Button> _hotkeyButtons = new();
    private VBoxContainer _hotkeyList = null!;
    private TabBar _tabs = null!;
    private TabContainer _pages = null!;
    private Label _detailTitle = null!;
    private Label _detailDescription = null!;
    private Label _detailValue = null!;
    private StyleBoxFlat _normalRowStyle = null!;
    private StyleBoxFlat _selectedRowStyle = null!;
    private PanelContainer? _selectedRow;
    private readonly Dictionary<string, PanelContainer> _hotkeyPanels = new();
    private readonly Dictionary<string, int> _defaultKeys = new();
    private Label _hotkeyHint = null!;
    private Button _cancelCapture = null!;
    private string _capturing = "";
    private List<Vector2I> _resolutions = new();
    public OptionButton ResolutionOption { get; private set; } = null!;
    public OptionButton ModeOption { get; private set; } = null!;
    public Label Status { get; private set; } = null!;
    public HSlider MusicSlider { get; private set; } = null!;
    public HSlider EffectsSlider { get; private set; } = null!;
    public HSlider SpeechSlider { get; private set; } = null!;
    public HSlider SpeedSlider { get; private set; } = null!;

    public override void _Ready()
    {
        ResolutionOption = GetNode<OptionButton>("%ResolutionOption");
        ModeOption = GetNode<OptionButton>("%ModeOption");
        Status = GetNode<Label>("%Status");
        MusicSlider = GetNode<HSlider>("%MusicSlider");
        EffectsSlider = GetNode<HSlider>("%EffectsSlider");
        SpeechSlider = GetNode<HSlider>("%SpeechSlider");
        SpeedSlider = GetNode<HSlider>("%SpeedSlider");
        _hotkeyList = GetNode<VBoxContainer>("%HotkeyList");
        _tabs = GetNode<TabBar>("%SettingsTabs");
        _pages = GetNode<TabContainer>("%SettingsPages");
        _detailTitle = GetNode<Label>("%DetailTitle");
        _detailDescription = GetNode<Label>("%DetailDescription");
        _detailValue = GetNode<Label>("%DetailValue");
        _hotkeyHint = GetNode<Label>("%HotkeyHint");
        _cancelCapture = GetNode<Button>("%CancelCaptureButton");
        _cancelCapture.Pressed += CancelCapture;
        GetNode<Button>("%DefaultHotkeysButton").Pressed += RestoreDefaultHotkeys;
        _normalRowStyle = new StyleBoxFlat { BgColor = Colors.Transparent,
            BorderWidthBottom = 1, BorderColor = new Color(0.22f, 0.27f, 0.31f),
            ContentMarginLeft = 12, ContentMarginRight = 12, ContentMarginTop = 2, ContentMarginBottom = 2 };
        _selectedRowStyle = (StyleBoxFlat)_normalRowStyle.Duplicate();
        _selectedRowStyle.BgColor = new Color(0.13f, 0.17f, 0.21f);
        _selectedRowStyle.BorderWidthLeft = 2;
        _selectedRowStyle.BorderColor = new Color(0.95f, 0.58f, 0.31f);
        BindSetting("ResolutionRow", ResolutionOption, "分辨率", "选择游戏画面的绘制分辨率。全屏时文字仍保持清晰；窗口超过屏幕可用区域时会自动缩小。");
        BindSetting("ModeRow", ModeOption, "显示模式", "选择窗口化、全屏窗口化或全屏。点击下方“应用”后生效。");
        BindSetting("SpeedRow", SpeedSlider, "游戏速度", "调整游戏逻辑推进的速度。向右提高速度；120／240 表示目标逻辑帧率，实际速度取决于机器性能。");
        BindSetting("MusicRow", MusicSlider, "音乐音量", "调整背景音乐的音量。设为 0% 可关闭音乐。");
        BindSetting("EffectsRow", EffectsSlider, "音效音量", "调整武器、爆炸和环境音效的音量。");
        BindSetting("SpeechRow", SpeechSlider, "语音音量", "调整单位回应和游戏提示语音的音量。");
        _tabs.AddTab("画面"); _tabs.AddTab("声音"); _tabs.AddTab("键盘");
        _tabs.TabChanged += SelectTab;
        SelectTab(0);
        Resized += UpdateLayout;
        VisibilityChanged += () => { if (!IsVisibleInTree()) CancelCapture(); };
        UpdateLayout();
        SpeedSlider.ValueChanged += value => GetNode<Label>("%SpeedValue").Text = GameConfig.SpeedLabel((int)(value + 0.5));
        MusicSlider.ValueChanged += value => GetNode<Label>("%MusicValue").Text = $"{value:0}%";
        EffectsSlider.ValueChanged += value => GetNode<Label>("%EffectsValue").Text = $"{value:0}%";
        SpeechSlider.ValueChanged += value => GetNode<Label>("%SpeechValue").Text = $"{value:0}%";
    }

    public void Configure(DisplayService display, AudioConfig audio, string audioConfigPath,
        GameConfig game, string gameConfigPath, RefCounted core)
    {
        _display = display;
        _audio = audio; _audioConfigPath = audioConfigPath; _core = core;
        _game = game; _gameConfigPath = gameConfigPath;
        ModeOption.Clear();
        ModeOption.AddItem("窗口化", 0);
        ModeOption.AddItem("全屏窗口化", 1);
        ModeOption.AddItem("全屏", 2);
    }

    public void Present()
    {
        RefreshHotkeys();
        _resolutions = _display.Resolutions();
        ResolutionOption.Clear();
        foreach (var resolution in _resolutions)
            ResolutionOption.AddItem($"{resolution.X} × {resolution.Y}");
        ResolutionOption.Select(_resolutions.IndexOf(_display.Config.Resolution));
        ModeOption.Select((int)_display.Config.WindowMode);
        MusicSlider.Value = _audio.Music * 100;
        EffectsSlider.Value = _audio.Effects * 100;
        SpeechSlider.Value = _audio.Speech * 100;
        using var mapStatus = _core.Call("get_map_status").AsGodotDictionary();
        var currentSpeed = mapStatus.TryGetValue("state", out var state) && state.AsString() == "ready"
            && mapStatus.TryGetValue("speed_level", out var speed) ? (int)speed.AsInt64() : _game.SpeedLevel;
        SpeedSlider.Value = currentSpeed;
        GetNode<Label>("%SpeedValue").Text = GameConfig.SpeedLabel(currentSpeed);
        Status.Text = "调整后点击应用；返回将放弃未应用的修改。";
        _tabs.CurrentTab = 0;
        SelectTab(0);
        ResolutionOption.GrabFocus();
    }

    public void OnApplyPressed()
    {
        CancelCapture();
        using var hotkeysApplied = _core.Call("set_hotkeys", _hotkeyDraft);
        if (!hotkeysApplied.AsBool())
        {
            _tabs.CurrentTab = 2;
            Status.Text = "快捷键重复或无效，请先取消冲突绑定。"; return;
        }
        var hotkeyError = SaveHotkeys();
        using var applied = _core.Call("set_audio_volumes", EffectsSlider.Value / 100,
            SpeechSlider.Value / 100, MusicSlider.Value / 100);
        if (!applied.AsBool()) { Status.Text = "无法应用音量设置，请稍后重试。"; return; }
        _audio.Music = MusicSlider.Value / 100;
        _audio.Effects = EffectsSlider.Value / 100;
        _audio.Speech = SpeechSlider.Value / 100;
        var audioError = _audio.SaveFile(_audioConfigPath);
        // Range values may be just below an integer after slider snapping.
        var speedLevel = (int)(SpeedSlider.Value + 0.5);
        using var speedApplied = _core.Call("set_game_speed_level", speedLevel);
        if (!speedApplied.AsBool()) { Status.Text = "音量已应用，但无法应用游戏速度，请稍后重试。"; return; }
        _game.SpeedLevel = speedLevel;
        var gameError = _game.SaveFile(_gameConfigPath);
        var error = _display.ApplySelection(
            _resolutions[ResolutionOption.Selected], ModeOption.GetSelectedId());
        var saveError = error != Error.Ok ? error : audioError != Error.Ok ? audioError : gameError != Error.Ok ? gameError : hotkeyError;
        Status.Text = saveError != Error.Ok
            ? $"设置已应用，但保存失败（{saveError}）。"
            : "显示、音量、游戏速度与快捷键已应用并保存。";
    }

    public void OnBackPressed()
    {
        CancelCapture();
        EmitSignal(SignalName.BackRequested);
    }

    private void RefreshHotkeys()
    {
        CancelCapture();
        _hotkeyRows.Dispose();
        _hotkeyRows = _core.Call("get_hotkeys").AsGodotArray();
        _hotkeyDraft.Dispose(); _hotkeyDraft = new();
        foreach (var entry in _hotkeyRows)
        {
            using var row = entry.AsGodotDictionary();
            _hotkeyDraft[row["command"]] = row["key"];
        }
        BuildHotkeyList();
    }

    public void LoadHotkeys()
    {
        using var config = new ConfigFile();
        if (config.Load(HotkeyConfigPath) != Error.Ok) return;
        using var bindings = new Godot.Collections.Dictionary();
        foreach (var key in config.GetSectionKeys("Hotkey"))
            bindings[key] = config.GetValue("Hotkey", key, 0);
        using var applied = _core.Call("set_hotkeys", bindings);
        if (!applied.AsBool()) GD.PushWarning("快捷键配置有冲突或无效值，继续使用当前绑定。");
    }

    private Error SaveHotkeys()
    {
        using var config = new ConfigFile();
        foreach (var key in _hotkeyDraft.Keys)
            config.SetValue("Hotkey", key.AsString(), _hotkeyDraft[key]);
        return config.Save(HotkeyConfigPath);
    }

    private void UpdateLayout()
    {
        GetNode<Control>("%Details").Visible = Size.X >= 900;
        GetNode<Control>("%DetailsDivider").Visible = Size.X >= 900;
    }

    private void SelectTab(long tab)
    {
        CancelCapture();
        _pages.CurrentTab = (int)tab;
        GetNode<Button>("%DefaultHotkeysButton").Visible = tab == 2;
        if (tab == 2)
            ShowDetails(null, "键盘设置", "为下列指令设置快捷键。点击当前按键后，按下新的按键或 Shift／Ctrl／Alt 组合键。", "清除：取消该指令的快捷键。\n恢复默认：还原原版默认按键。\n修改后点击“应用”保存。");
        else if (tab == 1)
            ShowDetails(null, "声音设置", "分别调整音乐、音效和单位语音的音量。", "选择左侧选项查看说明。\n修改后点击“应用”保存。");
        else
            ShowDetails(null, "画面设置", "调整游戏分辨率、显示模式与游戏速度。", "选择左侧选项查看说明。\n修改后点击“应用”保存。");
    }

    private void BindSetting(string rowName, Control input, string title, string description)
    {
        var row = GetNode<PanelContainer>("%" + rowName);
        row.AddThemeStyleboxOverride("panel", _normalRowStyle);
        row.TooltipText = description;
        Action select = () => { if (_capturing.Length == 0) ShowDetails(row, title, description, "修改后点击“应用”保存。"); };
        row.MouseEntered += select;
        input.MouseEntered += select;
        input.FocusEntered += select;
    }

    private void ShowDetails(PanelContainer? row, string title, string description, string value)
    {
        if (_selectedRow != null && GodotObject.IsInstanceValid(_selectedRow))
            _selectedRow.AddThemeStyleboxOverride("panel", _normalRowStyle);
        _selectedRow = row;
        row?.AddThemeStyleboxOverride("panel", _selectedRowStyle);
        _detailTitle.Text = title; _detailDescription.Text = description; _detailValue.Text = value;
    }

    private void ShowCommandDetails(string command)
    {
        var text = CommandText(command, command);
        ShowDetails(_hotkeyPanels[command], text.Name, text.Description,
            $"当前绑定    {KeyLabel(_hotkeyDraft[command].AsInt32())}\n默认按键    {KeyLabel(_defaultKeys[command])}\n\n点击左侧按键修改；点击“清除”取消绑定。");
    }

    private static (string Name, string Description) CommandText(string command, string fallback) => command switch
    {
        "Options" => ("游戏菜单", "暂停游戏并打开菜单"),
        "ToggleSell" => ("出售", "切换建筑出售模式"),
        "ToggleRepair" => ("修理", "切换建筑修理模式"),
        "ScatterObject" => ("散开", "让选中单位散开"),
        "GuardObject" => ("区域警戒", "让选中单位在附近警戒"),
        "StopObject" => ("停止", "停止选中单位的行动"),
        "DeployObject" => ("部署／展开", "执行选中单位的部署操作"),
        "TypeSelect" => ("同类选择", "选择同类型的单位"),
        "StructureTab" => ("建筑栏", "切换到建筑生产栏"),
        "DefenseTab" => ("防御栏", "切换到防御生产栏"),
        "UnitTab" => ("载具栏", "切换到载具生产栏"),
        "InfantryTab" => ("步兵栏", "切换到步兵生产栏"),
        _ => (fallback, "")
    };

    private void BuildHotkeyList()
    {
        if (_selectedRow != null && GodotObject.IsInstanceValid(_selectedRow))
            _selectedRow.AddThemeStyleboxOverride("panel", _normalRowStyle);
        _selectedRow = null;
        foreach (var child in _hotkeyList.GetChildren()) { _hotkeyList.RemoveChild(child); child.QueueFree(); }
        _hotkeyButtons.Clear(); _hotkeyPanels.Clear(); _defaultKeys.Clear();
        GetNode<Label>("%HotkeyTitle").Text = $"指令 · {_hotkeyRows.Count} 项";
        foreach (var entry in _hotkeyRows)
        {
            using var row = entry.AsGodotDictionary();
            var command = row["command"].AsString();
            var text = CommandText(command, row["name"].AsString());
            var card = new PanelContainer { Name = command, SizeFlagsHorizontal = SizeFlags.ExpandFill,
                TooltipText = text.Description };
            card.AddThemeStyleboxOverride("panel", _normalRowStyle);
            _hotkeyList.AddChild(card); _hotkeyPanels[command] = card;
            _defaultKeys[command] = row["default_key"].AsInt32();
            var controls = new HBoxContainer(); controls.AddThemeConstantOverride("separation", 8); card.AddChild(controls);
            var label = new Label { Text = text.Name, SizeFlagsHorizontal = SizeFlags.ExpandFill };
            label.AddThemeFontSizeOverride("font_size", 18); controls.AddChild(label);
            var button = new Button { Name = "Binding", Text = KeyLabel(_hotkeyDraft[command].AsInt32()),
                CustomMinimumSize = new Vector2(140, 28), TooltipText = "点击后按下新的快捷键，可组合 Shift／Ctrl／Alt。" };
            button.AddThemeFontSizeOverride("font_size", 16);
            button.Pressed += () => {
                CancelCapture(); ShowCommandDetails(command); _capturing = command; _cancelCapture.Show();
                _hotkeyHint.Text = $"正在设置“{text.Name}”：请按新快捷键，Esc 也可绑定。";
                button.Text = "等待按键…";
            };
            var defaultKey = new Label { Name = "DefaultKey", Text = KeyLabel(_defaultKeys[command]),
                CustomMinimumSize = new Vector2(64, 0), HorizontalAlignment = HorizontalAlignment.Center };
            defaultKey.AddThemeFontSizeOverride("font_size", 16);
            defaultKey.AddThemeColorOverride("font_color", new Color(0.66f, 0.71f, 0.77f));
            controls.AddChild(defaultKey);
            controls.AddChild(button); _hotkeyButtons[command] = button;
            var clear = new Button { Text = "清除", CustomMinimumSize = new Vector2(60, 28), TooltipText = "清除此项快捷键" };
            clear.AddThemeFontSizeOverride("font_size", 14);
            clear.Pressed += () => SetDraftKey(command, 0);
            controls.AddChild(clear);
            Action select = () => { if (_capturing.Length == 0) ShowCommandDetails(command); };
            card.MouseEntered += select;
            button.MouseEntered += select; button.FocusEntered += select;
            clear.MouseEntered += select; clear.FocusEntered += select;
        }
        UpdateHotkeyFeedback();
    }

    private void RestoreDefaultHotkeys()
    {
        CancelCapture();
        foreach (var entry in _hotkeyRows)
        {
            using var row = entry.AsGodotDictionary();
            _hotkeyDraft[row["command"]] = row["default_key"];
        }
        RefreshHotkeyButtons(); UpdateHotkeyFeedback();
        SelectTab(2);
        Status.Text = "已恢复默认快捷键，点击“应用”保存。";
    }

    private void RefreshHotkeyButtons()
    {
        foreach (var pair in _hotkeyButtons) pair.Value.Text = KeyLabel(_hotkeyDraft[pair.Key].AsInt32());
    }

    private void CancelCapture()
    {
        _capturing = ""; _cancelCapture.Hide();
        RefreshHotkeyButtons(); UpdateHotkeyFeedback();
    }

    public override void _Input(InputEvent input)
    {
        if (!IsVisibleInTree() || _tabs.CurrentTab != 2 || _capturing.Length == 0 || input is not InputEventKey key) return;
        // Consume key releases and repeats too, so capture never activates menu navigation.
        GetViewport().SetInputAsHandled();
        if (!key.Pressed || key.Echo) return;
        using var encoded = _core.Call("encode_hotkey", input);
        var value = encoded.AsInt32();
        if ((value & 0xFF) == 0xC0)
            _hotkeyHint.Text = "此键用于打开控制台，请选择其他按键。";
        else if (value != 0) SetDraftKey(_capturing, value);
    }

    private void SetDraftKey(string command, int key)
    {
        _capturing = ""; _cancelCapture.Hide(); _hotkeyDraft[command] = key;
        RefreshHotkeyButtons(); UpdateHotkeyFeedback();
        ShowCommandDetails(command);
        Status.Text = "快捷键已修改，点击“应用”保存；返回将放弃未应用的修改。";
    }

    private void UpdateHotkeyFeedback()
    {
        bool conflict = false;
        foreach (var pair in _hotkeyButtons)
        {
            var key = _hotkeyDraft[pair.Key].AsInt32();
            bool duplicate = false;
            foreach (var other in _hotkeyDraft.Keys)
                if (key != 0 && other.AsString() != pair.Key && _hotkeyDraft[other].AsInt32() == key) duplicate = true;
            pair.Value.Modulate = duplicate ? new Color(1, 0.55f, 0.45f) : Colors.White;
            conflict |= duplicate;
        }
        _hotkeyHint.Text = conflict ? "标红的按键重复，请修改或清除其中一项后再应用。"
            : "点击当前按键修改，支持 Shift／Ctrl／Alt 组合。";
    }

    public static string KeyLabel(int key)
    {
        if (key == 0) return "未绑定";
        var code = key & 0xFF;
        var name = code switch { 27 => "Esc", 13 => "Enter", 9 => "Tab", 8 => "Backspace",
            32 => "Space", 33 => "PageUp", 34 => "PageDown", 35 => "End", 36 => "Home",
            37 => "←", 38 => "↑", 39 => "→", 40 => "↓", 45 => "Insert", 46 => "Delete",
            0xBA => ";", 0xBB => "=", 0xBC => ",", 0xBD => "-", 0xBE => ".", 0xBF => "/",
            0xC0 => "`", 0xDB => "[", 0xDC => "\\", 0xDD => "]", 0xDE => "'",
            >= 112 and <= 135 => $"F{code - 111}", >= 48 and <= 90 => ((char)code).ToString(), _ => $"0x{code:X2}" };
        return ((key & 0x100) != 0 ? "Shift+" : "") + ((key & 0x200) != 0 ? "Ctrl+" : "") + ((key & 0x400) != 0 ? "Alt+" : "") + name;
    }
}
