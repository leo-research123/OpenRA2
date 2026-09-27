using Godot;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

/// <summary>Menu metadata only. Native INI/file lookup retains MIX and loose-file priority.</summary>
public static class MapCatalog
{
    public sealed record Entry(string Filename, string Name, string Theater, int Width, int Height, bool IsCampaign);

    // Yield even for unavailable candidates so the menu can remain responsive.
    public static IEnumerable<Entry?> Discover(RefCounted core, string directory)
    {
        if (!core.HasMethod("prepare_map_resources"))
            throw new InvalidOperationException("无法读取地图资源，请检查资源目录及原生库版本。");
        using (var prepared = core.Call("prepare_map_resources"))
            if (!prepared.AsBool()) throw new InvalidOperationException("无法挂载地图资源包，请检查游戏资源目录。");

        var candidates = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var indexes = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "battlemd.ini", "battle.ini", "missionmd.ini", "mission.ini",
            "missionsmd.pkt", "missions.pkt"
        };
        using (var folder = DirAccess.Open(directory))
        {
            if (folder == null) throw new InvalidOperationException("无法打开当前游戏资源目录。");
            foreach (var filename in folder.GetFiles())
            {
                if (IsMapFilename(filename)) candidates.Add(filename);
                else if (filename.EndsWith(".pkt", StringComparison.OrdinalIgnoreCase)) indexes.Add(filename);
            }
        }
        foreach (var filename in indexes)
        {
            using var ini = LoadIni(core, filename);
            if (ini != null)
            {
                using var sections = ini.Call("get_section_names");
                foreach (var section in sections.AsStringArray())
                {
                    if (IsMapFilename(section)) candidates.Add(section);
                    var scenario = Read(ini, section, "Scenario");
                    if (scenario.Length > 0) candidates.Add(scenario);
                }
                using var keys = ini.Call("get_key_names", "MultiMaps");
                foreach (var key in keys.AsStringArray())
                {
                    var name = Read(ini, "MultiMaps", key);
                    if (name.Length == 0) continue;
                    candidates.Add(IsMapFilename(name) ? name : name + ".map");
                }
            }
            yield return null;
        }
        foreach (var filename in candidates.OrderBy(name => name, StringComparer.OrdinalIgnoreCase))
        {
            using var ini = LoadIni(core, filename);
            Entry? entry = null;
            if (ini != null)
            {
                var size = Read(ini, "Map", "Size").Split(',');
                if (size.Length == 4 &&
                    int.TryParse(size[2].Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var width) && width > 0 &&
                    int.TryParse(size[3].Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var height) && height > 0)
                {
                    var name = Read(ini, "Basic", "Name");
                    // Keep the same classification and default as load_map_view.
                    using var multiplayer = ini.Call("get_bool", "Basic", "MultiplayerOnly", false);
                    entry = new Entry(filename, name.Length == 0 ? filename : name,
                        Read(ini, "Map", "Theater"), width, height, !multiplayer.AsBool());
                }
            }
            yield return entry;
        }
    }

    private static RefCounted? LoadIni(RefCounted core, string filename)
    {
        using var value = core.Call("load_ini", filename);
        return value.AsGodotObject() as RefCounted;
    }

    private static string Read(RefCounted ini, string section, string key)
    {
        using var value = ini.Call("get_string", section, key, "");
        return value.AsString().Trim();
    }

    private static bool IsMapFilename(string filename) =>
        filename.EndsWith(".map", StringComparison.OrdinalIgnoreCase) ||
        filename.EndsWith(".mpr", StringComparison.OrdinalIgnoreCase) ||
        filename.EndsWith(".yrm", StringComparison.OrdinalIgnoreCase);
}
