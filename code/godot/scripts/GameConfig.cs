using Godot;

/// <summary>Increasing host speed levels; original delay values stay in the core.</summary>
public sealed class GameConfig
{
    public int SpeedLevel { get; set; } = 5;

    public static string SpeedLabel(int level) => level switch
    {
        1 => "1 · 最慢",
        2 => "2 · 较慢",
        3 => "3 · 慢",
        4 => "4 · 适中",
        5 => "5 · 快（默认）",
        6 => "6 · 较快",
        7 => "7 · 超快（120 帧/秒）",
        8 => "8 · 极速（240 帧/秒）",
        9 => "9 · 最快（不限速）",
        _ => "无效速度"
    };

    public void LoadFile(string path)
    {
        using var file = new ConfigFile();
        if (file.Load(path) != Error.Ok) return;
        using var value = file.GetValue("game", "speed_level", 5);
        if (value.VariantType != Variant.Type.Int) return;
        var level = value.AsInt64();
        if (level >= 1 && level <= 9) SpeedLevel = (int)level;
    }

    public Error SaveFile(string path)
    {
        if (SpeedLevel < 1 || SpeedLevel > 9) return Error.InvalidParameter;
        using var file = new ConfigFile();
        file.SetValue("game", "speed_level", SpeedLevel);
        return file.Save(path);
    }
}
