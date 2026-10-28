#!/usr/bin/env python3
"""Export Fallout 2 CE scenes, one per map elevation, from read-only DAT archives."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "vendor"))

from f2lib import Dat2, GameFiles, MapFile  # noqa: E402
from f2lib.gamefiles import norm  # noqa: E402
from fp.scene import SceneBuilder  # noqa: E402
from baked_sidecar import VERSION as BAKED_SIDECAR_VERSION  # noqa: E402
from baked_sidecar import baked_objects, decode_baked_sidecar, encode_baked_sidecar  # noqa: E402
from floor_sidecar import VERSION as FLOOR_SIDECAR_VERSION  # noqa: E402
from floor_sidecar import encode_floor_sidecar  # noqa: E402
from projection_sidecar import VERSION as PROJECTION_SIDECAR_VERSION  # noqa: E402
from projection_sidecar import decode_projection_sidecar, encode_projection_sidecar  # noqa: E402
from wall_sidecar import VERSION as WALL_SIDECAR_VERSION  # noqa: E402
from wall_sidecar import decode_wall_sidecar, encode_wall_sidecar  # noqa: E402
from shadow_sidecar import VERSION as SHADOW_SIDECAR_VERSION  # noqa: E402
from shadow_sidecar import decode_shadow_sidecar, encode_shadow_sidecar  # noqa: E402

EXPORTER_VERSION = "1.11.0"
DONOR_REVISION = "0da1cd449f1dc7f2f6986ff7e8728a4b6ae20584"
DEFAULT_MAP = "kladwtwn"
# Map groups for --maps: every elevation each map has.
MAP_GROUPS = {
    "klamath": ("kladwtwn", "klatrap", "klacanyn", "klagraz", "klamall", "klaratcv", "klatoxcv"),
    # The starting vault: its screens (monitr, bvs, comp arts) carry live
    # cycling colours, unlike Klamath's dead ones (task 050).
    "vault15": ("vault15",),
    "den": ("denbus1", "denbus2", "denres1"),
}
EXPECTED_OUTPUT_FILES = {"scene.json", "provenance.json", "floor.bin", "walls.bin", "projection.bin", "baked.bin",
                         "shadows.bin", "tiles.png", "light.png", "rock.png"}


def choose_source(key, sources):
    """Return the first matching configured CE source, already high-to-low."""
    for kind, name, source in sources:
        if source.has(key):
            return (kind, name, source)
    return None


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def canonical_resource(path: str) -> str:
    key = norm(path)
    if not key or any(part in (".", "..") for part in key.split("/")):
        raise ValueError(f"unsafe game resource path: {path!r}")
    return key


def validate_output_contents(out_dir: Path) -> None:
    """Reject files SceneBuilder might otherwise delete or follow unexpectedly."""
    if not out_dir.exists():
        return
    for entry in out_dir.iterdir():
        if entry.is_symlink():
            raise ValueError(f"symlink is not allowed in export directory: {entry.name}")
        expected = entry.name in EXPECTED_OUTPUT_FILES or re.fullmatch(r"sprites\d+\.png", entry.name) is not None
        if not expected or not entry.is_file():
            raise ValueError(f"unexpected export directory entry: {entry.name}")


def patch_paths(game_root: Path, template: str) -> list[Path]:
    """Match game.cc's PatchFile loop, stopping at the first absent source."""
    placeholders = list(re.finditer(r"%0?\d*d", template))
    if len(placeholders) > 1 or "%" in re.sub(r"%0?\d*d", "", template):
        raise ValueError(f"unsupported ddraw.ini [Misc] PatchFile template: {template!r}")
    paths = []
    for index in range(1000):
        filename = template % index if placeholders else template
        path = Path(filename)
        if not path.is_absolute():
            path = game_root / path
        if not path.is_file():
            break
        paths.append(path)
        if not placeholders:
            break
    return paths


def parse_ini(path: Path) -> dict[str, dict[str, str]]:
    sections: dict[str, dict[str, str]] = {}
    section = ""
    for raw in path.read_text(encoding="cp1252").splitlines():
        line = raw.strip()
        if not line or line.startswith((";", "#")):
            continue
        if line.startswith("[") and "]" in line:
            section = line[1:line.index("]")].strip().lower()
            sections.setdefault(section, {})
            continue
        if "=" in line and section:
            key, value = line.split("=", 1)
            sections[section][key.strip().lower()] = value.strip().strip('"')
    return sections


