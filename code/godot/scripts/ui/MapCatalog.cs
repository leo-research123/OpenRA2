using Godot;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

/// <summary>Menu metadata only. Native INI/file lookup retains MIX and loose-file priority.</summary>
public static class MapCatalog
{
    public enum CampaignVersion { Unknown, RedAlert2, YurisRevenge }
    public enum CampaignSide { Unknown, Allied, Soviet }

    public sealed record Entry(string Filename, string Name, string Theater, int Width, int Height,
        bool IsCampaign, bool IsTest = false, CampaignVersion Version = CampaignVersion.Unknown,
        CampaignSide Side = CampaignSide.Unknown)
    {
        public string DisplayFilename => IsTest ? Filename.GetFile() : Filename;
        public string Classification
        {
            get
            {
                if (IsTest) return "测试地图";
                if (!IsCampaign) return "非战役地图";
                if (Version == CampaignVersion.Unknown || Side == CampaignSide.Unknown) return "其他战役";
                var version = Version switch
                {
                    CampaignVersion.RedAlert2 => "标准版",
                    CampaignVersion.YurisRevenge => "尤里的复仇",
                    _ => "其他／未识别"
                };
                var side = Side switch
                {
                    CampaignSide.Allied => "盟军战役",
                    CampaignSide.Soviet => "苏军战役",
                    _ => "战役任务"
                };
                return $"{version} · {side}";
            }
        }
    }

    // Yield even for unavailable candidates so the menu can remain responsive.
    public static IEnumerable<Entry?> Discover(RefCounted core, string directory)
    {
        if (!core.HasMethod("prepare_map_resources") || !core.HasMethod("try_get_localized_string"))
            throw new InvalidOperationException("无法读取地图资源，请检查资源目录及原生库版本。");
        using (var prepared = core.Call("prepare_map_resources"))
            if (!prepared.AsBool()) throw new InvalidOperationException("无法挂载地图资源包，请检查游戏资源目录。");

        var candidates = new HashSet<string>(CampaignMapIndex.Filenames, StringComparer.OrdinalIgnoreCase);
        var titles = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        var indexes = new List<string>
        {
            "battlemd.ini", "missionmd.ini", "battle.ini", "mission.ini",
            "missionsmd.pkt", "missions.pkt"
        };
        using (var folder = DirAccess.Open(directory))
        {
            if (folder == null) throw new InvalidOperationException("无法打开当前游戏资源目录。");
            foreach (var filename in folder.GetFiles().OrderBy(name => name, StringComparer.OrdinalIgnoreCase))
            {
                if (IsMapFilename(filename)) candidates.Add(filename);
                else if (filename.EndsWith(".pkt", StringComparison.OrdinalIgnoreCase) &&
                    !indexes.Contains(filename, StringComparer.OrdinalIgnoreCase)) indexes.Add(filename);
            }
            // Client installs (CnCNet and similar) keep loose multiplayer maps under Maps/.
            // The native file lookup resolves these relative paths directly.
            foreach (var subdirectory in folder.GetDirectories())
                if (subdirectory.Equals("Maps", StringComparison.OrdinalIgnoreCase))
                    AddLooseMaps(directory, subdirectory, candidates, 0);
        }
        foreach (var filename in indexes)
        {
            using var ini = LoadIni(core, filename);
            if (ini != null)
            {
                using var sections = ini.Call("get_section_names");
                foreach (var section in sections.AsStringArray())
                {
                    if (IsMapFilename(section))
                    {
                        candidates.Add(section);
                        RememberTitle(section, ReadTitle(core, ini, section));
                    }
                    var scenario = Read(ini, section, "Scenario");
                    if (scenario.Length > 0)
                    {
                        candidates.Add(scenario);
                    }
                }
                using var keys = ini.Call("get_key_names", "MultiMaps");
                foreach (var key in keys.AsStringArray())
                {
                    var name = Read(ini, "MultiMaps", key);
                    if (name.Length == 0) continue;
                    var map = IsMapFilename(name) ? name : name + ".map";
                    candidates.Add(map);
                    RememberTitle(map, ReadTitle(core, ini, name));
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
                    var name = Localized(core, Read(ini, "Basic", "UIName"));
                    if (name.Length == 0) name = titles.GetValueOrDefault(filename, titles.GetValueOrDefault(filename.GetFile(), ""));
                    if (name.Length == 0) name = Read(ini, "Basic", "Name");
                    // Built-in stock filenames retain their CSF names when installation
                    // mission indexes omit them.
                    if (name.Length == 0) name = ReadStockTitle(core, filename);
                    // Keep the same classification and default as load_map_view.
                    using var multiplayer = ini.Call("get_bool", "Basic", "MultiplayerOnly", false);
                    CampaignMapIndex.TryGet(filename.GetFile(), out var version, out var side);
                    entry = new Entry(filename, name.Length == 0 ? filename : name,
                        Read(ini, "Map", "Theater"), width, height, !multiplayer.AsBool(),
                        Version: version, Side: side);
                }
            }
            yield return entry;
        }

        // Bundled metadata avoids parsing thousands of object records while browsing.
        // Test identity belongs to the catalog, independently of the native game mode.
        foreach (var entry in BundledTestMaps.Entries) yield return entry;

        void RememberTitle(string filename, string title)
        {
            // Expansion indexes precede base indexes; an empty entry must not mask a fallback.
            if (title.Length > 0) titles.TryAdd(filename, title);
        }

    }

