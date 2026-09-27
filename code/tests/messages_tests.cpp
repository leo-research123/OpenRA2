#include "support/test_support.hpp"
#include "yrpp/MessageListClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/Unsorted.h"
#include "yrpp/RulesClass.h"
#include "yrpp/HouseClass.h"
#include "api/clock.hpp"
#include "api/filesystem.hpp"
#include "game_ui_runtime.hpp"
#include "scenario_runtime.hpp"
#include <cstdlib>
#include <cwchar>
#include <memory>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <functional>
#include <vector>
#include "game_loop.hpp"
#include "yrpp/BitText.h"
#include "yrpp/OwnerDraw.h"
#include "yrpp/Drawing.h"
#include "yrpp/Surface.h"

TEST(Messages, OriginalBufferLifetimeAndAuthority) {
    // Real production time, including the global elapsed consumer, throughout.
    const int before=Game::TickCount.GetTimeElapsed();
    MessageListClass list;
    EXPECT_EQ(list.Width,0);EXPECT_EQ(list.EditBuffer[0],0);EXPECT_EQ(list.BufferAvail[13],1);
    list.Init(12,30,20,200,55,-1,40,1,80,200,640);
    EXPECT_EQ(list.MaxMessageCount,14);EXPECT_EQ(list.MaxCharacters,112);EXPECT_EQ(list.Height,19);
    EXPECT_EQ(list.Width,632);EXPECT_TRUE(list.AdjustEdit);EXPECT_EQ(list.EditPos,Point2D(12,30));
    EXPECT_EQ(list.OverflowStart,80u);EXPECT_EQ(list.OverflowEnd,111u);
    auto* a=new TextLabelClass(list.MessageBuffers[0],12,30,0,TextPrintType(0));
    auto* b=new TextLabelClass(list.MessageBuffers[1],12,49,0,TextPrintType(0));
    auto* c=new TextLabelClass(list.MessageBuffers[2],12,68,0,TextPrintType(0));
    const auto due=DWORD(Game::TickCount.GetTimeElapsed()+3);
    a->UserData1=reinterpret_cast<void*>(std::uintptr_t(due));a->UserData2=reinterpret_cast<void*>(3);
    b->UserData1=nullptr;b->UserData2=reinterpret_cast<void*>(4);
    c->UserData1=reinterpret_cast<void*>(std::uintptr_t(due));c->UserData2=reinterpret_cast<void*>(5);
    std::wcscpy(a->Text,L"first");std::wcscpy(b->Text,L"permanent");std::wcscpy(c->Text,L"last");
    list.MessageList=a;b->AddTail(*a);c->AddTail(*a);list.BufferAvail[0]=list.BufferAvail[1]=list.BufferAvail[2]=0;
    EXPECT_EQ(list.GetLabel(4),b);EXPECT_STREQ(list.GetMessage(5),L"last");EXPECT_EQ(list.NumMessages(),3);
    EXPECT_EQ(list.Manage(),0);EXPECT_EQ(Game::TickCount.GetTimeElapsed(),before);
    for(int i=0;i<3;++i)ASSERT_TRUE(game::advance_clock(0.016));
    EXPECT_EQ(DWORD(Game::TickCount.GetTimeElapsed()),due);EXPECT_EQ(list.Manage(),0) << "strictly after expiry";
    ASSERT_TRUE(game::advance_clock(0.016));EXPECT_EQ(list.Manage(),1);
    EXPECT_EQ(list.MessageList,b);EXPECT_EQ(list.NumMessages(),1);EXPECT_EQ(b->Y,30);
    EXPECT_EQ(list.BufferAvail[0],1);EXPECT_EQ(list.BufferAvail[1],0);EXPECT_EQ(list.BufferAvail[2],1);
    EXPECT_EQ(list.GetMessage(3),nullptr);EXPECT_EQ(list.Manage(),0);
    list.SetWidth(320);EXPECT_EQ(b->PixWidth,312u);
    auto* edit=new TextLabelClass(list.EditBuffer,12,30,0,TextPrintType(0));
    list.EditLabel=edit;list.IsEdit=true;list.ComputeY();EXPECT_EQ(b->Y,49);EXPECT_EQ(list.NumMessages(),2);
    list.SetWidth(400);EXPECT_EQ(edit->PixWidth,392u);
    edit->SetFocus();EXPECT_EQ(GadgetClass::Focused,edit);
    const auto epoch=Game::TickCount.StartTime;
    list.Init(0,0,6,98,14,-1,-1,0,20,98,640);
    EXPECT_EQ(list.MessageList,nullptr);EXPECT_EQ(list.EditLabel,nullptr);EXPECT_EQ(GadgetClass::Focused,nullptr);
    EXPECT_EQ(Game::TickCount.StartTime,epoch);EXPECT_EQ(Game::TickCount.GetTimeElapsed(),before+4);
    wchar_t source[32]=L"alpha beta gamma",removed[32]{};
    EXPECT_EQ(list.TrimMessage(removed,source,3,8,1),6);
    EXPECT_STREQ(removed,L"alpha ");EXPECT_STREQ(source,L"beta gamma");
}
namespace {
void original_font_messages() {
    auto rules=std::make_unique<RulesClass>();const auto old_rules=RulesClass::Instance;RulesClass::Instance=rules.get();
    const auto cleanup=ra2::test::scope_exit([&]{RulesClass::Instance=old_rules;});
    game::UiResources resources;ASSERT_TRUE(resources.load(0))<<resources.error();
    ASSERT_EQ(BitFont::Instance,resources.font());ASSERT_NE(BitFont::Instance,nullptr);
    auto& global=MessageListClass::Instance;global.Init(3,0,6,98,14,-1,-1,0,20,98,640);
    const auto reset_global=ra2::test::scope_exit([&]{global.Init(0,0,0,0,0,0,0,0,0,0);});
    Game::ShowMessage(L"Planning feedback",-1);ASSERT_EQ(global.NumMessages(),1);
    ASSERT_NE(global.MessageList,nullptr);EXPECT_FALSE(global.MessageList->Animate);
    EXPECT_EQ(DWORD(reinterpret_cast<std::uintptr_t>(global.MessageList->UserData1)),DWORD(Game::TickCount.GetTimeElapsed()+240));
    MessageListClass list;list.Init(3,0,6,98,14,-1,-1,0,20,98,180);
    auto* font=resources.font();
    EXPECT_GT(font->GetTextWidth(L"Radar beacon"),0);EXPECT_EQ(font->GetTextFit(L"Radar",200,111,true),5);
    auto* label=list.AddMessage(L"Player",7,L"Beacon placed",0,TextPrintType(0x4046),225,false);
    ASSERT_NE(label,nullptr);EXPECT_STREQ(label->Text,L"Player:Beacon placed");EXPECT_TRUE(label->Animate);
    EXPECT_EQ(label->Style,0xC046u);EXPECT_EQ(label->PixWidth,172u);
    const auto deadline=DWORD(reinterpret_cast<std::uintptr_t>(label->UserData1));
    EXPECT_EQ(deadline,DWORD(Game::TickCount.GetTimeElapsed()+225));
    list.Init(3,0,14,98,14,-1,-1,0,20,98,100);
    const wchar_t* sentence=L"first word second word third word fourth word";
    label=list.AddMessage(nullptr,8,sentence,0,TextPrintType(0x4046),-1,false);ASSERT_NE(label,nullptr);
    ASSERT_GT(list.NumMessages(),1);EXPECT_TRUE(label->Animate);
    std::wstring assembled;
    for(auto* p=list.MessageList;p;p=static_cast<TextLabelClass*>(p->GetNext())){
        assembled+=p->Text;EXPECT_LE(font->GetTextWidth(p->Text),list.Width-8);
        if(p!=label)EXPECT_FALSE(p->Animate);EXPECT_EQ(p->UserData1,nullptr);
    }
    EXPECT_EQ(assembled,sentence);
    list.Init(3,0,1,98,14,-1,-1,0,20,98,640);
    for(int i=0;i<20;++i){ASSERT_NE(list.AddMessage(nullptr,i,L"replace",0,TextPrintType(0),-1,true),nullptr);EXPECT_EQ(list.NumMessages(),1);}
    EXPECT_EQ(list.GetLabel(18),nullptr);EXPECT_NE(list.GetLabel(19),nullptr);
    list.Init(3,0,0,98,14,-1,-1,0,20,98,640);
    for(int i=0;i<14;++i)ASSERT_NE(list.AddMessage(nullptr,i,L"buffer",0,TextPrintType(0),-1,true),nullptr);
    EXPECT_EQ(list.AddMessage(nullptr,14,L"full",0,TextPrintType(0),-1,true),nullptr);EXPECT_EQ(list.NumMessages(),14);
    const auto old_font=BitFont::Instance;BitFont::Instance=nullptr;
    EXPECT_EQ(list.AddMessage(nullptr,15,L"no font",0,TextPrintType(0),-1,true),nullptr);BitFont::Instance=old_font;
    const int ticks=Game::TickCount.GetTimeElapsed();
    resources.clear();EXPECT_EQ(BitFont::Instance,nullptr);EXPECT_EQ(Game::TickCount.GetTimeElapsed(),ticks);
    ASSERT_TRUE(resources.load(1));ASSERT_EQ(BitFont::Instance,resources.font());
    game::UiResources second;ASSERT_TRUE(second.load(0));auto* current=second.font();
    resources.clear();EXPECT_EQ(BitFont::Instance,current);second.clear();EXPECT_EQ(BitFont::Instance,nullptr);
}
}
TEST(Messages, RealFontWrappingAndUiResourceLifetime) {
    const char* data=std::getenv("RA2_GAME_DATA");if(!data || !*data)GTEST_SKIP()<<"RA2_GAME_DATA enables original message font integration";
    game::ResourceHandle* raw=nullptr;std::string error;ASSERT_TRUE(game::create_resources(data,raw,error))<<error;
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(raw,game::destroy_resources);
    ASSERT_EQ(game::load_resources(*files,{},error),game::ResourceLoadResult::complete)<<error;
    EXPECT_TRUE(game::with_resources(*files,[](void*){original_font_messages();},nullptr,error))<<error;
}

