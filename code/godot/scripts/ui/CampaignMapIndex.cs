using System;
using System.Collections.Generic;

/// <summary>Official campaign filenames, independent of installation mission indexes.</summary>
internal static class CampaignMapIndex
{
    private static readonly Dictionary<string, (MapCatalog.CampaignVersion Version, MapCatalog.CampaignSide Side)> Maps = Create();

    public static IEnumerable<string> Filenames => Maps.Keys;

    public static bool TryGet(string filename, out MapCatalog.CampaignVersion version, out MapCatalog.CampaignSide side)
    {
        if (Maps.TryGetValue(filename, out var campaign))
        {
            version = campaign.Version;
            side = campaign.Side;
            return true;
        }
        version = MapCatalog.CampaignVersion.Unknown;
        side = MapCatalog.CampaignSide.Unknown;
        return false;
    }

    private static Dictionary<string, (MapCatalog.CampaignVersion Version, MapCatalog.CampaignSide Side)> Create()
    {
        var maps = new Dictionary<string, (MapCatalog.CampaignVersion, MapCatalog.CampaignSide)>(StringComparer.OrdinalIgnoreCase);
        Add(MapCatalog.CampaignVersion.RedAlert2, MapCatalog.CampaignSide.Allied,
            "ALL01T.MAP", "ALL02S.MAP", "ALL03U.MAP", "ALL04U.MAP", "ALL05S.MAP", "ALL06U.MAP",
            "ALL07T.MAP", "ALL08U.MAP", "ALL09T.MAP", "ALL10S.MAP", "ALL11T.MAP", "ALL12S.MAP");
        Add(MapCatalog.CampaignVersion.RedAlert2, MapCatalog.CampaignSide.Soviet,
            "SOV01T.MAP", "SOV02T.MAP", "SOV03U.MAP", "SOV04S.MAP", "SOV05U.MAP", "SOV06T.MAP",
            "SOV07S.MAP", "SOV08U.MAP", "SOV09U.MAP", "SOV10T.MAP", "SOV11S.MAP", "SOV12S.MAP");
        Add(MapCatalog.CampaignVersion.YurisRevenge, MapCatalog.CampaignSide.Allied,
            "ALL01UMD.MAP", "ALL02UMD.MAP", "ALL03UMD.MAP", "ALL04DMD.MAP", "ALL05UMD.MAP", "ALL06UMD.MAP", "ALL07SMD.MAP");
        Add(MapCatalog.CampaignVersion.YurisRevenge, MapCatalog.CampaignSide.Soviet,
            "SOV01UMD.MAP", "SOV02SMD.MAP", "SOV03UMD.MAP", "SOV04DMD.MAP", "SOV05UMD.MAP", "SOV06LMD.MAP", "SOV07TMD.MAP");
        return maps;

        void Add(MapCatalog.CampaignVersion version, MapCatalog.CampaignSide side, params string[] filenames)
        {
            foreach (var filename in filenames) maps.Add(filename, (version, side));
        }
    }
}
