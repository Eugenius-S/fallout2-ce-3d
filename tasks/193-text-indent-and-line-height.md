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

- Before | after devshots of the same dialogue reply panel, the message log
  and a floating text in `runs/193-text-indent-and-line-height/`; the first
  and the wrapped lines start at the same x, measured in the Log.
- With sharp text off the game's own view stays pixel for pixel; ctest
  passes.

## Out of scope

The font itself, colours.

## Log
