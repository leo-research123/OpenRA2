// Existing TabClass consumes the original 72FC60 command surface in 6D0A20.
#include "yrpp/MouseClass.h"
#include "yrpp/Surface.h"
RectangleStruct TabClass::GetCommandBarBounds() const noexcept {
    return {0, DSurface::WindowBounds.Height - 32, DSurface::ViewBounds.Width, 32};
}
