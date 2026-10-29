#include "yrpp/OwnerDraw.h"
#include "movie_player.hpp"
#if !defined(RA2_YRPP_GAME)
WWMovieHandle* YRPP_FASTCALL OwnerDraw::InitMovieHandle(const char* name, DSurface* surface,
                                                        RectangleStruct* bounds) noexcept {
    return game::create_movie_handle(name, surface, bounds);
}
WWMovieHandle* OwnerDraw::InitSidebarMovieHandle(const char* name) noexcept {
    return game::create_sidebar_movie_handle(name);
}
#endif
