using Godot;
using System.Linq;
using System.Threading.Tasks;

/// <summary>Skirmish lobby data: map discovery, starts, preview and seat rules.</summary>
public partial class TestSkirmishLobby : TestRunner
{
    protected override async Task RunTestAsync()
    {
        InitializeApp("skirmish-lobby-test");
        await WaitForBoot();
        var app = Application;
        Expect(app.BootReady, "Skirmish lobby requires bootstrapped resources");
        if (!app.BootReady) return;
        Press(app.SkirmishButton);
        var lobby = app.SkirmishLobbyScreen;
        Expect(app.CurrentScreen == App.Screen.Skirmish && lobby.Visible, "Main menu button opens the lobby");
        var deadline = Time.GetTicksMsec() + 30_000;
        while (lobby.Scanning && Time.GetTicksMsec() < deadline) await NextFrame();
        Expect(!lobby.Scanning, "Map discovery finishes");

        GD.Print("LOBBY maps=", lobby.Maps.Count, " first=", string.Join(" | ", lobby.Maps.Take(5).Select(map => $"{map.Filename}:{map.Name}")));
        GD.Print("LOBBY selected=", lobby.SelectedMap, " starts=", lobby.StartCount, " preview=", lobby.PreviewSize);
        GD.Print("LOBBY start fractions=", string.Join(" ", lobby.Starts.Select(start => $"({start.X:F2},{start.Y:F2})")));
        GD.Print("LOBBY countries=", string.Join(",", lobby.Countries.Select(country => $"{country.Index}:{country.Name}")));
        GD.Print("LOBBY modes=", string.Join(",", Enumerable.Range(0, lobby.Mode.ItemCount).Select(lobby.Mode.GetItemText)));
        GD.Print("LOBBY money=", lobby.Money.MinValue, "..", lobby.Money.MaxValue, " step ", lobby.Money.Step, " value ", lobby.Money.Value,
            " units=", lobby.Units.MinValue, "..", lobby.Units.MaxValue, " value ", lobby.Units.Value);
        GD.Print("LOBBY slots=", string.Join(" ", lobby.Slots.Select(slot => $"{slot.Kind}/{slot.Start}")));

        Expect(lobby.Maps.Count > 0, "At least one multiplayer map is listed");
        Expect(lobby.Maps.All(map => !map.IsCampaign && !map.IsTest), "Only multiplayer maps are listed");
        Expect(lobby.StartCount is > 0 and <= SkirmishSetup.MaxStarts, "Selected map has multiplayer starts");
        Expect(lobby.Starts.All(start => start.X is > -0.05f and < 1.05f && start.Y is > -0.05f and < 1.05f),
            "Start markers fall inside the preview");
        Expect(lobby.PreviewSize.X > 0 && lobby.PreviewSize.Y > 0, "Selected map preview decodes");
        Expect(lobby.Countries.Count > 0, "Multiplayer countries come from the rules");
        Expect(lobby.Slots[0].Kind == SkirmishSlotKind.Human && lobby.Slots[1].IsOccupied, "Default seats: player and one computer");
        var captures = CaptureDirectory();
        if (captures.Length > 0)
        {
            await DrawFrame();
            await DrawFrame();
            Capture(captures, "skirmish_lobby.png");
        }
        await Escape();
        Expect(app.CurrentScreen == App.Screen.Main, "Esc returns to the main menu");
    }
}
