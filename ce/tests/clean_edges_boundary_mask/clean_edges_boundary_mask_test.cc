#include "clean_edges_boundary_mask.h"

#include <cstdio>

namespace {

bool check(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "FAIL: %s\n", message);
    return condition;
}

} // namespace

int main()
{
    using fallout::scene::isCleanEdgesBoundaryMask;

    if (!check(isCleanEdgesBoundaryMask(true, true, true, "art/scenery/BLACK002.FRM"),
            "case-insensitive scenery path resolves the dedicated mask")) return 1;
    if (!check(isCleanEdgesBoundaryMask(true, true, true, "ART\\SCENERY\\black002.frm"),
            "Windows art paths resolve the dedicated mask")) return 1;
    if (!check(isCleanEdgesBoundaryMask(true, true, true, "BLACK002.frm"),
            "bare art-list entry resolves the dedicated mask")) return 1;
    if (!check(!isCleanEdgesBoundaryMask(false, true, true, "art/scenery/black002.frm"),
            "disabled clean_edges preserves the mask")) return 1;
    if (!check(!isCleanEdgesBoundaryMask(true, false, true, "art/scenery/black002.frm"),
            "inactive effective clip preserves the mask even when clean_edges is on")) return 1;
    if (!check(!isCleanEdgesBoundaryMask(true, true, false, "art/scenery/black002.frm"),
            "non-scenery art is never classified as the mask")) return 1;
    if (!check(!isCleanEdgesBoundaryMask(true, true, true, "art/scenery/mount02.frm"),
            "legitimate mountain scenery is retained")) return 1;
    if (!check(!isCleanEdgesBoundaryMask(true, true, true, "art/scenery/black002.frx"),
            "similar filenames are not classified as the mask")) return 1;
    if (!check(!isCleanEdgesBoundaryMask(true, true, true, nullptr),
            "missing art path is retained safely")) return 1;
    return 0;
}