class ResourceSource:
    """Read-only directory or DAT source with engine-style member lookup."""

    def __init__(self, name: str, path: Path):
        self.name = name
        self.path = path
        self.archive = Dat2(path) if path.is_file() else None
        self.is_directory = path.is_dir()
        if self.archive is None and not self.is_directory:
            raise FileNotFoundError(f"configured resource source is not a file or directory: {path}")
        self._cache: dict[str, Path | None] = {}

    def _resolve(self, key: str) -> Path | None:
        if key in self._cache:
            return self._cache[key]
        current = self.path
        for part in key.split("/"):
            try:
                entries = {entry.name.lower(): entry for entry in current.iterdir()}
            except (FileNotFoundError, NotADirectoryError):
                current = None
                break
            current = entries.get(part)
            if current is None:
                break
        result = current if current is not None and current.is_file() else None
        self._cache[key] = result
        return result

    def has(self, key: str) -> bool:
        return self.archive.has(key) if self.archive is not None else self._resolve(key) is not None

    def read(self, key: str) -> bytes:
        if self.archive is not None:
            return self.archive.read(key)
        # The game writes some loose files gzipped (critter protos while it is
        # played) and reads them back as plain ones.
        data = self._resolve(key).read_bytes()
        return gzip.decompress(data) if data[:2] == b"\x1f\x8b" else data

    def find(self, key: str) -> str:
        return f"dat://{self.name}/{key}" if self.archive is not None else os.fspath(self._resolve(key))

    def listdir(self, key: str) -> list[str]:
        if self.archive is not None:
            prefix = key.rstrip("/") + "/" if key else ""
            names = set()
            for name in self.archive.entries:
                if name.startswith(prefix):
                    tail = name[len(prefix):]
                    if tail:
                        names.add(tail.split("/", 1)[0])
            return list(names)
        directory = self.path if not key else self._resolve_directory(key)
        if directory is None:
            return []
        try:
            return [entry.name.lower() for entry in directory.iterdir()]
        except (FileNotFoundError, NotADirectoryError):
            return []

    def _resolve_directory(self, key: str) -> Path | None:
        current = self.path
        for part in key.split("/"):
            try:
                entries = {entry.name.lower(): entry for entry in current.iterdir()}
            except (FileNotFoundError, NotADirectoryError):
                return None
            current = entries.get(part)
            if current is None or not current.is_dir():
                return None
        return current


def configured_sources(game_root: Path):
    config_path = game_root / "fallout2.cfg"
    if not config_path.is_file():
        raise FileNotFoundError(f"CE resource order requires {config_path}")
    cfg = parse_ini(config_path)
    system = cfg.get("system", {})

    def configured_path(key: str, default: str) -> Path | None:
        value = system.get(key, default).strip()
        if not value:
            return None
        path = Path(value)
        return path if path.is_absolute() else game_root / path

    patch_template = "patch%03d.dat"
    ddraw_path = game_root / "ddraw.ini"
    if ddraw_path.is_file():
        patch_template = parse_ini(ddraw_path).get("misc", {}).get("patchfile", patch_template) or patch_template

    patch_chain = patch_paths(game_root, patch_template)
    master_dat = configured_path("master_dat", "master.dat")
    critter_dat = configured_path("critter_dat", "critter.dat")
    for path in (master_dat, critter_dat):
        if path is None or not path.exists():
            raise FileNotFoundError(f"required CE DAT source missing: {path}")

    # CE opens low-priority sources first; dbOpen prepends each source.
    # Return the final chain head-to-tail (highest priority first).
    paths: list[tuple[str, Path]] = []
    for key in ("master_patches", "critter_patches"):
        path = configured_path(key, "")
        if path is not None and path.exists():
            paths.append((key, path))

    # sfall_ext.cc opens the listed mods in file order and skips missing
    # entries, so the last listed mod has the highest priority.
    mods_dir = game_root / "mods"
    mods_order = mods_dir / "mods_order.txt"
    mod_paths: list[Path] = []
    if mods_order.is_file():
        for raw in mods_order.read_text(encoding="cp1252").splitlines():
            if ";" in raw or "#" in raw:
                continue
            entry = raw.strip()
            if entry and (mods_dir / entry).exists():
                mod_paths.append(mods_dir / entry)
    elif mods_dir.is_dir() and any(mods_dir.glob("*.dat")):
        raise ValueError("mods/*.dat exists but mods/mods_order.txt is missing; refusing to guess CE mod order")

    paths.extend((f"mods/{path.name}", path) for path in reversed(mod_paths))
    paths.extend((path.name, path) for path in reversed(patch_chain))
    for key, default in (("ce.dat", "ce.dat"), ("f2_res_dat", "f2_res.dat")):
        path = game_root / default if key == "ce.dat" else configured_path(key, default)
        if path is not None and path.exists():
            paths.append((path.name, path))
    paths.extend((("critter_dat", critter_dat), ("master_dat", master_dat)))
    # xfileOpen falls back to the process working directory after exhausting
    # the registered xbase chain. CE runs with the game root as cwd.
    paths.append(("cwd_fallback", game_root))

    sources = []
    for name, path in paths:
        source = ResourceSource(name, path)
        kind = "directory" if source.is_directory else "archive"
        sources.append((kind, name, source))
    config_hashes = {config_path.name: sha256_file(config_path)}
    if ddraw_path.is_file():
        config_hashes[ddraw_path.name] = sha256_file(ddraw_path)
    if mods_order.is_file():
        config_hashes["mods/mods_order.txt"] = sha256_file(mods_order)
    return sources, config_hashes


