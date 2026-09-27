#include "bridge/ra2_map_view.hpp"
#include "map_resources.hpp"
#include "map_input.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/image.hpp>
#include <chrono>
#include <array>
#include <exception>
#include <new>
#include <cstdio>
#include <cstdlib>
using namespace godot;
void RA2MapView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("configure","core"),&RA2MapView::configure);
    ClassDB::bind_method(D_METHOD("open_map","filename"),&RA2MapView::open_map);
    ClassDB::bind_method(D_METHOD("close_map"),&RA2MapView::close_map);
    ClassDB::bind_method(D_METHOD("get_render_status"),&RA2MapView::get_render_status);
    ClassDB::bind_method(D_METHOD("handle_map_input","event","surface_size"),&RA2MapView::handle_map_input);
    ClassDB::bind_method(D_METHOD("reset_map_input"),&RA2MapView::reset_map_input);
}
RA2MapView::RA2MapView() : resources_(new(std::nothrow) game::MapResources) {
    if (!resources_) error_="Map resource/input allocation failed";
    set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
    set_process(true);
}
RA2MapView::~RA2MapView() { close_map(); }
void RA2MapView::_notification(int what) {
    if (what==NOTIFICATION_EXIT_TREE) close_map();
    if (what==NOTIFICATION_WM_WINDOW_FOCUS_IN) focused_=true;
    if (what==NOTIFICATION_WM_WINDOW_FOCUS_OUT) focused_=false;
    if (what==NOTIFICATION_PAUSED || what==NOTIFICATION_WM_WINDOW_FOCUS_OUT || what==NOTIFICATION_WM_MOUSE_EXIT) reset_map_input();
}
Dictionary RA2MapView::handle_map_input(const Ref<InputEvent>& event,const Vector2& size) {
    Dictionary device;
    if (core_.is_null() || !focused_ || !can_process()) return device;
    game::GameInputEvent input; game::GameInputResult result;
    const auto canvas=get_viewport_rect().size;
    if (game::encode_canvas_input(event,size,canvas,input,control_click_) && core_->submit_input(input,result)) {
        device["consumed"]=result.consumed;
        if (result.warp_pointer)
            device["warp_pointer"]=Vector2(result.pointer_x,result.pointer_y)*size/canvas;
    }
    return device;
}
void RA2MapView::reset_map_input() {
    control_click_=false;
    if (core_.is_valid()) { game::GameInputEvent event; event.kind=game::GameInputKind::pointer_leave;
        game::GameInputResult result; core_->submit_input(event,result); }
}
void RA2MapView::configure(const Ref<RA2Core>& core) { close_map(); core_=core; }
void RA2MapView::open_map(const String& filename) {
    if (core_.is_null()) { error_="Map core is not configured"; return; }
    close_map();
    renderer_.instantiate();
    core_->begin_map_loading(filename);
}
void RA2MapView::close_map() {
    reset_map_input();
    texture_.unref();
    texture_rid_={};
    if (renderer_.is_valid()) { renderer_->shutdown(); renderer_.unref(); }
    if (resources_) resources_->clear();
    if (core_.is_valid()) core_->close_map();
    last_clock_usec_=0;
    width_=height_=0; revision_=camera_revision_=ui_revision_=presentation_revision_=0; drawn_=0; packets_=0;
    error_=resources_ ? "" : "Map resource/input allocation failed";
    queue_redraw();
}
Dictionary RA2MapView::get_render_status() const {
    auto result=core_.is_valid() ? core_->get_map_status() : Dictionary();
    if (!result.has("state")) result["state"]="empty";
    auto error=error_;
    if (error.is_empty() && renderer_.is_valid()) error=renderer_->error();
    result["failure_stage"]="loading";
    if (!error.is_empty()) { result["state"]="failed"; result["error"]=error; result["failure_stage"]="rendering"; }
    else if (String(result["state"])=="ready") result["state"]=texture_.is_valid() && renderer_.is_valid() && renderer_->is_current() ? "drawn" : "rendering";
    result["drawn_cells"]=drawn_; result["packets"]=static_cast<std::int64_t>(packets_);
    result["viewport_width"]=width_; result["viewport_height"]=height_;
    result["map_input_enabled"]=focused_ && can_process();
    result["decoded_tiles"]=static_cast<std::int64_t>(resources_ ? resources_->decoded_tiles() : 0);
    result["decoded_lighting"]=static_cast<std::int64_t>(resources_ ? resources_->decoded_lighting() : 0);
    result["palettes"]=static_cast<std::int64_t>(resources_ ? resources_->prepared_palettes() : 0);
    return result;
}
void RA2MapView::_process(double) {
    if (core_.is_null() || renderer_.is_null() || !error_.is_empty()) return;
    try {
        auto status=core_->get_map_status();
        if (String(status.get("state","empty"))!="ready") return;
        // Device wall clock, unaffected by Engine.time_scale or delta clamping.
        const auto now=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
        const double seconds=last_clock_usec_?double(now-last_clock_usec_)/1000000.0:0.0;
        last_clock_usec_=now;
        game::GameInputEvent focus;game::GameInputResult ignored;
        focus.kind=focused_?game::GameInputKind::focus_gained:game::GameInputKind::focus_lost;
        core_->submit_input(focus,ignored);
        const auto presentation_revision=static_cast<std::uint64_t>(std::int64_t(status.get("presentation_revision",0)));
        const auto ui_revision=static_cast<std::uint64_t>(std::int64_t(status.get("ui_revision",0)));
        const auto revision=static_cast<std::uint64_t>(std::int64_t(status.get("revision",0)));
        const auto camera_revision=static_cast<std::uint64_t>(std::int64_t(status.get("camera_revision",0)));
        const auto size=get_viewport_rect().size;
        const int width=static_cast<int>(size.x),height=static_cast<int>(size.y);
        if (width<640 || height<480) return;
        {
            if (revision_!=revision || width_!=width || height_!=height) {
                texture_.unref(); texture_rid_={}; queue_redraw();
            }
            if (revision_!=revision) resources_->clear();
            auto frame=std::make_shared<game::MapFrameData>();
            frame->generation=revision; frame->width=width; frame->height=height;
            const auto drawing=resources_->begin_frame(*frame);
            bool rendered=false;
            const auto profile_start=std::chrono::steady_clock::now();
            const auto result=core_->advance_map(seconds,width,height,drawing,frame->stats,rendered);
            if(rendered && std::getenv("RA2_MAP_PROFILE")) std::fprintf(stderr,"MAP_PROFILE core_us=%lld\n",
                static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-profile_start).count()));
            resources_->end_frame();
            if (result!=game::DrawingStatus::drawn && result!=game::DrawingStatus::skipped) {
                const auto failed=core_->get_map_status();
                auto detail=String(failed.get("error",""));
                if(resources_->error()[0])detail+="\n"+String::utf8(resources_->error());
                error_=String("Frame preparation: ")+game::drawing_status_name(result)+"\n"+detail+
                    "\nframe="+String::num_int64(std::int64_t(failed.get("current_frame",0)))+
                    " camera=("+String::num_int64(std::int64_t(failed.get("camera_x",0)))+","+
                    String::num_int64(std::int64_t(failed.get("camera_y",0)))+")";
                return;
            }
            if(rendered){
            drawn_=frame->stats.drawn; packets_=frame->packets.size();
            renderer_->submit(std::move(frame));
            width_=width; height_=height; revision_=revision;
            camera_revision_=static_cast<std::uint64_t>(std::int64_t(core_->get_map_status().get("camera_revision",0)));
            ui_revision_=ui_revision;
            presentation_revision_=presentation_revision;
            }

        }
        auto texture=renderer_->texture();
        const auto rid=texture.is_valid() ? texture->get_texture_rd_rid() : RID();
        // Texture2DRD keeps its Ref identity when its GPU target is recreated.
        // Canvas draw commands must be rebuilt for the new RID after a resize.
        if (texture!=texture_ || rid!=texture_rid_) { texture_=texture; texture_rid_=rid; queue_redraw(); }
    } catch (const std::exception& e) { error_=String::utf8(e.what()); }
    catch (...) { error_="Map frame preparation failed"; }
}
void RA2MapView::_draw() { if (texture_.is_valid()) draw_texture(texture_,Vector2()); }
