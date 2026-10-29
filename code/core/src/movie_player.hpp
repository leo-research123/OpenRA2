#pragma once
#include "yrpp/OwnerDraw.h"
namespace game {
WWMovieHandle* create_movie_handle(const char*, DSurface*, const RectangleStruct*) noexcept;
WWMovieHandle* create_sidebar_movie_handle(const char*) noexcept;
unsigned movie_frame_period(unsigned numerator, unsigned denominator) noexcept;
bool movie_frame_finished(unsigned next, unsigned total, unsigned previous) noexcept;
void draw_movie_handle(WWMovieHandle*) noexcept;
void enter_movie_audio() noexcept;
void leave_movie_audio() noexcept;
}
