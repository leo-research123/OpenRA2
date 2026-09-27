#include "yrpp/MouseClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Unsorted.h"

namespace game { namespace {
bool input_locked=false;
bool drag_select_aborted=false;
int mute_sw_launches=0;
DSurface* logical_surface=nullptr;
bool game_in_focus=true;
int special_dialog=0;
RectangleStruct sidebar_bounds{}, view_bounds{}, window_bounds{}, tactical_bounds{};
Point2D repair_position{}, tab_position{}, cameo_position{}, cameo_pitch{}, scroll_position{}, scroll_pitch{};
int repair_pitch{}, tab_pitch{}, cameo_height{};
SelectClass select_buttons[4 * 60]{};
ShapeButtonClass tab_buttons[4],repair_button,sell_button;
ShapeButtonClass command_buttons[25],collapse_button,expand_button;
ShapeButtonClass radar_diplomacy_button,radar_options_button;
SHPStruct *radar_diplomacy_shape{},*radar_options_shape{},*radar_anim{};
bool owns_radar_diplomacy_shape{},owns_radar_options_shape{};
RectangleStruct radar_diplomacy_bounds{},radar_options_bounds{};
int command_positions[25]{}, command_count{};
} }
DSurface*& DSurface::Temp=game::logical_surface;
RectangleStruct& DSurface::SidebarBounds = game::sidebar_bounds;
RectangleStruct& DSurface::ViewBounds = game::view_bounds;
RectangleStruct& DSurface::WindowBounds = game::window_bounds;
RectangleStruct& TacticalClass::ViewBounds = game::tactical_bounds;
Point2D& SidebarClass::RepairPosition = game::repair_position;
int& SidebarClass::RepairPitch = game::repair_pitch;
Point2D& SidebarClass::TabPosition = game::tab_position;
int& SidebarClass::TabPitch = game::tab_pitch;
Point2D& SidebarClass::CameoPosition = game::cameo_position;
Point2D& SidebarClass::CameoPitch = game::cameo_pitch;
int& SidebarClass::CameoHeight = game::cameo_height;
Point2D& SidebarClass::ScrollPosition = game::scroll_position;
Point2D& SidebarClass::ScrollPitch = game::scroll_pitch;
SelectClass* SelectClass::Array() noexcept { return game::select_buttons; }
int (&TabClass::CommandPositions)[25] = game::command_positions;
int& TabClass::CommandCount = game::command_count;

#include "yrpp/InputManagerClass.h"
namespace game { namespace {
GadgetClass *stuck{},*last_list{},*focused{},*hovered{},*buttons{};
RadarClass::RTacticalClass radar_button;
InputManagerClass* input_manager{};
GameOptionsClass options=[] {
    GameOptionsClass value{};
    value.GameSpeed=2; value.DetailLevel=2;
    value.ScrollRate=3; value.ScrollMethod=0; value.AutoScroll=true;
    value.SidebarMode=true;
    value.Tooltips=true;
    value.KeyForceMove1=value.KeyForceMove2=18;
    value.KeyForceFire1=value.KeyForceFire2=17;
    value.KeyForceSelect1=value.KeyForceSelect2=16;
    return value;
}();
} }
GadgetClass*& GadgetClass::StuckOn=game::stuck;
GadgetClass*& GadgetClass::LastList=game::last_list;
GadgetClass*& GadgetClass::Focused=game::focused;
GadgetClass*& GadgetClass::Hovered=game::hovered;
GadgetClass*& GScreenClass::Buttons=game::buttons;
RadarClass::RTacticalClass& RadarClass::RadarButton=game::radar_button;
InputManagerClass*& InputManagerClass::Instance=game::input_manager;
GameOptionsClass& GameOptionsClass::Instance=game::options;
// Both original names address the same byte (e.g. 0x006211D5).
bool& Game::IsFocused=game::game_in_focus;
bool& Unsorted::GameInFocus=game::game_in_focus;
bool& Unsorted::UserInputLocked=game::input_locked;
bool& Unsorted::DragSelectAborted=game::drag_select_aborted;
int& Unsorted::MuteSWLaunches=game::mute_sw_launches;
int& Game::SpecialDialog=game::special_dialog;
int& Unsorted::SpecialDialog=game::special_dialog;

ShapeButtonClass (&SidebarClass::TabButtons)[4]=game::tab_buttons;
ShapeButtonClass& SidebarClass::ToggleRepairButton=game::repair_button;
ShapeButtonClass& SidebarClass::ToggleSellButton=game::sell_button;

ShapeButtonClass (&TabClass::CommandButtons)[25]=game::command_buttons;
ShapeButtonClass& TabClass::CollapseButton=game::collapse_button;
ShapeButtonClass& TabClass::ExpandButton=game::expand_button;
ShapeButtonClass& RadarClass::DiplomacyButton=game::radar_diplomacy_button;
ShapeButtonClass& RadarClass::OptionsButton=game::radar_options_button;
SHPStruct*& RadarClass::DiplomacyShape=game::radar_diplomacy_shape;
SHPStruct*& RadarClass::OptionsShape=game::radar_options_shape;
SHPStruct*& RadarClass::RadarAnim=game::radar_anim;
bool& RadarClass::OwnsDiplomacyShape=game::owns_radar_diplomacy_shape;
bool& RadarClass::OwnsOptionsShape=game::owns_radar_options_shape;
RectangleStruct& RadarClass::DiplomacyBounds=game::radar_diplomacy_bounds;
RectangleStruct& RadarClass::OptionsBounds=game::radar_options_bounds;

