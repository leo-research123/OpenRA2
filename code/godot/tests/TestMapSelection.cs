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
        Expect(picker.MapType.ItemCount == 3, "Picker must offer campaign, non-campaign and all maps");
        Press(app.StartButton);
        var requested = AutomationRunner.Argument("--map=");
        if (requested.Length > 0)
        {
            Expect(app.CurrentScreen == App.Screen.Game && app.GameScreen.MapFilename == requested,
                "CLI map must go straight to the requested file, even if it is missing");
            Expect(!picker.Visible && !picker.Scanning && picker.Maps.ItemCount == 0,
                "CLI path must not scan or open the map list");
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
            Expect(picker.GetNode<Label>("Content/Status").Text.Contains("未找到"), "Empty catalog needs actionable text");
            await Escape();
            Expect(app.MainMenu.Visible && !GetTree().Paused, "Empty catalog can return with Esc");
            return;
        }

        Expect(picker.Maps.ItemCount == 3, "Campaign filter uses MultiplayerOnly=false, including the native default");
        foreach (var filename in new[] { "PACK.MAP", "MISSION.MAP", "CUSTOM.MPR" })
            Expect(picker.SelectMap(filename), "Expected campaign map: " + filename);
        Expect(!picker.SelectMap("ARENA.MAP"), "Non-campaign maps must be hidden by default");
        SetCategory(MapSelectionScreen.Category.NonCampaign);
        Expect(picker.Maps.ItemCount == 3 && picker.SelectedMap.Length == 0 && picker.LoadButton.Disabled,
            "Switching category must filter the list and clear the previous selection");
        Expect(!picker.SelectMap("PACK.MAP"), "Campaign maps must be hidden in the non-campaign filter");
        foreach (var filename in new[] { "ARENA.MAP", "EXTRA.MAP", "USER.YRM" })
            Expect(picker.SelectMap(filename), "Expected non-campaign map: " + filename);
        SetCategory(MapSelectionScreen.Category.All);
        Expect(picker.Maps.ItemCount == 6 && picker.SelectedMap.Length == 0 && picker.LoadButton.Disabled,
            "All maps must include both categories without retaining an old selection");
        foreach (var filename in new[] { "PACK.MAP", "MISSION.MAP", "ARENA.MAP", "EXTRA.MAP", "USER.YRM", "CUSTOM.MPR" })
            Expect(picker.SelectMap(filename), "Expected available map: " + filename);
        Expect(!picker.SelectMap("missing.map") && !picker.SelectMap("invalid.map") && !picker.SelectMap("bad-size.map"),
            "Unavailable and malformed map metadata must not appear");
        picker.SelectMap("pack.map");
        var selected = picker.Maps.GetSelectedItems()[0];
        Expect(picker.Maps.GetItemText(selected).Contains("Loose override"), "Original loose-file priority must be retained");
        Expect(!picker.LoadButton.Disabled && picker.SelectedMap.ToLowerInvariant() == "pack.map", "Selection enables Load");
        Press(picker.LoadButton);
        Expect(app.CurrentScreen == App.Screen.Game && app.GameScreen.MapFilename.ToLowerInvariant() == "pack.map",
            "Load button must pass the selected filename to the game");
        app.OnMainMenuPressed();
        Press(app.StartButton);
        await WaitForList();
        Expect(picker.MapType.Selected == (int)MapSelectionScreen.Category.Campaign &&
            picker.Maps.ItemCount == 3 && picker.SelectedMap.Length == 0,
            "Reopening defaults to campaign without duplicates or a stale selection");
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
        Press(picker.GetNode<Button>("Content/Actions/BackButton"));
        Expect(app.MainMenu.Visible && !GetTree().Paused, "Back returns to an unpaused menu");
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
