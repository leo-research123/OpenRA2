using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;
using Dictionary = Godot.Collections.Dictionary;

/// <summary>Exercise the actual native bridge without a stand-in resource manager.</summary>
public partial class TestResourceWorker : TestRunner
{
    private static readonly StringName Begin = "begin_resource_loading";
    private static readonly StringName Progress = "get_resource_progress";
    private static readonly StringName Cancel = "cancel_resource_loading";

    private static RefCounted CreateCore()
    {
        using var instance = ClassDB.Instantiate("RA2Core");
        return instance.AsGodotObject() as RefCounted ?? throw new Exception("RA2Core is unavailable");
    }

    private static Dictionary Snapshot(RefCounted core) => core.Call(Progress).AsGodotDictionary();

    private async Task<Dictionary> Terminal(RefCounted core)
    {
        var deadline = Time.GetTicksMsec() + 10_000;
        long completed = 0;
        while (true)
        {
            var progress = Snapshot(core);
            Expect(progress["completed"].AsInt64() >= completed &&
                progress["completed"].AsInt64() <= progress["total"].AsInt64(), "Non-monotonic progress");
            completed = progress["completed"].AsInt64();
            if (progress["state"].AsString() != "loading") return progress;
            if (Time.GetTicksMsec() >= deadline)
            {
                core.Call(Cancel);
                throw new Exception("Resource worker timed out");
            }
            await NextFrame();
        }
    }

    protected override async Task RunTestAsync()
    {
        var directory = OS.GetCmdlineUserArgs().Single(arg => arg.StartsWith("--game-data="))[12..];
        string[] mounted;
        long entries;
        using (var core = CreateCore())
        {
            Expect(Snapshot(core)["state"].AsString() == "idle", "Initial bridge state");
            core.Call(Begin, directory);
            var initial = await Terminal(core);
            mounted = initial["mounted"].AsStringArray();
            entries = initial["index_entries"].AsInt64();
            Expect(initial["state"].AsString() == "complete" && initial["completed"].AsInt64() == 108 &&
                mounted.Length > 0, "First load failed");
            using (var competing = CreateCore())
            {
                competing.Call(Begin, directory);
                var conflict = await Terminal(competing);
                Expect(conflict["state"].AsString() == "failed" &&
                    conflict["error"].AsString().Contains("already active") &&
                    conflict["mounted"].AsStringArray().Length == 0, "Second bridge replaced the active environment");
                var unchanged = Snapshot(core);
                Expect(unchanged["state"].AsString() == "complete" &&
                    unchanged["mounted"].AsStringArray().SequenceEqual(mounted) &&
                    unchanged["index_entries"].AsInt64() == entries, "Competing bridge altered the owner");
            }
            for (var i = 0; i < 32; ++i)
            {
                core.Call(Begin, directory);
                core.Call(Cancel);
                Expect(Snapshot(core)["state"].AsString() is "complete" or "cancelled",
                    "Cancel returned with a live or failed worker");
            }
            core.Call(Begin, directory.PathJoin("absent-worker-test-directory"));
            var missing = await Terminal(core);
            Expect(missing["state"].AsString() == "failed" && missing["error"].AsString().Length > 0 &&
                missing["mounted"].AsStringArray().Length == 0, "Failed restart retained previous mounts");
            core.Call(Begin, directory);
            var restarted = await Terminal(core);
            Expect(restarted["state"].AsString() == "complete" &&
                restarted["mounted"].AsStringArray().SequenceEqual(mounted) &&
                restarted["index_entries"].AsInt64() == entries, "Restart duplicated or lost resources");
            core.Call(Begin, directory);
        } // Dispose must join the active worker before freeing global resources.
        using var successor = CreateCore();
        successor.Call(Begin, directory);
        var fresh = await Terminal(successor);
        Expect(fresh["state"].AsString() == "complete" && fresh["mounted"].AsStringArray().SequenceEqual(mounted),
            "Destruction left global ownership behind");
    }
}
