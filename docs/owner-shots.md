# Owner shots

The owner plays, presses F9, and the game saves the frame as it is on screen
with a JSON next to it: `shots/dev/<date>/YYYYMMDD-HHMMSS-<map>-e<elevation>.png`
and `.json`. A batch of these shots with the owner's notes becomes tasks.

## How an agent reads a batch

1. **Match a note to a shot by time and map.** The note names a place and a
   moment ("Klamath downtown, around 14:30"); the file name carries the map
   and the wall-clock stamp, the JSON repeats them in `map_file` and
   `build.taken`. A note without a time matches the shot whose map is the
   only one of that kind in the batch.
2. **Open the JSON first, the picture only when needed.** The JSON holds
   what the picture cannot tell: the player's tile and hex, the camera (eye,
   yaw, turn, zoom, center tile), the frame rate over the last seconds, the
   cursor's object (prototype id, name, tile), every render setting, the
   build commit. Most tasks close on the JSON alone; open the PNG when the
   note is about how something looks.
3. **One task per cause.** Several notes on one shot share a cause (one
   dark corner, one broken wall) - one task. One note spanning several
   shots with different maps is several tasks, one per map, each carrying
   its shot's JSON fields (tile, camera, commit) so an agent can reproduce.

## The JSON fields

| field | what it holds |
| --- | --- |
| `image` | the PNG file name next to this JSON |
| `fps` | the frame rate, the mean frame time and the 1% low over the last ~300 frames |
| `map`, `map_file`, `city` | the map as the game and maps.txt name it, and the city or area |
| `elevation` | the elevation the shot was taken on |
| `player` | the hero's tile, hex (`[tile % 200, tile / 200]`), world position, elevation, rotation |
| `camera` | the 3D camera: eye, yaw, fov, the `[` `]` turn angle, the zoom, the center tile |
| `cursor` | the object under the mouse cursor (prototype id, name, fid, tile), or null |
| `resolution` ... `windowed` | the window's size, the logical size, the scale, the window mode |
| `render` | every [3d] render setting plus the renderer in use |
| `build` | the build commit, the game version, the wall-clock stamp |

String fields that come from the game's data (`map`, `city`, `cursor.name`)
keep the game's own encoding: cp1251 on a Russian install. Read them as
game data, not as JSON-escaped Unicode.

The shot never stalls the frame noticeably: the game reads the frame back
and builds the JSON on the frame that took it (a few milliseconds, logged as
`perf owner_shot capture frame_ms=...` into devperf.log when F2CE_PERF is
on), and the PNG encode plus the two file writes run on a worker thread.

## Regions (the mark)

Ctrl+F9 or Alt+F9 enters the mark: the game holds on the frozen frame, the
owner drags one or more rectangles over the defect, Enter or a second press
saves, Esc cancels. The files: the clean PNG, the marked PNG
(`...-marked.png`, the red outlines on the clean capture) and the JSON with
a `regions` list - each region's pixel rect, the hexes whose centers fall
inside it (`tiles`), the square tiles covered (`squares`) and the objects
under those hexes (`objects`: prototype id, name, object type, tile). When
the batch has marked shots, start from `regions` and the objects listed in
them: they name the defect's place and pieces without opening the picture;
the clean PNG and the marked PNG sit next to the JSON.
