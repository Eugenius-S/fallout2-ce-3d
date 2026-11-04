# 144: 10-degree camera turns, edge scroll follows the turn

Snapshots of kladwtwn from the background test instance
(`tools/dev-snapshot.ps1 -Map kladwtwn -Still -TurnDegrees D`), turned 0, 10,
20 and 30 degrees: `kladwtwn-turn0.jpg`, `kladwtwn-turn10.jpg`,
`kladwtwn-turn20.jpg`, `kladwtwn-turn30.jpg`.

## How an edge picks its step

The game scrolls in eight steps of 32 px across and 24 px down, and its ground
projection is oblique, so the eight steps are not eight 45-degree screen
directions. For each step the real ground move is taken from the game view
(`gameViewGroundStep`: the ground under a step either side of the view center),
then projected through the turned camera (the unturned camera's yaw and pitch
from `staticGameViewCamera`, plus the turn). The step whose screen direction is
closest to the edge wins (`cameraTurnScrollStep` in `ce/src/scene/camera_turn.h`).
A zero turn returns the step untouched.

On kladwtwn the measured step is horizontal (-0.5, 0.25), vertical (0.5, 0.75)
in ground units, camera pitch -27.5 degrees. Chosen game step per edge (dx
right, dy down; the edge as seen on the turned screen):

| Edge | at +20 | at +30 |
| --- | --- | --- |
| top | (-1,-1) up-left | (-1,-1) up-left |
| bottom | (1,1) down-right | (1,1) down-right |
| left | (-1,0) | (-1,0) |
| right | (1,0) | (1,0) |
| top-left | (-1,-1) | (-1,0) |
| top-right | (0,-1) | (0,-1) |
| bottom-left | (0,1) | (0,1) |
| bottom-right | (1,1) | (1,0) |

At +20 the top edge is 38 degrees off vertical for the plain up step and 28 for
up-left, so up-left wins.

Verified by tests (`ce/tests/settings_3d/settings_3d_test.cc`): the step and
limit, the unlimited circle, the +20 top-edge case, and that at 0, 10, 20, 30
and -30 degrees every edge and corner picks the step with the smallest
projected angle.
