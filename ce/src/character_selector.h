#ifndef CHARACTER_SELECTOR_H
#define CHARACTER_SELECTOR_H

namespace fallout {

// When `autoTake` is true the first premade character is taken without
// waiting for input (dev mode `--dev-new-game`).
int characterSelectorOpen(bool autoTake = false);

void premadeCharactersInit();
void premadeCharactersExit();

} // namespace fallout

#endif /* CHARACTER_SELECTOR_H */