class ReadOnlyGameFiles(GameFiles):
    """GameFiles-compatible reader following the active CE source configuration."""

    def __init__(self, game_root: Path):
        self.game_root = game_root
        self.sources, self.config_hashes = configured_sources(game_root)
        self.archives = [(name, source.archive) for kind, name, source in self.sources if source.archive is not None]
        self._reads: dict[str, dict[str, str]] = {}
        # A concrete base is passed so GameFiles never calls resolve_game().
        super().__init__(base=os.fspath(game_root))

    def close(self):
        for _, archive in self.archives:
            archive.close()

    def _winner(self, path: str):
        key = canonical_resource(path)
        return key, choose_source(key, self.sources)

    def find(self, path):
        key, winner = self._winner(path)
        if winner is None:
            return None
        _, _, source = winner
        return source.find(key)

    def exists(self, path):
        return self._winner(path)[1] is not None

    def read(self, path):
        key, winner = self._winner(path)
        if winner is None:
            raise FileNotFoundError(f"not in selected game data: {path}")
        kind, name, source = winner
        data = source.read(key)
        record = {"source": f"{kind}:{name}", "sha256": hashlib.sha256(data).hexdigest()}
        prior = self._reads.setdefault(key, record)
        if prior != record:
            raise RuntimeError(f"resource source changed during export: {key}")
        return data

    def listdir(self, path=""):
        key = canonical_resource(path) if path else ""
        names = set()
        for _, _, source in self.sources:
            names.update(source.listdir(key))
        return sorted(names)


def live_solids(builder, scene):
    """Solids whose object animates (a locker, a fridge opening) stay live
    cards in the engine: (their boards, their props rows, rests near them)."""
    objects = builder.map.objects[builder.elevation]
    boards = scene.get("boards") or []
    live = set()
    for board in scene.get("hidden") or []:
        frm = builder.load_art(objects[boards[board][5]])
        if frm is not None and frm.frame_count > 1:
            live.add(board)
    props = scene.get("props") or []
    skip_props = {index for index, row in enumerate(props) if row[3] in live}
    centres = [(props[index][5], props[index][6]) for index in skip_props]
    skip_rests = {index for index, row in enumerate(scene.get("rests") or [])
                  if any(abs(row[1] - cu) < 0.75 and abs(row[2] - cv) < 0.75 for cu, cv in centres)}
    return live, skip_props, skip_rests


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    roots = parser.add_mutually_exclusive_group()
    roots.add_argument("--project-root", type=Path, default=Path(__file__).resolve().parents[2],
                       help="project root containing game/ (default: this repository)")
    roots.add_argument("--game-root", type=Path, help="game directory containing DAT files and data/")
    targets = parser.add_mutually_exclusive_group()
    targets.add_argument("--map", default=DEFAULT_MAP, help=f"one map, without .map (default: {DEFAULT_MAP})")
    targets.add_argument("--maps", help="every elevation of several maps: a group (" + ", ".join(MAP_GROUPS)
                         + ") or a comma-separated list")
    parser.add_argument("--elevation", type=int, choices=(0, 1, 2), default=0,
                        help="the elevation of --map (default: 0)")
    parser.add_argument("--cache-root", type=Path,
                        help="where the scenes go instead of <game>/mod-cache/scene (a worktree's own cache, "
                             "read by the test game through F2CE_SCENE_CACHE, so a running game keeps its own)")
    parser.add_argument("--check", action="store_true",
                        help="report whether every requested map's base scene exists and exit: 0 all present, "
                             "3 some missing (the 091 launcher asks before spending minutes on an export)")
    args = parser.parse_args(argv)
    names = MAP_GROUPS.get(args.maps.lower(), args.maps.split(",")) if args.maps else [args.map]
    args.map_names = [name.strip().lower() for name in names]
    for name in args.map_names:
        if not re.fullmatch(r"[a-z0-9_]+", name):
            parser.error(f"invalid map name: {name!r}")
    return args


