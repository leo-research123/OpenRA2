using Godot;
using Godot.Collections;

public partial class App : Control
{
    public enum Screen { Main, Settings, MapSelection, Skirmish, Game }

    [Export] public string DisplayConfigPath { get; set; } = "user://display.cfg";
    [Export] public string AudioConfigPath { get; set; } = "user://audio.cfg";
    public AudioConfig AudioConfig { get; } = new();
    [Export] public string GameConfigPath { get; set; } = "user://game.cfg";
    [Export] public string HotkeyConfigPath { get; set; } = "user://keyboard.cfg";
    public GameConfig GameConfig { get; } = new();
    [Export] public string GameDataPath { get; set; } = "";
    [Export] public string ResourceConfigPath { get; set; } = "user://resources.cfg";
    public string ActiveResourceDirectory { get; private set; } = "";
    public DisplayService DisplayService { get; private set; } = null!;
    public Screen CurrentScreen { get; private set; } = Screen.Main;
    public string CoreVersion { get; private set; } = "";
    public bool BootReady { get; private set; }
    public Dictionary ResourceProgress { get; private set; } = new();

    private RefCounted? _core;
    private bool _loadingStarted;
    private bool _finishLoadingNextFrame;
    private bool _canLoadResources;
    private bool _settingsFromGame;
    private bool _quickTestAutoStartPending;
    public bool IsQuickTest { get; private set; }
    public bool IsSkirmish { get; private set; }
    private static readonly StringName CoreClass = "RA2Core";
    private static readonly StringName GetVersion = "get_version";
    private static readonly StringName BeginLoading = "begin_resource_loading";
    private static readonly StringName GetProgress = "get_resource_progress";
    private static readonly StringName CancelLoading = "cancel_resource_loading";

    public Control MainMenu { get; private set; } = null!;
    public SettingsScreen SettingsScreen { get; private set; } = null!;
    public GameScreen GameScreen { get; private set; } = null!;
    public MapSelectionScreen MapSelectionScreen { get; private set; } = null!;
    public SkirmishLobbyScreen SkirmishLobbyScreen { get; private set; } = null!;
    public Control PauseMenu { get; private set; } = null!;
    public Button StartButton { get; private set; } = null!;
    public Button QuickTestButton { get; private set; } = null!;
    public Button SkirmishButton { get; private set; } = null!;
    public Button SettingsButton { get; private set; } = null!;
    public Button ResumeButton { get; private set; } = null!;
    public Label BootError { get; private set; } = null!;
    public Control LoadingScreen { get; private set; } = null!;
    public ProgressBar LoadingBar { get; private set; } = null!;
    private Button _resourceDirectoryButton = null!;
    private FileDialog _resourceDirectoryDialog = null!;
    private Button _restartTestButton = null!;
    private Label _quickTestError = null!;

