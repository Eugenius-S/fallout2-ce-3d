---
id: 193
title: Interface text starts flush left and gets room between lines
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every text block drawn by the sharp interface text pass starts each line at
the same left edge, with no indent on the first line, and its lines stand
at about 130% of the font size apart (at least 120%).

## Context

- Folded in (the owner, 2026-11-04,
  `runs/inbox/2026-11-04-message-log-left-edge.png`): the same goes for the
  message log window on the interface bar. Each line's bullet and text start
  further right line by line, a slanted left edge against the owner's red
  guide, and the lines are packed tight. Align the left edge and give the
  lines a little more room, the same as the dialogue. 151 (done) fixed the
  log's left edges once; find what brought the slant back.
- Folded in (2026-11-04 inbox sweep), the inbox line of 2026-10-25: the
  dialogue answer options keep a hanging indent. The bullet marker stands
  left of the text column and every wrapped line aligns with the first
  letter after the bullet, so the text has a straight left edge. The replies
  have no indent at all (flush left, as above).
- Batch: the UI text. 140 (letters cut in the dialogue answers, gpt's draft
  #206) is the same text layout code; whoever takes 193 starts from 140's
  merged state.
- The owner, 2026-11-04 (a screenshot in chat): a dialogue reply, green mono
  text on the dark grid panel, "The spirits with me, fren! They be all
  around. Sometimes talk." The first line starts indented (a leading space),
  the wrapped second line flush left; the lines sit too tight.
- The renderer is `renderSharpText` in `ce/src/scene_renderer/text_draw.cc`
  (task 151 fixed the message log's leading bullet there). Find where the
  first line's indent comes from (a leading space in the string, or the pen
  start) and drop it by a rule for every run, not for this one string.
- The line height applies to every block of this renderer: dialogue replies,
  the message log, floating text over critters. Keep each block inside its
  panel: a block that no longer fits scrolls or wraps the way it did before.
- Part of the UI batch (tier 3).

## Acceptance

- The message log: every line's bullet starts at the same x, wrapped lines
  line up with the text after the bullet, and the line spacing matches the
  dialogue's; before | after of the log at x1 and x2, at 16:9 and 21:9.
- A wrapped dialogue answer option: the bullet left of the text, every line
  of the text on one left edge.

- Before | after devshots of the same dialogue reply panel, the message log
  and a floating text in `runs/193-text-indent-and-line-height/`; the first
  and the wrapped lines start at the same x, measured in the Log.
- With sharp text off the game's own view stays pixel for pixel; ctest
  passes.

## Out of scope

The font itself, colours.

## Log
