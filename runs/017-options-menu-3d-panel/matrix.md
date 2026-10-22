# Final preferences capture matrix

Captured with the official `tools/dev-snapshot.ps1` after the clean rebase.
Scale 2 uses a 1920x1080 physical drawable (960x540 logical); scale 3 uses
1920x1440 (640x480 logical). Every PNG dimension matches the sidecar's
`resolution`; all 16 sidecars match scale, logical resolution, and the current
Release executable SHA-256 `4c3933c595323a981b0008add8d94ef47b397c77ef709011ab16a49e349aae28`.

The sidecar does not store language. The language column is the official
launcher `-Language` value recorded in each run-folder name. `renderer` is
`scene-gl` in both modes because it identifies the UI compositor; the
sidecar's `preview_requested` and `scene_view` fields distinguish Classic
(`false`, `off`) from 3D (`true`, `full`).

| Mode | Scale | Language | Position | PNG | Metadata | `preview_requested` | `scene_view` | Renderer |
| --- | ---: | --- | --- | --- | --- | --- | --- | --- |
| Classic | 2 | English | Top | [scr00032.png](final-matrix-20261022/2d-x2-english-top/scr00032.png) | [JSON](final-matrix-20261022/2d-x2-english-top/devshot-20261022-183530.json) | false | off | scene-gl |
| Classic | 2 | English | Bottom | [scr00033.png](final-matrix-20261022/2d-x2-english-bottom/scr00033.png) | [JSON](final-matrix-20261022/2d-x2-english-bottom/devshot-20261022-183626.json) | false | off | scene-gl |
| Classic | 2 | Russian | Top | [scr00034.png](final-matrix-20261022/2d-x2-russian-top/scr00034.png) | [JSON](final-matrix-20261022/2d-x2-russian-top/devshot-20261022-183638.json) | false | off | scene-gl |
| Classic | 2 | Russian | Bottom | [scr00035.png](final-matrix-20261022/2d-x2-russian-bottom/scr00035.png) | [JSON](final-matrix-20261022/2d-x2-russian-bottom/devshot-20261022-183650.json) | false | off | scene-gl |
| Classic | 3 | English | Top | [scr00040.png](final-matrix-20261022/2d-x3-english-top/scr00040.png) | [JSON](final-matrix-20261022/2d-x3-english-top/devshot-20261022-183806.json) | false | off | scene-gl |
| Classic | 3 | English | Bottom | [scr00041.png](final-matrix-20261022/2d-x3-english-bottom/scr00041.png) | [JSON](final-matrix-20261022/2d-x3-english-bottom/devshot-20261022-183817.json) | false | off | scene-gl |
| Classic | 3 | Russian | Top | [scr00042.png](final-matrix-20261022/2d-x3-russian-top/scr00042.png) | [JSON](final-matrix-20261022/2d-x3-russian-top/devshot-20261022-183828.json) | false | off | scene-gl |
| Classic | 3 | Russian | Bottom | [scr00043.png](final-matrix-20261022/2d-x3-russian-bottom/scr00043.png) | [JSON](final-matrix-20261022/2d-x3-russian-bottom/devshot-20261022-183839.json) | false | off | scene-gl |
| 3D | 2 | English | Top | [scr00036.png](final-matrix-20261022/3d-x2-english-top/scr00036.png) | [JSON](final-matrix-20261022/3d-x2-english-top/devshot-20261022-183710.json) | true | full | scene-gl |
| 3D | 2 | English | Bottom | [scr00037.png](final-matrix-20261022/3d-x2-english-bottom/scr00037.png) | [JSON](final-matrix-20261022/3d-x2-english-bottom/devshot-20261022-183722.json) | true | full | scene-gl |
| 3D | 2 | Russian | Top | [scr00038.png](final-matrix-20261022/3d-x2-russian-top/scr00038.png) | [JSON](final-matrix-20261022/3d-x2-russian-top/devshot-20261022-183734.json) | true | full | scene-gl |
| 3D | 2 | Russian | Bottom | [scr00039.png](final-matrix-20261022/3d-x2-russian-bottom/scr00039.png) | [JSON](final-matrix-20261022/3d-x2-russian-bottom/devshot-20261022-183746.json) | true | full | scene-gl |
| 3D | 3 | English | Top | [scr00044.png](final-matrix-20261022/3d-x3-english-top/scr00044.png) | [JSON](final-matrix-20261022/3d-x3-english-top/devshot-20261022-191402.json) | true | full | scene-gl |
| 3D | 3 | English | Bottom | [scr00045.png](final-matrix-20261022/3d-x3-english-bottom/scr00045.png) | [JSON](final-matrix-20261022/3d-x3-english-bottom/devshot-20261022-191412.json) | true | full | scene-gl |
| 3D | 3 | Russian | Top | [scr00046.png](final-matrix-20261022/3d-x3-russian-top/scr00046.png) | [JSON](final-matrix-20261022/3d-x3-russian-top/devshot-20261022-191422.json) | true | full | scene-gl |
| 3D | 3 | Russian | Bottom | [scr00047.png](final-matrix-20261022/3d-x3-russian-bottom/scr00047.png) | [JSON](final-matrix-20261022/3d-x3-russian-bottom/devshot-20261022-191432.json) | true | full | scene-gl |
