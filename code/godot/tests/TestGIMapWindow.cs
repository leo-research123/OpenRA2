using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;

// Real ALL01 map objects and production map renderer, not RA2TestInfantry sprites.
public partial class TestGIMapWindow : TestRunner
{
    private async Task CheckCombat(RefCounted core,Node2D map,SubViewport viewport,long world,long gi)
    {
        async Task Capture(string name){await DrawFrame();if(CaptureDirectory().Length>0){DirAccess.MakeDirRecursiveAbsolute(CaptureDirectory());using var shot=viewport.GetTexture().GetImage();Expect(shot.SavePng(CaptureDirectory().PathJoin(name+".png"))==Error.Ok,"Capture saved");}}
        async Task<Vector2> Find(long id){
            using var item=core.Call("get_map_object",world,id).AsGodotDictionary();
            int wx=item["world_x"].AsInt32(),wy=item["world_y"].AsInt32(),wz=item["world_z"].AsInt32();
            for(int pass=0;pass<32;++pass){
                using var status=core.Call("get_map_status").AsGodotDictionary();var bounds=status["map_bounds"].AsRect2I();
                int dx=30*(wx-wy)/256-status["camera_x"].AsInt32()-bounds.Size.X/2;
                int dy=15*(wx+wy)/256-15*wz/104-status["camera_y"].AsInt32()-bounds.Size.Y/2;
                if(Math.Abs(dx)<8&&Math.Abs(dy)<8)break;
                var start=new Vector2(450,300);var end=start+new Vector2(Math.Clamp(dx*4,-350,350),Math.Clamp(dy*4,-250,250));
                using var press=new InputEventMouseButton{Position=start,ButtonIndex=MouseButton.Right,Pressed=true};
                using var move=new InputEventMouseMotion{Position=end,ButtonMask=MouseButtonMask.Right};
                using var release=new InputEventMouseButton{Position=end,ButtonIndex=MouseButton.Right,Pressed=false};
                map.Call("handle_map_input",press,(Vector2)viewport.Size);map.Call("handle_map_input",move,(Vector2)viewport.Size);map.Call("handle_map_input",release,(Vector2)viewport.Size);await NextFrame();
            }
            using var camera=core.Call("get_map_status").AsGodotDictionary();
            var anchor=new Vector2(30*(wx-wy)/256-camera["camera_x"].AsInt32(),15*(wx+wy)/256-15*wz/104-camera["camera_y"].AsInt32());
            for(int y=-120;y<=10;++y)for(int x=-70;x<=70;++x){
                var point=anchor+new Vector2(x,y);using var motion=new InputEventMouseMotion{Position=point};map.Call("handle_map_input",motion,(Vector2)viewport.Size);
                using var status=core.Call("get_map_world_status").AsGodotDictionary();if(status["hovered_id"].AsInt64()==id)return point;
            }
            throw new Exception("Cannot pick combat object "+id);
        }
        void Click(Vector2 point,bool force=false,bool additive=false){foreach(bool down in new[]{true,false}){using var click=new InputEventMouseButton{Position=point,ButtonIndex=MouseButton.Left,Pressed=down,CtrlPressed=force,ShiftPressed=additive};map.Call("handle_map_input",click,(Vector2)viewport.Size);}}
        void Deploy(){using var key=new InputEventKey{Keycode=Key.D,Pressed=true};map.Call("handle_map_input",key,(Vector2)viewport.Size);}
        if(HasArgument("--harvesting-only")){
            var miners=new System.Collections.Generic.List<long>();
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();if(item["type_id"].AsString()=="CMIN")miners.Add(item["id"].AsInt64());}
            Expect(miners.Count==2,"First map loads its two original chrono miners without duplicate free units");
            if(miners.Count!=2)return;
            // Miner 0 contends for a refinery whose pad is still occupied by
            // miner 1. The native regression also runs six loads for both.
            long miner=miners[0];Click(await Find(miner));map.Call("reset_map_input");await Capture("harvester-start");
            bool moved=false,jumped=false;Vector2I previous;
            using(var actor=core.Call("get_map_object",world,miner).AsGodotDictionary())previous=new Vector2I(actor["world_x"].AsInt32(),actor["world_y"].AsInt32());
            ulong harvestDeadline=Time.GetTicksMsec()+60000;bool capturedMining=false;
            while(Time.GetTicksMsec()<harvestDeadline&&!jumped){
                Root.GrabFocus();await NextFrame();
                using var render=map.Call("get_render_status").AsGodotDictionary();
                if(render["state"].AsString()=="failed")throw new Exception("Harvest render failed: "+render["error"].AsString());
                using var actor=core.Call("get_map_object",world,miner).AsGodotDictionary();
                var at=new Vector2I(actor["world_x"].AsInt32(),actor["world_y"].AsInt32());
                var delta=at-previous;moved|=delta!=Vector2I.Zero;jumped=Math.Abs(delta.X)>512||Math.Abs(delta.Y)>512;previous=at;
                if(moved&&!capturedMining){
                    using var ore=core.Call("get_map_resource",at.X/256,at.Y/256).AsGodotDictionary();
                    if(ore.Count>0&&ore.ContainsKey("units")&&ore["units"].AsInt32()>0){await Find(miner);map.Call("reset_map_input");await Capture("harvester-mining");capturedMining=true;}
                }
            }
            Expect(moved&&jumped,"Miner drives to ore and teleports back through the live game loop");
            await Find(miner);map.Call("reset_map_input");await Capture("harvester-arrival");
            await Delay(2);await Capture("harvester-unloaded");
            GD.Print("HARVEST_WINDOW miner=",miner," moved=",moved," teleported=",jumped);
            return;
        }
        if(HasArgument("--production-only")){
            var initial=new System.Collections.Generic.HashSet<long>();long factory=0;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();initial.Add(item["id"].AsInt64());if(item["type_id"].AsString()=="GAWEAP")factory=item["id"].AsInt64();}
            if(factory==0)throw new Exception("No allied war factory");
            await Find(factory);map.Call("reset_map_input");await Capture("production-ready");
            // Original 1280x720 Allied layout; native test verifies GI slot 0 and MTNK slot 1.
            Click(new Vector2(1207,209));await NextFrame();Click(new Vector2(1154,247));
            Click(new Vector2(1236,209));await NextFrame();Click(new Vector2(1217,247));
            map.Call("reset_map_input");await Delay(0.5);await Capture("production-building");
            long soldier=0,tank=0;ulong productionDeadline=Time.GetTicksMsec()+90000;
            while(Time.GetTicksMsec()<productionDeadline&&(soldier==0||tank==0)){
                Root.GrabFocus();await Delay(0.1);
                using var objects=core.Call("get_map_objects").AsGodotArray();foreach(var entry in objects){using var item=entry.AsGodotDictionary();long id=item["id"].AsInt64();if(initial.Contains(id)||item["world_x"].AsInt32()<=0||item["world_y"].AsInt32()<=0)continue;
                    if(item["type_id"].AsString()=="E1")soldier=id;if(item["type_id"].AsString()=="MTNK")tank=id;
                }
            }
            Expect(soldier!=0&&tank!=0,"Sidebar builds both GI and Grizzly using real factory queues");
            if(tank==0)return;
            await Capture("production-tank-created");await Delay(5);await Capture("production-tank-exited");
            Click(await Find(tank));await Capture("production-tank-selected");
            using(var item=core.Call("get_map_object",world,tank).AsGodotDictionary())Expect(item["selected"].AsBool(),"Newly produced tank is selectable");
            GD.Print("PRODUCTION_WINDOW gi=",soldier," tank=",tank);
            return;
        }
        if(HasArgument("--vehicle-death-only")){
            long shooter=0,destroyedVehicle=0;
            using(var actors=core.Call("get_map_objects").AsGodotArray())foreach(var entry in actors){
                using var actor=entry.AsGodotDictionary();
                if(actor["type_id"].AsString()!="MTNK")continue;
                if(shooter==0)shooter=actor["id"].AsInt64();else if(destroyedVehicle==0)destroyedVehicle=actor["id"].AsInt64();
            }
            if(shooter==0||destroyedVehicle==0)throw new Exception("Missing Grizzly pair for death regression");
            Expect(core.Call("set_map_object_health",world,destroyedVehicle,1).AsBool(),"Prepare a lethal shell target");
            Click(await Find(shooter));var hitPoint=await Find(destroyedVehicle);
            await Capture("vehicle-before-death");
            Click(hitPoint,true);
            using(var release=new InputEventKey{Keycode=Key.Ctrl,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
            map.Call("reset_map_input");
            bool vehicleRemoved=false;ulong vehicleDeadline=Time.GetTicksMsec()+30000;
            while(Time.GetTicksMsec()<vehicleDeadline){
                Root.GrabFocus();await NextFrame();
                using var actor=core.Call("get_map_object",world,destroyedVehicle).AsGodotDictionary();
                if(actor.Count==0){vehicleRemoved=true;break;}
            }
            Expect(vehicleRemoved,"Actual cannon impact removes the original vehicle identity");
            if(!vehicleRemoved)return;
            await Capture("vehicle-exploding");
            long endTick;
            using(var state=core.Call("get_map_world_status").AsGodotDictionary())endTick=state["simulation_tick"].AsInt64()+120;
            vehicleDeadline=Time.GetTicksMsec()+30000;
            while(Time.GetTicksMsec()<vehicleDeadline){
                Root.GrabFocus();await NextFrame();
                using var state=core.Call("get_map_world_status").AsGodotDictionary();
                if(state["simulation_tick"].AsInt64()>=endTick)break;
            }
            using(var motion=new InputEventMouseMotion{Position=hitPoint})map.Call("handle_map_input",motion,(Vector2)viewport.Size);
            using(var state=core.Call("get_map_world_status").AsGodotDictionary()){
                Expect(state["simulation_tick"].AsInt64()>=endTick,"Death effect advances after removal");
                Expect(state["hovered_id"].AsInt64()!=destroyedVehicle,"Destroyed tank cannot be picked");
            }
            using(var actor=core.Call("get_map_object",world,destroyedVehicle).AsGodotDictionary())Expect(actor.Count==0,"Vehicle remains vehicleRemoved");
            await Capture("vehicle-death-cleared");
            GD.Print("VEHICLE_DEATH_WINDOW shooter=",shooter," destroyedVehicle=",destroyedVehicle," vehicleRemoved=True");return;
        }
        if(HasArgument("--grizzly-fire-only")||HasArgument("--grizzly-uphill-only")){
            bool uphill=HasArgument("--grizzly-uphill-only");
            long tank=0,target=0;int initialHealth=0;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){
                using var item=entry.AsGodotDictionary();
                if(tank==0&&item["type_id"].AsString()=="MTNK")tank=item["id"].AsInt64();
                if(target==0&&(uphill
                    ? item["type_id"].AsString()=="INIT"&&item["world_x"].AsInt32()==33088&&item["world_y"].AsInt32()==22720
                    : item["type_id"].AsString()=="GAWEAP")){
                    target=item["id"].AsInt64();initialHealth=item["health"].AsInt32();
                }
            }
            if(tank==0||target==0)throw new Exception("Missing first-map Grizzly/fire target");
            Click(await Find(tank));
            if(uphill){
                // Reach the real flat low cell used by the native regression
                // through a normal move order; no test relocation API.
                await Find(target);
                using(var camera=core.Call("get_map_status").AsGodotDictionary()){
                    var lowCell=new Vector2(30*(31872-22656)/256-camera["camera_x"].AsInt32(),
                        15*(31872+22656)/256-15*728/104-camera["camera_y"].AsInt32());
                    Click(lowCell);
                }
                map.Call("reset_map_input");bool arrived=false;ulong moveDeadline=Time.GetTicksMsec()+90000;
                while(!arrived&&Time.GetTicksMsec()<moveDeadline){
                    Root.GrabFocus();await NextFrame();
                    using var actor=core.Call("get_map_object",world,tank).AsGodotDictionary();
                    if(actor.Count==0)throw new Exception("Grizzly destroyed before reaching the low firing cell");
                    arrived=Math.Abs(actor["world_x"].AsInt32()-31872)<64&&Math.Abs(actor["world_y"].AsInt32()-22656)<64;
                }
                Expect(arrived,"Grizzly reaches the low firing cell");
            }
            Click(await Find(target),!uphill);
            using(var release=new InputEventKey{Keycode=Key.Ctrl,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
            map.Call("reset_map_input");
            long previous=-1;int captures=0;bool hit=false;ulong grizzlyDeadline=Time.GetTicksMsec()+60000;
            while(captures<96&&Time.GetTicksMsec()<grizzlyDeadline){
                Root.GrabFocus();await NextFrame();
                using var status=core.Call("get_map_status").AsGodotDictionary();
                long frame=status["current_frame"].AsInt64();if(frame==previous)continue;previous=frame;
                string name="grizzly-fire-"+captures.ToString("D3");await Capture(name);
                using var grizzlyVictim=core.Call("get_map_object",world,target).AsGodotDictionary();
                hit|=grizzlyVictim["health"].AsInt32()<initialHealth;
                if(CaptureDirectory().Length>0){
                    using var objects=core.Call("get_map_objects").AsGodotArray();
                    using var record=new Godot.Collections.Dictionary{{"status",status},{"objects",objects}};
                    System.IO.File.WriteAllText(CaptureDirectory().PathJoin(name+".json"),Json.Stringify(record,"  "));
                }
                ++captures;
            }
            Expect(captures==96&&hit,"Grizzly fires and hits the clicked target through normal input");
            GD.Print("GRIZZLY_FIRE_WINDOW frames=",captures," uphill=",uphill," target_hit=",hit);return;
        }
        if(HasArgument("--ifv-repair-only")||HasArgument("--ifv-unload-only")){
            long engineer=0,carrier=0,target=0;
            int owner;
            using(var actor=core.Call("get_map_object",world,gi).AsGodotDictionary())owner=actor["owner_index"].AsInt32();
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){
                using var item=entry.AsGodotDictionary();if(item["owner_index"].AsInt32()!=owner)continue;
                string type=item["type_id"].AsString();long id=item["id"].AsInt64();
                if(type=="ENGINEER"&&engineer==0)engineer=id;
                if(type=="FV"&&carrier==0)carrier=id;
                if(type=="MTNK"&&target==0)target=id;
            }
            if(engineer==0||carrier==0||target==0)throw new Exception("Missing first-map IFV repair actors");
            long Tick(){using var state=core.Call("get_map_world_status").AsGodotDictionary();return state["simulation_tick"].AsInt64();}
            async Task Advance(int ticks){
                long until=Tick()+ticks;ulong deadline=Time.GetTicksMsec()+30000;
                while(Tick()<until){Root.GrabFocus();await NextFrame();if(Time.GetTicksMsec()>deadline)throw new Exception("IFV simulation stalled");}
            }
            async Task Shot(string name){
                await Capture(name);
                using var state=core.Call("get_map_status").AsGodotDictionary();
                using var units=core.Call("get_map_objects").AsGodotArray();
                using var record=new Godot.Collections.Dictionary{{"status",state},{"objects",units}};
                if(CaptureDirectory().Length>0)System.IO.File.WriteAllText(CaptureDirectory().PathJoin(name+".json"),Json.Stringify(record,"  "));
            }
            await Find(carrier);map.Call("reset_map_input");await Advance(2);await Shot("ifv-before-order");
            using var unlit=viewport.GetTexture().GetImage();
            Vector2I carrierPixel;
            using(var actor=core.Call("get_map_object",world,carrier).AsGodotDictionary())
            using(var camera=core.Call("get_map_status").AsGodotDictionary()){
                int x=actor["world_x"].AsInt32(),y=actor["world_y"].AsInt32(),z=actor["world_z"].AsInt32();
                carrierPixel=new Vector2I(30*(x-y)/256-camera["camera_x"].AsInt32(),15*(x+y)/256-15*z/104-camera["camera_y"].AsInt32());
            }
            Click(await Find(engineer));Click(await Find(carrier));map.Call("reset_map_input");
            await Advance(1);await Shot("ifv-command-flash");
            await Advance(12);await Shot("ifv-flash-expired");
            using(var expired=viewport.GetTexture().GetImage()){
                int changed=0;
                // Stationary empty IFV, same camera, before the engineer arrives.
                // Compare the complete 60 x 34 pixel body region at threshold zero.
                for(int y=carrierPixel.Y-22;y<carrierPixel.Y+12;++y)
                    for(int x=carrierPixel.X-30;x<carrierPixel.X+30;++x)
                        if(unlit.GetPixel(x,y)!=expired.GetPixel(x,y))++changed;
                Expect(changed==0,"IFV returns to its pre-click pixels after the flash: "+changed+" changed pixels");
                GD.Print("IFV_FLASH_PIXELS changed=",changed," checked=2040");
            }
            await Advance(100);await Shot("ifv-engineer-entered");
            using(var actor=core.Call("get_map_object",world,engineer).AsGodotDictionary())Expect(!actor["selected"].AsBool(),"Engineer entered the IFV");
            if(HasArgument("--ifv-unload-only")){
                for(int cycle=0;cycle<2;++cycle){
                    Click(await Find(carrier));Deploy();map.Call("reset_map_input");
                    await Advance(120);
                    Click(await Find(engineer));
                    using(var actor=core.Call("get_map_object",world,engineer).AsGodotDictionary())
                        Expect(actor["selected"].AsBool(),"Deployed passenger is visible and selectable, cycle "+cycle);
                    await Shot("ifv-unloaded-"+cycle);
                    if(cycle==0){Click(await Find(carrier));map.Call("reset_map_input");await Advance(120);await Shot("ifv-reentered");}
                }
                GD.Print("IFV_UNLOAD_WINDOW deploy_cycles=2 visible_and_selectable=True");return;
            }
            Expect(core.Call("set_map_object_health",world,target,150).AsBool(),"Damage the repair target");
            Click(await Find(carrier));Click(await Find(target));map.Call("reset_map_input");
            int repairHealth=150;
            for(int frame=0;frame<48;++frame){
                await Advance(2);await Shot("ifv-repair-"+frame.ToString("D2"));
                using var repaired=core.Call("get_map_object",world,target).AsGodotDictionary();repairHealth=repaired["health"].AsInt32();
            }
            Expect(repairHealth>150,"Actual IFV repair restores target health");
            await Advance(200);await Shot("ifv-repair-finished");
            using(var repaired=core.Call("get_map_object",world,target).AsGodotDictionary())Expect(repaired["health"].AsInt32()==repaired["max_health"].AsInt32(),"Repair completes");
            GD.Print("IFV_REPAIR_WINDOW carrier=",carrier," engineer=",engineer," target=",target," health=",repairHealth);
            return;
        }
        if(HasArgument("--civilian-garrison-only")){
            var members=new System.Collections.Generic.List<long>();
            long building=0;int before=0,owner=-1,neutral=-1;double distance=double.MaxValue;
            using(var actor=core.Call("get_map_object",world,gi).AsGodotDictionary()){
                owner=actor["owner_index"].AsInt32();
                using var objects=core.Call("get_map_objects").AsGodotArray();
                foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                    if(item["type_id"].AsString()=="E1")members.Add(item["id"].AsInt64());
                    var type=item["type_id"].AsString();
                    if(type!="CASANF02"&&type!="CASANF03"&&type!="CASTL02")continue;
                    // Original CanBeOccupiedBy rejects red-health buildings;
                    // several nearby ALL01 houses start exactly at 25% HP.
                    if(item["health_band"].AsString()=="red"||!item["alive"].AsBool())continue;
                    double dx=item["world_x"].AsInt32()-actor["world_x"].AsInt32(),dy=item["world_y"].AsInt32()-actor["world_y"].AsInt32();
                    if(dx*dx+dy*dy<distance){distance=dx*dx+dy*dy;building=item["id"].AsInt64();before=item["health"].AsInt32();neutral=item["owner_index"].AsInt32();}
                }
            }
            Expect(members.Count==5&&building!=0,"Five map GI and non-red civilian building exist");
            if(members.Count!=5||building==0)return;
            int entering=HasArgument("--garrison-target-flash")?4:members.Count;
            for(int i=0;i<entering;++i)Click(await Find(members[i]),false,i!=0);
            Click(await Find(building));await Capture("civilian-plain-click");
            bool entered=false;var garrisonDeadline=Time.GetTicksMsec()+60000;
            while(Time.GetTicksMsec()<garrisonDeadline){
                await NextFrame();using var target=core.Call("get_map_object",world,building).AsGodotDictionary();
                if(target.Count==0||target["health"].AsInt32()!=before){Expect(false,"Plain click damaged the civilian building");return;}
                entered=target["owner_index"].AsInt32()==owner;
                foreach(long id in members.GetRange(0,entering)){using var actor=core.Call("get_map_object",world,id).AsGodotDictionary();
                    entered&=actor.Count>0&&actor["alive"].AsBool()&&!actor["selected"].AsBool();
                }
                if(entered)break;
            }
            Expect(entered,"Plain click moves the selected GI into the civilian building");
            if(!entered)return;
            if(HasArgument("--garrison-target-flash")){
                Click(await Find(members[4]));Click(await Find(building),true);
                using(var release=new InputEventKey{Keycode=Key.Ctrl,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
                await Capture("occupied-building-target-flash");
                // The crash occurred during the flash's next intensity change,
                // before damage was required. Advance beyond the whole flash.
                await Delay(1.0);
                int targetHealth=before;var hitDeadline=Time.GetTicksMsec()+30000;
                while(Time.GetTicksMsec()<hitDeadline){await NextFrame();using var target=core.Call("get_map_object",world,building).AsGodotDictionary();
                    Expect(target.Count>0,"Occupied target survives");if(target.Count==0)return;
                    targetHealth=target["health"].AsInt32();
                    Expect(target["owner_index"].AsInt32()==owner,"Target remains garrisoned after the flash");
                    if(targetHealth<before)break;
                }
                Expect(targetHealth<before,"Fifth GI attacks the occupied friendly building");
                await Capture("occupied-building-after-flash");
                GD.Print("GARRISON_TARGET_FLASH entered=",entering," health=",before,"->",targetHealth," survived=True");return;
            }
            Click(await Find(building));await Capture("civilian-garrisoned");
            long garrisonTarget=0;int targetBefore=0;double targetDistance=double.MaxValue;
            using(var source=core.Call("get_map_object",world,building).AsGodotDictionary())
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                if(item["kind"].AsString()!="building"||item["id"].AsInt64()==building||!item["type_id"].AsString().StartsWith("CASANF")||!item["alive"].AsBool())continue;
                double dx=item["world_x"].AsInt32()-source["world_x"].AsInt32(),dy=item["world_y"].AsInt32()-source["world_y"].AsInt32();
                if(dx*dx+dy*dy<targetDistance){targetDistance=dx*dx+dy*dy;garrisonTarget=item["id"].AsInt64();targetBefore=item["health"].AsInt32();}
            }
            Expect(garrisonTarget!=0,"Nearby civilian target exists for garrison force attack");if(garrisonTarget==0)return;
            Click(await Find(garrisonTarget),true);
            using(var release=new InputEventKey{Keycode=Key.Ctrl,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
            bool garrisonHit=false;garrisonDeadline=Time.GetTicksMsec()+10000;
            while(Time.GetTicksMsec()<garrisonDeadline){await NextFrame();using var target=core.Call("get_map_object",world,garrisonTarget).AsGodotDictionary();
                if(target.Count==0||target["health"].AsInt32()<targetBefore){garrisonHit=true;break;}
            }
            Expect(garrisonHit,"Occupied building executes Ctrl click and hits the target after Ctrl release");
            using(var selected=core.Call("get_map_object",world,building).AsGodotDictionary())Expect(selected["selected"].AsBool(),"Garrison attack keeps the building selected");
            await Capture("garrison-force-attack");
            if(HasArgument("--hud-deploy")){
                using var status=core.Call("get_map_status").AsGodotDictionary();var bar=status["hud_bounds"].AsRect2I();
                // Original ALL01/UIMD at 1280x720: the native gadget test reads
                // this centre from CommandPositions[4] and the loaded SHP size.
                Click(new Vector2(226,bar.Position.Y+16));
            }else Deploy();
            bool unloaded=false;garrisonDeadline=Time.GetTicksMsec()+10000;
            while(Time.GetTicksMsec()<garrisonDeadline){await NextFrame();using var target=core.Call("get_map_object",world,building).AsGodotDictionary();
                if(target["owner_index"].AsInt32()==neutral){unloaded=true;break;}
            }
            Expect(unloaded,"Deploy command unload returns the civilian building to Neutral");
            if(!unloaded)return;
            await Delay(0.5);
            // All five original identities must become pickable again after D.
            for(int i=0;i<members.Count;++i)Click(await Find(members[i]),false,i!=0);
            await Capture("civilian-unloaded");Click(await Find(building),true);
            using(var release=new InputEventKey{Keycode=Key.Ctrl,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
            int after=before;garrisonDeadline=Time.GetTicksMsec()+10000;
            while(Time.GetTicksMsec()<garrisonDeadline){await NextFrame();using var target=core.Call("get_map_object",world,building).AsGodotDictionary();
                after=target["health"].AsInt32();if(after<before)break;
            }
            Expect(after<before,"Ctrl click attacks the same civilian building after unloading");
            await Capture("civilian-control-attack");
            GD.Print("CIVILIAN_GARRISON members=",members.Count," entered=",entered," garrisonHit=",garrisonHit," hud=",HasArgument("--hud-deploy")," unloaded=",unloaded," attackHealth=",before,"->",after);
            return;
        }
        if(HasArgument("--ore-walk-only")){
            Click(await Find(gi));
            const int tx=106*256+128,ty=117*256+128,tz=416;
            for(int pass=0;pass<32;++pass){
                using var status=core.Call("get_map_status").AsGodotDictionary();var bounds=status["map_bounds"].AsRect2I();
                int dx=30*(tx-ty)/256-status["camera_x"].AsInt32()-bounds.Size.X/2;
                int dy=15*(tx+ty)/256-15*tz/104-status["camera_y"].AsInt32()-bounds.Size.Y/2;
                if(Math.Abs(dx)<8&&Math.Abs(dy)<8)break;
                var start=new Vector2(450,300);var end=start+new Vector2(Math.Clamp(dx*4,-350,350),Math.Clamp(dy*4,-250,250));
                using var press=new InputEventMouseButton{Position=start,ButtonIndex=MouseButton.Right,Pressed=true};
                using var move=new InputEventMouseMotion{Position=end,ButtonMask=MouseButtonMask.Right};
                using var release=new InputEventMouseButton{Position=end,ButtonIndex=MouseButton.Right,Pressed=false};
                map.Call("handle_map_input",press,(Vector2)viewport.Size);map.Call("handle_map_input",move,(Vector2)viewport.Size);map.Call("handle_map_input",release,(Vector2)viewport.Size);await NextFrame();
            }
            using(var camera=core.Call("get_map_status").AsGodotDictionary()){
                var destination=new Vector2(30*(tx-ty)/256-camera["camera_x"].AsInt32(),15*(tx+ty)/256-15*tz/104-camera["camera_y"].AsInt32());
                using var motion=new InputEventMouseMotion{Position=destination};map.Call("handle_map_input",motion,(Vector2)viewport.Size);
                using var status=core.Call("get_map_world_status").AsGodotDictionary();Expect(status["hovered_id"].AsInt64()==0,"Ore destination is ground");
                Click(destination);
            }
            using(var actor=core.Call("get_map_object",world,gi).AsGodotDictionary()){
                Expect(actor["selected"].AsBool(),"GI remains selected after ore movement order");GD.Print("ORE_ORDER ",actor);
            }
            int captured=0;long lastTick=-1;Vector2I previous=new(-1,-1);var oreDeadline=Time.GetTicksMsec()+60000;
            while(Time.GetTicksMsec()<oreDeadline&&captured<3){
                await NextFrame();using var state=core.Call("get_map_object",world,gi).AsGodotDictionary();
                Expect(state.Count>0&&state["alive"].AsBool(),"GI remains alive crossing ore");if(state.Count==0)return;
                using var resource=core.Call("get_map_resource",state["cell_x"],state["cell_y"]).AsGodotDictionary();
                using var worldStatus=core.Call("get_map_world_status").AsGodotDictionary();long tick=worldStatus["simulation_tick"].AsInt64();
                var at=new Vector2I(state["world_x"].AsInt32(),state["world_y"].AsInt32());
                if(resource.Count==0||resource["units"].AsInt32()==0||at==previous||tick<lastTick+3)continue;
                previous=at;lastTick=tick;await Capture("ore-walking-"+captured);++captured;
                GD.Print("ORE_WALK at=",at," tick=",tick," units=",resource["units"]);
            }
            if(captured!=3){using var actor=core.Call("get_map_object",world,gi).AsGodotDictionary();using var status=core.Call("get_map_world_status").AsGodotDictionary();GD.Print("ORE_TIMEOUT actor=",actor," status=",status);}
            Expect(captured==3,"Three advancing GI positions over actual ore are rendered");return;
        }
        if(HasArgument("--group-building-attack-only")){
            var starts=new System.Collections.Generic.Dictionary<long,Vector2I>();
            long factory=0;int before=0;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                if(item["type_id"].AsString()=="E1")starts.Add(item["id"].AsInt64(),new(item["world_x"].AsInt32(),item["world_y"].AsInt32()));
                if(item["type_id"].AsString()=="GAWEAP"){factory=item["id"].AsInt64();before=item["health"].AsInt32();}
            }
            Expect(starts.Count==5&&factory!=0,"Five GI and factory exist");
            bool additive=false;
            foreach(long actor in starts.Keys){Click(await Find(actor),false,additive);additive=true;}
            foreach(long actor in starts.Keys){using var state=core.Call("get_map_object",world,actor).AsGodotDictionary();Expect(state["selected"].AsBool(),"Shift-click selects each GI");}
            Click(await Find(factory),true);await Capture("group-building-order");
            var positions=new System.Collections.Generic.Dictionary<long,Vector2I>(starts);
            int stable=0,groupHealth=before;long lastSimulationTick=-1;var groupDeadline=Time.GetTicksMsec()+45000;
            while(Time.GetTicksMsec()<groupDeadline&&stable<60){await NextFrame();bool allMoved=true,changed=false;
                using(var status=core.Call("get_map_world_status").AsGodotDictionary()){
                    long tick=status["simulation_tick"].AsInt64();
                    // Losing window focus pauses simulation while rendering keeps
                    // ticking. Those frames cannot prove the group has stopped.
                    if(tick==lastSimulationTick)continue;lastSimulationTick=tick;
                }
                using(var target=core.Call("get_map_object",world,factory).AsGodotDictionary()){
                    Expect(target.Count>0,"Factory survives initial approach");if(target.Count==0)break;groupHealth=target["health"].AsInt32();
                }
                foreach(long actor in starts.Keys){using var state=core.Call("get_map_object",world,actor).AsGodotDictionary();
                    Expect(state.Count>0&&state["selected"].AsBool(),"Group member stays alive and selected");if(state.Count==0)return;
                    var at=new Vector2I(state["world_x"].AsInt32(),state["world_y"].AsInt32());
                    allMoved&=at!=starts[actor];changed|=at!=positions[actor];positions[actor]=at;
                }
                stable=allMoved&&!changed&&groupHealth<before?stable+1:0;
            }
            Expect(stable>=60&&groupHealth<before,"All five finish approach while the building takes fire");
            await Capture("group-building-firing");
            foreach(var actor in positions)GD.Print("GROUP_BUILDING actor=",actor.Key," at=",actor.Value);
            GD.Print("GROUP_BUILDING groupHealth=",before,"->",groupHealth," stable=",stable);
            return;
        }
        if(HasArgument("--force-attack-only")){
            bool mac=OS.GetName()=="macOS";
            void Button(Vector2 point,bool down,bool control,bool physicalRight=false){
                using var input=new InputEventMouseButton{Position=point,
                    ButtonIndex=physicalRight||mac?MouseButton.Right:MouseButton.Left,
                    ButtonMask=down?(physicalRight?MouseButtonMask.Right:MouseButtonMask.Left):0,
                    Pressed=down,CtrlPressed=control};
                map.Call("handle_map_input",input,(Vector2)viewport.Size);
            }
            bool Selected(long target){using var state=core.Call("get_map_object",world,target).AsGodotDictionary();return state.Count>0&&state["selected"].AsBool();}
            long factory=0;int factoryHealth=0;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                if(item["type_id"].AsString()=="GAWEAP"){factory=item["id"].AsInt64();factoryHealth=item["health"].AsInt32();break;}}
            Expect(factory!=0,"Friendly factory exists");Click(await Find(gi));var point=await Find(factory);
            Button(point,true,true);
            using(var motion=new InputEventMouseMotion{Position=point+new Vector2(1,0),ButtonMask=MouseButtonMask.Left,CtrlPressed=true}){
                using var response=map.Call("handle_map_input",motion,(Vector2)viewport.Size).AsGodotDictionary();
                Expect(!response.ContainsKey("warp_pointer"),"Ctrl-left does not start right-button scrolling");
            }
            Button(point,false,true);
            using(var release=new InputEventKey{Keycode=Key.Ctrl,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
            Expect(Selected(gi)&&!Selected(factory),"Force attack keeps the commanded infantry selected");
            bool damaged=false;var forceDeadline=Time.GetTicksMsec()+60000;
            while(Time.GetTicksMsec()<forceDeadline){await NextFrame();using var state=core.Call("get_map_object",world,factory).AsGodotDictionary();
                if(state["health"].AsInt32()<factoryHealth){damaged=true;break;}}
            Expect(damaged,"Ctrl-left attacks a friendly building after Ctrl is released");
            // The release keeps the down-time button identity, but uses current
            // modifiers for the game's ordinary action classifier.
            point=await Find(factory);Button(point,true,true);
            using(var release=new InputEventKey{Keycode=Key.Ctrl,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
            Button(point,false,false);
            Expect(Selected(factory),"Releasing Ctrl before mouse-up still completes a left click");
            Click(await Find(gi));point=await Find(factory);Button(point,true,true);map.Call("reset_map_input");
            Button(point,false,false,true);
            Expect(!Selected(gi),"Focus reset clears the emulated button latch");
            Click(await Find(gi));point=await Find(factory);
            Button(point,true,true,true);Button(point,false,true,true);
            Expect(!Selected(gi),"Physical Ctrl-right still cancels selection");
            await Capture("force-attack");return;
        }
        if(HasArgument("--ggi-visual-only")){
            long ggi=0,building=0;int buildingHealth=0;double distance=double.MaxValue;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();if(item["type_id"].AsString()=="GGI"){ggi=item["id"].AsInt64();break;}}
            Expect(ggi!=0,"Actual GGI exists");
            using(var actor=core.Call("get_map_object",world,ggi).AsGodotDictionary())
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                if(item["type_id"].AsString()!="CASANF03")continue;
                double dx=item["world_x"].AsInt32()-actor["world_x"].AsInt32(),dy=item["world_y"].AsInt32()-actor["world_y"].AsInt32();
                if(dx*dx+dy*dy<distance){distance=dx*dx+dy*dy;building=item["id"].AsInt64();buildingHealth=item["health"].AsInt32();}
            }
            Expect(building!=0,"GGI target exists");Click(await Find(ggi));Click(await Find(building),true);
            bool approached=false;var ggiDeadline=Time.GetTicksMsec()+60000;
            while(Time.GetTicksMsec()<ggiDeadline){await NextFrame();using var state=core.Call("get_map_object",world,building).AsGodotDictionary();if(state["health"].AsInt32()<buildingHealth){approached=true;break;}}
            Expect(approached,"GGI stops at firing position");Deploy();await Delay(1.5);
            int deployedHealth;
            using(var state=core.Call("get_map_object",world,building).AsGodotDictionary())deployedHealth=state["health"].AsInt32();
            Click(await Find(building),true);await Capture("ggi-deployed");
            bool missileHit=false;
            for(int i=0;i<50;++i){await Delay(0.15);await Capture("ggi-missile-"+i.ToString("D2"));
                using var state=core.Call("get_map_object",world,building).AsGodotDictionary();
                if(state["health"].AsInt32()<deployedHealth){missileHit=true;break;}}
            Expect(missileHit,"Deployed GGI missile completes flight and hits");
            using(var state=core.Call("get_map_object",world,building).AsGodotDictionary())GD.Print("GGI_VISUAL health=",buildingHealth,"->",state["health"].AsInt32());
            return;
        }
        if(HasArgument("--dog-combat-only")){
            long dog=0;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                if(item["type_id"].AsString()=="ADOG"){dog=item["id"].AsInt64();break;}}
            Expect(dog!=0,"Actual map dog exists");Click(await Find(dog));
            for(int bite=0;bite<2;++bite){
                long dogVictim=0;double dogNearest=double.MaxValue;
                using(var actor=core.Call("get_map_object",world,dog).AsGodotDictionary())
                using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                    if(item["type_id"].AsString()!="INIT"||item["health"].AsInt32()<=0)continue;
                    double dx=item["world_x"].AsInt32()-actor["world_x"].AsInt32(),dy=item["world_y"].AsInt32()-actor["world_y"].AsInt32();
                    if(dx*dx+dy*dy<dogNearest){dogNearest=dx*dx+dy*dy;dogVictim=item["id"].AsInt64();}
                }
                Expect(dogVictim!=0,"Dog has an enemy infantry target");Click(await Find(dogVictim));
                bool dogRemoved=false;var dogDeadline=Time.GetTicksMsec()+60000;
                while(Time.GetTicksMsec()<dogDeadline){await NextFrame();using var state=core.Call("get_map_object",world,dogVictim).AsGodotDictionary();if(state.Count==0){dogRemoved=true;break;}}
                Expect(dogRemoved,"Dog kills clicked infantry");
                using(var actor=core.Call("get_map_object",world,dog).AsGodotDictionary())Expect(actor.Count>0&&actor["health"].AsInt32()>0&&actor["selected"].AsBool(),"Dog returns alive and selected");
                await Capture("dog-bite-"+bite);GD.Print("DOG_BITE index=",bite," removed=",dogRemoved);
            }
            return;
        }
        if(HasArgument("--building-click-only")||HasArgument("--building-visual-only")){
            bool visualCheck=HasArgument("--building-visual-only");
            long factory=0;int before=0;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                if(item["kind"].AsString()=="building"&&item["type_id"].AsString()=="GAWEAP"){factory=item["id"].AsInt64();before=item["health"].AsInt32();break;}}
            Expect(factory!=0,"Actual map factory exists");
            Click(await Find(gi));Click(await Find(factory));
            using(var selected=core.Call("get_map_object",world,factory).AsGodotDictionary())Expect(selected["selected"].AsBool(),"Normal friendly building click selects it");
            Click(await Find(gi));var factoryPoint=await Find(factory);
            for(int i=0;i<3;++i){Click(factoryPoint,true);await NextFrame();}
            await Capture("gi-building-click-flash");
            int after=before;var buildingDeadline=Time.GetTicksMsec()+30000;
            while(Time.GetTicksMsec()<buildingDeadline&&after==before){await NextFrame();using var state=core.Call("get_map_object",world,factory).AsGodotDictionary();after=state["health"].AsInt32();}
            Expect(after<before,"Selected GI building attack executes its Event, flashes, approaches and hits");
            await Delay(0.5);await Capture("gi-building-click-hit");GD.Print("GI_BUILDING_CLICK health=",before,"->",after);
            long civilian=0;int civilianHealth=0;double distance=double.MaxValue;
            using(var actor=core.Call("get_map_object",world,gi).AsGodotDictionary())
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
                if(item["kind"].AsString()!="building"||item["type_id"].AsString()!="CASANF02")continue;
                double dx=item["world_x"].AsInt32()-actor["world_x"].AsInt32(),dy=item["world_y"].AsInt32()-actor["world_y"].AsInt32();
                if(dx*dx+dy*dy<distance){distance=dx*dx+dy*dy;civilian=item["id"].AsInt64();civilianHealth=item["health"].AsInt32();}
            }
            Expect(civilian!=0,"Actual occupiable civilian building exists");Click(await Find(civilian),true);
            if(visualCheck)await Capture("civilian-before");
            int hoverChecks=0,hoverLosses=0,lastCivilianHealth=civilianHealth,civilianHits=0;
            bool destroyed=false,deployed=false;buildingDeadline=Time.GetTicksMsec()+90000;
            while(Time.GetTicksMsec()<buildingDeadline){await NextFrame();using var state=core.Call("get_map_object",world,civilian).AsGodotDictionary();
                if(state.Count==0){destroyed=true;break;}
                int currentHealth=state["health"].AsInt32();
                if(visualCheck&&currentHealth>0){
                    using var hover=core.Call("get_map_world_status").AsGodotDictionary();
                    ++hoverChecks;if(hover["hovered_id"].AsInt64()!=civilian)++hoverLosses;
                    if(currentHealth<lastCivilianHealth){++civilianHits;if(civilianHits<=3)await Capture("civilian-hit-"+civilianHits);}
                    lastCivilianHealth=currentHealth;
                }
                if(!deployed&&currentHealth<civilianHealth){
                    Deploy();deployed=true;
                    if(visualCheck){await Delay(1.5);Click(await Find(civilian),true);}
                    await Capture("gi-building-destroy-firing");
                }
            }
            Expect(destroyed,"Occupiable building reaches zero and is removed after actual repeated shots");
            if(visualCheck){Expect(hoverChecks>0&&hoverLosses==0,"Hover health stays visible throughout building damage");
                GD.Print("BUILDING_VISUAL hits=",civilianHits," hover_checks=",hoverChecks," hover_losses=",hoverLosses);}
            if(visualCheck){
                await Capture("civilian-explosion-0");
                for(int i=1;i<=6;++i){await Delay(0.1);await Capture("civilian-explosion-"+i);}
            }
            await Delay(1.0);await Capture("gi-building-destroy-cleared");GD.Print("GI_BUILDING_DESTROY initial_health=",civilianHealth," removed=",destroyed);return;
        }
        Click(await Find(gi));Deploy();await Delay(1.5);await Capture("gi-combat-deployed");
        Click(await Find(gi));await Delay(1.5);
        long victim=0;double nearest=double.MaxValue;int health=0;
        using(var actor=core.Call("get_map_object",world,gi).AsGodotDictionary())
        using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){
            using var item=entry.AsGodotDictionary();if(item["kind"].AsString()!="infantry"||item["type_id"].AsString()!="INIT")continue;
            double dx=item["world_x"].AsInt32()-actor["world_x"].AsInt32(),dy=item["world_y"].AsInt32()-actor["world_y"].AsInt32();
            if(dx*dx+dy*dy<nearest){nearest=dx*dx+dy*dy;victim=item["id"].AsInt64();health=item["health"].AsInt32();}
        }
        Expect(victim!=0,"Original map contains an enemy initiate");var victimPoint=await Find(victim);
        using(var selected=core.Call("get_map_object",world,gi).AsGodotDictionary())Expect(selected["selected"].AsBool(),"Right drag retains the attacking GI selection");
        Click(victimPoint);
        int hits=0;bool removed=false;var deadline=Time.GetTicksMsec()+60000;
        while(Time.GetTicksMsec()<deadline){
            await NextFrame();using var state=core.Call("get_map_object",world,victim).AsGodotDictionary();
            if(state.Count==0){removed=true;break;}
            if(state["health"].AsInt32()<health){health=state["health"].AsInt32();++hits;
                if(hits==1){await Capture("gi-combat-first-hit");Deploy();}
                if(hits==2)await Capture("gi-combat-deployed-fire");
                if(health==0)await Capture("gi-combat-death");
            }
        }
        Expect(hits>=2&&removed,"Left click approaches enemy, fires repeatedly, deploys while fighting, and removes dead target");
        await Capture("gi-combat-cleared");GD.Print("GI_COMBAT hits=",hits," removed=",removed);
    }
    private async Task WaitState(GodotObject target,string method,string wanted)
    {
        var deadline=Time.GetTicksMsec()+60000;
        while(Time.GetTicksMsec()<deadline){
            using var state=target.Call(method).AsGodotDictionary();
            if(state["state"].AsString()==wanted)return;
            if(state["state"].AsString()=="failed")throw new Exception(state.ToString());
            await NextFrame();
        }
        throw new Exception("Timed out waiting for "+wanted);
    }
    private async Task CheckHover(RefCounted core,Node2D map,SubViewport viewport,long world,long id,int wx,int wy,int wz)
    {
        using var camera=core.Call("get_map_status").AsGodotDictionary();
        var center=new Vector2(30*(wx-wy)/256-camera["camera_x"].AsInt32(),15*(wx+wy)/256-15*wz/104-camera["camera_y"].AsInt32());
        var pick=Vector2.Zero;bool found=false;
        void Move(Vector2 point){using var motion=new InputEventMouseMotion{Position=point};map.Call("handle_map_input",motion,(Vector2)viewport.Size);}
        for(int y=-20;y<=10&&!found;++y)for(int x=-12;x<=12&&!found;++x){
            var point=center+new Vector2(x,y);Move(point);
            using var status=core.Call("get_map_world_status").AsGodotDictionary();
            if(status["hovered_id"].AsInt64()==id){pick=point;found=true;}
        }
        Expect(found,"Hover finds the real GI sprite");
        async Task Capture(string name){
            await DrawFrame();await DrawFrame();
            if(CaptureDirectory().Length>0){using var shot=viewport.GetTexture().GetImage();shot.SavePng(CaptureDirectory().PathJoin(name+".png"));}
        }
        using(var item=core.Call("get_map_object",world,id).AsGodotDictionary())Expect(!item["selected"].AsBool(),"Hover does not select GI");
        await Capture("gi-hover");
        // Stay inside the sidebar, away from the screen-edge scroll zone.
        Move(new Vector2(1180,360));await Capture("gi-hover-left");
        using(var status=core.Call("get_map_world_status").AsGodotDictionary())Expect(status["hovered_id"].AsInt64()==0,"Sidebar clears hover");
        Move(pick);
        foreach(bool down in new[]{true,false}){using var click=new InputEventMouseButton{Position=pick,ButtonIndex=MouseButton.Left,Pressed=down};map.Call("handle_map_input",click,(Vector2)viewport.Size);}
        await Capture("gi-hover-selected");
        using(var item=core.Call("get_map_object",world,id).AsGodotDictionary())Expect(item["selected"].AsBool(),"Left click selects the hovered GI");
        GD.Print("GI_HOVER enter/leave/selected complete");
    }
    private async Task CheckFactoryRoute(RefCounted core,Node2D map,SubViewport viewport,long world)
    {
        var members=new System.Collections.Generic.Dictionary<long,Vector2I>();
        var first=new Vector2(1280,720);var last=Vector2.Zero;
        using var camera=core.Call("get_map_status").AsGodotDictionary();
        int cx=camera["camera_x"].AsInt32(),cy=camera["camera_y"].AsInt32();
        Vector2 Project(int x,int y,int z)=>new(30*(x-y)/256-cx,15*(x+y)/256-15*z/104-cy);
        using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){
            using var item=entry.AsGodotDictionary();if(item["kind"].AsString()!="infantry"||item["type_id"].AsString()!="E1")continue;
            int x=item["world_x"].AsInt32(),y=item["world_y"].AsInt32();members[item["id"].AsInt64()]=new(x,y);
            var point=Project(x,y,item["world_z"].AsInt32());first=first.Min(point-new Vector2(15,22));last=last.Max(point+new Vector2(15,10));
        }
        Expect(members.Count==5,"Factory route starts with the five original GI");
        using(var press=new InputEventMouseButton{Position=last,ButtonIndex=MouseButton.Left,Pressed=true})map.Call("handle_map_input",press,(Vector2)viewport.Size);
        using(var drag=new InputEventMouseMotion{Position=first,ButtonMask=MouseButtonMask.Left})map.Call("handle_map_input",drag,(Vector2)viewport.Size);
        using(var release=new InputEventMouseButton{Position=first,ButtonIndex=MouseButton.Left,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
        await DrawFrame();await DrawFrame();
        if(CaptureDirectory().Length>0){using var shot=viewport.GetTexture().GetImage();shot.SavePng(CaptureDirectory().PathJoin("gi-factory-selected.png"));}
        // Fixed failing ALL01 ground cell, not a target chosen for path success.
        var target=new Vector2I(93,108);var click=Project(target.X*256+128,target.Y*256+128,416);
        using(var hover=new InputEventMouseMotion{Position=click})map.Call("handle_map_input",hover,(Vector2)viewport.Size);
        using(var status=core.Call("get_map_world_status").AsGodotDictionary())Expect(status["hovered_id"].AsInt64()==0,"Factory destination is ground, not a building click");
        using(var press=new InputEventMouseButton{Position=click,ButtonIndex=MouseButton.Left,Pressed=true})map.Call("handle_map_input",press,(Vector2)viewport.Size);
        using(var release=new InputEventMouseButton{Position=click,ButtonIndex=MouseButton.Left,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
        var positions=new System.Collections.Generic.Dictionary<long,Vector2I>(members);
        int arrived=0,stable=0;var deadline=Time.GetTicksMsec()+30000;
        while(Time.GetTicksMsec()<deadline && stable<60){
            await NextFrame();await DrawFrame();bool changed=false;arrived=0;
            foreach(var member in members){
                using var item=core.Call("get_map_object",world,member.Key).AsGodotDictionary();
                Expect(item["selected"].AsBool(),"Factory movement preserves group selection");
                var at=new Vector2I(item["world_x"].AsInt32(),item["world_y"].AsInt32());
                if(at!=positions[member.Key])changed=true;positions[member.Key]=at;
                if(Math.Max(Math.Abs(at.X/256-target.X),Math.Abs(at.Y/256-target.Y))<=2 && at!=member.Value)++arrived;
            }
            stable=arrived==members.Count&&!changed?stable+1:0;
        }
        Expect(arrived==5&&stable>=60,"All five GI reach factory ground, not merely stop en route");
        GD.Print("GI_FACTORY_ROUTE arrived=",arrived,"/",members.Count," target=",target," stable=",stable);
        if(CaptureDirectory().Length>0){using var shot=viewport.GetTexture().GetImage();shot.SavePng(CaptureDirectory().PathJoin("gi-factory-arrived.png"));}
    }
    private async Task CheckVehicles(RefCounted core,Node2D map,SubViewport viewport)
    {
        using var all=core.Call("get_map_objects").AsGodotArray();long world=0,id=0;int wx=0,wy=0,wz=0,count=0;
        foreach(var entry in all){using var item=entry.AsGodotDictionary();if(item["kind"].AsString()!="unit")continue;
            ++count;if(id==0&&item["type_id"].AsString()=="MTNK"){world=item["world"].AsInt64();id=item["id"].AsInt64();wx=item["world_x"].AsInt32();wy=item["world_y"].AsInt32();wz=item["world_z"].AsInt32();}}
        Expect(count==16,"Original first map has 16 ground vehicles");Expect(id!=0,"Grizzly tank exists");
        for(int attempt=0;attempt<6;++attempt){
            using var status=core.Call("get_map_status").AsGodotDictionary();
            int dx=30*(wx-wy)/256-status["camera_x"].AsInt32()-640;
            int dy=15*(wx+wy)/256-15*wz/104-status["camera_y"].AsInt32()-360;
            if(Math.Abs(dx)<80&&Math.Abs(dy)<80)break;
            var start=new Vector2(450,300);var end=start+new Vector2(Math.Clamp(dx*4,-350,350),Math.Clamp(dy*4,-250,250));
            using var press=new InputEventMouseButton{Position=start,ButtonIndex=MouseButton.Right,Pressed=true};
            using var move=new InputEventMouseMotion{Position=end,ButtonMask=MouseButtonMask.Right};
            using var release=new InputEventMouseButton{Position=end,ButtonIndex=MouseButton.Right,Pressed=false};
            map.Call("handle_map_input",press,(Vector2)viewport.Size);map.Call("handle_map_input",move,(Vector2)viewport.Size);map.Call("handle_map_input",release,(Vector2)viewport.Size);await NextFrame();
        }
        Vector2 anchor;int cameraX,cameraY;
        using(var camera=core.Call("get_map_status").AsGodotDictionary()){
            cameraX=camera["camera_x"].AsInt32();cameraY=camera["camera_y"].AsInt32();
            anchor=new Vector2(30*(wx-wy)/256-cameraX,15*(wx+wy)/256-15*wz/104-cameraY);
        }
        bool picked=false;Vector2 point=anchor;
        for(int y=-25;y<=15&&!picked;++y)for(int x=-25;x<=25&&!picked;++x){
            point=anchor+new Vector2(x,y);using var motion=new InputEventMouseMotion{Position=point};map.Call("handle_map_input",motion,(Vector2)viewport.Size);
            using var status=core.Call("get_map_world_status").AsGodotDictionary();picked=status["hovered_id"].AsInt64()==id;
        }
        Expect(picked,"Tank can be picked through map input");
        void Click(Vector2 at){foreach(bool down in new[]{true,false}){using var click=new InputEventMouseButton{Position=at,ButtonIndex=MouseButton.Left,Pressed=down};map.Call("handle_map_input",click,(Vector2)viewport.Size);}}
        async Task Capture(string name){await DrawFrame();if(CaptureDirectory().Length>0){DirAccess.MakeDirRecursiveAbsolute(CaptureDirectory());using var image=viewport.GetTexture().GetImage();Expect(image.SavePng(CaptureDirectory().PathJoin(name+".png"))==Error.Ok,"Vehicle capture saved");}}
        Click(point);await Capture("vehicles-selected");
        using(var selected=core.Call("get_map_object",world,id).AsGodotDictionary())Expect(selected["selected"].AsBool(),"Tank selection is retained");
        // The original first map also contains an empty IFV. Its passenger
        // pips must not turn a successful vehicle selection into a render error.
        foreach(var entry in all){using var item=entry.AsGodotDictionary();if(item["kind"].AsString()!="unit"||item["type_id"].AsString()!="FV")continue;
            long fv=item["id"].AsInt64();var center=new Vector2(30*(item["world_x"].AsInt32()-item["world_y"].AsInt32())/256-cameraX,15*(item["world_x"].AsInt32()+item["world_y"].AsInt32())/256-15*item["world_z"].AsInt32()/104-cameraY);
            bool found=false;Vector2 pick=center;
            for(int y=-25;y<=15&&!found;++y)for(int x=-25;x<=25&&!found;++x){pick=center+new Vector2(x,y);using var motion=new InputEventMouseMotion{Position=pick};map.Call("handle_map_input",motion,(Vector2)viewport.Size);using var hover=core.Call("get_map_world_status").AsGodotDictionary();found=hover["hovered_id"].AsInt64()==fv;}
            Expect(found,"First-map IFV can be picked");Click(pick);await DrawFrame();await WaitState(map,"get_render_status","drawn");
            using(var selected=core.Call("get_map_object",world,fv).AsGodotDictionary())Expect(selected["selected"].AsBool(),"IFV passenger pips render on selection");
            Click(point);break;
        }
        var destination=new Vector2(30*(90-104)-cameraX,15*(90+104+1)-15*416/104-cameraY);
        using(var motion=new InputEventMouseMotion{Position=destination})map.Call("handle_map_input",motion,(Vector2)viewport.Size);
        using(var status=core.Call("get_map_world_status").AsGodotDictionary())Expect(status["hovered_id"].AsInt64()==0,"Vehicle destination is ground");
        Click(destination);
        bool moved=false;int stable=0,lastX=wx,lastY=wy;long lastTick=-1,firstTick=-1;var deadline=Time.GetTicksMsec()+180000;
        while(Time.GetTicksMsec()<deadline){await Delay(0.1);
            using var status=core.Call("get_map_world_status").AsGodotDictionary();long tick=status["simulation_tick"].AsInt64();if(firstTick<0)firstTick=tick;if(tick==lastTick)continue;lastTick=tick;
            using var current=core.Call("get_map_object",world,id).AsGodotDictionary();int x=current["world_x"].AsInt32(),y=current["world_y"].AsInt32();
            if(!moved&&(x!=wx||y!=wy)){moved=true;await Capture("vehicles-moving");}
            stable=x==lastX&&y==lastY?stable+1:0;lastX=x;lastY=y;
            if(moved&&stable>=10)break;
        }
        Expect(moved,"Tank moves after a ground click");Expect(stable>=10,"Tank stops after arrival");
        Expect(Math.Abs(lastX/256-90)<=1&&Math.Abs(lastY/256-104)<=1,"Tank reaches the clicked cell");
        await Capture("vehicles-arrived");GD.Print("VEHICLE_WINDOW count=",count," moved=",moved," cell=",lastX/256,",",lastY/256," ticks=",lastTick-firstTick);
    }

    private async Task CheckVehicleGate(RefCounted core,Node2D map,SubViewport viewport)
    {
        Root.GrabFocus();
        async Task Center(int wx,int wy,int wz){
            for(int pass=0;pass<40;++pass){
                using var status=core.Call("get_map_status").AsGodotDictionary();var bounds=status["map_bounds"].AsRect2I();
                int dx=30*(wx-wy)/256-status["camera_x"].AsInt32()-bounds.Size.X/2;
                int dy=15*(wx+wy)/256-15*wz/104-status["camera_y"].AsInt32()-bounds.Size.Y/2;
                if(Math.Abs(dx)<8&&Math.Abs(dy)<8)break;
                var start=new Vector2(450,300);var end=start+new Vector2(Math.Clamp(dx*4,-350,350),Math.Clamp(dy*4,-250,250));
                using var press=new InputEventMouseButton{Position=start,ButtonIndex=MouseButton.Right,Pressed=true};
                using var motion=new InputEventMouseMotion{Position=end,ButtonMask=MouseButtonMask.Right};
                using var release=new InputEventMouseButton{Position=end,ButtonIndex=MouseButton.Right,Pressed=false};
                map.Call("handle_map_input",press,(Vector2)viewport.Size);map.Call("handle_map_input",motion,(Vector2)viewport.Size);map.Call("handle_map_input",release,(Vector2)viewport.Size);await NextFrame();
            }
        }
        Vector2 Screen(int x,int y,int z){using var status=core.Call("get_map_status").AsGodotDictionary();return new Vector2(30*(x-y)/256-status["camera_x"].AsInt32(),15*(x+y)/256-15*z/104-status["camera_y"].AsInt32());}
        async Task Capture(string name){await DrawFrame();if(CaptureDirectory().Length>0){DirAccess.MakeDirRecursiveAbsolute(CaptureDirectory());using var image=viewport.GetTexture().GetImage();Expect(image.SavePng(CaptureDirectory().PathJoin(name+".png"))==Error.Ok,"Gate capture saved");}}
        void Click(Vector2 at,bool shift=false){foreach(bool down in new[]{true,false}){using var click=new InputEventMouseButton{Position=at,ButtonIndex=MouseButton.Left,Pressed=down,ShiftPressed=shift};map.Call("handle_map_input",click,(Vector2)viewport.Size);}}
        long world=0,target=0;var members=new System.Collections.Generic.List<long>();
        using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){using var item=entry.AsGodotDictionary();
            var type=item["type_id"].AsString();world=item["world"].AsInt64();
            if(type=="MTNK"||type=="MGTK"||type=="FV")members.Add(item["id"].AsInt64());
            if(type=="INIT"&&item["world_x"].AsInt32()/256==116&&item["world_y"].AsInt32()/256==102)target=item["id"].AsInt64();
        }
        async Task<Vector2> Find(long id){using var item=core.Call("get_map_object",world,id).AsGodotDictionary();int x=item["world_x"].AsInt32(),y=item["world_y"].AsInt32(),z=item["world_z"].AsInt32();await Center(x,y,z);await DrawFrame();var anchor=Screen(x,y,z);
            for(int dy=-120;dy<=20;++dy)for(int dx=-70;dx<=70;++dx){var point=anchor+new Vector2(dx,dy);using var motion=new InputEventMouseMotion{Position=point};map.Call("handle_map_input",motion,(Vector2)viewport.Size);using var hover=core.Call("get_map_world_status").AsGodotDictionary();if(hover["hovered_id"].AsInt64()==id)return point;}
            await Capture("gate-pick-miss-"+id);
            throw new Exception("Cannot pick gate scenario object "+id);
        }
        Expect(members.Count==9&&target!=0,"Nine player vehicles and lower gate Initiate exist");
        using(var first=core.Call("get_map_object",world,members[0]).AsGodotDictionary())
            await Center(first["world_x"].AsInt32(),first["world_y"].AsInt32(),first["world_z"].AsInt32());
        var bandFirst=new Vector2(1280,720);var bandLast=Vector2.Zero;
        foreach(long member in members){using var actor=core.Call("get_map_object",world,member).AsGodotDictionary();
            var point=Screen(actor["world_x"].AsInt32(),actor["world_y"].AsInt32(),actor["world_z"].AsInt32());
            bandFirst=bandFirst.Min(point-new Vector2(18,25));bandLast=bandLast.Max(point+new Vector2(18,15));}
        using(var press=new InputEventMouseButton{Position=bandLast,ButtonIndex=MouseButton.Left,Pressed=true})map.Call("handle_map_input",press,(Vector2)viewport.Size);
        using(var drag=new InputEventMouseMotion{Position=bandFirst,ButtonMask=MouseButtonMask.Left})map.Call("handle_map_input",drag,(Vector2)viewport.Size);
        using(var release=new InputEventMouseButton{Position=bandFirst,ButtonIndex=MouseButton.Left,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
        foreach(long member in members){using var actor=core.Call("get_map_object",world,member).AsGodotDictionary();
            Expect(actor["selected"].AsBool(),"Gate attack has every player vehicle selected: "+member);}
        Click(await Find(target));await Capture("gate-attack");
        long tick=-1;int stable=0;var previous=new Vector2I[members.Count];var deadline=Time.GetTicksMsec()+180000;
        while(Time.GetTicksMsec()<deadline&&stable<90){await NextFrame();using var status=core.Call("get_map_world_status").AsGodotDictionary();long now=status["simulation_tick"].AsInt64();if(now==tick)continue;int elapsed=tick<0?1:(int)(now-tick);tick=now;bool same=true;
            for(int i=0;i<members.Count;++i){using var actor=core.Call("get_map_object",world,members[i]).AsGodotDictionary();var pos=new Vector2I(actor["world_x"].AsInt32(),actor["world_y"].AsInt32());same&=previous[i]==pos;previous[i]=pos;}stable=same?stable+elapsed:0;
        }
        Expect(stable>=90,"All nine vehicles settle after attacking gate Initiate");await Capture("gate-stopped");
        if(HasArgument("--gate-capture-only")){await Center(127*256+128,102*256+128,416);await Capture("gate-forward");return;}
        // The actual ALL01 cell floor is 832 here. Projecting it at the
        // starting plateau's 416 makes the click hit a different ground cell.
        const int gx=122,gy=100,gz=832;await Center(gx*256+128,gy*256+128,gz);
        var ground=Screen(gx*256+128,gy*256+128,gz);
        using(var motion=new InputEventMouseMotion{Position=ground})map.Call("handle_map_input",motion,(Vector2)viewport.Size);
        using(var hover=core.Call("get_map_world_status").AsGodotDictionary())Expect(hover["hovered_id"].AsInt64()==0,"Gate move click is on the ground");
        Click(ground);
        deadline=Time.GetTicksMsec()+180000;bool arrived=false;
        while(Time.GetTicksMsec()<deadline&&!arrived){await Delay(0.1);arrived=true;foreach(long member in members){using var actor=core.Call("get_map_object",world,member).AsGodotDictionary();int dx=actor["world_x"].AsInt32()-(gx*256+128),dy=actor["world_y"].AsInt32()-(gy*256+128);arrived&=dx*dx+dy*dy<=1024*1024;}}
        Expect(arrived,"All nine vehicles move from attack positions to the junction");await Capture("gate-moved");GD.Print("VEHICLE_GATE arrived=",arrived);
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
            map.Call("open_map","ALL01UMD.MAP");await WaitState(map,"get_render_status","drawn");
            if(HasArgument("--vehicle-gate-only")){await CheckVehicleGate(core,map,viewport);return;}
            if(HasArgument("--vehicles-only")){await CheckVehicles(core,map,viewport);return;}
            long world=0,id=0;int wx=0,wy=0,wz=0,gi=0,infantry=0;
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){
                using var item=entry.AsGodotDictionary();if(item["kind"].AsString()!="infantry")continue;
                ++infantry;Expect(item["has_image"].AsBool()&&item["frame"].AsInt32()>=0,"Infantry has a loaded image and sequence frame");
                if(item["type_id"].AsString()!="E1")continue;++gi;
                if(id!=0)continue;world=item["world"].AsInt64();id=item["id"].AsInt64();
                wx=item["world_x"].AsInt32();wy=item["world_y"].AsInt32();wz=item["world_z"].AsInt32();
            }
            Expect(infantry==56&&gi==5,"Original ALL01 contains 56 infantry and 5 actual GI, got "+infantry+"/"+gi);
            if(id==0)return;
            for(int pass=0;pass<16;++pass){
                using var status=core.Call("get_map_status").AsGodotDictionary();var bounds=status["map_bounds"].AsRect2I();
                int dx=30*(wx-wy)/256-status["camera_x"].AsInt32()-bounds.Size.X/2;
                int dy=15*(wx+wy)/256-15*wz/104-status["camera_y"].AsInt32()-bounds.Size.Y/2;
                if(Math.Abs(dx)<8&&Math.Abs(dy)<8)break;
                var start=new Vector2(450,300);var end=start+new Vector2(Math.Clamp(dx*4,-350,350),Math.Clamp(dy*4,-250,250));
                using var press=new InputEventMouseButton{Position=start,ButtonIndex=MouseButton.Right,Pressed=true};
                using var move=new InputEventMouseMotion{Position=end,ButtonMask=MouseButtonMask.Right};
                using var release=new InputEventMouseButton{Position=end,ButtonIndex=MouseButton.Right,Pressed=false};
                map.Call("handle_map_input",press,(Vector2)viewport.Size);map.Call("handle_map_input",move,(Vector2)viewport.Size);map.Call("handle_map_input",release,(Vector2)viewport.Size);await NextFrame();
            }
            await DrawFrame();await DrawFrame();
            if(HasArgument("--harvesting-only")||HasArgument("--production-only")||HasArgument("--ifv-unload-only")||HasArgument("--vehicle-death-only")||HasArgument("--grizzly-fire-only")||HasArgument("--grizzly-uphill-only")||HasArgument("--ifv-repair-only")||HasArgument("--civilian-garrison-only")||HasArgument("--ore-walk-only")||HasArgument("--group-building-attack-only")||HasArgument("--combat-only")||HasArgument("--building-click-only")||HasArgument("--building-visual-only")||HasArgument("--dog-combat-only")||HasArgument("--ggi-visual-only")||HasArgument("--force-attack-only")){await CheckCombat(core,map,viewport,world,id);return;}
            if(HasArgument("--hover-only")){await CheckHover(core,map,viewport,world,id,wx,wy,wz);return;}
            if(HasArgument("--factory-route-only")){await CheckFactoryRoute(core,map,viewport,world);return;}
            // One real-map smoke check: GISequence Idle1/Idle2 are 56..85 in
            // the shipped ARTMD.INI. Turning a Ready sprite is not animation.
            int firstIdle=-1,nextIdle=-1;bool returnedToReady=false;
            var idleDeadline=Time.GetTicksMsec()+20000;
            while(Time.GetTicksMsec()<idleDeadline){
                await NextFrame();await DrawFrame();
                using var state=core.Call("get_map_object",world,id).AsGodotDictionary();
                int frame=state["frame"].AsInt32();
                Expect(state["world_x"].AsInt32()==wx&&state["world_y"].AsInt32()==wy,"Idle does not issue movement orders");
                if(frame>=56&&frame<86){
                    if(firstIdle<0){
                        firstIdle=frame;
                        if(CaptureDirectory().Length>0){using var shot=viewport.GetTexture().GetImage();shot.SavePng(CaptureDirectory().PathJoin("gi-idle-start.png"));}
                    }else if(frame!=firstIdle&&nextIdle<0){
                        nextIdle=frame;
                        if(CaptureDirectory().Length>0){using var shot=viewport.GetTexture().GetImage();shot.SavePng(CaptureDirectory().PathJoin("gi-idle-next.png"));}
                    }
                }else if(nextIdle>=0&&frame>=0&&frame<8){returnedToReady=true;break;}
            }
            Expect(firstIdle>=0&&nextIdle>=0&&returnedToReady,"Actual GI advances idle frames and returns to Ready");
            GD.Print("GI_IDLE frames=",firstIdle,",",nextIdle," returned_ready=",returnedToReady);
            // The same real-map smoke scene now uses the public mouse entrance:
            // select an actual sprite, left-click terrain, observe native motion.
            Vector2 anchor;
            using(var status=core.Call("get_map_status").AsGodotDictionary())
                anchor=new Vector2(30*(wx-wy)/256-status["camera_x"].AsInt32(),15*(wx+wy)/256-15*wz/104-status["camera_y"].AsInt32());
            Vector2 pick=anchor;bool hit=false;
            for(int y=-20;y<=4&&!hit;y+=2)for(int x=-10;x<=10&&!hit;x+=2){
                var point=anchor+new Vector2(x,y);
                using var hover=new InputEventMouseMotion{Position=point};map.Call("handle_map_input",hover,(Vector2)viewport.Size);
                using var status=core.Call("get_map_world_status").AsGodotDictionary();
                if(status["hovered_id"].AsInt64()==id){pick=point;hit=true;}
            }
            Expect(hit,"Actual GI sprite can be picked before giving an order");
            void LeftClick(Vector2 point){
                using var press=new InputEventMouseButton{Position=point,ButtonIndex=MouseButton.Left,Pressed=true};
                using var release=new InputEventMouseButton{Position=point,ButtonIndex=MouseButton.Left,Pressed=false};
                map.Call("handle_map_input",press,(Vector2)viewport.Size);map.Call("handle_map_input",release,(Vector2)viewport.Size);
            }
            LeftClick(pick);
            using(var state=core.Call("get_map_object",world,id).AsGodotDictionary())Expect(state["selected"].AsBool(),"Left click selects GI");
            LeftClick(anchor+new Vector2(0,-30));
            bool moved=false,walking=false,capturedMove=false;int stable=0,lastX=wx,lastY=wy;
            var moveDeadline=Time.GetTicksMsec()+10000;
            while(Time.GetTicksMsec()<moveDeadline&&stable<60){
                await NextFrame();await DrawFrame();
                using var state=core.Call("get_map_object",world,id).AsGodotDictionary();
                int x=state["world_x"].AsInt32(),y=state["world_y"].AsInt32(),frame=state["frame"].AsInt32();
                bool changed=x!=lastX||y!=lastY;stable=moved&&!changed?stable+1:0;
                if(x!=wx||y!=wy)moved=true;
                if(changed&&frame>=8&&frame<56)walking=true;
                if(changed&&!walking)stable=0;
                lastX=x;lastY=y;
                if(changed&&!capturedMove&&CaptureDirectory().Length>0){using var shot=viewport.GetTexture().GetImage();shot.SavePng(CaptureDirectory().PathJoin("gi-left-move.png"));capturedMove=true;}
            }
            Expect(moved&&walking&&stable>=60,"Terrain left click moves actual GI with Walk frames, then stops");
            using(var state=core.Call("get_map_object",world,id).AsGodotDictionary())Expect(state["selected"].AsBool(),"Movement order preserves selection");
            GD.Print("GI_LEFT_MOVE from=",wx,",",wy," to=",lastX,",",lastY," walk=",walking," stopped=",stable>=60);
            // Rapid real mouse retargeting, not direct native position changes.
            var offsets=new[]{new Vector2(0,-30),new Vector2(30,-45),new Vector2(-30,-45),new Vector2(0,-60)};
            int repeated=0;bool retargetMoved=false;
            for(int command=0;command<64;++command){
                var point=anchor+offsets[command%offsets.Length];
                using var hover=new InputEventMouseMotion{Position=point};map.Call("handle_map_input",hover,(Vector2)viewport.Size);
                using(var status=core.Call("get_map_world_status").AsGodotDictionary()){
                    // A moving sprite can pass beneath the next click. Keep
                    // this regression on terrain orders rather than selection.
                    if(status["hovered_id"].AsInt64()==0){LeftClick(point);++repeated;}
                }
                await NextFrame();await DrawFrame();
                using var state=core.Call("get_map_object",world,id).AsGodotDictionary();
                int x=state["world_x"].AsInt32(),y=state["world_y"].AsInt32();
                if(x!=lastX||y!=lastY)retargetMoved=true;lastX=x;lastY=y;
                Expect(state["selected"].AsBool(),"Continuous terrain orders retain GI selection");
            }
            stable=0;moveDeadline=Time.GetTicksMsec()+10000;
            while(Time.GetTicksMsec()<moveDeadline&&stable<60){
                await NextFrame();await DrawFrame();
                using var state=core.Call("get_map_object",world,id).AsGodotDictionary();
                int x=state["world_x"].AsInt32(),y=state["world_y"].AsInt32();
                stable=x==lastX&&y==lastY?stable+1:0;lastX=x;lastY=y;
            }
            Expect(repeated>=32&&retargetMoved&&stable>=60,"Repeated left clicks move GI safely and the final order settles");
            GD.Print("GI_CONTINUOUS_CLICK commands=",repeated," moved=",retargetMoved," stopped=",stable>=60);
            var group=new System.Collections.Generic.Dictionary<long,Vector2I>();
            var boxStart=new Vector2(1280,720);var boxEnd=Vector2.Zero;
            using(var status=core.Call("get_map_status").AsGodotDictionary())
            using(var objects=core.Call("get_map_objects").AsGodotArray())foreach(var entry in objects){
                using var item=entry.AsGodotDictionary();if(item["kind"].AsString()!="infantry"||item["type_id"].AsString()!="E1")continue;
                int x=item["world_x"].AsInt32(),y=item["world_y"].AsInt32(),z=item["world_z"].AsInt32();
                group[item["id"].AsInt64()]=new Vector2I(x,y);
                var point=new Vector2(30*(x-y)/256-status["camera_x"].AsInt32(),15*(x+y)/256-15*z/104-status["camera_y"].AsInt32());
                boxStart=boxStart.Min(point-new Vector2(15,22));boxEnd=boxEnd.Max(point+new Vector2(15,10));
            }
            // Reverse drag is intentional; render while held to verify the band.
            using(var press=new InputEventMouseButton{Position=boxEnd,ButtonIndex=MouseButton.Left,Pressed=true})map.Call("handle_map_input",press,(Vector2)viewport.Size);
            using(var drag=new InputEventMouseMotion{Position=boxStart,ButtonMask=MouseButtonMask.Left})map.Call("handle_map_input",drag,(Vector2)viewport.Size);
            for(int i=0;i<6;++i){await NextFrame();await DrawFrame();}
            if(CaptureDirectory().Length>0){using var shot=viewport.GetTexture().GetImage();shot.SavePng(CaptureDirectory().PathJoin("gi-bandbox.png"));}
            using(var release=new InputEventMouseButton{Position=boxStart,ButtonIndex=MouseButton.Left,Pressed=false})map.Call("handle_map_input",release,(Vector2)viewport.Size);
            foreach(var member in group){using var state=core.Call("get_map_object",world,member.Key).AsGodotDictionary();Expect(state["selected"].AsBool(),"Band selects each of the five GI");}
            LeftClick(anchor+new Vector2(90,-82));
            var lastPositions=new System.Collections.Generic.Dictionary<long,Vector2I>(group);stable=0;
            var groupDeadline=Time.GetTicksMsec()+20000;
            while(Time.GetTicksMsec()<groupDeadline&&stable<90){
                await NextFrame();await DrawFrame();bool changed=false;
                foreach(var member in group){
                    using var state=core.Call("get_map_object",world,member.Key).AsGodotDictionary();
                    var at=new Vector2I(state["world_x"].AsInt32(),state["world_y"].AsInt32());
                    if(at!=lastPositions[member.Key])changed=true;lastPositions[member.Key]=at;
                    Expect(state["selected"].AsBool(),"Group movement preserves selection");
                }
                stable=changed?0:stable+1;
            }
            int groupMoved=group.Count(member=>lastPositions[member.Key]!=member.Value);
            Expect(group.Count==5&&groupMoved==5&&stable>=90,"Five selected GI move together and settle");
            GD.Print("GI_GROUP_MOVE selected=",group.Count," moved=",groupMoved," stopped=",stable>=90);
            if(CaptureDirectory().Length>0){using var shot=viewport.GetTexture().GetImage();shot.SavePng(CaptureDirectory().PathJoin("gi-group-arrived.png"));}
            using var before=viewport.GetTexture().GetImage();
            if(CaptureDirectory().Length>0)Expect(before.SavePng(CaptureDirectory().PathJoin("gi-map.png"))==Error.Ok,"GI capture saved");
            // Removing the real GI must invalidate the production renderer.
            // Allow its asynchronous submission/readback to complete.
            var beforeBytes=before.GetData();Expect(core.Call("set_map_object_health",world,id,0).AsBool(),"Remove actual native GI");
            for(int i=0;i<6;++i){await NextFrame();await DrawFrame();}
            using(var dead=core.Call("get_map_object",world,id).AsGodotDictionary())Expect(!dead["alive"].AsBool(),"GI object is dead before pixel check");
            using var after=viewport.GetTexture().GetImage();var afterBytes=after.GetData();
            Expect(!beforeBytes.SequenceEqual(afterBytes),"Real GI removal changes rendered map pixels");
            if(CaptureDirectory().Length>0)Expect(after.SavePng(CaptureDirectory().PathJoin("gi-removed.png"))==Error.Ok,"GI removal capture saved");
            map.Call("close_map");map.Call("open_map","ALL01UMD.MAP");await WaitState(map,"get_render_status","drawn");
            using var stale=core.Call("get_map_object",world,id).AsGodotDictionary();Expect(stale.Count==0,"Reload rejects old GI identity");
            using var worldStatus=core.Call("get_map_world_status").AsGodotDictionary();Expect(worldStatus["infantry"].AsInt64()==56,"Reload has no duplicate infantry");
            GD.Print("GI_MAP_RENDER infantry=",infantry," GI=",gi," world=",world);
        }finally{map.Call("close_map");screen.QueueFree();viewport.QueueFree();await NextFrame();}
    }
}