    public override void _Ready()
    {
        MainMenu = GetNode<Control>("MainMenu");
        SettingsScreen = GetNode<SettingsScreen>("SettingsScreen");
        GameScreen = GetNode<GameScreen>("GameScreen");
        GameScreen.OptionsRequested += OnOptionsRequested;
        GameScreen.MissionClosed += OnMissionClosed;
        MapSelectionScreen = GetNode<MapSelectionScreen>("MapSelectionScreen");
        MapSelectionScreen.BackRequested += OnMainMenuPressed;
        MapSelectionScreen.MapChosen += StartChosenMap;
        SkirmishLobbyScreen = GetNode<SkirmishLobbyScreen>("SkirmishLobbyScreen");
        SkirmishLobbyScreen.BackRequested += OnMainMenuPressed;
        SkirmishLobbyScreen.StartRequested += StartSkirmish;
        PauseMenu = GetNode<Control>("PauseMenu");
        StartButton = GetNode<Button>("MainMenu/Buttons/StartButton");
        QuickTestButton = GetNode<Button>("MainMenu/Buttons/QuickTestButton");
        SkirmishButton = GetNode<Button>("MainMenu/Buttons/SkirmishButton");
        _restartTestButton = GetNode<Button>("PauseMenu/Panel/Buttons/RestartTestButton");
        _quickTestError = GetNode<Label>("MainMenu/QuickTestError");
        SettingsButton = GetNode<Button>("MainMenu/Buttons/SettingsButton");
        ResumeButton = GetNode<Button>("PauseMenu/Panel/Buttons/ResumeButton");
        BootError = GetNode<Label>("BootError");
        LoadingScreen = GetNode<Control>("LoadingScreen");
        LoadingBar = GetNode<ProgressBar>("LoadingScreen/Content/ProgressBar");
        _resourceDirectoryButton = GetNode<Button>("ResourceDirectoryButton");
        _resourceDirectoryDialog = GetNode<FileDialog>("ResourceDirectoryDialog");
        _resourceDirectoryButton.Pressed += () => _resourceDirectoryDialog.PopupCenteredRatio(0.7f);
        _resourceDirectoryDialog.DirSelected += UseResourceDirectory;
        var displayOverride = AutomationRunner.Argument("--display-config=");
        if (displayOverride.Length > 0) DisplayConfigPath = displayOverride;
        var audioOverride = AutomationRunner.Argument("--audio-config=");
        if (audioOverride.Length > 0) AudioConfigPath = audioOverride;
        var gameOverride = AutomationRunner.Argument("--game-config=");
        if (gameOverride.Length > 0) GameConfigPath = gameOverride;
        var hotkeyOverride = AutomationRunner.Argument("--hotkey-config=");
        if (hotkeyOverride.Length > 0) HotkeyConfigPath = hotkeyOverride;
        var resourceOverride = AutomationRunner.Argument("--resource-config=");
        if (resourceOverride.Length > 0) ResourceConfigPath = resourceOverride;
        AutomationRunner.Attach(this);
        _quickTestAutoStartPending = System.Array.IndexOf(OS.GetCmdlineUserArgs(), "--quick-test") >= 0;
        SetProcess(false);
        StartButton.Disabled = true;
        QuickTestButton.Disabled = true;
        SkirmishButton.Disabled = true;
        SettingsButton.Disabled = true;

        // Dynamic lookup keeps missing native libraries on the visible boot-error path.
        if (!ClassDB.ClassExists(CoreClass) && FileAccess.FileExists("res://ra2_core.gdextension"))
            GDExtensionManager.LoadExtension("res://ra2_core.gdextension");
        if (!ClassDB.ClassExists(CoreClass))
        {
            FailBoot("无法加载 C++ 核心。\n请先按项目 README 编译原生库，然后重新启动。");
            return;
        }
        using var instance = ClassDB.Instantiate(CoreClass);
        _core = instance.AsGodotObject() as RefCounted;
        if (_core == null || !_core.HasMethod(GetVersion))
        {
            FailBoot("C++ 核心接口不可用，请重新编译原生库。");
            return;
        }
        foreach (var method in new[] { BeginLoading, GetProgress, CancelLoading,
            new StringName("set_audio_volumes"), new StringName("set_game_speed_level") })
        {
            if (!_core.HasMethod(method))
            {
                FailBoot("C++ 资源接口不可用，请重新编译原生库。");
                return;
            }
        }
        using var version = _core.Call(GetVersion);
        if (version.VariantType != Variant.Type.String || string.IsNullOrEmpty(version.AsString()))
        {
            FailBoot("C++ 核心返回了无效版本号。");
            return;
        }
        CoreVersion = version.AsString();
        GD.Print("RA2 C++ core loaded: ", CoreVersion);
        DisplayService = new DisplayService();
        AddChild(DisplayService);
        DisplayService.Initialize(DisplayConfigPath, GetNode<SubViewport>("GameScreen/WorldViewport"));
        AudioConfig.LoadFile(AudioConfigPath);
        _core.Call("set_audio_volumes", AudioConfig.Effects, AudioConfig.Speech, AudioConfig.Music).Dispose();
        GameConfig.LoadFile(GameConfigPath);
        using var speedApplied = _core.Call("set_game_speed_level", GameConfig.SpeedLevel);
        if (!speedApplied.AsBool()) { FailBoot("无法应用游戏速度设置。"); return; }
        SettingsScreen.Configure(DisplayService, AudioConfig, AudioConfigPath, GameConfig, GameConfigPath, _core);
        SettingsScreen.HotkeyConfigPath = HotkeyConfigPath;
        SettingsScreen.BackRequested += ReturnFromSettings;
        _canLoadResources = true;
        LoadingScreen.Show();
        SetProcess(true);
    }

