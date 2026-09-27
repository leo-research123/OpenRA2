using Godot;
using System.Collections.Generic;

/// <summary>Owns OS window changes; only this runtime module talks to DisplayServer.</summary>
public partial class DisplayService : Node
{
    public static Window.ModeEnum WindowModeFor(DisplayConfig.Mode mode) => mode switch
    {
        DisplayConfig.Mode.Windowed => Window.ModeEnum.Windowed,
        DisplayConfig.Mode.BorderlessFullscreen => Window.ModeEnum.Fullscreen,
        DisplayConfig.Mode.Fullscreen => Window.ModeEnum.ExclusiveFullscreen,
        _ => throw new System.ArgumentOutOfRangeException(nameof(mode)),
    };

    public DisplayConfig Config { get; } = new();
    private string _configPath = "";
    private SubViewport? _gameViewport;

    public void Initialize(string path, SubViewport? gameViewport = null)
    {
        _configPath = path;
        _gameViewport = gameViewport;
        Config.LoadFile(path);
        ApplyWindow();
    }

    public List<Vector2I> Resolutions()
    {
        var values = new List<Vector2I>(DisplayConfig.Presets);
        foreach (var value in new[] { DisplayServer.ScreenGetSize(), Config.Resolution })
        {
            if (DisplayConfig.ValidResolution(value) && !values.Contains(value))
                values.Add(value);
        }
        values.Sort((a, b) => (a.X * a.Y).CompareTo(b.X * b.Y));
        return values;
    }

    public Error ApplySelection(Vector2I resolution, int mode)
    {
        if (!DisplayConfig.ValidResolution(resolution) || !DisplayConfig.ValidMode(mode))
            return Error.InvalidParameter;
        Config.Resolution = resolution;
        Config.WindowMode = (DisplayConfig.Mode)mode;
        ApplyWindow();
        return Config.SaveFile(_configPath);
    }

    private void ApplyWindow()
    {
        var window = GetWindow();
        // UI uses output pixels; only the game surface uses the selected render resolution.
        window.ContentScaleMode = Window.ContentScaleModeEnum.CanvasItems;
        window.ContentScaleAspect = Window.ContentScaleAspectEnum.Keep;
        window.ContentScaleSize = Config.Resolution;
        if (_gameViewport != null)
            _gameViewport.Size = Config.Resolution;
        if (DisplayServer.GetName() == "headless")
            return;

        window.MinSize = new Vector2I(640, 360);
        window.Mode = WindowModeFor(Config.WindowMode);
        if (Config.WindowMode == DisplayConfig.Mode.Windowed)
        {
            window.Borderless = false;
            var usable = DisplayServer.ScreenGetUsableRect(window.CurrentScreen);
            var available = (usable.Size - new Vector2I(64, 96)).Max(new Vector2I(640, 360));
            var ratio = Mathf.Min(1.0f, Mathf.Min(
                (float)available.X / Config.Resolution.X, (float)available.Y / Config.Resolution.Y));
            window.Size = (Vector2I)((Vector2)Config.Resolution * ratio);
            window.Position = usable.Position + (usable.Size - window.Size) / 2;
        }
    }
}
