using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;

public partial class TestINI : TestRunner
{
    protected override async Task RunTestAsync()
    {
        var directory = OS.GetCmdlineUserArgs().Single(arg => arg.StartsWith("--game-data="))[12..];
        RefCounted ini;
        using (var instance = ClassDB.Instantiate("RA2Core"))
        using (var core = (RefCounted)instance.AsGodotObject())
        {
            Expect(core.Call("load_ini", "config.ini").AsGodotObject() == null, "INI load before resource completion");
            core.Call("begin_resource_loading", directory);
            var deadline = Time.GetTicksMsec() + 10_000;
            while (true)
            {
                using var progress = core.Call("get_resource_progress").AsGodotDictionary();
                if (progress["state"].AsString() != "loading")
                {
                    Expect(progress["state"].AsString() == "complete", "INI test bootstrap failed");
                    break;
                }
                if (Time.GetTicksMsec() > deadline) throw new Exception("INI resource timeout");
                await NextFrame();
            }
            using var loaded = core.Call("load_ini", "config.ini");
            ini = loaded.AsGodotObject() as RefCounted ?? throw new Exception("Native INI loading failed");
            Expect(core.Call("load_ini", "missing.ini").AsGodotObject() == null, "Missing INI must return null");
            Expect(ini.Call("get_section_names").AsStringArray().SequenceEqual(new[] { "General", "Other" }), "Section order");
            Expect(ini.Call("get_key_names", "General").AsStringArray().SequenceEqual(new[] { "Name", "Count", "Enabled", "Ratio" }), "Key order");
            Expect(ini.Call("get_integer", "General", "Count", -1).AsInt32() == 42, "Integer query");
            Expect(ini.Call("get_bool", "General", "Enabled", false).AsBool(), "Bool query");
            Expect(ini.Call("get_double", "General", "Ratio", 0.0).AsDouble() == 0.125, "Double query");
            // Exercise UTF-8 conversion buffers reusing addresses for distinct sections.
            for (var i = 0; i < 50; ++i)
            {
                Expect(ini.Call("get_string", "General", "Name", "").AsString() == "primary", "First section query");
                Expect(ini.Call("get_string", "Other", "Name", "").AsString() == "secondary", "Second section query");
            }
            Expect(!ini.Call("has_key", "general", "Name").AsBool(), "Name hashing is case sensitive");
        }
        using (ini)
        {
            Expect(ini.Call("get_string", "General", "Name", "").AsString() == "primary", "INI must survive resource owner disposal");
        }
    }
}