    public void UseResourceDirectory(string directory)
    {
        if (!_canLoadResources || _core == null) return;
        _resourceDirectoryDialog.Hide();
        GameScreen.CloseGame();
        MapSelectionScreen.InvalidateCatalog();
        MapSelectionScreen.Hide();
        SkirmishLobbyScreen.InvalidateCatalog();
        SkirmishLobbyScreen.Hide();
        GetTree().Paused = false;
        CurrentScreen = Screen.Main;
        BootReady = false;
        MainMenu.Hide();
        SettingsScreen.Hide();
        GameScreen.Hide();
        PauseMenu.Hide();
        BootError.Hide();
        BootError.Text = "";
        StartButton.Disabled = true;
        QuickTestButton.Disabled = true;
        SkirmishButton.Disabled = true;
        IsQuickTest = false;
        _restartTestButton.Hide();
        _quickTestError.Hide();
        SettingsButton.Disabled = true;
        _resourceDirectoryButton.Hide();
        LoadingBar.Value = 0;
        LoadingScreen.Show();
        ActiveResourceDirectory = ResourceConfig.Normalize(directory);
        _resourceDirectoryDialog.CurrentDir = ActiveResourceDirectory;
        _finishLoadingNextFrame = false;
        _loadingStarted = true;
        SetProcess(true);
        GD.Print("Resource directory: ", ActiveResourceDirectory);
        _core.Call(BeginLoading, ActiveResourceDirectory);
    }

    private Variant ProgressValue(string key, Variant fallback) =>
        ResourceProgress.TryGetValue(key, out var value) ? value : fallback;

    public override void _Process(double delta)
    {
        if (_finishLoadingNextFrame)
        {
            if (ProgressValue("mounted", default).AsStringArray().Length == 0)
            {
                FailBoot($"未找到游戏资源。\n当前目录：{ActiveResourceDirectory}\n\n请选择红色警戒 2／尤里的复仇资源所在目录。");
                return;
            }
            if (ResourceConfig.Save(ResourceConfigPath, ActiveResourceDirectory) != Error.Ok)
                GD.PushWarning("无法记住游戏资源目录：" + ResourceConfigPath);
            BootReady = true;
            SettingsScreen.LoadHotkeys();
            LoadingScreen.Hide();
            StartButton.Disabled = false;
            QuickTestButton.Disabled = false;
            SkirmishButton.Disabled = false;
            SettingsButton.Disabled = false;
            SetProcess(false);
            ShowScreen(Screen.Main);
            GD.Print("Resource bootstrap complete: ", ProgressValue("mounted", default));
            if (_quickTestAutoStartPending)
            {
                _quickTestAutoStartPending = false;
                OnQuickTestPressed();
            }
            return;
        }
        if (!_loadingStarted)
        {
            UseResourceDirectory(ResourceConfig.Resolve(GameDataPath, ResourceConfigPath));
            return;
        }
        var previous = ResourceProgress;
        ResourceProgress = _core!.Call(GetProgress).AsGodotDictionary();
        previous.Dispose();
        LoadingBar.MaxValue = ProgressValue("total", 108).AsDouble();
        LoadingBar.Value = ProgressValue("completed", 0).AsDouble();
        switch (ProgressValue("state", "failed").AsString())
        {
            case "complete":
                _finishLoadingNextFrame = true;
                break;
            case "failed":
            case "cancelled":
                FailBoot($"资源加载失败。\n{ProgressValue("error", "加载已取消").AsString()}\n\n" +
                         $"当前目录：{ActiveResourceDirectory}\n请选择正确的游戏资源目录。");
                break;
        }
    }

