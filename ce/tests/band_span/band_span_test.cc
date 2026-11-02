#include "band_span.h"

#include <cstdio>
#include <vector>
#include <utility>

using fallout::scene::bandPieceSpan;

int main()
{
    int failures = 0;
    auto check = [&](bool ok, const char* what) {
        if (!ok) { std::printf("FAIL: %s\n", what); failures++; }
    };
    // A run of squares 10..13 whose wall covers only [12.2, 13.8]: only the squares 12 and 13 draw,
    // inside the span, none reversed or overlapping.
    std::vector<std::pair<float, float>> pieces;
    for (int along = 10; along <= 13; along++) {
        float low = 0, high = 0;
        if (bandPieceSpan(along, along == 10, along == 13, 12.2f, 13.8f, &low, &high)) pieces.push_back({ low, high });
    }
    check(pieces.size() == 2, "two squares draw");
    if (pieces.size() == 2) {
        check(pieces[0].first == 12.2f && pieces[0].second == 13.0f, "square 12 draws 12.2 to 13");
        check(pieces[1].first == 13.0f && pieces[1].second == 13.8f, "square 13 draws 13 to 13.8");
    }
    // A corner join: the first square reaches below its start, the last above its end.
    float low = 0, high = 0;
    check(bandPieceSpan(60, true, false, 59.75f, 62.0f, &low, &high) && low == 59.75f && high == 61.0f, "first square reaches the corner");
    check(bandPieceSpan(61, false, true, 59.75f, 62.25f, &low, &high) && low == 61.0f && high == 62.25f, "last square reaches the corner");
    check(!bandPieceSpan(11, false, false, 12.2f, 13.8f, &low, &high), "a square before the span draws nothing");
    if (failures == 0) std::printf("band_span_tests passed\n");
    return failures == 0 ? 0 : 1;
}
