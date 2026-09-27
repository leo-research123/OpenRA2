using Godot;
using Godot.Collections;

public partial class App : Control
{
    public enum Screen { Main, Settings, MapSelection, Game }

    [Export] public string DisplayConfigPath { get; set; } = "user://display.cfg";
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
    private static readonly StringName CoreClass = "RA2Core";
    private static readonly StringName GetVersion = "get_version";
    private static readonly StringName BeginLoading = "begin_resource_loading";
    private static readonly StringName GetProgress = "get_resource_progress";
    private static readonly StringName CancelLoading = "cancel_resource_loading";

    public Control MainMenu { get; private set; } = null!;
    public SettingsScreen SettingsScreen { get; private set; } = null!;
    public GameScreen GameScreen { get; private set; } = null!;
    public MapSelectionScreen MapSelectionScreen { get; private set; } = null!;
    public Control PauseMenu { get; private set; } = null!;
    public Button StartButton { get; private set; } = null!;
    public Button SettingsButton { get; private set; } = null!;
    public Button ResumeButton { get; private set; } = null!;
    public Label BootError { get; private set; } = null!;
    public Control LoadingScreen { get; private set; } = null!;
    public ProgressBar LoadingBar { get; private set; } = null!;
    private Button _resourceDirectoryButton = null!;
    private FileDialog _resourceDirectoryDialog = null!;

    public override void _Ready()
    {
        MainMenu = GetNode<Control>("MainMenu");
        SettingsScreen = GetNode<SettingsScreen>("SettingsScreen");
        GameScreen = GetNode<GameScreen>("GameScreen");
        MapSelectionScreen = GetNode<MapSelectionScreen>("MapSelectionScreen");
        MapSelectionScreen.BackRequested += OnMainMenuPressed;
        MapSelectionScreen.MapChosen += StartMap;
        PauseMenu = GetNode<Control>("PauseMenu");
        StartButton = GetNode<Button>("MainMenu/Buttons/StartButton");
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
        var resourceOverride = AutomationRunner.Argument("--resource-config=");
        if (resourceOverride.Length > 0) ResourceConfigPath = resourceOverride;
        AutomationRunner.Attach(this);
        SetProcess(false);
        StartButton.Disabled = true;
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
        foreach (var method in new[] { BeginLoading, GetProgress, CancelLoading })
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
        SettingsScreen.Configure(DisplayService);
        SettingsScreen.BackRequested += OnMainMenuPressed;
        _canLoadResources = true;
        LoadingScreen.Show();
        SetProcess(true);
    }

    public void UseResourceDirectory(string directory)
    {
        if (!_canLoadResources || _core == null) return;
        _resourceDirectoryDialog.Hide();
        GameScreen.CloseGame();
        MapSelectionScreen.CancelScan();
        MapSelectionScreen.Hide();
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
            LoadingScreen.Hide();
            StartButton.Disabled = false;
            SettingsButton.Disabled = false;
            SetProcess(false);
            ShowScreen(Screen.Main);
            GD.Print("Resource bootstrap complete: ", ProgressValue("mounted", default));
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
        BootReady = false;
        SetProcess(false);
        LoadingScreen.Hide();
        MainMenu.Hide();
        SettingsScreen.Hide();
        GameScreen.Hide();
        PauseMenu.Hide();
        StartButton.Disabled = true;
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
        if (CurrentScreen == Screen.Game && screen != Screen.Game) GameScreen.CloseGame();
        if (CurrentScreen == Screen.MapSelection && screen != Screen.MapSelection) MapSelectionScreen.CancelScan();
        GetTree().Paused = false;
        PauseMenu.Hide();
        CurrentScreen = screen;
        MainMenu.Visible = screen == Screen.Main;
        SettingsScreen.Visible = screen == Screen.Settings;
        GameScreen.Visible = screen == Screen.Game;
        MapSelectionScreen.Visible = screen == Screen.MapSelection;
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
        ShowScreen(Screen.MapSelection);
        MapSelectionScreen.Present(_core, ActiveResourceDirectory);
    }

    // Explicit entry used by the picker and scripted map-loading regressions.
    public void StartMap(string filename)
    {
        if (!BootReady || _core == null || string.IsNullOrWhiteSpace(filename)) return;
        ShowScreen(Screen.Game);
        GameScreen.BeginGame(_core, filename, ActiveResourceDirectory);
    }
    public void OnSettingsPressed() => ShowScreen(Screen.Settings);
    public void OnQuitPressed() => GetTree().Quit();
    public void OnMainMenuPressed() => ShowScreen(Screen.Main);

    public void OnResumePressed()
    {
        PauseMenu.Hide();
        GetTree().Paused = false;
    }

    public override void _UnhandledKeyInput(InputEvent @event)
    {
        if (!BootReady || !@event.IsActionPressed("ui_cancel"))
            return;
        switch (CurrentScreen)
        {
            case Screen.Settings:
            case Screen.MapSelection:
                ShowScreen(Screen.Main);
                break;
            case Screen.Game:
                if (PauseMenu.Visible)
                    OnResumePressed();
                else
                {
                    PauseMenu.Show();
                    GetTree().Paused = true;
                    ResumeButton.GrabFocus();
                }
                break;
        }
        GetViewport().SetInputAsHandled();
    }

    public override void _ExitTree()
    {
        MapSelectionScreen.CancelScan();
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
