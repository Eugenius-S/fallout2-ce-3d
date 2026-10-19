# Task 013: roofs

- `before/`, `after/`: regression spots without and with roofs; the roofs sit
  where the game draws them, 96 game pixels above their squares.
- `inside/`: the hero put inside a building (`-HeroTile 22885`): the game hides
  that building's roof, the neighbours keep theirs.
- `behind-*`: the hero at the back of the store. The game hides the back
  room's own roof piece (its flags), and the hero shows through the roof in
  the soft see-through circle (the egg), as in 2D.
- `tour/`: KLATRAP and two caves; caves look as before.