    private static string ReadTitle(RefCounted core, RefCounted ini, string section)
    {
        var title = Localized(core, Read(ini, section, "UIName"));
        if (title.Length > 0) return title;
        var description = Read(ini, section, "Description");
        // PKT descriptions can also be literal names. Missing CSF keys are not titles.
        if (description.Length > 0 && !description.Contains(':')) return description;
        title = Localized(core, description);
        if (title.Length > 0) return title;
        return Read(ini, section, "Name");
    }

    private static string ReadStockTitle(RefCounted core, string filename)
    {
        var leaf = filename.GetFile();
        if (CampaignMapIndex.TryGet(leaf, out var version, out _))
            return Localized(core, "Name:" + leaf[..5] + (version == CampaignVersion.YurisRevenge ? "md" : ""));
        return leaf.Equals("TRN01T.MAP", StringComparison.OrdinalIgnoreCase) ||
            leaf.Equals("TRN02T.MAP", StringComparison.OrdinalIgnoreCase)
            ? Localized(core, "Name:" + leaf[..5]) : "";
    }

    public static string Localized(RefCounted core, string label)
    {
        if (label.Length == 0) return "";
        using var output = new Godot.Collections.Dictionary();
        using var found = core.Call("try_get_localized_string", label, output);
        return found.AsBool() ? output["text"].AsString().Trim() : "";
    }

    public static RefCounted? LoadIni(RefCounted core, string filename)
    {
        using var value = core.Call("load_ini", filename);
        return value.AsGodotObject() as RefCounted;
    }

    public static string Read(RefCounted ini, string section, string key)
    {
        if (!ini.HasMethod("try_get_string"))
            throw new InvalidOperationException("原生 INI 文本接口不可用，请重新编译原生库。");
        using var output = new Godot.Collections.Dictionary();
        using var found = ini.Call("try_get_string", section, key, output);
        return found.AsBool() ? output["text"].AsString().Trim() : "";
    }

    private static void AddLooseMaps(string root, string relative, HashSet<string> candidates, int depth)
    {
        using var folder = DirAccess.Open(root.PathJoin(relative));
        if (folder == null) return;
        foreach (var filename in folder.GetFiles())
            if (IsMapFilename(filename)) candidates.Add(relative + "/" + filename);
        if (depth < 2)
            foreach (var subdirectory in folder.GetDirectories())
                AddLooseMaps(root, relative + "/" + subdirectory, candidates, depth + 1);
    }

    private static bool IsMapFilename(string filename) =>
        filename.EndsWith(".map", StringComparison.OrdinalIgnoreCase) ||
        filename.EndsWith(".mpr", StringComparison.OrdinalIgnoreCase) ||
        filename.EndsWith(".yrm", StringComparison.OrdinalIgnoreCase);
}
