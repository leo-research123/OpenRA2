using Godot;
using System;
using System.Threading.Tasks;

/// <summary>C# integration tests run as scenes, with failures returned to the command line.</summary>
public abstract partial class TestRunner : Node
{
    protected int Failures { get; private set; }
    protected App Application { get; private set; } = null!;
    protected string ConfigPath { get; private set; } = "";
    protected Window Root => GetTree().Root;

    public override async void _Ready()
    {
        ProcessMode = ProcessModeEnum.Always;
        try
        {
            await NextFrame();
            await RunTestAsync();
        }
        catch (Exception exception)
        {
            Expect(false, exception.ToString());
        }
        if (IsInstanceValid(Application))
        {
            Application.QueueFree();
            await NextFrame();
        }
        RemoveConfig();
        if (Failures == 0)
            GD.Print("PASS: ", GetType().Name);
        GetTree().Quit(Failures == 0 ? 0 : 1);
    }

    protected abstract Task RunTestAsync();

    protected void InitializeApp(string prefix)
    {
        ConfigPath = $"user://{prefix}-{OS.GetProcessId()}.cfg";
        Application = GD.Load<PackedScene>("res://scenes/main.tscn").Instantiate<App>();
        Application.DisplayConfigPath = ConfigPath;
        Application.ResourceConfigPath = ConfigPath + ".resources";
        Root.AddChild(Application);
    }

    protected void Expect(bool condition, string message)
    {
        if (!condition)
        {
            Failures++;
            GD.PushError(message);
        }
    }

    protected bool HasArgument(string argument) => Array.IndexOf(OS.GetCmdlineUserArgs(), argument) >= 0;

    protected string CaptureDirectory()
    {
        foreach (var argument in OS.GetCmdlineUserArgs())
        {
            if (argument.StartsWith("--capture-dir="))
                return argument["--capture-dir=".Length..];
        }
        return "";
    }

    protected static void Press(Button button) => button.EmitSignal(BaseButton.SignalName.Pressed);
    protected async Task NextFrame() => await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
    protected async Task Delay(double seconds) =>
        await ToSignal(GetTree().CreateTimer(seconds), SceneTreeTimer.SignalName.Timeout);
    protected async Task DrawFrame() =>
        await ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);

    protected async Task Escape()
    {
        using var press = new InputEventKey { Keycode = Key.Escape, Pressed = true };
        Input.ParseInputEvent(press);
        await NextFrame();
        using var release = new InputEventKey { Keycode = Key.Escape, Pressed = false };
        Input.ParseInputEvent(release);
        await NextFrame();
    }

    protected async Task WaitForBoot()
    {
        var deadline = Time.GetTicksMsec() + 20_000;
        var previousProgress = 0.0;
        while (!Application.BootReady && !Application.BootError.Visible && Time.GetTicksMsec() < deadline)
        {
            await Delay(0.001);
            Expect(Application.LoadingBar.Value >= previousProgress, "Loading progress must not go backwards");
            previousProgress = Application.LoadingBar.Value;
        }
    }

    protected void Capture(string directory, string name)
    {
        using var image = Root.GetTexture().GetImage();
        Expect(image.SavePng(directory.PathJoin(name)) == Error.Ok, "Screenshot must be saved");
    }

    private void RemoveConfig()
    {
        if (!string.IsNullOrEmpty(ConfigPath) && FileAccess.FileExists(ConfigPath))
            DirAccess.RemoveAbsolute(ConfigPath);
        if (!string.IsNullOrEmpty(ConfigPath) && FileAccess.FileExists(ConfigPath + ".resources"))
            DirAccess.RemoveAbsolute(ConfigPath + ".resources");
    }

    public override void _ExitTree() => RemoveConfig();
}
