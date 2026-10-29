using Godot;
using System.Threading.Tasks;

public partial class TestMapSelection : TestRunner
{
    protected override async Task RunTestAsync()
    {
        InitializeApp("map-selection-test");
        await WaitForBoot();
        var app = Application;
        Expect(app.BootReady, "Map selection requires bootstrapped resources");
        if (!app.BootReady) return;
        var picker = app.MapSelectionScreen;
        Expect(picker.MapType.ItemCount == 11, "Picker must offer version, four faction categories and other campaign filters");
        Press(app.StartButton);
        var requested = AutomationRunner.Argument("--map=");
        if (requested.Length > 0)
        {
            Expect(app.CurrentScreen == App.Screen.Game && app.GameScreen.MapFilename == requested,
                "CLI map must go straight to the requested file, even if it is missing");
            Expect(!picker.Visible && !picker.Scanning && picker.Maps.ItemCount == 0,
                "CLI path must not scan or open the map list");
            app.GameScreen.GetNode<AcceptDialog>("MissionResultDialog").EmitSignal(AcceptDialog.SignalName.Confirmed);
            Expect(app.CurrentScreen == App.Screen.MapSelection && picker.Scanning,
                "A mission opened through CLI returns to the picker even without an existing catalog");
            await WaitForList();
            Expect(picker.Visible && !app.GameScreen.Visible && !GetTree().Paused,
                "CLI mission return closes the world and finishes map discovery");
            app.OnMainMenuPressed();
            return;
        }

        Expect(app.CurrentScreen == App.Screen.MapSelection && !app.GameScreen.Visible,
            "Unrelated CLI arguments must not bypass map selection");
        Expect(picker.Scanning && picker.LoadButton.Disabled, "Scan must start without a selected map");
        Press(picker.LoadButton);
        Expect(!app.GameScreen.Visible, "Programmatic Load cannot bypass scanning or selection");
        app.OnMainMenuPressed();
        Expect(!picker.Scanning, "Returning before the first scan frame cancels it");
        Press(app.StartButton);
        await WaitForList();
        Expect(picker.MapType.Selected == (int)MapSelectionScreen.Category.Campaign, "Map type must default to campaign");
        Expect(picker.LoadButton.Disabled, "Finishing discovery must not automatically start or select a map");
        Expect(!app.GameScreen.Visible && app.GameScreen.MapFilename.Length == 0,
            "Reading metadata must not open a map world");
        if (HasArgument("--expect-catalog-failure"))
        {
            Expect(picker.Maps.ItemCount == 0 && picker.LoadButton.Disabled, "Failed package mount must not expose a partial catalog");
            Expect(picker.GetNode<Label>("Content/Status").Text.Contains("失败"), "Map discovery failure must be visible");
            await Escape();
            Expect(app.MainMenu.Visible, "Failed discovery can return to the menu");
            return;
        }
        if (HasArgument("--expect-no-maps"))
        {
            Expect(picker.Maps.ItemCount == 0 && picker.LoadButton.Disabled, "Empty catalog disables loading");
            Expect(picker.GetNode<Label>("Content/Status").Text.Contains("当前分类"), "Empty category needs actionable text");
            CheckBundledMaps();
            await Escape();
            Expect(app.MainMenu.Visible && !GetTree().Paused, "Empty catalog can return with Esc");
            return;
        }




        Expect(picker.Maps.ItemCount == 13, "Campaign filter uses MultiplayerOnly=false, including the native default");
        foreach (var filename in new[] { "PACK.MAP", "MISSION.MAP", "CUSTOM.MPR" })
            Expect(picker.SelectMap(filename), "Expected campaign map: " + filename);
        Expect(!picker.SelectMap("ARENA.MAP"), "Non-campaign maps must be hidden by default");
        CheckCampaignCategory(MapSelectionScreen.Category.RedAlert2Allied, "ALL01T.MAP");
        CheckCampaignCategory(MapSelectionScreen.Category.RedAlert2Soviet, "SOV01T.MAP");
        CheckCampaignCategory(MapSelectionScreen.Category.YurisRevengeAllied, "ALL01UMD.MAP");
        CheckCampaignCategory(MapSelectionScreen.Category.YurisRevengeSoviet, "SOV01UMD.MAP");
        CheckCampaignCategory(MapSelectionScreen.Category.RedAlert2Campaign,
            "ALL01T.MAP", "SOV01T.MAP");
        CheckCampaignCategory(MapSelectionScreen.Category.YurisRevengeCampaign,
            "ALL01UMD.MAP", "SOV01UMD.MAP");
        CheckCampaignCategory(MapSelectionScreen.Category.OtherCampaign,
            "CUSTOM.MPR", "alliance-md.map", "misnamedmd.map", "PACK.MAP", "MISSION.MAP",
            "TRN01T.MAP", "ALL99T.MAP", "ALL01S.MAP", "SOV08TMD.MAP");
        Expect(picker.SelectMap("misnamedmd.map") &&
            picker.GetNode<Label>("Content/Details").Text.Contains("其他战役"),
            "Installation index provenance and filename patterns must not classify unlisted campaigns");
        Expect(!picker.SelectMap("ALL01T.MAP") && !picker.SelectMap("ALL01UMD.MAP"),
            "Recognized faction campaigns must stay out of other campaigns");
        Expect(picker.SelectMap("CUSTOM.MPR") &&
            picker.GetNode<Label>("Content/Details").Text.Contains("GBK 中文自定义任务"),
            "Legacy GBK map names must decode without replacement characters");
        SetCategory(MapSelectionScreen.Category.YurisRevengeAllied);
        Expect(picker.SelectMap("ALL01UMD.MAP") &&
            picker.GetNode<Label>("Content/Details").Text.Contains("尤里盟军首关"),
            "An indexed stock mission must retain its localized CSF title");
        SetCategory(MapSelectionScreen.Category.YurisRevengeSoviet);
        Expect(picker.SelectMap("SOV01UMD.MAP") &&
            picker.GetNode<Label>("Content/Details").Text.Contains("尤里苏军首关"),
            "Built-in candidates must discover packed stock missions omitted by indexes and recover their CSF names");
        SetCategory(MapSelectionScreen.Category.NonCampaign);
        Expect(picker.Maps.ItemCount == 3 && picker.SelectedMap.Length == 0 && picker.LoadButton.Disabled,
            "Switching category must filter the list and clear the previous selection");
        Expect(!picker.SelectMap("PACK.MAP"), "Campaign maps must be hidden in the non-campaign filter");
        foreach (var filename in new[] { "ARENA.MAP", "EXTRA.MAP", "USER.YRM" })
            Expect(picker.SelectMap(filename), "Expected non-campaign map: " + filename);
        Expect(picker.GetNode<Label>("Content/Details").Text.Contains("UTF-8 中文联机地图"),
            "UTF-8 map names must remain supported alongside GBK names");
        CheckBundledMaps();
        SetCategory(MapSelectionScreen.Category.All);
        Expect(picker.Maps.ItemCount == 16 + BundledTestMaps.Entries.Count && picker.SelectedMap.Length == 0 && picker.LoadButton.Disabled,
            "All maps must include ordinary and bundled maps without retaining an old selection");
        foreach (var filename in new[] { "PACK.MAP", "MISSION.MAP", "ARENA.MAP", "EXTRA.MAP", "USER.YRM", "CUSTOM.MPR" })
            Expect(picker.SelectMap(filename), "Expected available map: " + filename);
        Expect(!picker.SelectMap("missing.map") && !picker.SelectMap("invalid.map") && !picker.SelectMap("bad-size.map"),
            "Unavailable and malformed map metadata must not appear");
        picker.SelectMap("pack.map");
        var selected = picker.Maps.GetSelectedItems()[0];
        Expect(picker.Maps.GetItemText(selected).Contains("Loose override"), "Loose-file priority must be retained");
        Expect(!picker.LoadButton.Disabled && picker.SelectedMap.ToLowerInvariant() == "pack.map", "Selection enables Load");
        Press(picker.LoadButton);
        Expect(app.CurrentScreen == App.Screen.Game && app.GameScreen.MapFilename.ToLowerInvariant() == "pack.map",
            "Load button must pass the selected filename to the game");
        var resultDialog = app.GameScreen.GetNode<AcceptDialog>("MissionResultDialog");
        Expect(!resultDialog.Visible, "A mission must not show its result dialog before completion");
        resultDialog.EmitSignal(AcceptDialog.SignalName.Confirmed);
        Expect(app.CurrentScreen == App.Screen.MapSelection && picker.Visible && !picker.Scanning &&
            !app.GameScreen.Visible && !GetTree().Paused && app.GameScreen.Core == null,
            "Confirming a mission result closes the world and returns to the cached picker");
        Expect(picker.MapType.GetSelectedId() == (int)MapSelectionScreen.Category.All &&
            picker.Maps.ItemCount == 16 + BundledTestMaps.Entries.Count && picker.SelectedMap.ToLowerInvariant() == "pack.map" && !picker.LoadButton.Disabled,
            "Returning from a mission retains the filter, list and selected map");
        Press(picker.LoadButton);
        resultDialog.EmitSignal(AcceptDialog.SignalName.Canceled);
        Expect(app.CurrentScreen == App.Screen.MapSelection && !picker.Scanning &&
            picker.MapType.GetSelectedId() == (int)MapSelectionScreen.Category.All &&
            picker.SelectedMap.ToLowerInvariant() == "pack.map",
            "Dismissing the result dialog follows the same return path without resetting the filter");
        app.OnMainMenuPressed();
        Press(app.StartButton);
        await WaitForList();
        Expect(!picker.Scanning && picker.MapType.GetSelectedId() == (int)MapSelectionScreen.Category.All &&
            picker.Maps.ItemCount == 16 + BundledTestMaps.Entries.Count && picker.SelectedMap.ToLowerInvariant() == "pack.map",
            "Reopening from the menu retains the last filter and selection without duplicating maps");
        app.UseResourceDirectory(app.ActiveResourceDirectory);
        await WaitForBoot();
        Press(app.StartButton);
        Expect(picker.Scanning && picker.SelectedMap.Length == 0,
            "Reloading the same resource directory invalidates the old catalog and selection");
        await WaitForList();
        Expect(picker.MapType.GetSelectedId() == (int)MapSelectionScreen.Category.All && picker.Maps.ItemCount == 16 + BundledTestMaps.Entries.Count,
            "Refreshing resources preserves the filter while rebuilding the complete catalog");
        SetCategory(MapSelectionScreen.Category.NonCampaign);
        picker.SelectMap("user.yrm");
        picker.Maps.EmitSignal(ItemList.SignalName.ItemActivated, picker.Maps.GetSelectedItems()[0]);
        Expect(app.CurrentScreen == App.Screen.Game && app.GameScreen.MapFilename == "user.yrm", "Double click loads the selected map");
        app.OnMainMenuPressed();
        app.UseResourceDirectory(app.ActiveResourceDirectory.PathJoin("catalog-empty"));
        await WaitForBoot();
        Expect(app.BootReady, "Replacement fixture resources must load");
        Press(app.StartButton);
        await WaitForList();
        Expect(picker.Maps.ItemCount == 0 && picker.LoadButton.Disabled, "Changing resources must discard the previous map list");
        Expect(picker.MapType.GetSelectedId() == (int)MapSelectionScreen.Category.NonCampaign,
            "Changing resources invalidates maps while retaining the last filter");
        CheckBundledMaps();
        Press(picker.GetNode<Button>("Content/Actions/BackButton"));
        Expect(app.MainMenu.Visible && !GetTree().Paused, "Back returns to an unpaused menu");
    }