namespace {
struct MessageCanvas {
    static constexpr int width=640,height=120;
    std::vector<WORD> pixels=std::vector<WORD>(width*height,0x1234);
    game::MapDrawingContext drawing;
    std::function<void()> operation;
    bool fail=false;
    MessageCanvas(){
        drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(this);
        drawing.types.backend_context=this;
        drawing.plain_palette=[](void* p,const BytePalette&,const game::DrawingPaletteHandle*& out)noexcept{
            out=reinterpret_cast<const game::DrawingPaletteHandle*>(p);return game::DrawingStatus::drawn;};
        drawing.types.backend.raster=[](void* p,const game::RasterDrawingRequest& r){
            auto& self=*static_cast<MessageCanvas*>(p);
            if(self.fail)return game::DrawingStatus::backend_failure;
            for(int y=std::max({0,r.position.Y,r.clip.Y});y<std::min({height,r.position.Y+r.height,r.clip.Y+r.clip.Height});++y)
                for(int x=std::max({0,r.position.X,r.clip.X});x<std::min({width,r.position.X+r.width,r.clip.X+r.clip.Width});++x)
                    self.pixels[y*width+x]=r.color;
            return game::DrawingStatus::drawn;
        };
    }
    game::DrawingStatus draw(game::UiResources& resources,std::function<void()> callback,const game::TextComposition* composition=nullptr){
        operation=std::move(callback);game::MapDrawStatistics stats{};game::GameUiFrame frame{drawing,resources,stats};
        frame.composition=composition;
        return game::with_game_ui_frame(frame,[]{auto* f=game::game_ui_frame();static_cast<MessageCanvas*>(f->drawing.types.backend_context)->operation();});
    }
    void save(const char* path){
        std::ofstream file(path,std::ios::binary);file<<"P6\n"<<width<<' '<<height<<"\n255\n";
        for(WORD c:pixels){const char rgb[]{char(((c>>11)&31)*255/31),char(((c>>5)&63)*255/63),char((c&31)*255/31)};file.write(rgb,3);}
    }
};
struct MessageFont {
    BitFont font{""};BitFont* previous=BitFont::Instance;
    MessageFont(){
        auto* data=GameCreate<BitFont::InternalData>();*data={5,1,7,9,2,8,nullptr,nullptr,2};
        data->SymbolTable=static_cast<short*>(YRMemory::Allocate(0x20000));std::fill_n(data->SymbolTable,0x10000,short(1));
        data->SymbolTable[L' ']=2;data->Bitmaps=static_cast<char*>(YRMemory::Allocate(16));
        data->Bitmaps[0]=5;std::memset(data->Bitmaps+1,0xA8,7);data->Bitmaps[8]=3;std::memset(data->Bitmaps+9,0,7);
        font.InternalPTR=data;font.field_18=1;font.field_1C=9;font.Unknown_28=16;BitFont::Instance=&font;
    }
    ~MessageFont(){BitFont::Instance=previous;}
};
struct MessageDrawingState {
    RectangleStruct bounds=DSurface::WindowBounds;bool focused=Game::IsFocused;
    MessageDrawingState(){DSurface::WindowBounds={0,0,640,120};Game::IsFocused=true;}
    ~MessageDrawingState(){DSurface::WindowBounds=bounds;Game::IsFocused=focused;GadgetClass::Focused=nullptr;OwnerDraw::IMEComposing=0;OwnerDraw::SuppressCaret=0;}
};
}
TEST(Messages, AnimatedTextPixelsMatchSurfaceAndRestoreColor){
    MessageDrawingState restore;MessageFont source;MessageCanvas canvas;game::UiResources resources;
    auto& font=source.font;
    BSurface surface(640,120,2);
    for(const auto* text:{L"ABCDE",L"A\t B\r\nC",L"中A中"})for(int animation:{0,1,2,8,20,-1})for(int length:{0,1,3}){
        std::fill(canvas.pixels.begin(),canvas.pixels.end(),WORD(0x1234));surface.Fill(0x1234);
        RectangleStruct rect{2,3,29,12};
        const int expected=OwnerDraw::PrintTextFixedLength(0x35CFEC,&font,&rect,text,length,0,0,&surface,animation);
        int actual=0;
        const auto status=canvas.draw(resources,[&]{actual=OwnerDraw::PrintTextFixedLength(0x35CFEC,&font,&rect,text,length,0,0,nullptr,animation);});
        EXPECT_TRUE(status==game::DrawingStatus::drawn || status==game::DrawingStatus::skipped);
        EXPECT_EQ(actual,expected);EXPECT_EQ(font.Color,WORD(RGBClass(0x35CFEC).ToInt()));
        const auto* pixels=static_cast<const WORD*>(surface.Lock(0,0));ASSERT_NE(pixels,nullptr);
        EXPECT_TRUE(std::equal(canvas.pixels.begin(),canvas.pixels.end(),pixels))<<"animation="<<animation<<" length="<<length;
        surface.Unlock();
    }
    // A failed explicit Surface lock must never redirect its glyphs into the
    // concurrently active modern frame; an unavailable glyph destination does
    // not advance the native BitFont pen.
    struct FailedSurface:BSurface {
        int unlocks=0;
        FailedSurface():BSurface(640,120,2){}
        void* Lock(int,int)override{return nullptr;}
        bool Unlock()override{++unlocks;return false;}
    } unavailable;
    const auto unchanged=canvas.pixels;
    EXPECT_EQ(canvas.draw(resources,[&]{EXPECT_EQ(BitText::Print(&font,&unavailable,L"ABCDE",10,3,0,0),10);}),game::DrawingStatus::skipped);
    EXPECT_EQ(unavailable.unlocks,1);EXPECT_EQ(canvas.pixels,unchanged);
    canvas.fail=true;font.Color=0x1357;LTRBStruct bounds{0,0,100,20};font.SetRectangle(&bounds);
    EXPECT_EQ(canvas.draw(resources,[&]{BitText::Print(&font,nullptr,L"ABCDE",2,3,0,2);}),game::DrawingStatus::backend_failure);
    EXPECT_EQ(font.Color,0x1357);EXPECT_EQ(font.pGraphBuffer,nullptr);
}
TEST(Messages, LabelClockWrapRepeatedDrawFocusAndComposition){
    std::uint32_t now=0xFFFFFFF0u;
    ASSERT_TRUE(game::with_clock({&now,[](void* p)noexcept{return *static_cast<std::uint32_t*>(p);}},[](void* p){
        auto& now=*static_cast<std::uint32_t*>(p);
        MessageDrawingState restore;MessageFont font;MessageCanvas canvas;game::UiResources resources;
        BytePalette palette{};ColorScheme scheme("message-test",{0,255,255},palette,1,true);
        wchar_t text[]=L"ABCDE";TextLabelClass label(text,12,21,0,TextPrintType(0x4046));label.PixWidth=200;label.Animate=true;
        auto draw=[&]{EXPECT_TRUE(label.Draw(true));};
        EXPECT_EQ(canvas.draw(resources,draw),game::DrawingStatus::drawn);EXPECT_EQ(label.AnimPos,1u);EXPECT_EQ(label.AnimTiming,now);
        const auto first=canvas.pixels;
        label.PixWidth=0xFFFFFFFFu;canvas.draw(resources,draw);
        EXPECT_EQ(font.font.Bounds.Right,640)<<"TextLabel clamps width to the logical surface before drawing";
        label.PixWidth=200;
        EXPECT_EQ(canvas.draw(resources,draw),game::DrawingStatus::drawn);EXPECT_EQ(canvas.pixels,first);EXPECT_EQ(label.AnimPos,1u);
        now=0xFFFFFFFFu;canvas.draw(resources,draw);EXPECT_EQ(label.AnimPos,1u);EXPECT_EQ(label.AnimTiming,0xFFFFFFF0u);
        now=0;canvas.draw(resources,draw);EXPECT_EQ(label.AnimPos,2u);EXPECT_EQ(label.AnimTiming,0u);
        const auto second=canvas.pixels;EXPECT_NE(second,first);
        label.SkipDraw=true;now=100;canvas.draw(resources,[&]{EXPECT_FALSE(label.Draw(true));});EXPECT_EQ(label.AnimPos,2u);
        label.SkipDraw=false;label.Animate=false;label.AnimPos=0;label.SetFocus();
        game::TextComposition composition{L"中A",1};OwnerDraw::IMEComposing=1;
        canvas.draw(resources,draw,&composition);EXPECT_EQ(OwnerDraw::IMECompositionStringLength,2);EXPECT_EQ(OwnerDraw::IMECompositionCursorPos,1);
        EXPECT_STREQ(OwnerDraw::IMECompositionString,L"中A");
        canvas.draw(resources,draw);EXPECT_EQ(OwnerDraw::IMECompositionStringLength,0);EXPECT_EQ(OwnerDraw::IMECompositionCursorPos,0);
        // Snapshot reads leave the original composing-state flag untouched.
        EXPECT_EQ(OwnerDraw::IMEComposing,1);
        OwnerDraw::SuppressCaret=1;canvas.draw(resources,draw);EXPECT_EQ(OwnerDraw::SuppressCaret,0);
        Game::IsFocused=false;
        std::fill(canvas.pixels.begin(),canvas.pixels.end(),WORD(0x1234));label.KillFocus();canvas.draw(resources,draw);
        EXPECT_EQ(canvas.pixels[21*640+12],0u)<<"Original black background remains, but no glyphs while unfocused";
    },&now));
}
TEST(Messages, MainLoopMaintainsMessagesAfterLogicAndAfterRender){
    MessageDrawingState restore;MessageFont font;
    auto& list=MessageListClass::Instance;list.Init(3,0,6,98,14,-1,-1,0,20,98,640);
    const auto reset=ra2::test::scope_exit([&]{list.Init(0,0,0,0,0,0,0,0,0,0);});
    ASSERT_NE(list.AddMessage(nullptr,17,L"expired",0,TextPrintType(0),1,true),nullptr);
    ASSERT_TRUE(game::advance_clock(0.032));
    game::GameLoopState state;game::GameLoopContext context{state,false,nullptr,nullptr,
        [](void*){EXPECT_NE(MessageListClass::Instance.GetLabel(17),nullptr);return true;},
        [](void*){EXPECT_NE(MessageListClass::Instance.GetLabel(17),nullptr);return true;}};
    ASSERT_TRUE(game::advance_game_loop(context,0));EXPECT_EQ(list.GetLabel(17),nullptr);
    ASSERT_NE(list.AddMessage(nullptr,18,L"paused",0,TextPrintType(0),1,true),nullptr);
    ASSERT_TRUE(game::advance_clock(0.064));context.paused=true;context.render=nullptr;context.logic=nullptr;
    ASSERT_TRUE(game::advance_game_loop(context,0));EXPECT_NE(list.GetLabel(18),nullptr);
    context.paused=false;ASSERT_TRUE(game::advance_game_loop(context,0.128));EXPECT_EQ(list.GetLabel(18),nullptr);
}

