#pragma once
#include "type_drawing_context.hpp"
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"

namespace game {
// Project entry points that submit the original DSurface line and ellipse
// primitives to a drawing scope. They are not YRpp methods: the original
// functions write a locked 16-bit surface, these produce clipped per-pixel
// raster requests. Endpoints are clipped like Line_In_Bounds (0x007BC2B0).
//
// The submit_* functions below taking a PresentationContext draw to its type context
// (ctx.type) and fold their result into the type drawing scope the context is in
// (with_type_drawing). They return false without a type context and never throw.

// XSurface ellipse, YR 0x007BB350.
bool submit_ellipse(PresentationContext& ctx, const RectangleStruct& clip, Point2D center, int radius_x, int radius_y, WORD color) noexcept;
// DrawBlendedLine primitive, YR 0x004BEAC0, without cropping to a surface.
bool submit_blended_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D first, Point2D last, ColorStruct color, int opacity,
                         int first_z, int last_z) noexcept;
// DrawLineColor primitive, YR 0x004BFD30.
bool submit_color_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D first, Point2D last, WORD color, int first_z,
                       int last_z) noexcept;
// YR 0x004BDF00: the source is quantized before geometry and added to the destination.
bool submit_rgb_multiplying_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D first, Point2D last, ColorStruct color,
                                 float intensity, int first_z, int last_z) noexcept;
// YR 0x004BC750: cubic coverage with a paired destination read.
bool submit_subtractive_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D first, Point2D last, ColorStruct color, int first_z,
                             int last_z, bool red, bool green, bool blue, float intensity) noexcept;
// Spotlight glow line: Z-tested, brightens the pixels it covers by `strength`.
bool submit_glow_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D first, Point2D last, int first_z, int last_z,
                      int strength) noexcept;
// YR 0x004BF750's clipped RGB565 gradient through an explicit context. Endpoints
// are relative to clip origin; phase/step are in-out, as in the original.
DrawingStatus submit_gradient_line(const TypeDrawingContext&, const RectangleStruct& clip, Point2D start, Point2D end,
                                   ColorStruct first, ColorStruct last, float& step, float& phase) noexcept;
}
