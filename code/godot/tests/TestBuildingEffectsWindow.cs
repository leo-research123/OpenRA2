using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;

// Uses an isolated map override and the real core/Metal map renderer. No host
// building simulation: mutations go through the existing native command API.
public partial class TestBuildingEffectsWindow : TestRunner
{
    private async Task WaitState(GodotObject target, string method, string wanted)
    {
        var deadline=Time.GetTicksMsec()+30000;
        while(Time.GetTicksMsec()<deadline){
            using var status=target.Call(method).AsGodotDictionary();
            if(status["state"].AsString()==wanted)return;
            if(status["state"].AsString()=="failed")throw new Exception(status["error"].AsString());
            await NextFrame();
        }
        throw new Exception("Timed out waiting for "+wanted);
    }
    protected override async Task RunTestAsync()
    {
        using var instance=ClassDB.Instantiate("RA2Core");using var core=(RefCounted)instance.AsGodotObject();
        var directory=OS.GetCmdlineUserArgs().First(s=>s.StartsWith("--game-data="))[12..];
        core.Call("begin_resource_loading",directory);await WaitState(core,"get_resource_progress","complete");
        var viewport=new SubViewport{Size=new Vector2I(1280,720),RenderTargetUpdateMode=SubViewport.UpdateMode.Always};Root.AddChild(viewport);
        var screen=new TextureRect{Texture=viewport.GetTexture(),Size=new Vector2(1280,720)};Root.AddChild(screen);
        using var mapInstance=ClassDB.Instantiate("RA2MapView");var map=(Node2D)mapInstance.AsGodotObject();viewport.AddChild(map);map.Call("configure",core);
        try{
            map.Call("open_map","RA2OPEN_LIGHT_TEST.MAP");await WaitState(map,"get_render_status","drawn");
            long world=0,id=0;int wx=0,wy=0;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){
                using var item=entry.AsGodotDictionary();if(item["type_id"].AsString()!="CAWASH01")continue;
                world=item["world"].AsInt64();id=item["id"].AsInt64();wx=item["world_x"].AsInt32();wy=item["world_y"].AsInt32();break;
            }
            Expect(id!=0,"fixture light/rubble building exists");if(id==0)return;
            // Center that exact original building through ordinary drag input.
            for(int pass=0;pass<12;++pass){
                using var status=core.Call("get_map_status").AsGodotDictionary();var bounds=status["map_bounds"].AsRect2I();
                int dx=30*(wx-wy)/256-status["camera_x"].AsInt32()-bounds.Size.X/2;
                int dy=15*(wx+wy)/256-status["camera_y"].AsInt32()-bounds.Size.Y/2;
                if(Math.Abs(dx)<8&&Math.Abs(dy)<8)break;
                var start=new Vector2(450,300);var end=start+new Vector2(Math.Clamp(dx*4,-350,350),Math.Clamp(dy*4,-250,250));
                using var press=new InputEventMouseButton{Position=start,ButtonIndex=MouseButton.Right,Pressed=true};
                using var move=new InputEventMouseMotion{Position=end,ButtonMask=MouseButtonMask.Right};
                using var release=new InputEventMouseButton{Position=end,ButtonIndex=MouseButton.Right,Pressed=false};
                map.Call("handle_map_input",press,(Vector2)viewport.Size);map.Call("handle_map_input",move,(Vector2)viewport.Size);map.Call("handle_map_input",release,(Vector2)viewport.Size);await NextFrame();
            }
            async Task CaptureFrame(string name){await DrawFrame();await DrawFrame();using var img=viewport.GetTexture().GetImage();if(CaptureDirectory().Length>0)Expect(img.SavePng(CaptureDirectory().PathJoin(name+".png"))==Error.Ok,"capture "+name);}
            using(var online=core.Call("get_map_object",world,id).AsGodotDictionary())Expect(online["powered"].AsBool(),"fixture spotlight is online");
            await Delay(0.2);await CaptureFrame("effects-online");
            Expect(core.Call("set_map_building_enabled",world,id,false).AsBool(),"disable native building");await Delay(0.2);await CaptureFrame("effects-offline");
            Expect(core.Call("set_map_building_enabled",world,id,true).AsBool(),"restore native building");await Delay(0.2);
            Expect(core.Call("set_map_object_health",world,id,0).AsBool(),"destroy native building");await CaptureFrame("effects-destruction");
            await Delay(1.0);await CaptureFrame("effects-rubble");
            using var dead=core.Call("get_map_object",world,id).AsGodotDictionary();Expect(dead.Count==0,"destroyed building has been removed and its ID expired: "+dead.ToString());
            using var rendered=map.Call("get_render_status").AsGodotDictionary();Expect(rendered["state"].AsString()=="drawn","effects render through production GPU: "+rendered.ToString());
            map.Call("close_map");map.Call("open_map","RA2OPEN_LIGHT_TEST.MAP");await WaitState(map,"get_render_status","drawn");
            using var stale=core.Call("get_map_object",world,id).AsGodotDictionary();Expect(stale.Count==0,"reload rejects destroyed world identity");
        }finally{map.Call("close_map");screen.QueueFree();viewport.QueueFree();await NextFrame();}
    }
}
