#pragma once

#include <algorithm>

namespace fallout::scene {

// The part of a run's band that one roof square of it draws: the square
// [along, along + 1] cut to the band's span [spanLow, spanHigh] (the wall's
// rows, the ends joined to the bands across at the corners). The run's first
// square may reach below its start and its last above its end (a corner
// join); every other end stays inside the square. Returns false when nothing
// of the span lies in the square: no piece, never a reversed or overlapping one.
inline bool bandPieceSpan(int along, bool first, bool last, float spanLow, float spanHigh, float* low, float* high)
{
    *low = first ? spanLow : std::max(static_cast<float>(along), spanLow);
    *high = last ? spanHigh : std::min(static_cast<float>(along + 1), spanHigh);
    return *high > *low;
}

} // namespace fallout::scene