def check_scenes(game_root: Path, map_names: list[str], cache_root: Path = None) -> list[str]:
    """The requested maps whose elevation 0 scene is not in the cache yet."""
    scenes = cache_root if cache_root is not None else game_root / "mod-cache" / "scene"
    return [name for name in map_names if not (scenes / name / "0" / "scene.json").is_file()]


def scene_directory(game_root: Path, map_name: str, elevation: int, cache_root: Path = None) -> Path:
    """The cache directory of one map elevation, checked before anything is written."""
    scenes = cache_root if cache_root is not None else game_root / "mod-cache" / "scene"
    out_dir = scenes / map_name / str(elevation)
    if out_dir.resolve() != (scenes / map_name / str(elevation)).resolve():
        raise SystemExit("output path resolution escaped the fixed target")
    parents = (scenes, scenes / map_name, out_dir) if cache_root is not None else (game_root / "mod-cache", scenes,
                                                                                     scenes / map_name, out_dir)
    for parent in parents:
        if parent.exists() and parent.is_symlink():
            raise SystemExit(f"refusing symlink in output path: {parent}")
    try:
        validate_output_contents(out_dir)
    except ValueError as error:
        raise SystemExit(str(error)) from error
    return out_dir


def export_scene(gf, game_root: Path, map_name: str, elevation: int, out_dir: Path):
    """One map elevation into its cache directory (scene_directory)."""
    out_dir.mkdir(parents=True, exist_ok=True)
    gf._reads = {}  # provenance lists what this scene read
    builder = SceneBuilder(gf, map_name, elevation)
    builder.export(out_dir)
    scene_path = out_dir / "scene.json"
    scene = json.loads(scene_path.read_text(encoding="utf-8"))
    if scene.get("map") != map_name or scene.get("elevation") != elevation:
        raise RuntimeError("SceneBuilder returned an unexpected map/elevation")
    live_boards, skip_props, skip_rests = live_solids(builder, scene)
    wall_payload = encode_wall_sidecar(scene, skip_props, skip_rests, builder.map.objects[elevation],
                                       builder.atlas.pages, builder.tile_sheet[..., 3])
    (out_dir / "walls.bin").write_bytes(wall_payload)
    wall_data = decode_wall_sidecar(wall_payload)
    baked_payload = encode_baked_sidecar(baked_objects(scene, builder.map.objects[elevation], live_boards))
    (out_dir / "baked.bin").write_bytes(baked_payload)
    baked_count = len(decode_baked_sidecar(baked_payload))
    projection_payload = encode_projection_sidecar(scene)
    (out_dir / "projection.bin").write_bytes(projection_payload)
    decode_projection_sidecar(projection_payload)
    shadow_payload = encode_shadow_sidecar(scene, builder.atlas.images, builder.map.objects[elevation])
    (out_dir / "shadows.bin").write_bytes(shadow_payload)
    shadow_count = len(decode_shadow_sidecar(shadow_payload))

    vendor_files = sorted((HERE / "vendor").rglob("*.py"))
    vendor_hashes = {p.relative_to(HERE / "vendor").as_posix(): sha256_file(p) for p in vendor_files}
    adapter_hashes = {name: sha256_file(HERE / name) for name in
                      ("export.py", "floor_sidecar.py", "wall_sidecar.py", "projection_sidecar.py", "baked_sidecar.py",
                       "shadow_sidecar.py", "scene_check.py", "solids.py")}
    archives = [{"name": name, "path": source.path.relative_to(game_root).as_posix()
                 if source.path.is_relative_to(game_root) else str(source.path),
                 "size": source.path.stat().st_size, "sha256": sha256_file(source.path)}
                for _, name, source in gf.sources if source.archive is not None]
    source_priority = [
        {"kind": kind, "name": name,
         "path": source.path.relative_to(game_root).as_posix()
         if source.path.is_relative_to(game_root) else str(source.path)}
        for kind, name, source in gf.sources
    ]
    fingerprint_data = {
        "exporter_version": EXPORTER_VERSION,
        "donor_revision": DONOR_REVISION,
        "vendor_sha256": vendor_hashes,
        "adapter_sha256": adapter_hashes,
        "archives": archives,
        "config_sha256": gf.config_hashes,
        "source_priority": source_priority,
        "resources": gf._reads,
        "map": map_name,
        "elevation": elevation,
    }
    fingerprint = hashlib.sha256(json.dumps(fingerprint_data, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    scene["_modCache"] = {
        "exporterVersion": EXPORTER_VERSION,
        "inputFingerprintSha256": fingerprint,
    }
    scene_path.write_text(json.dumps(scene, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    (out_dir / "floor.bin").write_bytes(encode_floor_sidecar(scene))
    artifact_hashes = {p.name: sha256_file(p) for p in sorted(out_dir.iterdir()) if p.name != "provenance.json"}
    provenance = {
        "exporter": "fallout2-sprite-lighting scene_export",
        "exporter_version": EXPORTER_VERSION,
        "donor_revision": DONOR_REVISION,
        "adapter_sha256": adapter_hashes,
        "map": map_name,
        "elevation": elevation,
        "resource_priority": source_priority,
        "configuration_sha256": gf.config_hashes,
        "fingerprint_sha256": fingerprint,
        "floor_sidecar": {"file": "floor.bin", "format_version": FLOOR_SIDECAR_VERSION,
                          "sha256": artifact_hashes["floor.bin"]},
        "wall_sidecar": {"file": "walls.bin", "format_version": WALL_SIDECAR_VERSION,
                         "sha256": artifact_hashes["walls.bin"],
                         "triangles": wall_data["triangles"],
                         "page_batches": [{"page": batch["page"], "size": batch["size"],
                                           "triangles": batch["triangles"]}
                                          for batch in wall_data["batches"]]},
        "baked_sidecar": {"file": "baked.bin", "format_version": BAKED_SIDECAR_VERSION,
                          "sha256": artifact_hashes["baked.bin"], "objects": baked_count},
        "projection_sidecar": {"file": "projection.bin", "format_version": PROJECTION_SIDECAR_VERSION,
                               "sha256": artifact_hashes["projection.bin"], "float64_values": 10},
        "shadow_sidecar": {"file": "shadows.bin", "format_version": SHADOW_SIDECAR_VERSION,
                           "sha256": artifact_hashes["shadows.bin"], "sprites": shadow_count},
        "archives": archives,
        "resources_read": gf._reads,
        "artifacts_sha256": artifact_hashes,
    }
    (out_dir / "provenance.json").write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(out_dir), "files": len(artifact_hashes) + 1,
                      "fingerprint_sha256": fingerprint, "resources_read": len(gf._reads)}, sort_keys=True))


def main(argv=None):
    args = parse_args(argv)
    game_root = (args.game_root or args.project_root / "game").resolve()
    if not game_root.is_dir():
        raise SystemExit(f"game root not found: {game_root}")
    cache_root = args.cache_root.resolve() if args.cache_root else None
    if args.check:
        missing = check_scenes(game_root, args.map_names, cache_root)
        if missing:
            print("scenes missing: " + ", ".join(missing))
            raise SystemExit(3)
        print("scenes present: " + ", ".join(args.map_names))
        return 0
    if not args.maps:
        # One scene: its directory is checked before the game data is opened.
        targets = [(args.map_names[0], args.elevation, scene_directory(game_root, args.map_names[0], args.elevation,
                                                                            cache_root))]
    gf = ReadOnlyGameFiles(game_root)
    failed = []
    try:
        if args.maps:
            targets = []
            for name in args.map_names:
                try:
                    elevations = sorted(MapFile.load(name, gf).tiles)
                except (OSError, ValueError, SystemExit) as error:
                    failed.append(f"{name}: {error}")
                    continue
                targets.extend((name, elevation, scene_directory(game_root, name, elevation, cache_root))
                               for elevation in elevations)
        for name, elevation, out_dir in targets:
            if not args.maps:
                export_scene(gf, game_root, name, elevation, out_dir)
                continue
            try:
                export_scene(gf, game_root, name, elevation, out_dir)
            except (Exception, SystemExit) as error:  # one bad map must not stop the others
                failed.append(f"{name} e{elevation}: {error}")
    finally:
        gf.close()
    for line in failed:
        print(f"FAILED {line}", file=sys.stderr)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
