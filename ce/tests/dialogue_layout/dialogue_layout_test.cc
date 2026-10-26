#include "dialogue_layout.h"

#include <cstdio>
#include <vector>

using namespace fallout::scene;

int main()
{
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

    // Seven choices, eight 10px rows, and the existing 2px gap after each.
    std::vector<DialogueOptionLayoutProbe> sevenChoiceFixture = {
        { true, 17, 15 },
        { true, 29, 27 },
        { true, 41, 39 },
        { true, 53, 51 },
        { true, 65, 63 },
        { true, 77, 75 },
        { true, 99, 97 },
    };
    CHECK(dialogueOptionGroupGap(sevenChoiceFixture, 112, true) == 2);
    CHECK(dialogueOptionGroupGap(sevenChoiceFixture, 112, false) == 0);

    // The last row leaves only 2px under the stricter fit estimate.
    std::vector<DialogueOptionLayoutProbe> dense = {
        { true, 17, 15 },
        { true, 29, 27 },
        { true, 41, 39 },
        { true, 53, 51 },
        { true, 65, 63 },
        { true, 77, 75 },
        { true, 109, 107 },
    };
    CHECK(dialogueOptionGroupGap(dense, 112, true) == 0);

    // The gap is limited by actual draw end when an unbreakable word yields
    // fewer lines than the conservative width estimate predicts.
    std::vector<DialogueOptionLayoutProbe> clippedWord = {
        { true, 20, 18 },
        { true, 70, 111 },
    };
    CHECK(dialogueOptionGroupGap(clippedWord, 112, true) == 1);

    // Skipped baseline options do not create an extra boundary or consume
    // more space than the existing layout did.
    std::vector<DialogueOptionLayoutProbe> withSkipped = {
        { true, 17, 15 },
        { false, 0, 0 },
        { true, 29, 27 },
    };
    CHECK(dialogueOptionGroupGap(withSkipped, 112, true) == 2);

    std::vector<DialogueOptionLayoutProbe> oneChoice = { { true, 109, 107 } };
    CHECK(dialogueOptionGroupGap(oneChoice, 112, true) == 0);
    CHECK(dialogueOptionGroupGap({}, 112, true) == 0);

    // The production orchestration uses the same callback contract: the fake
    // adapter returns legacy fit estimates and the exact draw end for a top.
    const std::vector<int> sevenEstimateHeights = { 12, 12, 12, 12, 12, 12, 22 };
    const std::vector<int> sevenDrawHeights = { 10, 10, 10, 10, 10, 10, 20 };
    auto sevenEstimate = [&](std::size_t index, int top) { return top + sevenEstimateHeights[index]; };
    auto sevenDraw = [&](std::size_t index, int top) { return top + sevenDrawHeights[index]; };
    const DialogueLayout sevenOff = layoutDialogueOptions(7, 5, 112, false, sevenEstimate, sevenDraw);
    const DialogueLayout sevenOn = layoutDialogueOptions(7, 5, 112, true, sevenEstimate, sevenDraw);
    CHECK(sevenOff.groupGap == 0);
    CHECK(sevenOn.groupGap == 2);
    for (std::size_t index = 0; index < 7; index++) {
        CHECK(sevenOff.options[index].accepted);
        CHECK(sevenOff.options[index].top == 5 + static_cast<int>(index) * 12);
        CHECK(sevenOn.options[index].accepted);
        CHECK(sevenOn.options[index].top == 5 + static_cast<int>(index) * 14);
    }

    const DialogueLayout denseLayout = layoutDialogueOptions(7, 5, 112, true,
        [](std::size_t index, int top) { return top + (index == 6 ? 32 : 12); },
        [](std::size_t index, int top) { return top + (index == 6 ? 30 : 10); });
    CHECK(denseLayout.groupGap == 0);
    CHECK(denseLayout.options.back().top == 77);

    int skippedDrawCalls = 0;
    const DialogueLayout skippedLayout = layoutDialogueOptions(3, 5, 112, true,
        [](std::size_t index, int top) { return index == 1 ? 200 : top + 12; },
        [&](std::size_t, int top) { skippedDrawCalls++; return top + 10; });
    CHECK(skippedLayout.options[0].accepted);
    CHECK(!skippedLayout.options[1].accepted);
    CHECK(skippedLayout.options[2].accepted);
    CHECK(skippedLayout.options[2].top == 19);
    CHECK(skippedDrawCalls == 2);

    const DialogueLayout clippedWordLayout = layoutDialogueOptions(2, 5, 112, true,
        [](std::size_t index, int top) { return top + (index == 0 ? 12 : 53); },
        [](std::size_t index, int top) { return top + (index == 0 ? 10 : 94); });
    CHECK(clippedWordLayout.groupGap == 1);
    CHECK(clippedWordLayout.options[1].top == 18);

    const DialogueLayout oneLayout = layoutDialogueOptions(1, 5, 112, true,
        [](std::size_t, int top) { return top + 12; },
        [](std::size_t, int top) { return top + 10; });
    CHECK(oneLayout.options[0].accepted);
    CHECK(oneLayout.options[0].top == 5);
    CHECK(oneLayout.groupGap == 0);

    std::puts("dialogue_layout_tests: all passed");
    return 0;
}
