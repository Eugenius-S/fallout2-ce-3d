# Task 103 capture notes

The four crops show the same wall overlap at hero/view tile 19075, with the halo enabled. Captures came from `e46846e3` (EXE SHA-256 `86248764df0d1493c241df448f97be91321378bb34bcec9b8cd65661532ca03b`). Scale is 2 throughout. Matching halo-off captures were compared pixel for pixel; only pixels inside the projected egg changed.

| Crop | Resolution | Logical resolution | Zoom | Changed pixels | Changed-pixel bounds |
|---|---:|---:|---:|---:|---|
| 16x9-z1.024.jpg | 1920x1080 | 960x540 | 1.024 | 33,004 | (829,293)-(1093,479) |
| 16x9-z2.048.jpg | 1920x1080 | 960x540 | 2.048 | 132,461 | (697,145)-(1225,517) |
| 4x3-z1.024.jpg | 1440x1080 | 720x540 | 1.024 | 33,005 | (589,293)-(853,479) |
| 4x3-z2.048.jpg | 1440x1080 | 720x540 | 2.048 | 132,461 | (457,145)-(985,517) |

A second wall row, tile 19102, was tested with the camera centered on it. Its halo on/off pair changed 19,888 pixels at both aspect ratios at zoom 1.024. With the camera left at tile 19075, tile 19102 projected offscreen and changed 0 pixels, so that pair is not evidence for the marked wall. Neither spot establishes the exact red-marked point in `runs/inbox/2026-10-28-halo-roof-only.png`; the supplied image has no save/camera metadata, and image matching did not yield a reliable tile mapping. No code change or first-bad commit is claimed.
