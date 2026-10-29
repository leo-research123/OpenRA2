#include "yrpp/GScreenClass.h"
#include "game_ui_runtime.hpp"
#include <cstdlib>
void GScreenClass::Render() { if (game::game_ui_frame()) std::abort(); }
