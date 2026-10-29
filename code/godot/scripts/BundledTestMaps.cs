using Godot;
using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;

/// <summary>Catalog and native-file materialization for maps shipped in the application.</summary>
public static class BundledTestMaps
{
    public const string QuickTestPath = "res://maps/quick_test.map";
    public const string InfantryStressPath = "res://maps/gi_10000.map";
    public const string RhinoStressPath = "res://maps/rhino_2000.map";
    public const string VeraStandbyPath = "res://maps/vera-20000-30p-standby.map";
    public const string VeraCombatPath = "res://maps/vera-20000-30p-combat.map";
    public static IReadOnlyList<MapCatalog.Entry> Entries { get; } = Array.AsReadOnly(new[]
    {
        new MapCatalog.Entry(QuickTestPath, "快速测试地图", "TEMPERATE", 80, 80, true, true),
        new MapCatalog.Entry(InfantryStressPath, "万人美国大兵测试地图", "TEMPERATE", 256, 256, true, true),
        new MapCatalog.Entry(RhinoStressPath, "2000 犀牛独立小循环测试地图", "TEMPERATE", 256, 256, true, true),
        new MapCatalog.Entry(VeraStandbyPath, "VERA20k 2 万犀牛／30 阵营（待命）", "TEMPERATE", 250, 250, true, true),
        new MapCatalog.Entry(VeraCombatPath, "VERA20k 2 万犀牛／30 阵营（混战）", "TEMPERATE", 250, 250, true, true)
    });

    public static bool Prepare(string resourcePath, out string filename, out string error)
    {
        filename = "";
        error = "";
        try
        {
            MapCatalog.Entry? map = null;
            foreach (var entry in Entries)
                if (entry.Filename == resourcePath) { map = entry; break; }
            if (map == null) throw new IOException("未知的内置测试地图。");
            using var source = Godot.FileAccess.Open(resourcePath, Godot.FileAccess.ModeFlags.Read);
            if (source == null) throw new IOException("无法读取内置测试地图，请重新构建应用。");
            var bytes = source.GetBuffer((long)source.GetLength());
            if (bytes.Length == 0) throw new IOException("内置测试地图为空。");
            // Content-qualified files remain stable while a native loader reads them.
            var digest = Convert.ToHexString(SHA256.HashData(bytes))[..16].ToLowerInvariant();
            var directory = ProjectSettings.GlobalizePath("user://maps");
            Directory.CreateDirectory(directory);
            var target = Path.Combine(directory, $"{Path.GetFileNameWithoutExtension(resourcePath)}_{digest}.map");
            if (!File.Exists(target) || !File.ReadAllBytes(target).AsSpan().SequenceEqual(bytes))
                File.WriteAllBytes(target, bytes);
            filename = target;
            return true;
        }
        catch (Exception exception)
        {
            error = "测试地图准备失败：" + exception.Message;
            return false;
        }
    }
}