    private void CheckCampaignCategory(MapSelectionScreen.Category category, params string[] filenames)
    {
        var picker = Application.MapSelectionScreen;
        SetCategory(category);
        Expect(picker.Maps.ItemCount == filenames.Length && picker.SelectedMap.Length == 0 && picker.LoadButton.Disabled,
            $"{category} must contain {filenames.Length} maps and clear the previous selection");
        foreach (var filename in filenames)
            Expect(picker.SelectMap(filename), $"{category} must include {filename}");
        Expect(!picker.SelectMap("ARENA.MAP") && !picker.SelectMap(BundledTestMaps.QuickTestPath),
            "Campaign subcategories must exclude multiplayer and bundled test maps");
    }



    private void CheckBundledMaps()
    {
        var picker = Application.MapSelectionScreen;
        SetCategory(MapSelectionScreen.Category.Test);
        Expect(picker.Maps.ItemCount == BundledTestMaps.Entries.Count && picker.SelectedMap.Length == 0 && picker.LoadButton.Disabled,
            "Test maps are independent of the installation catalog and clear the old selection");
        Expect(!picker.SelectMap("PACK.MAP"), "Ordinary campaigns must not appear as test maps");
        foreach (var entry in BundledTestMaps.Entries)
        {
            Expect(picker.SelectMap(entry.Filename), "Expected bundled map: " + entry.Filename);
            Expect(picker.GetNode<Label>("Content/Details").Text.Contains($"{entry.Width} × {entry.Height}"),
                "Bundled map dimensions must appear in details");
            Expect(BundledTestMaps.Prepare(entry.Filename, out var filename, out var error), error);
            Expect(System.IO.File.Exists(filename), "Bundled map is materialized for the native file service");
            Expect(BundledTestMaps.Prepare(entry.Filename, out var repeated, out error) && repeated == filename,
                "Repeated preparation reuses the content-qualified file");
        }
        SetCategory(MapSelectionScreen.Category.Campaign);
        Expect(!picker.SelectMap(BundledTestMaps.QuickTestPath), "Bundled campaigns stay out of ordinary campaigns");
        SetCategory(MapSelectionScreen.Category.NonCampaign);
        Expect(!picker.SelectMap(BundledTestMaps.InfantryStressPath), "Bundled maps stay out of ordinary non-campaign maps");
    }

    private async Task WaitForList()
    {
        var deadline = Time.GetTicksMsec() + 20_000;
        while (Application.MapSelectionScreen.Scanning && Time.GetTicksMsec() < deadline) await NextFrame();
        Expect(!Application.MapSelectionScreen.Scanning, "Map scan must finish");
    }

    private void SetCategory(MapSelectionScreen.Category category)
    {
        var option = Application.MapSelectionScreen.MapType;
        option.Select((int)category);
        option.EmitSignal(OptionButton.SignalName.ItemSelected, (long)category);
    }
}
