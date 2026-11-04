# 151: the message log lines all start at the same left edge

## Where it came from

Vanilla draws the log with font 101's own glyph for the leading bullet
(cp1251 0x95): no side bearing, ink on the pen, the first letter after it at
the bullet's advance. The sharp interface text pass re-renders the same runs
with the mono interface font, whose U+2022 has a wide advance and a side
bearing: the bullet's ink started about one and a half logical pixels right
of the pen and the first letter drifted further, so every bulleted message
sat visibly right of the wrapped continuation lines (the owner's find:
`runs/inbox/2026-11-02-message-log-indent.png`).

## The fix

`renderSharpText` (ce/src/scene_renderer/text_draw.cc): for a message-log run
that leads with the bullet, the bullet's ink is drawn on the pen (its bearing
removed) and the rest of the line re-flows from the first letter's vanilla
position (`run.characterX[1]`). A rule for every message in both languages;
lines without the bullet are untouched. The vanilla one-pixel-per-line stair
(the inclined monitor) stays.

## Evidence

`log-ru-before.jpg` | `log-ru-after.jpg` and `log-en-before.jpg` |
`log-en-after.jpg`: the interface bar log with the seeded messages
(`-UiFixture log -SharpText`, the fixture now seeds the config's language),
before and after. The measured leftmost green ink per line, physical pixels
at scale 2:

- ru before: 209, 211, 213, 217, 219
- ru after: 206, 208, 210, 214, 216
- en before: 209, 211, 213, 215, 217, 219
- en after: 206, 208, 211, 212, 215, 216

Every bullet moved three physical pixels (one and a half logical) left onto
the pen; the remaining +2 per line is the vanilla stair. With sharp text off
the log is vanilla and was already aligned.
