using Godot;

/// <summary>
/// Lobby choices in the original skirmish dialog encoding (Skirmish.cpp, 0x6ACEE0):
/// AI difficulty 0 hard, 1 normal, 2 easy; -2 marks a random country, color or start.
/// </summary>
public sealed class SkirmishSlot
{
    public const int Random = -2;
    public const int NoTeam = -1;

    public SkirmishSlotKind Kind { get; set; } = SkirmishSlotKind.Open;
    public int Country { get; set; } = Random;
    public int Color { get; set; } = Random;
    public int Start { get; set; } = Random;
    public int Team { get; set; } = NoTeam;

    public bool IsOccupied => Kind != SkirmishSlotKind.Open;
}

public enum SkirmishSlotKind
{
    Open = -1,
    HardAI = 0,
    NormalAI = 1,
    EasyAI = 2,
    Human = 3
}

public static class SkirmishSetup
{
    public const int SlotCount = 8;
    public const int MaxStarts = 8;

    // The eight multiplayer color slots in the original dialog order.
    public static readonly (string Name, Color Value)[] Colors =
    {
        ("金色", new Color(0.93f, 0.77f, 0.12f)),
        ("红色", new Color(0.86f, 0.1f, 0.08f)),
        ("蓝色", new Color(0.13f, 0.38f, 0.9f)),
        ("绿色", new Color(0.24f, 0.72f, 0.2f)),
        ("橙色", new Color(0.95f, 0.5f, 0.08f)),
        ("天蓝", new Color(0.25f, 0.78f, 0.92f)),
        ("紫色", new Color(0.56f, 0.24f, 0.8f)),
        ("粉色", new Color(0.95f, 0.47f, 0.78f))
    };

    public static readonly string[] Teams = { "A队", "B队", "C队", "D队" };

    public static string KindLabel(SkirmishSlotKind kind) => kind switch
    {
        SkirmishSlotKind.EasyAI => "简单电脑",
        SkirmishSlotKind.NormalAI => "中等电脑",
        SkirmishSlotKind.HardAI => "冷酷电脑",
        SkirmishSlotKind.Human => "玩家",
        _ => "等待加入"
    };
}
