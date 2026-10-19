"""Side-by-side 2D | 3D pictures from two snapshot tours of the same views
(tools/dev-snapshot.ps1 -Tour, with and without -Classic), at half size.

py -3 tools/gallery_pairs.py <2d dir> <3d dir> <output dir>
"""

import json
import sys
from pathlib import Path

from PIL import Image


def views(directory):
    """(map, elevation, view hex) -> image path, from the snapshots' JSON."""
    found = {}
    for sidecar in sorted(Path(directory).glob("devshot-*.json")):
        meta = json.loads(sidecar.read_text(encoding="utf-8-sig"))
        key = (meta["map"].split(".")[0].lower(), meta["elevation"], meta["view_tile"])
        found[key] = sidecar.parent / meta["image"]
    return found


def main(argv):
    flat, scene, out = (Path(arg) for arg in argv[1:4])
    out.mkdir(parents=True, exist_ok=True)
    flat_views, scene_views = views(flat), views(scene)
    for key in sorted(flat_views.keys() & scene_views.keys()):
        with Image.open(flat_views[key]) as left, Image.open(scene_views[key]) as right:
            size = (left.width // 2, left.height // 2)
            pair = Image.new("RGB", (size[0] * 2, size[1]))
            pair.paste(left.convert("RGB").resize(size, Image.LANCZOS), (0, 0))
            pair.paste(right.convert("RGB").resize(size, Image.LANCZOS), (size[0], 0))
        name = f"{key[0]}-e{key[1]}-{key[2]}.jpg"
        pair.save(out / name, quality=85)
        print(name)
    missing = sorted((flat_views.keys() ^ scene_views.keys()))
    for key in missing:
        print("only in one tour:", key, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
