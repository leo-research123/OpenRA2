using Godot;
using System.Collections.Generic;
using System.Threading.Tasks;

/// <summary>Opt-in integration test of real OS window modes.</summary>
public partial class TestDisplayWindow : TestRunner
{
    protected override async Task RunTestAsync()
    {
        InitializeApp("display-window-test");
        await WaitForBoot();
        Expect(Application.BootReady, "Resource bootstrap did not complete");
        if (!Application.BootReady)
            return;

        var app = Application;
        Press(app.SettingsButton);
        foreach (var mode in new[] { 0, 1, 2, 0 })
        {
            Expect(app.DisplayService.ApplySelection(new Vector2I(1280, 720), mode) == Error.Ok, "Apply must succeed");
            // macOS fullscreen transitions animate asynchronously.
            await Delay(2.0);
            Expect(Root.Mode == DisplayService.WindowModeFor((DisplayConfig.Mode)mode),
                $"Native window mode mismatch: {mode}");
            Expect(app.GetNode<SubViewport>("GameScreen/WorldViewport").Size == new Vector2I(1280, 720),
                "Game render resolution must remain selected size");
            Expect(Root.ContentScaleMode == Window.ContentScaleModeEnum.CanvasItems,
                "UI must render independently at output pixel density");
            if (mode == (int)DisplayConfig.Mode.Windowed)
            {
                Expect(!Root.Borderless, "Windowed mode must restore title bar");
                var usable = DisplayServer.ScreenGetUsableRect(Root.CurrentScreen);
                var available = (usable.Size - new Vector2I(64, 96)).Max(new Vector2I(640, 360));
                Expect(Root.Size.X <= available.X && Root.Size.Y <= available.Y,
                    "Windowed mode must fit the usable desktop");
                if (available.X >= 1280 && available.Y >= 720)
                    Expect(Root.Size == new Vector2I(1280, 720), "Windowed mode must restore selected size when it fits");
            }
            else
            {
                var sizes = new List<Vector2I> { DisplayServer.ScreenGetSize(Root.CurrentScreen) };
                if (OS.GetName() == "macOS")
                    sizes.Add(DisplayServer.GetDisplaySafeArea().Size);
                Expect(sizes.Contains(Root.Size), $"Fullscreen must fill monitor or safe area: {Root.Size}");
                if (Root.Size.X > 1280)
                    Expect(Root.GetTexture().GetSize().X > 1280, "Fullscreen UI must not be enlarged from a 1280px bitmap");
            }
            GD.Print($"Display verified: mode={Root.Mode} window={Root.Size} render={Root.GetTexture().GetSize()}");
        }
        foreach (var size in new[] { new Vector2I(960, 540), new Vector2I(1920, 1080) })
        {
            Expect(app.DisplayService.ApplySelection(size, 0) == Error.Ok, "Resolution change must succeed");
            await Delay(0.5);
            Expect(Root.ContentScaleSize == size, "Resolution change must reach viewport");
            Expect(app.GetNode<SubViewport>("GameScreen/WorldViewport").Size == size,
                "Game surface must match selected resolution");
        }
        var captureDirectory = CaptureDirectory();
        if (captureDirectory.Length > 0)
        {
            app.SettingsScreen.Present();
            await DrawFrame();
            Capture(captureDirectory, "display-settings.png");
            Press(app.GetNode<Button>("PauseMenu/Panel/Buttons/MainMenuButton"));
            await DrawFrame();
            Capture(captureDirectory, "main-menu.png");
        }
        if (Failures != 0)
            return;
        Press(app.GetNode<Button>("MainMenu/Buttons/QuitButton"));
        await Delay(1.0);
        Expect(false, "Quit button did not terminate the game");
    }
}
