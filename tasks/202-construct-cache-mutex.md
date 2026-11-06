---
id: 202
title: Construct the cache mutex in cacheInit (a malloc'd cache crashed on first lock)
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

# 202: Construct the cache mutex in cacheInit

## Goal

Every cache locks `cache->mutex` (the 172 warm pool made the lock real). The
sound effects cache is allocated with `internal_malloc`, so its `std::mutex`
started as garbage bytes. The first `cacheLock` on a map load read the garbage
as "locked by this thread" and MSVC threw `resource deadlock would occur`,
which terminated the game: an interactive `--dev-new-game --dev-map=mbase34`
died right after `startup target accepted=1`. The same garbage killed other
runs as an access violation (0xC0000005) or heap corruption (0xC0000374)
inside `_Mtx_lock`. Static caches (`Cache gArtCache`) worked by luck because
zero bytes are a valid SRWLOCK, which is why headless snapshots looked healthy
while every interactive start was a coin flip.

## Context

- Found while preparing the owner demo of MBASE34 e1 tile 17131. The
  terminate was caught with a temporary diagnostic handler; the dump chain:
  `mapLoad -> _gsound_sfx_q_start -> soundPlayFile -> soundLoad ->
  soundEffectsCacheFileOpen -> cacheLock -> _Throw_Cpp_error(EDEADLK)`.
- The fix is a rule for every cache of its kind: `cacheInit` placement-news
  the mutex before the first lock and `cacheFree` destroys it after the last
  one, so the allocation pattern (static, malloc, whatever comes later) does
  not matter.

## Acceptance

- [x] An interactive `--dev-new-game --dev-map=mbase34 --dev-elevation=1` run
  with sound passes map load and stays alive (before the fix: terminate
  within seconds, every time).
- [x] Build and tests green on CI.

## Out of scope

- The 172 warm pool discipline itself.
- Two adjacent startup blockers met on the way (both fixed outside the repo
  in the demo copy): the sfall `WorldMapSlots=21` ddraw.ini check blocks
  startup with a modal, and a map stays 2D when its `floor.bin` export is
  missing.

## Log

### 2026-11-06 gpt
Done: cache.cc placement-new in cacheInit + destroy in cacheFree. Verified on
an isolated vanilla scratch copy: the pre-fix run died with the terminate
above; the fixed exe passed map load, showed MBASE34 e1 in the 3D view and
stayed alive. The owner demo (fullscreen, Russian) opened from the fixed exe.
Next: CI, merge, done.
Check: a dev-snapshot of mbase34 e1 against the scratch game root with its
own scene cache; the interactive flags above reach the map and keep running.
