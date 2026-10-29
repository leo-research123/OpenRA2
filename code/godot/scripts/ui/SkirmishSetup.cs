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
    // Matches game::SkirmishSlotCount: thirty players fill the 32-bit house masks.
    public const int SlotCount = 30;
    public const int MaxStarts = 30;

    // The eight multiplayer color slots in the original dialog order, then the
    // native colors 8..29 (HSV in map_world.cpp extra_player_colors).
    public static readonly (string Name, Color Value)[] Colors =
    {
        ("金色", new Color(0.93f, 0.77f, 0.12f)),
        ("红色", new Color(0.86f, 0.1f, 0.08f)),
        ("蓝色", new Color(0.13f, 0.38f, 0.9f)),
        ("绿色", new Color(0.24f, 0.72f, 0.2f)),
        ("橙色", new Color(0.95f, 0.5f, 0.08f)),
        ("天蓝", new Color(0.25f, 0.78f, 0.92f)),
        ("紫色", new Color(0.56f, 0.24f, 0.8f)),
        ("粉色", new Color(0.95f, 0.47f, 0.78f)),
        Extra("浅红", 12, true), Extra("浅黄绿", 58, true), Extra("浅绿", 81, true), Extra("浅青绿", 104, true),
        Extra("浅青", 127, true), Extra("浅蓝", 150, true), Extra("浅靛", 173, true), Extra("浅紫", 196, true),
        Extra("浅品红", 219, true), Extra("浅玫红", 242, true), Extra("浅橙", 35, true),
        Extra("深红", 12, false), Extra("深黄绿", 58, false), Extra("深绿", 81, false), Extra("深青绿", 104, false),
        Extra("深青", 127, false), Extra("深蓝", 150, false), Extra("深靛", 173, false), Extra("深紫", 196, false),
        Extra("深品红", 219, false), Extra("深玫红", 242, false), Extra("深橙", 35, false)
    };

    // Original HSV components are 0..255; light shades are S 140 V 255, deep ones S 255 V 170.
    private static (string, Color) Extra(string name, int hue, bool light) =>
        (name, Color.FromHsv(hue / 255f, (light ? 140 : 255) / 255f, (light ? 255 : 170) / 255f));

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
