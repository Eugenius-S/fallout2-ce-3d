# FOV25 and sprite facing review: 2026-10-18

The reviewer restarted the sandbox on the new build and inspected its actual window and GL screenshots. Wall/hero/walk now use FOV 25 degrees, unchanged target/pitch/azimuth, and eye-target distance scaled by `tan(25 degrees)/tan(12.5 degrees)` to retain the approximate subject scale. The horizontal basis corrects the reflection introduced by mapping donor `(u,v,z)` to GL `(u,z,v)`.

Exact executable SHA-256: `04ed649a71dd3b03781959015b0bf60f065e36dce78c7a2c751fe4ce2bc1ee3d`. Verified against the sandbox executable and every retained snapshot. CE base remains `f6c1969`; changes are uncommitted.

Camera: eye `(4.87092,22.03378,34.35318)`, target `(-11.95610,1.00000,9.11265)`, near `0.1`, far `400`. Screen-right is `-cross(forward,world_up)`; screen-up retains its upward direction. Rendering, ray picking and hero card placement use the same basis. Original FRM pixels/UVs and CE rotations are preserved. Consequently the environment's screen orientation changes from the earlier mirrored preview. Floor-only overview retains its former camera/basis.

Independent incremental Release rebuild and CTest passed: **2/2**, with always-active checks. The direction regression failed on NE under the old basis before the fix. Production camera tests now verify independent literal screen coordinates for the center and all six CE neighbors, alongside picker/DPI/letterbox checks. Modal policy tests cover yielding to the vanilla/menu frame and pausing the movement observer.

| Live check | Result | Evidence |
| --- | --- | --- |
| New camera actually displayed | PASS | Initial `devshot-20261018-163420-000.json` and paired PNG; actual window inspected |
| Rightward CE step | PASS | `25075 -> 25074`, arrived, offset zero, sprite rotation **1/E**; `devshot-20261018-163444-000.json` |
| Opposite step | PASS | `25074 -> 24876`, arrived, offset zero, sprite rotation **4/W**; `devshot-20261018-163501-000.json` |
| Return to initial hex | PASS | `24876 -> 25075`, arrived, offset zero; `devshot-20261018-163526-000.json` |
| Visible yellow hex and original FRM facing | PASS | Actual window and GL PNGs inspected |
| Live diagonal click | UNVERIFIED | Later injected clicks did not produce additional game events; no diagonal live acceptance is claimed |
| Actual options/pause opening and closing | UNVERIFIED | Policy/source checks passed, but no controlled live modal cycle was completed |

The two latter limits do not invalidate the recorded right/left arrival checks or substitute for future interactive P1 acceptance. Six-direction orientation is covered by the native mathematical regression; only the listed live actions were independently observed.

Prepared test process was left open. No user saves or Steam installation files were modified. Full P1, NPC/object selection, interactive doors, camera follow and lighting remain outside this review.
