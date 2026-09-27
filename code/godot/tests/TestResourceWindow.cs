using Godot;
using System.Threading.Tasks;

/// <summary>Opt-in visual check of resource loading, without artificial delays.</summary>
public partial class TestResourceWindow : TestRunner
{
    protected override async Task RunTestAsync()
    {
        var captureDirectory = CaptureDirectory();
        InitializeApp("resource-window-test");
        var deadline = Time.GetTicksMsec() + 20_000;
        var capturedLoading = false;
        while (!Application.BootReady && !Application.BootError.Visible && Time.GetTicksMsec() < deadline)
        {
            await DrawFrame();
            if (Application.LoadingScreen.Visible && captureDirectory.Length > 0)
            {
                Capture(captureDirectory, "loading.png");
                capturedLoading = true;
            }
        }
        Expect(Application.BootReady, "Native resource loading failed or timed out");
        if (!Application.BootReady)
            return;
        await DrawFrame();
        if (captureDirectory.Length > 0)
        {
            Capture(captureDirectory, "main-menu.png");
            Expect(capturedLoading, "Loading page never rendered");
        }
    }
}
