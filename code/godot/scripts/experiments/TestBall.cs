using Godot;

/// <summary>C++ backend for the ball stress test.</summary>
public partial class TestBall : TestBallHost
{
    private RefCounted? _renderer;
    private string _nativeBuild = "";
    private static readonly StringName NativeClass = "RA2TestBall";
    private static readonly StringName Initialize = "initialize";
    private static readonly StringName Advance = "advance";
    private static readonly StringName Shutdown = "shutdown";
    protected override string ExperimentName => "test-ball";
    protected override string BackendName => "C++";
    protected override string RenderingPath => "SpriteDrawBatch -> MultiMesh bulk buffer";
    protected override string NativeBuild => _nativeBuild;
    protected override string BuildLabel => $"C++ {_nativeBuild}";

    protected override bool InitializeRenderer(Vector2 size)
    {
        if (!ClassDB.ClassExists(NativeClass) && FileAccess.FileExists("res://ra2_core.gdextension"))
            GDExtensionManager.LoadExtension("res://ra2_core.gdextension");
        if (!ClassDB.ClassExists(NativeClass)) return false;
        using var instance = ClassDB.Instantiate(NativeClass);
        _renderer = instance.AsGodotObject() as RefCounted;
        if (_renderer == null) return false;
        using var initialized = _renderer.Call(Initialize, GetCanvasItem(), BallTexture.GetRid(), size, BallCount, BallRadius);
        if (!initialized.AsBool()) return false;
        using var release = _renderer.Call("is_release_build");
        _nativeBuild = release.AsBool() ? "Release" : "Debug";
        return true;
    }

    protected override Vector3 AdvanceRenderer(double seconds, Vector2 size)
    {
        using var result = _renderer!.Call(Advance, seconds, size);
        return result.AsVector3();
    }

    protected override void ReleaseRenderer()
    {
        if (_renderer == null) return;
        using var result = _renderer.Call(Shutdown);
        _renderer.Dispose();
        _renderer = null;
    }
}
