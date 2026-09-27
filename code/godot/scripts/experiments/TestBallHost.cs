using Godot;
using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text.Json;

/// <summary>Shared window, frame scheduling and measurement for both ball backends.</summary>
public abstract partial class TestBallHost : Node2D
{
    [Export(PropertyHint.Range, "1,100000,1")] public int BallCount { get; set; } = 10_000;
    [Export(PropertyHint.Range, "1,40,0.5")] public float BallRadius { get; set; } = 3;
    [Export] public Vector2I WindowSize { get; set; } = new(1920, 1080);
    [Export] public bool ResizeWindow { get; set; } = true;
    [Export] public bool ShowStats { get; set; } = true;
    [Export] public bool UnlimitedFps { get; set; } = true;
    public int SubmittedCount { get; private set; }

    protected abstract string ExperimentName { get; }
    protected abstract string BackendName { get; }
    protected abstract string RenderingPath { get; }
    protected Texture2D BallTexture { get; private set; } = null!;
    protected virtual string NativeBuild => "none";
    protected virtual string BuildLabel => $"{BackendName} {(OS.HasFeature("debug") ? "Debug" : "Release")}";
    protected abstract bool InitializeRenderer(Vector2 size);
    protected abstract Vector3 AdvanceRenderer(double seconds, Vector2 size);
    protected abstract void ReleaseRenderer();

    private bool _rendererReady;
    private Label? _stats;
    private ColorRect? _background;
    private TestBallMetrics? _metrics;
    private string _reportPath = "";
    private long _started, _lastFrame;
    private double _hudSeconds, _hudUpdateMs, _hudSubmitMs, _hudMaxMs;
    private int _hudFrames;
    private string _previousTitle = "";
    private Vector2I _previousScaleSize;
    private Window.ContentScaleModeEnum _previousScaleMode;
    private Vector2I _previousSize, _previousPosition;
    private int _previousMaxFps;
    private DisplayServer.VSyncMode _previousVsync;

    public override void _Ready()
    {
        var window = GetWindow();
        _previousTitle = window.Title;
        _previousScaleSize = window.ContentScaleSize;
        _previousScaleMode = window.ContentScaleMode;
        _previousSize = window.Size;
        _previousPosition = window.Position;
        _previousMaxFps = Engine.MaxFps;
        _previousVsync = DisplayServer.WindowGetVsyncMode(window.GetWindowId());
        window.Title = ExperimentName;
        // Draw and collide in the current viewport's pixels, including after resize.
        window.ContentScaleMode = Window.ContentScaleModeEnum.Disabled;
        window.ContentScaleSize = Vector2I.Zero;
        if (ResizeWindow)
        {
            var usable = DisplayServer.ScreenGetUsableRect(window.CurrentScreen);
            window.Size = WindowSize.Min((usable.Size - new Vector2I(32, 64)).Max(new Vector2I(160, 120)));
            window.Position = usable.Position + (usable.Size - window.Size) / 2;
        }
        if (UnlimitedFps)
        {
            Engine.MaxFps = 0;
            DisplayServer.WindowSetVsyncMode(DisplayServer.VSyncMode.Disabled, window.GetWindowId());
        }
        foreach (var argument in OS.GetCmdlineUserArgs())
        {
            if (argument.StartsWith("--ball-count="))
            {
                if (!int.TryParse(argument["--ball-count=".Length..], NumberStyles.Integer,
                    CultureInfo.InvariantCulture, out var count) || count < 1 || count > 100_000)
                {
                    Fail("test-ball：ball-count 必须在 1 到 100000 之间。");
                    return;
                }
                BallCount = count;
            }
            if (argument.StartsWith("--benchmark-seconds="))
            {
                if (!double.TryParse(argument["--benchmark-seconds=".Length..], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out var seconds) || !double.IsFinite(seconds) || seconds <= 0 || seconds > 3600)
                {
                    Fail("test-ball：benchmark-seconds 必须在 0 到 3600 秒之间。");
                    return;
                }
                _metrics = new TestBallMetrics(seconds);
            }
            if (argument.StartsWith("--report=")) _reportPath = argument["--report=".Length..];
        }

        // One shared texture, uploaded before measurement. No per-frame shape construction.
        using (var image = Image.CreateEmpty(64, 64, false, Image.Format.Rgba8))
        {
            for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
            {
                var distance = new Vector2(x + 0.5f - 32, y + 0.5f - 32).Length();
                image.SetPixel(x, y, new Color(1, 1, 1, Mathf.Clamp(32 - distance, 0, 1)));
            }
            BallTexture = ImageTexture.CreateFromImage(image);
        }
        _background = new ColorRect { Color = new Color(0.035f, 0.05f, 0.08f),
            Size = GetViewportRect().Size, ZIndex = -1, MouseFilter = Control.MouseFilterEnum.Ignore };
        AddChild(_background);
        _rendererReady = InitializeRenderer(GetViewportRect().Size);
        if (!_rendererReady)
        {
            Fail($"{ExperimentName}：无法初始化绘制对象，请检查构建与参数。");
            return;
        }
        if (ShowStats)
        {
            var panel = new PanelContainer { Position = new Vector2(16, 16), ZIndex = 10,
                MouseFilter = Control.MouseFilterEnum.Ignore };
            panel.AddThemeStyleboxOverride("panel", new StyleBoxFlat
            {
                BgColor = new Color(0.02f, 0.03f, 0.05f, 0.94f), ContentMarginLeft = 14,
                ContentMarginRight = 14, ContentMarginTop = 10, ContentMarginBottom = 10,
            });
            _stats = new Label { Text = $"{BallCount:N0} balls · {BuildLabel}\nWarming up…" };
            _stats.AddThemeFontSizeOverride("font_size", 18);
            panel.AddChild(_stats);
            AddChild(panel);
        }
        _started = _lastFrame = Stopwatch.GetTimestamp();
        GD.Print($"{ExperimentName}: {BallCount} balls, radius={BallRadius}, viewport={GetViewportRect().Size}, " +
            $"{BuildLabel}, VSync={DisplayServer.WindowGetVsyncMode(window.GetWindowId())}; Esc to quit.");
    }

