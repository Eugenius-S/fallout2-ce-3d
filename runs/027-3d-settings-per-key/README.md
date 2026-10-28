# Task 027 verification

The normal 16-spot regression completed with `success: true`, `update: false`; all 16 references passed with 0 changed pixels. Machine-readable results are in [normal-regression-summary.json](normal-regression-summary.json), with a short report in [normal-regression-summary.txt](normal-regression-summary.txt).

A separate ARTEMPLE e0 snapshot used an isolated empty scene cache. The game kept the vanilla framebuffer (`scene_view: off`) and emitted exactly one failure reason: `floor.bin is missing or unreadable`. The snapshot is [fallback-artemple.png](fallback-artemple.png), metadata is [fallback-artemple.json](fallback-artemple.json), and captured stdout is [fallback-stdout.log](fallback-stdout.log).

Both runs used the background test helper binary with SHA-256 `1953b19d47db2a4140f46fb41a8bcd83325168f0316158ba2e4db009b555f27a`. The snapshot metadata records `ce_commit: c9910840`, the checked-out HEAD stamp when the binary was built; the implementation was then committed as `b2b8184159c4026c04a7c3791afa46cbe0f878c4`. The binary hash identifies the tested executable.

The primary game's `<worktree>\game\fallout2.cfg` had SHA-256 `8D95CE8F0A11FDB975C2EAFE4836248DD4F9983074161BA4578EE463F2CCEF7F` before and after the regression. No config files or executables are included here.