#include "yrpp/RadarEventClass.h"
#include "yrpp/FileSystem.h"
namespace game { namespace {
ConvertClass* mouse_palette{};
BytePalette waypoint_palette{};
DynamicVectorClass<RadarEventClass*> radar_events;
CellStruct radar_history[8]{};
int radar_history_index{},radar_history_cursor{};
} }
ConvertClass*& FileSystem::MOUSE_PAL=game::mouse_palette;
BytePalette& FileSystem::WAYPOINT_PAL=game::waypoint_palette;
DynamicVectorClass<RadarEventClass*>& RadarEventClass::Array=game::radar_events;
CellStruct (&RadarEventClass::History)[8]=game::radar_history;
int& RadarEventClass::HistoryIndex=game::radar_history_index;
int& RadarEventClass::HistoryCursor=game::radar_history_cursor;

#include "yrpp/WWMouseClass.h"
namespace game { namespace {
SHPStruct* cursor_shape{};
WWMouseClass* mouse_driver{};
SysTimerClass cursor_timer;
bool cursor_initialized{};
// Fixed YR table at 0x0082D028; original frame/count/hotspot data.
MouseCursor cursors[86]={
    {0,1,0,1,1,MouseHotSpotX(0),MouseHotSpotY(0)},
    {2,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(0)},
    {3,1,0,-1,-1,MouseHotSpotX(54321),MouseHotSpotY(0)},
    {4,1,0,-1,-1,MouseHotSpotX(54321),MouseHotSpotY(12345)},
    {5,1,0,-1,-1,MouseHotSpotX(54321),MouseHotSpotY(54321)},
    {6,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(54321)},
    {7,1,0,-1,-1,MouseHotSpotX(0),MouseHotSpotY(54321)},
    {8,1,0,-1,-1,MouseHotSpotX(0),MouseHotSpotY(12345)},
    {9,1,0,-1,-1,MouseHotSpotX(0),MouseHotSpotY(0)},
    {10,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(0)},
    {11,1,0,-1,-1,MouseHotSpotX(54321),MouseHotSpotY(0)},
    {12,1,0,-1,-1,MouseHotSpotX(54321),MouseHotSpotY(12345)},
    {13,1,0,-1,-1,MouseHotSpotX(54321),MouseHotSpotY(54321)},
    {14,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(54321)},
    {15,1,0,-1,-1,MouseHotSpotX(0),MouseHotSpotY(54321)},
    {16,1,0,-1,-1,MouseHotSpotX(0),MouseHotSpotY(12345)},
    {17,1,0,-1,-1,MouseHotSpotX(0),MouseHotSpotY(0)},
    {18,13,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {31,10,4,42,10,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {41,1,0,52,1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {53,5,4,63,5,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {58,5,4,63,5,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {68,5,4,73,5,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {78,10,4,-1,10,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {88,1,0,-1,1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {89,10,4,100,10,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {99,1,0,63,1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {110,9,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {119,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {120,9,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {129,10,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {139,10,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {149,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {150,20,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {170,20,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {190,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {191,7,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {199,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {204,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {209,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {214,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {219,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {224,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {229,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {234,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {239,10,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {249,10,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {259,10,0,516,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {269,10,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {356,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {279,20,4,514,1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {299,10,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {309,10,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {319,10,4,513,1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {329,10,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {339,6,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {345,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {346,5,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {357,12,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {369,15,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {384,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {385,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {386,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {387,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {388,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {389,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {390,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {391,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {392,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {393,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {394,10,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {404,9,4,63,5,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {413,9,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {422,9,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {431,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {432,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {433,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {434,1,0,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {435,15,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {450,10,4,-1,1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {460,10,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {470,10,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {480,8,4,-1,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {488,8,4,516,-1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {496,8,4,515,1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
    {504,8,4,512,1,MouseHotSpotX(12345),MouseHotSpotY(12345)},
};
} }
MouseCursor (&MouseCursor::Cursors)[86]=game::cursors;
SHPStruct*& MouseClass::CursorShape=game::cursor_shape;
SysTimerClass& MouseClass::CursorTimer=game::cursor_timer;
bool& MouseClass::CursorInitialized=game::cursor_initialized;
WWMouseClass*& WWMouseClass::Instance=game::mouse_driver;

#include "yrpp/BeaconClass.h"
#include "yrpp/BeaconManagerClass.h"
namespace game { namespace {
int selection_command_mode{};
bool type_selection_active{},type_selection_includes_map{};
BeaconManagerClass beacon_manager;
TechnoClass* sidebar_tab_objects[2]{};
} }
int& Game::SelectionCommandMode=game::selection_command_mode;
bool& Game::TypeSelectionActive=game::type_selection_active;
bool& Game::TypeSelectionIncludesMap=game::type_selection_includes_map;
BeaconManagerClass& BeaconManagerClass::Instance=game::beacon_manager;
BeaconClass* (&BeaconClass::Array)[8][3]=game::beacon_manager.Beacons;
int& BeaconClass::Count=game::beacon_manager.AllocatedCount;
TechnoClass* (&Game::SidebarTabObjects)[2]=game::sidebar_tab_objects;

#include "yrpp/MessageListClass.h"
#include "yrpp/SessionClass.h"
namespace game { namespace {
MessageListClass messages;
bool animate_lan_messages{},animate_internet_messages{};
} }
MessageListClass& MessageListClass::Instance=game::messages;
bool& SessionClass::AnimateLanMessages=game::animate_lan_messages;
bool& SessionClass::AnimateInternetMessages=game::animate_internet_messages;
