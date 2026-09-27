#include "bridge/type_drawing_check.hpp"
#include "bridge/ra2_core.hpp"
#include "bridge/ra2_map_view.hpp"
#include "bridge/test_ball.hpp"
#include "bridge/test_infantry.hpp"
#include <godot_cpp/godot.hpp>

namespace {
void initialize(godot::ModuleInitializationLevel level) {
    if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        godot::ClassDB::register_class<RA2INI>();
        godot::ClassDB::register_class<RA2TypeDrawingCheck>();
        godot::ClassDB::register_class<RA2Core>();
        godot::ClassDB::register_class<RA2MapRenderer>();
        godot::ClassDB::register_class<RA2MapView>();
#ifdef RA2_HAS_TEST_BALL
        godot::ClassDB::register_class<RA2TestBall>();
#endif
        godot::ClassDB::register_class<RA2TestInfantry>();
    }
}

void terminate(godot::ModuleInitializationLevel) {}
}

extern "C" {
GDExtensionBool GDE_EXPORT ra2_core_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) {
    godot::GDExtensionBinding::InitObject init(get_proc_address, library, initialization);
    init.register_initializer(initialize);
    init.register_terminator(terminate);
    init.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}
}
