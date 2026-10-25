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

    std::puts("dialogue_layout_tests: all passed");
    return 0;
}
