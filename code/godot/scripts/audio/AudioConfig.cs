using Godot;

/// <summary>Host volume preferences in the original normalized 0..1 range.</summary>
public sealed class AudioConfig
{
    public double Music { get; set; } = 0.8;
    public double Effects { get; set; } = 0.8;
    public double Speech { get; set; } = 0.8;

    public static bool Valid(double value) => double.IsFinite(value) && value >= 0 && value <= 1;

    public void LoadFile(string path)
    {
        using var file = new ConfigFile();
        if (file.Load(path) != Error.Ok) return;
        Music = Read(file, "ScoreVolume", Music);
        Effects = Read(file, "SoundVolume", Effects);
        Speech = Read(file, "VoiceVolume", Speech);
    }

    private static double Read(ConfigFile file, string key, double fallback)
    {
        using var value = file.GetValue("audio", key, fallback);
        if (value.VariantType != Variant.Type.Float && value.VariantType != Variant.Type.Int) return fallback;
        var number = value.AsDouble();
        return Valid(number) ? number : fallback;
    }

    public Error SaveFile(string path)
    {
        if (!Valid(Music) || !Valid(Effects) || !Valid(Speech)) return Error.InvalidParameter;
        using var file = new ConfigFile();
        file.SetValue("audio", "ScoreVolume", Music);
        file.SetValue("audio", "SoundVolume", Effects);
        file.SetValue("audio", "VoiceVolume", Speech);
        return file.Save(path);
    }
}