    private void FailBoot(string message)
    {
        MapSelectionScreen.CancelScan();
        MapSelectionScreen.Hide();
        SkirmishLobbyScreen.CancelScan();
        SkirmishLobbyScreen.Hide();
        BootReady = false;
        SetProcess(false);
        LoadingScreen.Hide();
        MainMenu.Hide();
        SettingsScreen.Hide();
        GameScreen.Hide();
        PauseMenu.Hide();
        StartButton.Disabled = true;
        QuickTestButton.Disabled = true;
        SkirmishButton.Disabled = true;
        SettingsButton.Disabled = true;
        BootError.Text = message;
        BootError.Show();
        _resourceDirectoryButton.Visible = _canLoadResources;
        GD.PushError(message);
    }

    private void ShowScreen(Screen screen)
    {
        if (!BootReady)
            return;
        if (CurrentScreen == Screen.Game && screen != Screen.Game)
        {
            GameScreen.CloseGame();
            IsQuickTest = false;
            _restartTestButton.Hide();
        }
        if (CurrentScreen == Screen.MapSelection && screen != Screen.MapSelection) MapSelectionScreen.CancelScan();
        if (CurrentScreen == Screen.Skirmish && screen != Screen.Skirmish) SkirmishLobbyScreen.CancelScan();
        GetTree().Paused = false;
        PauseMenu.Hide();
        CurrentScreen = screen;
        MainMenu.Visible = screen == Screen.Main;
        SettingsScreen.Visible = screen == Screen.Settings;
        GameScreen.Visible = screen == Screen.Game;
        MapSelectionScreen.Visible = screen == Screen.MapSelection;
        SkirmishLobbyScreen.Visible = screen == Screen.Skirmish;
        _resourceDirectoryButton.Visible = screen == Screen.Main;
        if (screen == Screen.Main)
            StartButton.GrabFocus();
        else
            GetViewport().GuiGetFocusOwner()?.ReleaseFocus();
        if (screen == Screen.Settings)
            SettingsScreen.Present();
    }

    public void OnStartPressed()
    {
        if (!BootReady || _core == null || CurrentScreen != Screen.Main) return;
        var filename = AutomationRunner.Argument("--map=").Trim();
        if (filename.Length > 0)
        {
            StartMap(filename);
            return;
        }
        ShowMapSelection();
    }

    private void ShowMapSelection()
    {
        if (!BootReady || _core == null) return;
        ShowScreen(Screen.MapSelection);
        MapSelectionScreen.Present(_core, ActiveResourceDirectory);
    }

    public void OnSkirmishPressed()
    {
        if (!BootReady || _core == null || CurrentScreen != Screen.Main) return;
        ShowScreen(Screen.Skirmish);
        SkirmishLobbyScreen.Present(_core, ActiveResourceDirectory);
    }

    private void OnMissionClosed()
    {
        if (CurrentScreen != Screen.Game) return;
        if (IsSkirmish) ReturnToSkirmish();
        else ShowMapSelection();
    }

    private void StartSkirmish(string filename, Dictionary options)
    {
        if (!BootReady || _core == null || CurrentScreen != Screen.Skirmish) return;
        _quickTestError.Hide();
        IsQuickTest = false;
        IsSkirmish = true;
        _restartTestButton.Hide();
        ShowScreen(Screen.Game);
        GameScreen.BeginGame(_core, filename, ActiveResourceDirectory, options);
    }

    private void ReturnToSkirmish()
    {
        ShowScreen(Screen.Skirmish);
        SkirmishLobbyScreen.Present(_core!, ActiveResourceDirectory);
    }

    // Explicit entry used by the picker and scripted map-loading regressions.
    public void StartMap(string filename)
    {
        if (!BootReady || _core == null || string.IsNullOrWhiteSpace(filename)) return;
        BeginMap(filename, false);
    }

