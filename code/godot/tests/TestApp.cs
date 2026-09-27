using Godot;
using System.Threading.Tasks;

public partial class TestApp : TestRunner
{
    protected override async Task RunTestAsync()
    {
        InitializeApp("display-test");
        var app = Application;
        if (HasArgument("--expect-missing-core"))
        {
            Expect(app.CoreVersion.Length == 0, "Missing core must not invent a version");
            Expect(app.BootError.Visible, "Missing core must show an error");
            Expect(!app.MainMenu.Visible, "Missing core must block the main menu");
            Expect(app.StartButton.Disabled, "Missing core must disable Start");
            Press(app.StartButton);
            Expect(!app.GameScreen.Visible, "Missing core must block game entry");
            return;
        }

        Expect(ClassDB.ClassExists("RA2Core"), "Native class must be registered");
        Expect(app.CoreVersion == "0.1.0", "Version must come from the compiled core");
        Expect(app.LoadingScreen.Visible && !app.MainMenu.Visible, "Boot must begin on loading screen");
        Expect(!app.BootReady && app.StartButton.Disabled, "Loading must block navigation");
        Press(app.StartButton);
        Expect(!app.GameScreen.Visible, "Programmatic Start must also be gated during loading");
        await WaitForBoot();

        if (HasArgument("--expect-empty"))
        {
            Expect(app.ResourceProgress["state"].AsString() == "complete", "Core preserves empty-directory bootstrap semantics");
            Expect(app.ResourceProgress["mounted"].AsStringArray().Length == 0, "Empty fixture mounts nothing");
            Expect(!app.BootReady && app.BootError.Visible && app.StartButton.Disabled, "App must block Start without game data");
            Expect(app.GetNode<Button>("ResourceDirectoryButton").Visible, "Empty resources must offer directory selection");
            Press(app.StartButton);
            Expect(!app.GameScreen.Visible, "Programmatic Start cannot bypass missing game data");
            return;
        }

        if (HasArgument("--expect-resource-failure"))
        {
            Expect(app.BootError.Visible && !app.BootReady, "Bad resources must show a terminal error");
            Expect(!app.MainMenu.Visible && !app.LoadingScreen.Visible, "Failed loading must block main menu");
            Expect(app.ResourceProgress["state"].AsString() == "failed", "Failure must come from C++");
            Press(app.StartButton);
            Expect(!app.GameScreen.Visible, "Failed loading must block game entry");
            return;
        }

        Expect(app.BootReady && app.MainMenu.Visible, "Completed bootstrap must show main menu");
        Expect(app.ResourceProgress["state"].AsString() == "complete", "Menu needs native completion");
        Expect(app.LoadingBar.Value == app.LoadingBar.MaxValue, "Completion must reach full progress");
        Expect(!app.LoadingScreen.Visible, "Completed loading screen must be hidden");
        var mounted = app.ResourceProgress["mounted"].AsStringArray();
        Expect(HasArgument("--expect-empty") ? mounted.Length == 0 : mounted.Length > 0,
            "Native mounts must match the resource fixture");
        if (Failures != 0)
            return;

        await Escape();
        Expect(app.MainMenu.Visible && !GetTree().Paused, "Esc on main menu is harmless");
        Press(app.SettingsButton);
        Expect(app.SettingsScreen.Visible && !app.MainMenu.Visible, "Settings button must open settings");
        var settings = app.SettingsScreen;
        Expect(settings.ModeOption.ItemCount == 3, "Settings must offer three window modes");
        settings.ResolutionOption.Select(app.DisplayService.Resolutions().IndexOf(new Vector2I(1920, 1080)));
        settings.ModeOption.Select((int)DisplayConfig.Mode.BorderlessFullscreen);
        Press(settings.GetNode<Button>("%ApplyButton"));
        var saved = new DisplayConfig();
        saved.LoadFile(ConfigPath);
        Expect(saved.Resolution == new Vector2I(1920, 1080), "Applied resolution must persist");
        Expect(saved.WindowMode == DisplayConfig.Mode.BorderlessFullscreen, "Applied mode must persist");
        Expect(Root.ContentScaleSize == new Vector2I(1920, 1080), "Resolution must affect the render viewport");
        Expect(Root.ContentScaleMode == Window.ContentScaleModeEnum.CanvasItems, "UI must render at output pixel density");
        Expect(app.GetNode<SubViewport>("GameScreen/WorldViewport").Size == new Vector2I(1920, 1080),
            "Game surface must retain selected render resolution");
        settings.ModeOption.Select((int)DisplayConfig.Mode.Fullscreen);
        await Escape();
        Expect(app.MainMenu.Visible, "Esc must return from settings");
        Press(app.SettingsButton);
        Expect(settings.ModeOption.Selected == (int)DisplayConfig.Mode.BorderlessFullscreen,
            "Returning must discard unapplied edits");
        Press(settings.GetNode<Button>("Center/Form/Actions/BackButton"));
        Expect(app.MainMenu.Visible, "Back button must return to main menu");

        using var invalid = new ConfigFile();
        invalid.SetValue("display", "width", "bad");
        invalid.SetValue("display", "height", -1);
        invalid.SetValue("display", "mode", 99);
        Expect(invalid.Save(ConfigPath) == Error.Ok, "Malformed preference fixture must be written");
        var fallback = new DisplayConfig();
        fallback.LoadFile(ConfigPath);
        Expect(fallback.Resolution == new Vector2I(1280, 720) &&
            fallback.WindowMode == DisplayConfig.Mode.Windowed, "Malformed preferences must fall back safely");
        // ConfigFile integers are 64-bit; narrowing them must not turn invalid values into valid settings.
        invalid.SetValue("display", "width", 4_294_968_896L);
        invalid.SetValue("display", "height", 900);
        invalid.SetValue("display", "mode", 4_294_967_297L);
        Expect(invalid.Save(ConfigPath) == Error.Ok, "Overflow preference fixture must be written");
        fallback.LoadFile(ConfigPath);
        Expect(fallback.Resolution == new Vector2I(1280, 720) &&
            fallback.WindowMode == DisplayConfig.Mode.Windowed, "Oversized integers must not wrap into valid settings");
        Expect(app.DisplayService.ApplySelection(Vector2I.Zero, 99) == Error.InvalidParameter,
            "Invalid selections must be rejected");
        Expect(app.DisplayService.Config.Resolution == new Vector2I(1920, 1080),
            "Invalid selections must leave active settings intact");
        for (var cycle = 0; cycle < 3; cycle++)
        {
            Press(app.StartButton);
            Expect(app.MapSelectionScreen.Visible && !app.GameScreen.Visible && !GetTree().Paused,
                "Start without a map argument must open the picker without loading a map");
            await Escape();
            Expect(app.MainMenu.Visible && !app.MapSelectionScreen.Scanning, "Esc cancels map discovery");
            app.StartMap("ALL01UMD.MAP");
            Expect(app.GameScreen.Visible && !GetTree().Paused, "Explicit map entry must enter unpaused game");
            await Escape();
            Expect(app.PauseMenu.Visible && GetTree().Paused, "Esc must open pause menu and pause tree");
            await Escape();
            Expect(!app.PauseMenu.Visible && !GetTree().Paused, "Esc must resume game");
            await Escape();
            Press(app.ResumeButton);
            Expect(!app.PauseMenu.Visible && !GetTree().Paused, "Resume button must resume game");
            await Escape();
            Press(app.GetNode<Button>("PauseMenu/Panel/Buttons/MainMenuButton"));
            Expect(app.MainMenu.Visible && !app.GameScreen.Visible, "Return must show main menu");
            Expect(!GetTree().Paused && !app.PauseMenu.Visible, "Return must clear pause state");
            Expect(app.CoreVersion == "0.1.0", "Core must remain available across sessions");
        }
    }
}
