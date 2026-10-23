---
id: 039
title: All of Klamath looks decent from turned views
milestone: M1
status: doing
owner: glm
suggested: claude
claude-only: no
user-check: yes
depends-on: [037, 038]
reviewed: no
---

## Goal

Every static thing on the seven Klamath maps is looked at once from turned
views; the frequent failures get exceptions; 70% or more pass, weighted by
how often they stand.

## Context

- The design: [docs/2026-10-20-shapes-and-rotation-design.md](../docs/2026-10-20-shapes-and-rotation-design.md). Read it first; its "How the agents work on
  this" holds for this task.
- Exceptions go to `tools/scene_export/shapes/overrides.json`, each with a
  reason, at most 10% of Klamath's distinct static FRMs.
- The owner's report (2026-10-20, `runs/039-shapes-klamath/owner-vic-shack.png`,
  ours `vic-shack-turn-270.jpg`: kladwtwn, `-HeroTile 17665 -ViewTile 17467
  -Turn 3`): at Vic's shack the roof does not meet the front, the corner left
  of the sign is a hole into the dark inside, and the VIC'S sign hangs in the
  air. Buildings matter more than furniture here: start with them.

## Acceptance

- The weighted Klamath score 70% or more, written in the `Log` with the
  list of failures left.
- Exceptions within the cap.
- Regression 16/16, or changed references with before | after.

## Out of scope

Maps outside Klamath.

## Log

### 2026-10-23 glm
Done: the whole-Klamath pass is shot and judged for the first time.
tools/shapes/eval/klamath.py (plan, shoot, check, sheets, score): cards and
flats pass by design and are not shot (63% of the weight); every other FRM
(195 of 465) was shot alone from 0, 90, 180 and 270 degrees in one game run
per map elevation (a tour), plus solo-radius-0 empties for the pixel check.
Verdicts from my eye: tools/shapes/eval/klamath-verdicts.json, 50 FRMs.
Weighted score today: 69% pass, 0.7% fail, 30.1% unjudged (55 FRMs shot but
not yet eyed). Failures left: njunk6, njunk5, njunk2 (junk piles: a stray
shard, fans apart, or vanishes at 270), bucket1 (the pot sinks under the
floor), brl1000 (the crushed barrel fans into a bent sheet), locker6,
tirs000, toilet5, tub2, njunk7, necpew3 (lose the thing from a side),
buckas1, girlp005, bilchsig, anmste14, bolcas2 (panels and one box vanish
from behind).
Findings for the follow-ups:
- Panels (wall signs, posters) and some boxes vanish from the back in turned
  views: girlp005 at 180 shows nothing at all (runs/039-shapes-klamath/,
  sheet copies there). No face culling is left on for object cards
  (scene_draw.cc disables it after the wall batches), so the cause is
  elsewhere; needs Claude's renderer look (task 055).
- The owner's Vic's shack report reproduces (runs/039-shapes-klamath/
  vic-shack-turn-270*.png): the roof does not meet the front wall, the
  corner left of the VIC'S sign is open into the interior, and the sign
  hangs beside its wall. That is the 034 wall assembly from a turned view,
  not a prop shape (task 056).
- Maps whose entrance starts a fight (klacanyn, klatoxcv) stall the
  dev-snapshot tour: combat=1 blocks the tour's per-frame block forever.
  klacanyn was shot one view per run (klamath.py shoot --single);
  klatoxcv's 26 FRMs (0.5% of the weight) stay unshot this pass.
Next: eye the 55 unjudged sheets (the score then lands at 70% or the
failures list grows), then the 055 panel fix lifts the panel class at once.
Check: py -3 tools/shapes/eval/klamath.py score; sheets and flags in
build/shapes-klamath/ (shots for 169 of 195 solid FRMs).

### 2026-10-23 glm (session 2)
Done: the remaining 55 unjudged FRMs are eyed and judged (105 verdicts now).
The 70% gate is met: weighted score 98% pass, 0.8% fail, 0.9% unjudged
(`py -3 tools/shapes/eval/klamath.py score`). Of the 55, 45 read whole from
all four sides (barrels, crates, beds, tables, tires, newspaper machines,
scrub and bushes); 10 fail with one shared cause: the plane-locked panel
vanishes from behind (graf07, graf08, graf11, bath01, bokcas3, girlp017,
klasign1, klasign3, ripsign2, wareh04), exactly the class task 055 covers.
klamath.py now separates "not measurable" (shot, but the solo cannot
separate the thing, counts as pass like flats) from "not shot" (counts as
unjudged): the klatoxcv 26 unshot FRMs were silently passing before, they
are the honest 0.9% unjudged now. The unjudged-55 sheet pages are kept in
build/shapes-klamath/unjudged1..7.png.
Next: the 055 panel fix should flip the 10 new panel failures and the older
buckas1/girlp005/bilchsig/anmste14/bolcas2 at once; after that the score's
only gap is klatoxcv (shoot --single per view, 0.9% of the weight).
Check: `py -3 tools/shapes/eval/klamath.py score`; verdicts in
tools/shapes/eval/klamath-verdicts.json, flags in build/shapes-klamath/flags.json.
