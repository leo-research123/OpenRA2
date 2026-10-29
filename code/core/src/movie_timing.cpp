// Original BinkMoviePlayer completion query, 0x00432C50.
#include "movie_player.hpp"
namespace game {
unsigned movie_frame_period(unsigned numerator, unsigned denominator) noexcept {
    return denominator && numerator / denominator ? 1000u / (numerator / denominator) : 0;
}
bool movie_frame_finished(unsigned next, unsigned total, unsigned previous) noexcept {
    return next >= total || next < previous;
}
}