    private void StartChosenMap(MapCatalog.Entry entry)
    {
        if (!BootReady || _core == null || CurrentScreen != Screen.MapSelection) return;
        if (!entry.IsTest) {
            BeginMap(entry.Filename, false, entry.IsCampaign ? MapSelectionScreen.CampaignDifficulty : -1);
            return;
        }
        if (!BundledTestMaps.Prepare(entry.Filename, out var filename, out var error))
        {
            MapSelectionScreen.ShowLoadError(error);
            return;
        }
        BeginMap(filename, true);
    }

    public void OnQuickTestPressed()
    {
        if (!BootReady || _core == null || CurrentScreen != Screen.Main) return;
        if (!QuickTestMap.Prepare(out var filename, out var error))
        {
            _quickTestError.Text = error;
            _quickTestError.Show();
            return;
        }
        BeginMap(filename, true);
    }

    public void OnRestartTestPressed()
    {
        if (!BootReady || _core == null || CurrentScreen != Screen.Game || !IsQuickTest) return;
        var filename = GameScreen.MapFilename;
        GameScreen.CloseGame();
        BeginMap(filename, true);
    }

    private void BeginMap(string filename, bool quickTest, int campaignDifficulty = -1)
    {
        _quickTestError.Hide();
        IsQuickTest = quickTest;
        IsSkirmish = false;
        _restartTestButton.Visible = quickTest;
        ShowScreen(Screen.Game);
        GameScreen.BeginGame(_core!, filename, ActiveResourceDirectory, campaignDifficulty: campaignDifficulty);
    }
    public void OnSettingsPressed()
    {
        if (!BootReady) return;
        _settingsFromGame = CurrentScreen == Screen.Game;
        if (!_settingsFromGame) { ShowScreen(Screen.Settings); return; }
        // Keep the loaded world paused while its host preferences are edited.
        GetTree().Paused = true;
        PauseMenu.Hide(); GameScreen.Hide();
        CurrentScreen = Screen.Settings;
        SettingsScreen.Show(); SettingsScreen.Present();
    }
    private void ReturnFromSettings()
    {
        if (!_settingsFromGame) { ShowScreen(Screen.Main); return; }
        _settingsFromGame = false;
        SettingsScreen.Hide(); GameScreen.Show(); PauseMenu.Show();
        CurrentScreen = Screen.Game;
        ResumeButton.GrabFocus();
    }
    public void OnQuitPressed() => GetTree().Quit();
    public void OnMainMenuPressed() => ShowScreen(Screen.Main);

    public void OnResumePressed()
    {
        PauseMenu.Hide();
        GetTree().Paused = false;
    }

    public void OnOptionsRequested()
    {
        if (CurrentScreen != Screen.Game || PauseMenu.Visible) return;
        GameScreen.ResetMapInput();
        PauseMenu.Show(); GetTree().Paused = true;
        ResumeButton.GrabFocus();
    }

    public override void _UnhandledKeyInput(InputEvent @event)
    {
        if (!BootReady || !@event.IsActionPressed("ui_cancel"))
            return;
        switch (CurrentScreen)
        {
            case Screen.Settings:
                ReturnFromSettings();
                break;
            case Screen.MapSelection:
            case Screen.Skirmish:
                ShowScreen(Screen.Main);
                break;
            case Screen.Game:
                if (PauseMenu.Visible)
                    OnResumePressed();
                else
                {
                    using var status = GameScreen.GetMapStatus();
                    var state = status.TryGetValue("state", out var value) ? value.AsString() : "empty";
                    if (state == "failed" || state == "cancelled") {
                        if (IsSkirmish) ReturnToSkirmish();
                        else OnMainMenuPressed();
                    }
                }
                break;
        }
        GetViewport().SetInputAsHandled();
    }

    public override void _ExitTree()
    {
        MapSelectionScreen.CancelScan();
        SkirmishLobbyScreen.CancelScan();
        if (_core != null)
        {
            if (_core.HasMethod(CancelLoading))
                _core.Call(CancelLoading);
            _core.Dispose();
            _core = null;
        }
        ResourceProgress.Dispose();
        GetTree().Paused = false;
    }
}