TEST(Messages, DrawListOrderAndRealFontAnimationArtifact){
    const char* data=std::getenv("RA2_GAME_DATA");if(!data || !*data)GTEST_SKIP()<<"Original message font integration";
    game::ResourceHandle* raw=nullptr;std::string error;ASSERT_TRUE(game::create_resources(data,raw,error))<<error;
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(raw,game::destroy_resources);
    ASSERT_EQ(game::load_resources(*files,{},error),game::ResourceLoadResult::complete)<<error;
    EXPECT_TRUE(game::with_resources(*files,[](void*){
        MessageDrawingState restore;game::UiResources resources;ASSERT_TRUE(resources.load(0))<<resources.error();
        BytePalette palette{};ColorScheme scheme("message-art",{0,0,255},palette,1,true);
        MessageCanvas canvas;
        std::vector<int> order;
        struct Label:TextLabelClass {
            std::vector<int>& order;int id;
            Label(wchar_t* text,std::vector<int>& order,int id):TextLabelClass(text,10,id*20,0,TextPrintType(0)),order(order),id(id){}
            bool Draw(bool forced)override{order.push_back(id);return TextLabelClass::Draw(forced);}
        };
        MessageListClass list;list.Init(10,20,6,98,14,-1,-1,0,20,98,600);
        auto* first=new Label(list.MessageBuffers[0],order,1);auto* second=new Label(list.MessageBuffers[1],order,2);
        std::wcscpy(first->Text,L"First message");std::wcscpy(second->Text,L"Second message");
        list.MessageList=first;second->AddTail(*first);
        auto* edit=new Label(list.EditBuffer,order,0);std::wcscpy(edit->Text,L"Editing");list.EditLabel=edit;list.IsEdit=true;
        EXPECT_EQ(canvas.draw(resources,[&]{list.Draw();}),game::DrawingStatus::drawn);
        EXPECT_EQ(order,(std::vector<int>{0,1,2}));
        list.Init(0,0,0,0,0,0,0,0,0,0);std::fill(canvas.pixels.begin(),canvas.pixels.end(),WORD(0x1234));
        wchar_t text[]=L"Beacon placed";TextLabelClass label(text,12,42,0,TextPrintType(0x4046));label.Animate=true;label.PixWidth=190;
        const wchar_t* titles[]{L"0 ms",L"80 ms",L"560 ms"};
        for(int panel=0;panel<3;++panel){
            if(panel)ASSERT_TRUE(game::advance_clock(panel==1?0.080:0.480));
            label.X=12+panel*210;
            EXPECT_EQ(canvas.draw(resources,[&]{label.Draw(true);wchar_t* title=const_cast<wchar_t*>(titles[panel]);
                TextLabelClass caption(title,label.X,12,0,TextPrintType(0));caption.Draw(true);}),game::DrawingStatus::drawn);
        }
        EXPECT_GT(std::count_if(canvas.pixels.begin(),canvas.pixels.end(),[](WORD c){return c!=0 && c!=0x1234;}),100);
        if(const char* path=std::getenv("RA2_MESSAGE_OUTPUT"))canvas.save(path);
    },nullptr,error))<<error;
}
