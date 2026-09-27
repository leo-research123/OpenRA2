using Godot;
using System;
using System.Collections.Generic;

/// <summary>Validated preferences, independent of the window and settings UI.</summary>
public sealed class DisplayConfig
{
    public enum Mode { Windowed, BorderlessFullscreen, Fullscreen }

    public static IReadOnlyList<Vector2I> Presets { get; } = Array.AsReadOnly(new[]
    {
        new Vector2I(960, 540), new Vector2I(1152, 648), new Vector2I(1280, 720),
        new Vector2I(1600, 900), new Vector2I(1920, 1080), new Vector2I(2560, 1440),
        new Vector2I(3840, 2160),
    });

    public Vector2I Resolution { get; set; } = new(1280, 720);
    public Mode WindowMode { get; set; } = Mode.Windowed;

    public static bool ValidResolution(Vector2I value) =>
        value.X >= 960 && value.Y >= 540 && value.X <= 8192 && value.Y <= 8192;

    public static bool ValidMode(int value) => value >= 0 && value <= (int)Mode.Fullscreen;

    public void LoadFile(string path)
    {
        using var file = new ConfigFile();
        if (file.Load(path) != Error.Ok)
            return;

        using var width = file.GetValue("display", "width", Resolution.X);
        using var height = file.GetValue("display", "height", Resolution.Y);
        if (width.VariantType == Variant.Type.Int && height.VariantType == Variant.Type.Int &&
            width.AsInt64() is >= 960 and <= 8192 && height.AsInt64() is >= 540 and <= 8192)
            Resolution = new Vector2I(width.AsInt32(), height.AsInt32());

        using var mode = file.GetValue("display", "mode", (int)WindowMode);
        if (mode.VariantType == Variant.Type.Int &&
            mode.AsInt64() is >= 0 and <= (long)Mode.Fullscreen)
            WindowMode = (Mode)mode.AsInt32();
    }

    public Error SaveFile(string path)
    {
        using var file = new ConfigFile();
        file.SetValue("display", "width", Resolution.X);
        file.SetValue("display", "height", Resolution.Y);
        file.SetValue("display", "mode", (int)WindowMode);
        return file.Save(path);
    }
}
