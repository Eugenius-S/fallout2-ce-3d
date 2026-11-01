# 144: 10-degree camera turns, edge scroll follows the turn

No screenshots: the background snapshot cannot press `[` or move the mouse
to an edge from a worktree without the owner's game files, so this was
checked by unit tests and by reading the code.

Verified by tests (`ce/tests/settings_3d/settings_3d_test.cc`):

- One press turns 10 degrees; held right stops at +30, held left at -30;
  three presses back return to 0.
- Unlimited: 36 presses give 360, which is a multiple of 360 and lands on the
  game's own camera.
- Edge scroll mapping (`cameraTurnScrollStep`), unturned, is unchanged. Turned,
  each of the 8 edges and corners at 10, 20, 30 and -30 degrees maps to the
  game scroll direction nearest to the turned screen direction.

What the map should do at +30 (the mouse at the edge, the map moves toward it):

| Edge | Game scroll sent |
| --- | --- |
| right | right and up |
| top | up and left |
| left | left and down |
| bottom | down and right |

At -30 the pairs mirror (right edge: right and down). At 10 and 20 degrees the
turn is under half a step of the game's 45-degree directions, so every edge
and corner sends the unturned direction; the corners move one step at 30.

For the owner to check live: turn with `]` to +30, push the mouse to each
edge, and see that the map slides toward that edge on screen; repeat at -30.
Sprites keep the game's side up to 30 degrees (a side changes at 30 degrees
plus a hysteresis), so they do not turn within the default limit.
