# Task 112 visual evidence

The screen sheets pair current `origin/main` before captures with branch after captures at 16:9 and 4:3. Detail sheets show the options panel edge and scrollbar at both aspect ratios.

Capture identity:

- Before: repository HEAD `41dfc616`, 16:9 1920x1080 (logical 960x540, scale 2), 4:3 1920x1440 (logical 640x480, scale 3), EXE SHA-256 `97440aacf315cb6ec941ce43a3a98fe6d313de5af2471eb9ed7fa26205b0e78e`.
- After: task branch HEAD `27312a40`, same physical/logical resolutions and scale, EXE SHA-256 `f7920b3a95579eb3e3dc5cba622473402254dfd5b80a3db2b367f845e455ad7e`.
- The JSON sidecars beside each raw PNG record the resolution, logical resolution, scale, game submodule commit, and EXE SHA-256.

Direct RGBA comparison of all 16 original-view regression captures from `origin/main` and the branch found zero changed pixels. The repository's older curated references independently report the same two existing differences on both builds: `klagraz-e0` (183 pixels) and `klaratcv-e2` (6299 pixels).

Raw captures remain in the local `runs/112-options-3d-panel-finish/{main-before-*,branch-after-*}` folders. The four committed JPEG sheets are each below 1 MB.