    public override void _Process(double delta)
    {
        if (!_rendererReady) return;
        var now = Stopwatch.GetTimestamp();
        var frameMs = Stopwatch.GetElapsedTime(_lastFrame, now).TotalMilliseconds;
        _lastFrame = now;
        if (_background!.Size != GetViewportRect().Size) _background.Size = GetViewportRect().Size;
        var timing = AdvanceRenderer(delta, GetViewportRect().Size);
        SubmittedCount = (int)timing.Z;
        _hudSeconds += frameMs / 1000;
        _hudUpdateMs += timing.X;
        _hudSubmitMs += timing.Y;
        _hudMaxMs = Math.Max(_hudMaxMs, frameMs);
        ++_hudFrames;
        if (_hudSeconds >= 0.5)
        {
            if (_stats != null)
                _stats.Text = $"{SubmittedCount:N0} balls · r={BallRadius} px · {GetViewportRect().Size.X:0} × {GetViewportRect().Size.Y:0}\n" +
                    $"{_hudFrames / _hudSeconds:F1} FPS · frame {_hudSeconds * 1000 / _hudFrames:F2} ms · max {_hudMaxMs:F2} ms\n" +
                    $"{BuildLabel} · update {_hudUpdateMs / _hudFrames:F2} ms · submit {_hudSubmitMs / _hudFrames:F2} ms\n" +
                    $"VSync {(UnlimitedFps ? "off" : "inherited")} · Esc to quit";
            _hudSeconds = _hudUpdateMs = _hudSubmitMs = _hudMaxMs = 0;
            _hudFrames = 0;
        }
        if (_metrics?.Observe(Stopwatch.GetElapsedTime(_started, now).TotalSeconds,
            frameMs, timing, GetViewportRect().Size) == true)
        {
            SetProcess(false);
            FinishBenchmark();
        }
    }

    private async void FinishBenchmark()
    {
        try
        {
            var report = JsonSerializer.Serialize(new
            {
                schema_version = 3, backend = BackendName,
                timestamp_utc = DateTime.UtcNow,
                godot = Engine.GetVersionInfo()["string"].AsString(),
                os = OS.GetName(), gpu = RenderingServer.GetVideoAdapterName(),
                renderer = RenderingServer.GetCurrentRenderingMethod(), driver = RenderingServer.GetCurrentRenderingDriverName(),
                native_build = NativeBuild, managed_build = OS.HasFeature("debug") ? "Debug" : "Release",
                loaded_extensions = GDExtensionManager.GetLoadedExtensions(),
                ball_count = BallCount, radius = BallRadius,
                viewport = new[] { GetViewportRect().Size.X, GetViewportRect().Size.Y },
                vsync = DisplayServer.WindowGetVsyncMode(GetWindow().GetWindowId()).ToString(), max_fps = Engine.MaxFps,
                path = RenderingPath, texture_size = new[] { 64, 64 }, gpu_ms = (double?)null,
                measurements = _metrics!.Summary(),
            }, new JsonSerializerOptions { WriteIndented = true });
            GD.Print(report);
            if (_reportPath.Length > 0)
            {
                var path = Path.GetFullPath(_reportPath);
                Directory.CreateDirectory(Path.GetDirectoryName(path)!);
                File.WriteAllText(path, report);
                File.WriteAllText(Path.ChangeExtension(path, ".csv"), _metrics.FrameCsv());
                await ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);
                using var image = GetViewport().GetTexture().GetImage();
                if (image.SavePng(Path.ChangeExtension(path, ".png")) != Error.Ok)
                    throw new IOException("Could not save test-ball screenshot.");
            }
            GetTree().Quit();
        }
        catch (Exception exception)
        {
            GD.PushError(exception.ToString());
            GetTree().Quit(1);
        }
    }

    public override void _UnhandledKeyInput(InputEvent @event)
    {
        if (@event is InputEventKey { Pressed: true, Echo: false, Keycode: Key.Escape })
            GetTree().Quit();
    }

    private void Fail(string message)
    {
        SetProcess(false);
        AddChild(new Label { Text = message, Position = new Vector2(24, 24) });
        GD.PushError(message);
    }

    public override void _ExitTree()
    {
        ReleaseRenderer();
        BallTexture?.Dispose();
        _rendererReady = false;
        var window = GetWindow();
        window.Title = _previousTitle;
        window.ContentScaleSize = _previousScaleSize;
        window.ContentScaleMode = _previousScaleMode;
        if (ResizeWindow)
        {
            window.Size = _previousSize;
            window.Position = _previousPosition;
        }
        Engine.MaxFps = _previousMaxFps;
        DisplayServer.WindowSetVsyncMode(_previousVsync, window.GetWindowId());
    }
}
