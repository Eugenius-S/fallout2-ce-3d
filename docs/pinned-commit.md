# CE source pin

Current renderer source: `b05d9498dfa3cf2dbeb7d665da16dc1d88254091`.
Repository: https://github.com/Eugenius-S/fallout2-ce-scene-renderer
Branch: `scene-renderer`.

The authoritative pin is the `ce` gitlink in this repository. Clone with
`--recurse-submodules`; after pulling, run `git submodule update --init --recursive`.
When advancing CE, publish its commit first, then update the gitlink and this note.

## Upstream base

- Commit: `fb8ba51e016b09597bc0b4277fbac26899e43342`.
- Date: 2026-10-17 08:45:38 +0200.
- Subject: Typesafe sound.h enum usage (#868).
- Source: https://github.com/fallout2-ce/fallout2-ce.git (partial, blob:none).
- Original clone date: 2026-10-17. Preserved as remote `upstream` in the existing checkout.

Runtime evidence under `runs/` retains the source identity and EXE hash recorded
at capture time, including runs made before these source changes were committed.
Publishing those changes does not constitute a new runtime test of a rebuilt EXE.
