using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;

// Child audio nodes remain owned by the real SceneTree during native teardown.
public partial class TestAudioShutdown : Node
{
    private RefCounted? _core;
    private Node? _view;
    private string Mode => OS.GetCmdlineUserArgs().Single(arg => arg.StartsWith("--shutdown-mode="))[16..];
    private void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
    private async Task Frame() => await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
    private int Players => GetTree().Root.GetChildren().Count(node => node is AudioStreamPlayer);
    private ulong Playback()
    {
        using var status = _core!.Call("get_audio_status").AsGodotDictionary();
        return status["output_playback_id"].AsUInt64();
    }

    public override async void _Ready()
    {
        try
        {
            Require(new[] { "loading", "direct", "close-first", "repeat", "reopen" }.Contains(Mode), "Unknown shutdown mode");
            var directory = OS.GetCmdlineUserArgs().Single(arg => arg.StartsWith("--game-data="))[12..];
            using (var instance = ClassDB.Instantiate("RA2Core"))
                _core = instance.AsGodotObject() as RefCounted;
            Require(_core != null, "Native core is unavailable");
            _core!.Call("begin_resource_loading", directory);
            if (Mode != "loading")
            {
                var deadline = Time.GetTicksMsec() + 10_000;
                while (true)
                {
                    using var status = _core.Call("get_resource_progress").AsGodotDictionary();
                    if (status["state"].AsString() != "loading")
                    {
                        Require(status["state"].AsString() == "complete", "Authored resource boot failed");
                        break;
                    }
                    Require(Time.GetTicksMsec() < deadline, "Resource boot timed out");
                    await Frame();
                }
                using (var instance = ClassDB.Instantiate("RA2MapView"))
                    _view = instance.AsGodotObject() as Node;
                Require(_view != null, "Native view is unavailable");
                GetTree().Root.AddChild(_view!);
                _view!.Call("configure", _core);
                var rate = AudioServer.GetMixRate();
                // The authored missing scenario requests startup; no game state is an oracle.
                _core.Call("begin_map_loading", "lifecycle-missing.map");
                Require(Playback() != 0 && Players == 1, "Startup must create one owned audio output");
                Require(GodotObject.InstanceFromId(Playback()) is AudioStreamPlaybackResampled,
                    "The output must participate in the host resampler");
                Require(AudioServer.GetMixRate() == rate, "Startup must preserve the host device rate");
                if (Mode is "close-first" or "repeat" or "reopen")
                {
                    var previous = Playback();
                    _view.Call("close_map");
                    Require(Playback() == 0, "Close must detach the audio playback synchronously");
                    if (Mode == "repeat")
                    {
                        _view.Call("close_map");
                        _core.Call("close_map");
                    }
                    if (Mode == "reopen")
                    {
                        _core.Call("begin_map_loading", "lifecycle-missing.map");
                        Require(Playback() != 0 && Playback() != previous, "Reopen must own a new playback");
                    }
                    await Frame();
                    await Frame();
                    Require(Players == (Mode == "reopen" ? 1 : 0), "Retired audio nodes must leave the tree");
                }
            }
            GD.Print("PASS: TestAudioShutdown ", Mode);
            GetTree().Quit(0);
        }
        catch (Exception error)
        {
            GD.PushError(error.ToString());
            GetTree().Quit(1);
        }
    }

    public override void _ExitTree()
    {
        _core?.Dispose();
        _core = null;
    }
}
