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
            using var output = new Godot.Collections.Dictionary();
            Expect(ini.Call("try_get_string", "General", "Name", output).AsBool() &&
                output["text"].AsString() == "primary",
                "Native text queries return ICU-decoded text");
            Expect(!ini.Call("try_get_string", "General", "Missing", output).AsBool() && !output.ContainsKey("text"),
                "Missing text queries report failure and clear their previous output");
            foreach (var (codec, expected) in new[]
            {
                ("gbk", "我们需要完成这个任务，保护基地，指挥部队击败敌人。"),
                ("big5", "我們需要完成這個任務，保護基地，指揮部隊擊敗敵人。"),
                ("shift_jis", "この任務を完了して基地を守り、部隊を指揮して敵を倒してください。"),
                ("cp1251", "Необходимо выполнить задание, защитить базу и уничтожить вражеские войска.")
            })
            {
                using var document = core.Call("load_ini", $"text-{codec}.ini").AsGodotObject() as RefCounted;
                Expect(document != null, "Encoding fixture must load: " + codec);
                if (document == null) continue;
                Expect(document.Call("try_get_string", "Text", "Name", output).AsBool() &&
                    output["text"].AsString() == expected, "ICU document detection: " + codec);
                Expect(document.Call("get_string", "Text", "Name", "").AsString() == expected,
                    "Existing string API must also decode through ICU: " + codec);
                Expect(document.Call("get_string", "Text", "Edited", "").AsString() == "UTF-8 编辑",
                    "UTF-8 edits remain valid inside a legacy document: " + codec);
            }
            using var shortText = core.Call("load_ini", "text-short.ini").AsGodotObject() as RefCounted;
            Expect(shortText != null && shortText.Call("get_string", "Text", "Name", "").AsString() == "中文",
                "Ambiguous short Chinese text uses the explicit ICU GB18030 fallback");
            using var invalidText = core.Call("load_ini", "text-invalid.ini").AsGodotObject() as RefCounted;
            Expect(invalidText != null && !invalidText.Call("try_get_string", "Text", "Name", output).AsBool() &&
                !output.ContainsKey("text"), "Invalid input must fail and clear the previous result");
            Expect(invalidText != null && invalidText.Call("get_string", "Text", "Name", "缺省").AsString() == "缺省",
                "Invalid text returns the caller's Unicode fallback without parser errors");
        }
        using (ini)
        {
            Expect(ini.Call("get_string", "General", "Name", "").AsString() == "primary", "INI must survive resource owner disposal");
        }
    }
}
