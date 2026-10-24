---
id: 048
title: Shapes eval gets things from a second town, not only Klamath
milestone: M1
status: doing
owner: glm
suggested: gpt
claude-only: no
user-check: yes
depends-on: [041]
reviewed: no
---

## Goal

The shapes algorithm is checked outside Klamath, so its 88% is not a fit to
one town.

## Context

- The eval: `tools/shapes/eval/` (`labels.json`, `make_labels.py`,
  `eval.py`), the rules in `docs/2026-10-20-shapes-and-rotation-design.md`
  ("The eval set").
- Take the Den: export its town maps as a comma list (`export.py --maps
  denbus1,denbus2,denres1`; check the names in `master.dat`), add a `den`
  group next to `klamath` in `export.py`, pick the 15 most frequent static FRMs not already in the labels, plus any
  big ones (a building part, a car, a large machine).
- Label them by the same rules as the Klamath labels (plants and low rocks are
  cards, shelves and crates boxes); mark each label with its town.
- The owner may change any label later; that changes nothing in this task.

## Acceptance

- `labels.json` has 15 or more Den labels; `eval.py shoot` and `score` run
  on them.
- The `Log` holds the Klamath score and the Den score apart, and the failing
  Den FRMs.
- No change to the algorithm in this task: the failures become a new task.

## Log
