"""Validate named captures and compare them with explicitly updated references."""

import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

from PIL import Image

from compare import compare_images

HERE = Path(__file__).resolve().parent


def load_spots(path, selected=None):
    config = json.loads(path.read_text(encoding="utf-8-sig"))
    screen = config["screen"]
    if (not isinstance(screen, dict) or type(screen.get("width")) is not int or type(screen.get("height")) is not int
            or type(screen.get("scale")) is not int or not 640 <= screen["width"] <= 7680
            or not 480 <= screen["height"] <= 4320 or not 1 <= screen["scale"] <= 4
            or screen["width"] % screen["scale"] or screen["height"] % screen["scale"]
            or screen["width"] // screen["scale"] < 640 or screen["height"] // screen["scale"] < 480):
        raise ValueError("invalid fixed screen size or scale")
    spots = config["spots"]
    names = set()
    if not spots:
        raise ValueError("spot list is empty")
    for spot in spots:
        name = spot["name"]
        if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", name) or name in names:
            raise ValueError(f"invalid or duplicate spot name: {name}")
        names.add(name)
        if not re.fullmatch(r"[a-zA-Z0-9_]+", spot["map"]):
            raise ValueError(f"invalid map: {name}")
        if type(spot["tile"]) is not int or not 0 <= spot["tile"] < 40000 or spot["elevation"] not in (0, 1, 2):
            raise ValueError(f"invalid tile/elevation: {name}")
    if selected is not None:
        spots = [spot for spot in spots if spot["name"] == selected]
        if not spots:
            raise ValueError(f"unknown spot: {selected}; choose from {', '.join(sorted(names))}")
    if not 0 <= config["channel_tolerance"] <= 255 or not 0 <= config["max_changed_fraction"] <= 1:
        raise ValueError("invalid pixel tolerance")
    return config, spots


def read_capture(directory, spot, screen):
    files = list(directory.glob("devshot-*.json"))
    if len(files) != 1:
        raise ValueError("expected exactly one capture JSON")
    metadata = json.loads(files[0].read_text(encoding="utf-8-sig"))
    expected = {"map": spot["map"].upper() + ".MAP", "elevation": spot["elevation"],
                "view_tile": spot["tile"], "still_snapshot": True, "renderer": "scene-gl",
                "view": "static-wall-plus-hero-preview", "preview_availability": "available"}
    for key, value in expected.items():
        if metadata.get(key) != value:
            raise ValueError(f"capture {key}: expected {value!r}, got {metadata.get(key)!r}")
    resolution = f"{screen['width']}x{screen['height']}"
    logical_resolution = f"{screen['width'] // screen['scale']}x{screen['height'] // screen['scale']}"
    if metadata.get("scale") != screen["scale"]:
        raise ValueError(f"capture scale: expected {screen['scale']!r}, got {metadata.get('scale')!r}")
    if metadata.get("windowed") is not True:
        raise ValueError("capture is not windowed")
    if metadata.get("logical_resolution") != logical_resolution:
        raise ValueError(f"capture logical_resolution: expected {logical_resolution!r}, got {metadata.get('logical_resolution')!r}")
    image = metadata["image"]
    if Path(image).name != image or not image.endswith(".png"):
        raise ValueError("invalid capture image name")
    path = directory / image
    if not path.is_file():
        raise ValueError("capture PNG is missing")
    with Image.open(path) as decoded:
        if f"{decoded.width}x{decoded.height}" != resolution:
            raise ValueError(f"capture PNG resolution: expected {resolution}, got {decoded.width}x{decoded.height}")
        if metadata.get("resolution") != f"{decoded.width}x{decoded.height}":
            raise ValueError("capture resolution does not match the PNG")
        decoded.verify()
    return metadata, path


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--spots", type=Path, default=HERE / "spots.json")
    parser.add_argument("--spot")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--run", type=Path)
    parser.add_argument("--reference", type=Path, default=HERE / "reference")
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args(argv)
    config, spots = load_spots(args.spots, args.spot)
    if args.list:
        print(json.dumps(spots))
        return 0
    if args.run is None:
        parser.error("--run is required unless --list is used")
    results, captures = [], []
    for spot in spots:
        row = {"spot": spot["name"], "status": "error"}
        try:
            metadata, actual = read_capture(args.run / spot["name"], spot, config["screen"])
            row["capture"] = metadata
            row["image_sha256"] = hashlib.sha256(actual.read_bytes()).hexdigest()
            captures.append((spot, metadata, actual))
            if not args.update:
                reference = args.reference / (spot["name"] + ".png")
                row.update(compare_images(reference, actual, actual.parent,
                                          config["channel_tolerance"], config["max_changed_fraction"]))
                if row["status"] == "fail":
                    shutil.copy2(reference, actual.parent / "reference.png")
            else:
                row["status"] = "updated"
        except (OSError, ValueError, KeyError) as error:
            row["error"] = str(error)
        results.append(row)

    success = all(row["status"] in ("pass", "updated") for row in results)
    # Validate every capture before replacing any reference.
    if args.update and success:
        args.reference.mkdir(parents=True, exist_ok=True)
        for spot, metadata, actual in captures:
            target = args.reference / (spot["name"] + ".png")
            if target.exists():
                shutil.copy2(target, actual.parent / "reference-before.png")
            shutil.copy2(actual, target)
            metadata = {**metadata, "capture_image": metadata["image"], "image": spot["name"] + ".png", "spot": spot}
            (args.reference / (spot["name"] + ".json")).write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    elif args.update:
        for row in results:
            if row["status"] == "updated":
                row["status"] = "not-updated"
    summary = {"success": success, "update": args.update, "channel_tolerance": config["channel_tolerance"],
               "max_changed_fraction": config["max_changed_fraction"], "spots": results}
    (args.run / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    lines = [f"{row['spot']}: {row['status']} " + row.get("error", f"{row.get('changed_pixels', 0)} changed pixels") for row in results]
    (args.run / "summary.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))
    return 0 if success else 1


if __name__ == "__main__":
    raise SystemExit(main())
