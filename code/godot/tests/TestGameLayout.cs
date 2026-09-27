using Godot;
using System;
using System.Threading.Tasks;

// Runs the real single canvas. Coordinates below are the binary fixture's
// 1280x720 layout, not coordinates read back from the implementation to click.
public partial class TestGameLayout : TestRunner
{
    private Vector2 _pointer;
    private MouseButtonMask _mask;
    private bool _entered;
    protected override async Task RunTestAsync()
    {
        InitializeApp("game-layout");
        await WaitForBoot();
        Expect(Application.BootReady, "Real resources bootstrap");
        if (!Application.BootReady) return;
        Application.StartMap("ALL01UMD.MAP");
        await WaitDrawn(1280);
        if (Failures != 0) return;
        Expect(!Application.HasNode("GameScreen/RadarPanel"), "Game UI has a single canvas, no radar Control");
        CheckLayout(1280,720);
        await SaveCanvas("initial");
        var stationary=Camera();
        await Move(new Vector2(30,704));
        await Button(MouseButton.Left,true); await Button(MouseButton.Left,false);
        await WaitDrawn(1280);
        using (var state=Application.GameScreen.GetMapStatus()) Expect(!state["command_bar_expanded"].AsBool(),"Original collapse gadget updates ThumbActive");
        Expect(Camera()==stationary,"HUD input leaves Tactical unchanged");
        await SaveCanvas("hud-collapsed");
        await Move(new Vector2(1070,704));
        await Button(MouseButton.Left,true); await Button(MouseButton.Left,false);
        await WaitDrawn(1280);
        using (var state=Application.GameScreen.GetMapStatus()) Expect(state["command_bar_expanded"].AsBool(),"Original expand gadget restores command bar");
        var before = Camera();
        await Move(new Vector2(1198,119));
        await Button(MouseButton.Left,true);
        await WaitDrawn(1280);
        var clicked = Camera();
        Expect(clicked != before, "Raw canvas press navigates through core radar gadget");
        await Move(new Vector2(1212,132));
        await WaitDrawn(1280);
        var dragged = Camera();
        Expect(dragged != clicked, "Core radar held event navigates");
        await Move(new Vector2(700,400));
        await Button(MouseButton.Left,false);
        Expect(Camera() == dragged, "Radar drag outside stays isolated from map scroll");
        using (var status=Application.GameScreen.GetMapStatus()) Expect(!status["pointer_captured"].AsBool(),"Outside release clears capture");
        await SaveCanvas("radar-drag");
        await Move(new Vector2(1200,450));
        await Button(MouseButton.Right,true);
        await Move(new Vector2(600,400));
        await Button(MouseButton.Right,false);
        Expect(Camera()==dragged,"Sidebar-start right drag cannot pan map");
        Root.PropagateNotification((int)NotificationWMWindowFocusOut);
        await Move(new Vector2(0,400));
        await Delay(0.06);
        Expect(Camera()==dragged,"Focus loss ignores movement and edge scroll");
        Root.PropagateNotification((int)NotificationWMWindowFocusIn);
        await Move(new Vector2(600,400));
        Expect(Application.DisplayService.ApplySelection(new Vector2I(1600,900),(int)DisplayConfig.Mode.Windowed)==Error.Ok,"Resize full canvas");
        await WaitDrawn(1600); CheckLayout(1600,900); await SaveCanvas("resized");
        Application.GameScreen.CloseGame();
    }
    private void CheckLayout(int width,int height)
    {
        using var state=Application.GameScreen.GetMapStatus();
        Expect(state["canvas_bounds"].AsRect2I()==new Rect2I(0,0,width,height),"Full canvas dimensions");
        Expect(state["map_bounds"].AsRect2I()==new Rect2I(0,0,width-168,height-32),"Original tactical rectangle");
        Expect(state["radar_bounds"].AsRect2I()==new Rect2I(width-152,49,140,108),"Original radar inner rectangle");
        Expect(state["hud_bounds"].AsRect2I()==new Rect2I(0,height-32,width-168,32),"Original HUD excludes sidebar");
    }
    private Vector2I Camera()
    {
        using var state=Application.GameScreen.GetMapStatus();
        return new Vector2I(state["camera_x"].AsInt32(),state["camera_y"].AsInt32());
    }
    private async Task WaitDrawn(int width)
    {
        var deadline=Time.GetTicksMsec()+30000;
        while (Time.GetTicksMsec()<deadline)
        {
            using var state=Application.GameScreen.GetMapStatus();
            if (state["state"].AsString()=="failed") { Expect(false,state["error"].AsString()); return; }
            if (state["state"].AsString()=="drawn" && state["viewport_width"].AsInt32()==width) { await DrawFrame(); await DrawFrame(); return; }
            await NextFrame();
        }
        Expect(false,"Full canvas render timed out");
    }
    private async Task Move(Vector2 logical)
    {
        var surface=Application.GetNode<Control>("GameScreen/WorldSurface");
        var point=surface.GetGlobalTransformWithCanvas()*logical;
        if (!_entered) { Root.NotifyMouseEntered(); _entered=true; }
        using var motion=new InputEventMouseMotion { Position=point,GlobalPosition=point,Relative=point-_pointer,ButtonMask=_mask };
        Input.ParseInputEvent(motion); Input.FlushBufferedEvents(); _pointer=point;
        await NextFrame(); await NextFrame();
    }
    private async Task Button(MouseButton button,bool pressed)
    {
        var bit=(MouseButtonMask)(1<<((int)button-1));
        _mask=pressed ? _mask|bit : _mask&~bit;
        using var click=new InputEventMouseButton { Position=_pointer,GlobalPosition=_pointer,ButtonIndex=button,Pressed=pressed,ButtonMask=_mask };
        Input.ParseInputEvent(click); Input.FlushBufferedEvents();
        await NextFrame(); await NextFrame();
    }
    private async Task SaveCanvas(string name)
    {
        await DrawFrame(); await DrawFrame();
        var directory=CaptureDirectory(); if (directory.Length==0) return;
        using var image=Application.GetNode<SubViewport>("GameScreen/WorldViewport").GetTexture().GetImage();
        Expect(image.SavePng(directory.PathJoin(name+".png"))==Error.Ok,"Save actual GPU canvas");
        image.Convert(Image.Format.Rgba8); var rgba=image.GetData(); var pixels=new byte[rgba.Length/2];
        for (int i=0;i<rgba.Length/4;++i) {
            int pixel=((rgba[i*4]>>3)<<11)|((rgba[i*4+1]>>2)<<5)|(rgba[i*4+2]>>3);
            pixels[i*2]=(byte)pixel; pixels[i*2+1]=(byte)(pixel>>8);
        }
        System.IO.File.WriteAllBytes(directory.PathJoin(name+".rgb565"),pixels);
    }
}
