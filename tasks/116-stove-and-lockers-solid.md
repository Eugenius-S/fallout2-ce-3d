---
id: 116
title: The stove and the lockers stand as solids
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: [115]
reviewed: no
---

## Goal

The stoves stand as drums with their lid on top, and the lockers as boxes,
in the 3D view; neither stays a flat picture.

## Context

- The owner, 2026-10-28, live on main `ff5f4ed0`:
  - `runs/inbox/2026-10-28-stove-flat.png`: the stove is a flat card.
    100 removed its drum override because the projected lid wrapped round
    the cylinder and read as a torn disc. The answer is a drum whose top
    face takes the lid, not a card.
  - `runs/inbox/2026-10-28-locker-flat.png`: lockers are tall boxes in the
    art; the owner: easy to make 3D.
- By rule, not by name lists: what in the art or the prototype tells a
  drum with a lid from a box (shapes/fits in `tools/scene_export`).

## Acceptance

- before | after of the bathhouse stove, the other Klamath stoves and the
  lockers, game angle and flight, in `runs/116-stove-and-lockers-solid/`.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Holes in solids (115), other furniture kinds, the landmark objects.

## Log

### 2026-10-31 glm: done through task 119

The render side landed in task 119 (PR #167): the stoves stand as lid drums
and the lockers as closed boxes in flight, the game's own views keep their
pixels (16/16 regression spots at 0 changed pixels, ctest 18/18, exporter
unittests 74). This task's PR (#166) is marked superseded by #167; the fit
groundwork and its tests of 2026-10-30 ride there. status: done.

### 2026-10-30 glm, blocked on the render side, handed over mid task

Correction to the recon below: the tall standing lockers ARE in Klamath
(locker5 x5, locker6 x7 - kladwtwn 14498, klamall 15283/15483/15683,
klaratcv e1 27697, klatoxcv e0 23712, e2 19287/19289/20877/21471/21671);
my earlier "no instances" came from grepping for "lkr" and missing the name
"locker".

Built and tested (props.py, its tests, the rules in scene.py and
things.py): the lid-drum fit (_bulges: a two-bulge silhouette through a
waist; LID_NECK 0.6 rejects the junk piles; the cap a solid disc; the belly
arc with feet excused), the fit priority (a floor lid drum beats the box's
foot line, never a top fit - a round sign on a pole keeps its stem: that
collision was found by the cache diff, props[100] kladwtwn), the loose and
container gates, 4 unit tests, 74 pass. The bench over the target and guard
arts: sstove1/2/3 drum LID, barrels plain drums, njunk2 box, block None,
locker5/6 box, walllkr None.

Then stood the rules back down (commit 7615ae02): the exports are
content-identical to the pre-116 caches again. What broke, in order:

1. The scene row said z1=1.12 while the fit probe put the raise at 58 px:
   the raise's EMPTY cost stops in the gap under the lid, so the drum is the
   body only - a stump against its card. And seen=1 already hides the card,
   so the lit interior pair (stove3-lit-blocker.jpg: before left, after
   right) shows the stove VANISH from the fly view.
2. The kladwtwn stove hexes (ground anchors 49..73) sit outside the region
   the wall sidecar exports (its vertices span about -35..32): whatever the
   fit says, those solids never reach the engine. The cards render (live
   objects), so the game views never noticed.
3. The lockers are OBJ_TYPE_ITEM containers with 3-frame arts: live_solids
   keeps them live cards on purpose (they open). The closed-box fly solid
   needs an engine decision (live solids, or the owner accepts fly showing
   them closed - live_solids already exempts seen==1 rows).
4. Wall scenery lesson (fixed on the way, kept in the Log): bladsign's art
   fits a box, but letting it keep a card instead of its wall sheet moved it
   44 px on screen and its wall occluded a third of it
   (wall-sign-lesson.jpg). The stand bypass, if re-tried, is obj_type-gated.

Checks at the stand-down: ctest 18/18, exporter unittests 74, the caches
content-identical to the pre-116 ones (only fingerprints differ), the
regression run clean. The roof trick for any future interior fly shot:
-HeroTile on or next to the target makes the engine hide that roof
(roof-trick-hero-tile.jpg); -Solo strips the whole sidecar; -FlyPitch takes
positive degrees only.

Next: task 119 carries the render side; this task's fit groundwork and its
tests ride in this branch's PR (draft).

### 2026-10-29 glm, recon done mid task, work continues on this branch

Branch note: branched from task/115-no-holes-in-3d-things (115 not merged yet),
per the board's depends-on.

- What the things are (map scan of klamall, klatoxcv, kladwtwn):
  - Stoves, all loose clutter (flag word 0x20000000, with 0x20 on one): sstove2.frm
    kladwtwn 20937 and 26106, sstove1.frm kladwtwn 22528, sstove3.frm klamall 24138.
    The 100 loose rule (build_props PLAIN_OBJECT_FLAGS) skips them before the fit:
    that is why they are cards today.
  - Lockers: walllkr2.frm klamall 12482 and walllkr4.frm klatoxcv e1 17248,
    OBJ_TYPE_ITEM containers (flags 0x20001000), rendered as flat panels today.
    footlkr1 (kladwtwn, 4) and footlkr2 (klamall) are already boxes.
    locker1-6 (the tall standing lockers of the owner's shot) have no Klamath
    instances; the rule must still take them. cab1/cab2 are plant trays, not
    cabinets: their cards stay.
- Arts (frames dumped in runs/_116_tmp/, contact.png): a stove is a round-bellied
  body with an open lid hovering above it (a compact cap over a transparent gap).
  walllkr are narrow wall cabinets: content 14-24 px wide, 38-41 px tall at the
  top of an 80-86 canvas.
- Fit findings (props.py _from_floor): the stove's drum candidate dies because the
  drum outline test runs on the foot line and the stove stands on small feet
  (error above OUTLINE_PX), while the box path never checks its outline at all:
  sstove2 and sstove3 pass as BOXES today (cover 0.97, error 2.97 and 1.91), and
  the forced drum covers the whole sprite (z1 at the lid top, cover 1.0), which is
  the torn-disc look of the removed 100 override. barrel2: drum error 1.40 beats
  box 1.59. walllkr: the box is refused by MIN_SIDE (14 px wide is under 0.2 units).
- Plan (rules, not name lists):
  1. Loose clutter is fitted like everything else; its solid is the fly camera's
     (seen 1): the game camera keeps the card, which keeps 100's guarantee that
     the game's own views never diverge. A refusal still keeps the card (the junk
     piles must stay cards: njunk2/block fits must stay empty).
  2. Drum with a lid: the drum outline error is measured over the body's rows
     (feet allowed, like the box's CORNER_PX); when the raise crosses a
     transparent gap and lands on a compact cap, the fit is a lid drum: z1 at the
     cap top, the top face lands on the cap pixels by projection, and the fly
     sides sample below the gap so the body art does not smear up to the lid.
     The lid consumes its rest card. A round body beats a box: the min-cost pick
     already prefers the drum when its candidate exists (barrel2 today).
  3. A thin box is allowed down to about 0.12 units of its narrow side when the
     box hexagon tracks both outlines within OUTLINE_PX and the top face is
     filled: wall lockers stand as the thin boxes the art draws. Where the ITEM
     container path makes its panel today is still to trace.
- Checks planned: the fit unit tests, the shapes eval sweep (eval.py check/score)
  for collateral flips, ctest, regression (-Update expected: stoves and lockers
  gain solids in fly-only spots... game views keep cards, so most spots should
  hold), fly tour vs origin/main in fallout2-ce-3d-glm-main.
