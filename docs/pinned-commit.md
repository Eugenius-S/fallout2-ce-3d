# CE source

The engine lives in `ce/` of this repository with its full history.
Until 2026-10-18 it was a separate repository (`fallout2-ce-scene-renderer`,
branch `scene-renderer`, last commit `b05d9498dfa3cf2dbeb7d665da16dc1d88254091`)
attached as a submodule; both histories were merged in one commit.

Upstream changes are merged with:

```powershell
git fetch upstream
git merge -X subtree=ce upstream/main
```

## Upstream base

- Commit: `fb8ba51e016b09597bc0b4277fbac26899e43342`.
- Date: 2026-10-17 08:45:38 +0200.
- Subject: Typesafe sound.h enum usage (#868).
- Source: https://github.com/fallout2-ce/fallout2-ce.git (remote `upstream`).

Runtime evidence under `runs/` retains the source identity and EXE hash recorded
at capture time, including runs made before these source changes were committed.
