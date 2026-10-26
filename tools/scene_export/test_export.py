"""Focused source-priority regression for the read-only DAT adapter."""

import unittest
import base64
import gzip
import json
import struct
from pathlib import Path
from tempfile import TemporaryDirectory

from baked_sidecar import (ENTRY as BAKED_ENTRY, HEADER as BAKED_HEADER, LIFT as BAKED_LIFT, MAGIC as BAKED_MAGIC,
                           NOCAST as BAKED_NOCAST, SKIP as BAKED_SKIP, FLAT as BAKED_FLAT, FLY_FLAT as BAKED_FLY_FLAT,
                           FLY_SKIP as BAKED_FLY_SKIP, PLANE as BAKED_PLANE,
                           baked_objects, decode_baked_sidecar, encode_baked_sidecar)
from export import ReadOnlyGameFiles, choose_source, main, parse_args, patch_paths, scene_directory, validate_output_contents
from floor_sidecar import HEADER, MAGIC, VERSION, cell_content_rect, decode_floor_sidecar, encode_floor_sidecar
from projection_sidecar import (HEADER as PROJECTION_HEADER, MAGIC as PROJECTION_MAGIC,
                                VERSION as PROJECTION_VERSION, decode_projection_sidecar,
                                encode_projection_sidecar, hero_anchor, standing_card)
from shadow_sidecar import (MAGIC as SHADOW_MAGIC, VERSION as SHADOW_VERSION,
                            decode_shadow_sidecar, encode_shadow_sidecar)
from solids import Projection, row_plane
from wall_sidecar import (BATCH, HEADER as WALL_HEADER, MAGIC as WALL_MAGIC, VERTEX,
                          VERSION as WALL_VERSION, decode_wall_sidecar, encode_wall_sidecar)
from vendor.fp import proj as donor_proj
from f2lib.dat2 import write_dat2


class ArchiveFixture:
    def __init__(self, names):
        self.names = set(names)

    def has(self, key):
        return key in self.names


class SourcePriorityTests(unittest.TestCase):
    def test_conflicting_archive_member_uses_engine_priority(self):
        sources = [
            ("archive", "patch001", ArchiveFixture({"maps/kladwtwn.map"})),
            ("archive", "patch000", ArchiveFixture({"maps/kladwtwn.map"})),
            ("archive", "critter.dat", ArchiveFixture({"maps/kladwtwn.map"})),
            ("archive", "master.dat", ArchiveFixture({"maps/kladwtwn.map"})),
        ]
        winner = choose_source("maps/kladwtwn.map", sources)
        self.assertEqual((winner[0], winner[1]), ("archive", "patch001"))

    def test_later_listed_mod_wins_and_patch_chain_may_be_empty(self):
        with TemporaryDirectory() as temp:
            game = Path(temp)
            (game / "fallout2.cfg").write_text(
                "[system]\nmaster_dat=master.dat\ncritter_dat=critter.dat\n",
                encoding="cp1252",
            )
            (game / "mods").mkdir()
            (game / "mods" / "mods_order.txt").write_text(
                "rpu.dat\n; comment.dat\nmissing.dat\nrpu_russian.dat\n", encoding="cp1252"
            )
            write_dat2({"maps/kladwtwn.map": b"rpu", "maps/arcaves.map": b"rpu only"}, game / "mods" / "rpu.dat")
            write_dat2({"maps/kladwtwn.map": b"russian"}, game / "mods" / "rpu_russian.dat")
            write_dat2({"maps/kladwtwn.map": b"critter"}, game / "critter.dat")
            write_dat2({"maps/kladwtwn.map": b"master"}, game / "master.dat")

            files = ReadOnlyGameFiles(game)
            try:
                self.assertEqual(files.read("maps/kladwtwn.map"), b"russian")
                self.assertEqual(files.read("maps/arcaves.map"), b"rpu only")
                self.assertEqual(
                    [name for _, name, _ in files.sources],
                    ["mods/rpu_russian.dat", "mods/rpu.dat", "critter_dat", "master_dat", "cwd_fallback"],
                )
            finally:
                files.close()

    def test_a_loose_file_the_game_wrote_gzipped_reads_as_plain(self):
        with TemporaryDirectory() as temp:
            game = Path(temp)
            (game / "fallout2.cfg").write_text(
                "[system]\nmaster_dat=master.dat\nmaster_patches=data\ncritter_dat=critter.dat\n",
                encoding="cp1252",
            )
            write_dat2({"maps/kladwtwn.map": b"master"}, game / "master.dat")
            write_dat2({"maps/kladwtwn.map": b"critter"}, game / "critter.dat")
            (game / "data" / "proto" / "critters").mkdir(parents=True)
            (game / "data" / "proto" / "critters" / "00000097.pro").write_bytes(gzip.compress(b"proto"))

            files = ReadOnlyGameFiles(game)
            try:
                self.assertEqual(files.read("proto/critters/00000097.pro"), b"proto")
            finally:
                files.close()

    def test_configured_master_patches_directory_beats_conflicting_dat(self):
        with TemporaryDirectory() as temp:
            game = Path(temp)
            (game / "data" / "maps").mkdir(parents=True)
            (game / "data" / "maps" / "kladwtwn.map").write_bytes(b"configured loose winner")
            (game / "maps").mkdir()
            (game / "maps" / "kladwtwn.map").write_bytes(b"cwd fallback must lose")
            (game / "fallout2.cfg").write_text(
                "[system]\nmaster_dat=master.dat\ncritter_dat=critter.dat\n"
                "master_patches=data\ncritter_patches=data\nf2_res_dat=f2_res.dat\n",
                encoding="cp1252",
            )
            (game / "mods").mkdir()
            (game / "mods" / "mods_order.txt").write_text("", encoding="cp1252")
            write_dat2({"maps/kladwtwn.map": b"patch"}, game / "patch000.dat")
            write_dat2({"maps/kladwtwn.map": b"critter"}, game / "critter.dat")
            write_dat2({"maps/kladwtwn.map": b"master"}, game / "master.dat")

            files = ReadOnlyGameFiles(game)
            try:
                self.assertEqual(files.read("maps/kladwtwn.map"), b"configured loose winner")
                self.assertEqual(files._reads["maps/kladwtwn.map"]["source"], "directory:master_patches")
                self.assertEqual(files.listdir("maps"), ["kladwtwn.map"])
                self.assertEqual(
                    [name for _, name, _ in files.sources],
                    ["master_patches", "critter_patches", "patch000.dat",
                     "critter_dat", "master_dat", "cwd_fallback"],
                )
            finally:
                files.close()

    def test_critter_patches_precede_patch_and_base_archives(self):
        sources = [
            ("directory", "critter_patches", ArchiveFixture({"art/critters/test.frm"})),
            ("archive", "patch000", ArchiveFixture({"art/critters/test.frm"})),
            ("archive", "critter.dat", ArchiveFixture({"art/critters/test.frm"})),
            ("archive", "master.dat", ArchiveFixture({"art/critters/test.frm"})),
        ]
        winner = choose_source("art/critters/test.frm", sources)
        self.assertEqual((winner[0], winner[1]), ("directory", "critter_patches"))

    def test_patch_discovery_stops_at_first_gap(self):
        with TemporaryDirectory() as temp:
            root = Path(temp)
            for name in ("patch000.dat", "patch002.dat"):
                (root / name).touch()
            self.assertEqual([p.name for p in patch_paths(root, "patch%03d.dat")], ["patch000.dat"])

    def test_patch_discovery_keeps_contiguous_chain_only(self):
        with TemporaryDirectory() as temp:
            root = Path(temp)
            for name in ("patch000.dat", "patch001.dat", "patch003.dat"):
                (root / name).touch()
            self.assertEqual([p.name for p in patch_paths(root, "patch%03d.dat")], ["patch000.dat", "patch001.dat"])


class TargetTests(unittest.TestCase):
    def test_one_map_and_elevation(self):
        args = parse_args(["--map", "KLATRAP", "--elevation", "1"])
        self.assertEqual((args.map_names, args.elevation, args.maps), (["klatrap"], 1, None))

    def test_default_is_kladwtwn_elevation_0(self):
        args = parse_args([])
        self.assertEqual((args.map_names, args.elevation), (["kladwtwn"], 0))

    def test_group_and_list(self):
        self.assertEqual(len(parse_args(["--maps", "klamath"]).map_names), 7)
        self.assertEqual(parse_args(["--maps", "kladwtwn, klatrap"]).map_names, ["kladwtwn", "klatrap"])

    def test_rejects_unsafe_map_names(self):
        with self.assertRaises(SystemExit):
            parse_args(["--map", "../kladwtwn"])
        with self.assertRaises(SystemExit):
            parse_args(["--maps", "kladwtwn,a/b"])

    def test_each_elevation_has_its_directory(self):
        with TemporaryDirectory() as tmp:
            game = Path(tmp)
            self.assertEqual(scene_directory(game, "klatrap", 2), game / "mod-cache" / "scene" / "klatrap" / "2")


class OutputSafetyTests(unittest.TestCase):
    def test_accepts_only_known_export_artifact_names(self):
        with TemporaryDirectory() as temp:
            out = Path(temp)
            for name in ("scene.json", "provenance.json", "floor.bin", "walls.bin", "projection.bin",
                         "baked.bin", "tiles.png", "light.png", "rock.png", "sprites10.png"):
                (out / name).touch()
            validate_output_contents(out)

    def test_rejects_unknown_png_without_removing_it(self):
        with TemporaryDirectory() as temp:
            out = Path(temp)
            user_file = out / "keep-me.png"
            user_file.write_bytes(b"user data")
            with self.assertRaisesRegex(ValueError, "keep-me.png"):
                validate_output_contents(out)
            self.assertEqual(user_file.read_bytes(), b"user data")

    def test_main_aborts_before_export_and_preserves_unknown_png(self):
        with TemporaryDirectory() as temp:
            game = Path(temp) / "game"
            out = game / "mod-cache" / "scene" / "kladwtwn" / "0"
            out.mkdir(parents=True)
            user_file = out / "keep-me.png"
            user_file.write_bytes(b"user data")
            with self.assertRaisesRegex(SystemExit, "keep-me.png"):
                main(["--game-root", str(game)])
            self.assertEqual(user_file.read_bytes(), b"user data")

    def test_rejects_unknown_non_png_file(self):
        with TemporaryDirectory() as temp:
            out = Path(temp)
            (out / "notes.txt").touch()
            with self.assertRaisesRegex(ValueError, "notes.txt"):
                validate_output_contents(out)

    def test_rejects_directories(self):
        with TemporaryDirectory() as temp:
            out = Path(temp)
            (out / "sprites0.png").mkdir()
            with self.assertRaisesRegex(ValueError, "sprites0.png"):
                validate_output_contents(out)

    def test_rejects_symlinks(self):
        with TemporaryDirectory() as temp:
            # The target lies outside the export directory, so the link is
            # the only entry there, whatever order the directory lists in.
            out = Path(temp) / "out"
            out.mkdir()
            target = Path(temp) / "target.txt"
            target.write_text("safe", encoding="utf-8")
            link = out / "scene.json"
            try:
                link.symlink_to(target)
            except (OSError, NotImplementedError):
                self.skipTest("symlink creation is unavailable")
            with self.assertRaisesRegex(ValueError, "scene.json"):
                validate_output_contents(out)


class FloorSidecarTests(unittest.TestCase):
    @staticmethod
    def sample_scene():
        floor = bytearray(100 * 100 * 2)
        struct.pack_into("<H", floor, 0, 1)
        struct.pack_into("<H", floor, 2, 2)
        roof = bytearray(100 * 100 * 2)
        struct.pack_into("<H", roof, 4, 2)
        return {
            "format": 6,
            "map": "kladwtwn",
            "elevation": 0,
            "floor": base64.b64encode(floor).decode("ascii"),
            "roof": base64.b64encode(roof).decode("ascii"),
            "tiles": {"size": [512, 512], "cells": [[0, 0], [89, 0]]},
        }

    def test_versioned_binary_round_trip(self):
        payload = encode_floor_sidecar(self.sample_scene())
        decoded = decode_floor_sidecar(payload)
        self.assertEqual(payload[:4], MAGIC)
        self.assertEqual(decoded["version"], VERSION)
        self.assertEqual(decoded["grid_size"], (100, 100))
        self.assertEqual(decoded["image_size"], (512, 512))
        self.assertEqual(decoded["cells"], [(0, 0), (89, 0)])
        self.assertEqual(decoded["floor"][:2], (1, 2))
        self.assertEqual(decoded["roof"][:3], (0, 0, 2))
        self.assertEqual(len(payload), HEADER.size + 2 * 10000 * 2 + 2 * 4)

    def test_cell_coordinates_select_inner_square_and_validate_outer_cell(self):
        self.assertEqual(cell_content_rect((0, 0), (100, 100)), (6, 6, 94, 94))
        scene = self.sample_scene()
        scene["tiles"]["cells"] = [[420, 0], [89, 0]]
        with self.assertRaisesRegex(ValueError, "tile cell 0 is outside"):
            encode_floor_sidecar(scene)

    def test_rejects_scene_for_wrong_map_or_elevation(self):
        scene = self.sample_scene()
        scene["elevation"] = 3
        with self.assertRaisesRegex(ValueError, "one map elevation"):
            encode_floor_sidecar(scene)

    def test_rejects_floor_reference_outside_cell_table(self):
        scene = self.sample_scene()
        floor = bytearray(base64.b64decode(scene["floor"]))
        struct.pack_into("<H", floor, 0, 3)
        scene["floor"] = base64.b64encode(floor).decode("ascii")
        with self.assertRaisesRegex(ValueError, "missing tiles.cells"):
            encode_floor_sidecar(scene)

    def test_rejects_unsupported_version_and_truncation(self):
        payload = encode_floor_sidecar(self.sample_scene())
        bad_version = bytearray(payload)
        struct.pack_into("<H", bad_version, 4, VERSION + 1)
        with self.assertRaisesRegex(ValueError, "magic/version"):
            decode_floor_sidecar(bad_version)
        with self.assertRaisesRegex(ValueError, "truncated"):
            decode_floor_sidecar(payload[:-1])


class WallSidecarTests(unittest.TestCase):
    @staticmethod
    def sample_scene():
        projection = json.loads(json.dumps(donor_proj.as_json()))
        hpx = projection["hpx"]
        return {
            "format": 6,
            "map": "kladwtwn",
            "elevation": 0,
            "pages": [{"image": "sprites0.png", "size": [100, 100]}],
            "proj": projection,
            "sprites": [[0, 10, 20, 10, 20, 0, 0], [0, 30, 40, 10, 20, 0, 0]],
            "walls": [[0, 1.0, 2.0, 3.0, 2.0, 0.0, 2.0, 0, 1,
                       2.0 - 10.0 / hpx, 0.25, 1]],
            "trims": [],
            "doors": [[0, 5.0, 6.0, 7.0, 6.0, 0.0, 1.0, 46, 400]],
        }

    def test_encodes_game_view_front_and_cap_only(self):
        decoded = decode_wall_sidecar(encode_wall_sidecar(self.sample_scene()))
        self.assertEqual(decoded["version"], WALL_VERSION)
        batch = decoded["batches"][0]
        self.assertEqual(batch["page"], 0)
        self.assertEqual(batch["size"], (100, 100))
        vertices = batch["vertices"]

        # First front triangle: donor upright() lower crop, with GL x/z offset.
        for actual, expected in zip(vertices[0], (-49.0, 0.0, -48.0, 0.1, 0.4)):
            self.assertAlmostEqual(actual, expected)
        for actual, expected in zip(vertices[1], (-47.0, 0.0, -48.0, 0.2, 0.4)):
            self.assertAlmostEqual(actual, expected)
        # Then the top: a horizontal face as deep as the wall is thick, cut square
        # at the free ends, and what of the picture is higher as an upright face
        # on the slab's far side. Every point samples the texel the game drew it
        # on and keeps the depth of the front plane that texel lies on.
        scene = self.sample_scene()
        hpx = scene["proj"]["hpx"]
        zc, thick = 2.0 - 10.0 / hpx, 0.25
        proj = Projection(scene["proj"])
        e0, e1 = proj.ground_to_px(1.0, 2.0, zc), proj.ground_to_px(3.0, 2.0, zc)
        slope = (e1[1] - e0[1]) / (e1[0] - e0[0])
        top = vertices[6:]
        self.assertTrue([vertex for vertex in top if abs(vertex[1] - zc) < 1e-5])
        for vertex in top:
            u, z, v = vertex[0] + 50.0, vertex[1], vertex[2] + 50.0
            self.assertTrue(2.0 - thick - 1e-5 <= v <= 2.0 + 1e-5)
            self.assertTrue(zc - 1e-5 <= z <= 2.0 + 1e-5)
            if z > zc + 1e-5:
                self.assertAlmostEqual(v, 2.0 - thick, places=5)
            if v > 2.0 - thick + 1e-5:
                self.assertTrue(1.0 - 1e-5 <= u <= 3.0 + 1e-5)
            qx, qy = proj.ground_to_px(u, v, z)
            column = (qx - e0[0]) * 10 / (e1[0] - e0[0])
            rows = min(e0[1] + slope * (qx - e0[0]) - qy, 10.0)
            self.assertAlmostEqual(vertex[3], (10 + column) / 100, places=4)
            self.assertAlmostEqual(vertex[4], (20 + 10 - rows) / 100, places=4)
            self.assertAlmostEqual(vertex[7], -48.0, places=5)
            self.assertAlmostEqual(vertex[6], zc + rows / hpx, places=5)
        # No back face (sprite 1), slab end or door: the game's camera does not see
        # backs and ends, and the engine draws doors live so they open.
        for vertex in vertices:
            self.assertTrue(0.1 - 1e-6 <= vertex[3] <= 0.2 + 1e-6)

    def test_rejects_invalid_sprite_index_and_bad_page(self):
        scene = self.sample_scene()
        scene["walls"][0][0] = 2
        with self.assertRaisesRegex(ValueError, "outside scene.sprites"):
            encode_wall_sidecar(scene)
        scene = self.sample_scene()
        scene["sprites"][0][0] = 2
        with self.assertRaisesRegex(ValueError, "outside scene.pages"):
            encode_wall_sidecar(scene)

    def test_furniture_solid_shows_its_sprite_from_the_game_camera(self):
        scene = self.sample_scene()
        scene["walls"] = []
        scene["sprites"].append([0, 40, 40, 50, 50, -25, -40])
        # box: [sprite, u, v, board, round, cu, cv, a, b, z0, z1, wallU, wallV]
        scene["props"] = [[2, 10.0, 10.0, 0, 0, 10.0, 10.1, 0.5, 0.4, 0.0, 0.3, 0.0, 0.6]]
        scene["boards"] = [[2, 10.0, 10.0, 1, 0, 0]]  # [sprite, u, v, stack, name, object]
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        vertices = decoded["batches"][0]["vertices"]
        self.assertGreater(len(vertices), 12)
        p = scene["proj"]
        def px(u, v, z):
            return (p["origin"][0] + u * p["squ"][0] + v * p["sqv"][0],
                    p["origin"][1] + u * p["squ"][1] + v * p["sqv"][1] - p["hpx"] * z)
        # Every vertex shows the sprite pixel it lands on from the game's camera
        # (anchor (u, v), sprite top-left 25 px left and 40 px up of it).
        ax, ay = px(10.0, 10.0, 0.0)
        proj = Projection(p)
        toward = proj.toward
        plane = row_plane(proj, 10.0, 10.0, 1) + 2e-5
        # The far sides (v = 9.9, u = 9.75, up to the top; the headboard above
        # faces the camera) face away from the game's camera and show the near
        # sides' pixels mirrored.
        def far(vertex):
            return (abs(vertex[2] + 50.0 - 9.9) < 1e-4 or abs(vertex[0] + 50.0 - 9.75) < 1e-4) and vertex[1] <= 0.3 + 1e-4
        for vertex in vertices:
            x, y, z, s_, t = vertex[:5]
            sx, sy = px(x + 50.0, z + 50.0, y)
            if far(vertex):
                continue
            self.assertAlmostEqual(s_ * 100.0, 40 + sx - ax + 25, places=3)
            self.assertAlmostEqual(t * 100.0, 40 + sy - ay + 40, places=3)
            # Depth: the card's hex row plane, a hair nearer, along the game's ray.
            dx, dy, dz = (vertex[5 + k] - vertex[k] for k in range(3))
            self.assertAlmostEqual(vertex[7] + 50.0, plane, places=4)
            self.assertAlmostEqual(dx * toward[1] - dz * toward[0], 0.0, places=4)
            self.assertAlmostEqual(dy * toward[1] - dz * toward[2], 0.0, places=4)
        # The near faces face the game's camera (counter-clockwise on its screen,
        # y up); the far sides face away, so it culls them.
        away = 0
        for index in range(0, len(vertices), 3):
            triangle = vertices[index:index + 3]
            a, b, c = (px(v[0] + 50.0, v[2] + 50.0, v[1]) for v in triangle)
            area = (b[0] - a[0]) * (-(c[1] - a[1])) - (c[0] - a[0]) * (-(b[1] - a[1]))
            if all(far(v) for v in triangle):
                self.assertLess(area, 0.0)
                away += 1
            else:
                self.assertGreater(area, 0.0)
        self.assertGreater(away, 0)

        # A drum: sixteen sides and a top (the sides away from the camera
        # repeat the facing quarter mirrored, so they are not checked here).
        # It stands for several cards (board -1), so it keeps its own depth.
        scene["props"] = [[2, 20.0, 20.0, -1, 1, 20.0, 20.0, 9.0, 6.0, 0.0, 0.4, 0.0, 0.0]]
        drum = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertGreater(drum["triangles"], 16)
        for vertex in drum["batches"][0]["vertices"]:
            self.assertEqual(vertex[5:8], vertex[:3])

    def test_the_fly_camera_gets_a_whole_drum_of_its_own(self):
        # Its near half, for the game's camera alone, shows the sprite's outline
        # stretched round the sides; the fly camera's drum repeats the front.
        scene = self.sample_scene()
        scene["walls"] = []
        scene["pages"][0]["size"] = [400, 400]
        scene["sprites"].append([0, 100, 100, 200, 200, -100, -150])
        scene["props"] = [[2, 10.0, 10.0, 0, 1, 10.0, 10.0, 18.0, 8.0, 0.0, 0.5, 0.0, 0.0]]
        scene["boards"] = [[2, 10.0, 10.0, 0, 0, 0]]
        batches = decode_wall_sidecar(encode_wall_sidecar(scene))["batches"]
        self.assertEqual(sorted((b["fly_only"], b["game_only"]) for b in batches),
                         [(False, False), (False, True), (True, False)])
        game = next(b for b in batches if b["game_only"])["triangles"]
        fly = next(b for b in batches if b["fly_only"])["triangles"]
        self.assertEqual(fly, 2 * game)

    def test_drum_body_is_as_wide_as_its_rim(self):
        # A drum on its card's depth shows the near half of its body: on the game's
        # screen the body spans the rim's width (a burning barrel's rim stood out).
        scene = self.sample_scene()
        scene["walls"] = []
        scene["pages"][0]["size"] = [400, 400]
        scene["sprites"].append([0, 100, 100, 200, 200, -100, -150])
        scene["props"] = [[2, 10.0, 10.0, 0, 1, 10.0, 10.0, 18.0, 8.0, 0.0, 0.5, 0.0, 0.0]]
        scene["boards"] = [[2, 10.0, 10.0, 0, 0, 0]]
        p = scene["proj"]
        def screen_x(vertex):
            u, v = vertex[0] + 50.0, vertex[2] + 50.0
            return p["origin"][0] + u * p["squ"][0] + v * p["sqv"][0]
        # What the game's camera draws: every batch but the fly camera's own.
        vertices = [vertex for batch in decode_wall_sidecar(encode_wall_sidecar(scene))["batches"]
                    if not batch["fly_only"] for vertex in batch["vertices"]]
        body = [screen_x(v) for v in vertices if v[1] < 0.25]
        rim = [screen_x(v) for v in vertices]
        self.assertAlmostEqual(min(body), min(rim), places=3)
        self.assertAlmostEqual(max(body), max(rim), places=3)
        self.assertAlmostEqual(max(rim) - min(rim), 2 * 18.0, places=3)

    def test_faces_name_the_map_object_they_show(self):
        from types import SimpleNamespace
        scene = self.sample_scene()
        thin = [1, 4.0, 2.0, 5.0, 2.0, 0.0, 1.0, 0, 0, 1.0, 0.0, -1]
        scene["walls"].append(thin)
        objects = [SimpleNamespace(tile=20100, fid=0x3000010), SimpleNamespace(tile=20102, fid=0x3000011)]
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene, map_objects=objects))
        self.assertEqual(decoded["objects"], [(20102, 0x3000011), (20100, 0x3000010)])
        owners = {vertex[8] for vertex in decoded["batches"][0]["vertices"]}
        self.assertEqual(owners, {0.0, 1.0})
        # Without the map's objects no face names one.
        plain = decode_wall_sidecar(encode_wall_sidecar(self.sample_scene()))
        self.assertEqual(plain["objects"], [])
        self.assertEqual({vertex[8] for vertex in plain["batches"][0]["vertices"]}, {-1.0})

    def test_wall_tops_meet_where_a_wall_goes_on(self):
        from wall_sidecar import _continuations
        row = lambda ua, ub, thick=0.25: [0, ua, 2.0, ub, 2.0, 0.0, 2.0, 0, 1, 1.8, thick, -1]
        across = [0, 3.0, 2.0, 3.0, 4.0, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        ends = _continuations([row(1.0, 3.0), row(3.0, 5.0), across, row(5.0, 6.0, 0.0)])
        self.assertEqual(ends[0], (False, True))
        self.assertEqual(ends[1], (True, False))   # a thin wall does not go on a thick one
        self.assertEqual(ends[2], (False, False))  # nor does one across it
        self.assertNotIn(3, ends)

    def test_a_corner_closes_for_the_free_camera_only(self):
        from unittest import mock
        import wall_sidecar
        # An L: the wall along u runs 0.4 past the far face of the one along v
        # (u 3.0), which stops 0.5 short of the first's front (v 2.0). Both
        # fronts face out of the corner: each runs to the other's front, the
        # backs and tops stop at the other's back; nothing ends in between.
        along_u = [0, 1.0, 2.0, 3.4, 2.0, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        along_v = [1, 3.0, 0.0, 3.0, 1.5, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        corners = wall_sidecar._corner_fits([along_u, along_v], set(), (0.49, 0.74, 0.46))
        self.assertAlmostEqual(corners.keep[0][1], 2.0 / 2.4)  # the front to u 3.0
        self.assertAlmostEqual(corners.back_keep[0][1], 1.75 / 2.4)  # the back and top to u 2.75
        self.assertAlmostEqual(corners.top_keep[0][1], 1.75 / 2.4)
        self.assertEqual([(round(s0, 3), round(s1, 3), back) for _, _, s0, s1, back in corners.fills],
                         [(1.5, 2.0, False), (1.5, 1.75, True)])
        self.assertEqual(len(corners.caps), 3)  # each end's, and the square's
        self.assertEqual(len(corners.moved), 2)
        # What the game's camera draws is the same triangles as without the pass.
        scene = self.sample_scene()
        scene["walls"] = [along_u, along_v]
        def game(data):
            return sorted(tuple(round(n, 5) for n in vertex) for batch in data["batches"] if not batch["fly_only"]
                          for vertex in batch["vertices"])
        with mock.patch.object(wall_sidecar, "_corner_fits", return_value=wall_sidecar._Corners()):
            plain = decode_wall_sidecar(encode_wall_sidecar(scene))
        closed = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertEqual(game(closed), game(plain))
        fly = lambda data: sum(batch["triangles"] for batch in data["batches"] if batch["fly_only"])
        self.assertGreater(fly(closed), fly(plain))

    def test_a_wall_ending_above_the_floor_gets_a_foot_for_the_free_camera(self):
        # Task 081: a picture whose clear bottom rows were trimmed ends 0.12
        # over the floor; its front and back run down to it in the fly
        # batches, in one picture row; the game's camera keeps what it had.
        scene = self.sample_scene()
        scene["walls"] = [[0, 1.0, 2.0, 3.0, 2.0, 0.12, 2.0, 0, 1, 1.8, 0.25, -1]]
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        fly = [v for batch in decoded["batches"] if batch["fly_only"] for v in batch["vertices"]]
        game = [v for batch in decoded["batches"] if not batch["fly_only"] for v in batch["vertices"]]
        self.assertAlmostEqual(min(v[1] for v in game), 0.12)
        feet = [v for v in fly if v[1] < 0.12 - 1e-6]
        self.assertEqual(len(feet), 2 * 3)  # the front's and the back's lower edge
        self.assertTrue(all(abs(v[1]) < 1e-6 for v in feet))
        page_h = scene["pages"][0]["size"][1]
        self.assertLess(max(v[4] for v in feet) - min(v[4] for v in feet), 1.0 / page_h)  # one picture row
        scene["walls"][0][5] = 0.6  # a row this high stands on something else
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertFalse([v for batch in decoded["batches"] for v in batch["vertices"] if v[1] < 0.6 - 1e-6])

    def test_a_wall_over_a_roof_is_cut_at_the_roof_for_the_free_camera_only(self):
        import wall_sidecar
        # Task 081: a wall face 2.5 high from u 1 to 3 at v 2.5; only square
        # (2, 2) has a roof. The game's camera keeps both triangles whole; the
        # free camera keeps all of it but the part over that square, its rim
        # included, above the roof height.
        off, height = wall_sidecar.GL_MAP_OFFSET, 1.88
        opaque, clear = [[255] * 100] * 100, [[0] * 100] * 100  # the roof cell's texels, rim included
        corner = lambda u, y: (u - off, y, 2.5 - off, u - 1.0, y, u, y, 2.5, 0.0)
        a, b, c, d = corner(1.0, 0.0), corner(3.0, 0.0), corner(3.0, 2.5), corner(1.0, 2.5)
        shared = {0: [n for vertex in (a, b, c, a, c, d) for n in vertex]}
        game, fly = {}, {}
        wall_sidecar._cut_by_roofs(shared, wall_sidecar._Roofs(height, {(2, 2): (0, 0)}, opaque), game, fly)
        self.assertEqual(shared, {})
        self.assertEqual(len(game[0]), 6 * wall_sidecar.FLOATS)
        points = [fly[0][i:i + wall_sidecar.FLOATS] for i in range(0, len(fly[0]), wall_sidecar.FLOATS)]
        rim = wall_sidecar.ROOF_RIM
        self.assertFalse([p for p in points if p[1] > height + 0.01 and p[0] + off > 2.0 - rim + 1e-6])
        self.assertTrue([p for p in points if p[1] > 2.4 and p[0] + off < 2.0 - rim])
        self.assertTrue(all(abs(p[3] - (p[0] + off - 1.0)) < 1e-6 for p in points))  # values follow the cut
        area = sum(abs((q[0] - p[0]) * (r[1] - p[1]) - (r[0] - p[0]) * (q[1] - p[1])) / 2
                   for p, q, r in zip(points[0::3], points[1::3], points[2::3]))
        self.assertAlmostEqual(area, 2 * 2.5 - (1 + rim) * (2.5 - height - wall_sidecar.ROOF_SLACK), places=4)
        # a wall in the rim, just off the square, is under the roof the engine
        # draws where the rim's texels are opaque, and stays where they are clear
        near = lambda v, y: (1.97 - off, y, v - off, v, y, 1.97, y, v, 0.0)
        a, b, c, d = near(2.2, 0.0), near(2.8, 0.0), near(2.8, 2.5), near(2.2, 2.5)
        for tiles, cut in ((opaque, True), (clear, False)):
            shared = {0: [n for vertex in (a, b, c, a, c, d) for n in vertex]}
            wall_sidecar._cut_by_roofs(shared, wall_sidecar._Roofs(height, {(2, 2): (0, 0)}, tiles))
            self.assertEqual(not [v for v in shared[0][1::wall_sidecar.FLOATS] if v > height + 0.01], cut)
        # a wall lying on a square's edge is cut, not doubled
        edge = lambda u, y: (u - off, y, 2.0 - off, u - 1.0, y, u, y, 2.0, 0.0)
        a, b, c, d = edge(1.0, 0.0), edge(3.0, 0.0), edge(3.0, 2.5), edge(1.0, 2.5)
        shared = {0: [n for vertex in (a, b, c, a, c, d) for n in vertex]}
        wall_sidecar._cut_by_roofs(shared, wall_sidecar._Roofs(height, {(2, 2): (0, 0)}, clear))
        points = [shared[0][i:i + wall_sidecar.FLOATS] for i in range(0, len(shared[0]), wall_sidecar.FLOATS)]
        area = sum(abs((q[0] - p[0]) * (r[1] - p[1]) - (r[0] - p[0]) * (q[1] - p[1])) / 2
                   for p, q, r in zip(points[0::3], points[1::3], points[2::3]))
        self.assertAlmostEqual(area, 2 * 2.5 - (2.5 - height - wall_sidecar.ROOF_SLACK), places=4)

    def test_a_corner_picture_running_out_past_the_crossing_still_makes_a_corner(self):
        import wall_sidecar
        # Task 081: the wall along v ends in a corner picture (v 1.5 .. 2.8)
        # that runs 0.8 past the other wall's front, out of reach: still an
        # L, and the picture is the corner piece the free camera folds, no
        # fin standing out into the street.
        along_u = [0, 1.0, 2.0, 3.0, 2.0, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        along_v = [1, 3.0, 0.0, 3.0, 1.5, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        piece = [2, 3.0, 1.5, 3.0, 2.8, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        corners = wall_sidecar._corner_fits([along_u, along_v, piece], set(), (0.49, 0.74, 0.46),
                                            lambda index: 16, (48.0, 32.0))
        self.assertIn(2, corners.pieces)

    def test_a_corner_block_takes_a_wide_picture_not_the_end_strip(self):
        import wall_sidecar
        # The wall along v ends in a 2-column strip short of the corner: the
        # faces that fill the shortfall take the 16-column row behind it, not
        # the strip stretched into stripes.
        along_u = [0, 1.0, 2.0, 3.4, 2.0, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        along_v = [1, 3.0, 0.0, 3.0, 1.45, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        strip = [2, 3.0, 1.45, 3.0, 1.5, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]
        walls = [along_u, along_v, strip]
        width = lambda index: (16, 16, 2)[index]
        corners = wall_sidecar._corner_fits(walls, set(), (0.49, 0.74, 0.46), width, (48.0, 32.0))
        self.assertEqual([pool for _, pool, _, _, _ in corners.fills], [[1], [1]])

    @staticmethod
    def decor_corner():
        """An L whose wall along u ends in a corner piece (u 2.6 .. 3.4) that
        runs into the square where the walls cross: walls, sprites (16 x 60
        pictures side by side), projection, corners."""
        import wall_sidecar
        proj = Projection(donor_proj.as_json())
        walls = [[0, 1.0, 2.0, 2.6, 2.0, 0.0, 2.0, 0, 1, 1.8, 0.25, -1],
                 [1, 2.6, 2.0, 3.4, 2.0, 0.0, 2.0, 0, 1, 1.8, 0.25, -1],
                 [2, 3.0, 0.0, 3.0, 1.5, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]]
        sprites = [[0, 20 * index, 0, 16, 60, 0, 0] for index in range(3)]
        corners = wall_sidecar._corner_fits(walls, set(), proj.toward, lambda index: 16, (48.0, 32.0))
        return walls, sprites, proj, corners

    def test_a_decor_corner_folds_onto_both_fronts_seen_as_the_game_draws_it(self):
        from unittest import mock
        import wall_sidecar
        walls, sprites, proj, corners = self.decor_corner()
        self.assertEqual(list(corners.pieces), [1])
        slab, out, twin, twin_out, start = corners.pieces[1]
        self.assertAlmostEqual(start, 2.6)
        faces = []
        with mock.patch.object(wall_sidecar, "_polygon", lambda batches, proj, page, size, corners: faces.append(corners)):
            wall_sidecar._corner_decor(None, proj, sprites, [[100, 100]], proj.toward, walls[1], slab, out, twin, twin_out)
        self.assertGreater(len(faces), 1)
        lift = wall_sidecar.DECOR_LIFT
        x, y, w, h = sprites[1][1:5]
        e0 = proj.ground_to_px(2.6, 2.0, 0.0)
        e1 = proj.ground_to_px(3.4, 2.0, 0.0)
        spans = []
        for face in faces:
            for u, v, z, tx, ty in face:
                # On a front: the u wall's (v 2.0) up to the corner's edge, or
                # the v wall's (u 3.0) up to it, a hair out of the face.
                on_u = abs(v - (2.0 + lift)) < 1e-6 and u <= 3.0 + lift + 1e-6
                on_v = abs(u - (3.0 + lift)) < 1e-6 and v <= 2.0 + lift + 1e-6
                self.assertTrue(on_u or on_v, (u, v))
                # The game's camera sees the texel where the game drew it.
                column, rows = tx - x, h - (ty - y)
                sx, sy = proj.ground_to_px(u, v, z)
                self.assertAlmostEqual(sx, e0[0] + (e1[0] - e0[0]) * column / w, places=4)
                self.assertAlmostEqual(sy, e0[1] + (e1[1] - e0[1]) * column / w - rows, places=4)
            spans.append((min(tx for *_, tx, _ in face) - x, max(tx for *_, tx, _ in face) - x))
        # Every column of the picture lands somewhere: no gap opens in it.
        reach = 0.0
        for lo, hi in sorted(spans):
            self.assertLessEqual(lo, reach + 1e-6)
            reach = max(reach, hi)
        self.assertAlmostEqual(reach, w)

    def test_only_a_dull_corner_piece_unlike_its_wall_counts_as_decor(self):
        import numpy as np
        import wall_sidecar
        walls, sprites, proj, corners = self.decor_corner()
        hpx = donor_proj.as_json()["hpx"]
        def decor(piece):
            atlas = np.full((100, 100, 4), 255, np.uint8)
            atlas[:, :, :3] = 120  # plain gray walls
            atlas[:60, 20:36, :3] = piece
            return wall_sidecar._decor_pieces([atlas], sprites, hpx, walls, corners)
        self.assertEqual(decor((120, 120, 120)), [])  # the plain wall turning the corner
        self.assertEqual(decor((70, 70, 70)), [1])  # a darker pilaster
        self.assertEqual(decor((150, 80, 60)), [])  # bricks: the wall's own material
        stripes = np.full((60, 16, 3), 120, np.uint8)
        stripes[:, ::2] = 40
        self.assertEqual(decor(stripes), [1])  # same shade, but a pattern the wall lacks
        self.assertEqual(wall_sidecar._decor_pieces(None, sprites, hpx, walls, corners), [])

    def test_a_front_up_to_decor_repeats_its_last_columns(self):
        from unittest import mock
        import wall_sidecar
        walls, sprites, proj, corners = self.decor_corner()
        hpx = donor_proj.as_json()["hpx"]
        slab, pool, s0, s1, back = next(fill for fill in corners.fills if fill[0] is corners.pieces[1][0]
                                        and not fill[4])
        faces = []
        with mock.patch.object(wall_sidecar, "_polygon", lambda batches, proj, page, size, corners, away=False:
                               faces.append(corners)):
            wall_sidecar._corner_fill(None, proj, sprites, [[100, 100]], hpx, (48.0, 32.0), walls, pool, slab,
                                      s0, s1, back, None, True)
        # 0.4 + seam + edge along u at 48 columns a unit: one whole run of
        # the row's last 16 columns (its b end at u 2.6 shows column 16), then
        # the start of the next.
        self.assertEqual(len(faces), 2)
        first, second = ([tx for _, _, _, tx, _ in face] for face in faces)
        self.assertAlmostEqual(first[0], 0)
        self.assertAlmostEqual(first[1], 16)
        self.assertAlmostEqual(second[0], 0)
        self.assertLess(second[1], 16)
        self.assertAlmostEqual(faces[0][1][0], faces[1][0][0])  # the runs meet

    def test_a_corner_core_stands_in_solid_walls_only(self):
        import numpy as np
        # The L of the test above gets an opaque core inside the corner for
        # the free camera; a clear mesh (half its texels clear) gets none.
        scene = self.sample_scene()
        scene["walls"] = [[0, 1.0, 2.0, 3.4, 2.0, 0.0, 2.0, 0, 1, 1.8, 0.25, -1],
                          [1, 3.0, 0.0, 3.0, 1.5, 0.0, 2.0, 0, 1, 1.8, 0.25, -1]]
        solid = np.full((100, 100, 4), 255, np.uint8)
        mesh = solid.copy()
        mesh[::2, :, 3] = 0
        fly = lambda atlas: sum(batch["triangles"] for batch in decode_wall_sidecar(
            encode_wall_sidecar(scene, atlas=None if atlas is None else [atlas]))["batches"] if batch["fly_only"])
        self.assertGreater(fly(solid), fly(mesh))
        self.assertEqual(fly(None), fly(solid))  # without the atlas every wall counts as solid
        # A lower wall gets no fin: the core's arm in it stays below its top.
        scene["walls"][1][6], scene["walls"][1][9] = 0.9, 0.8
        vertices = lambda atlas: {tuple(round(n, 4) for n in v[:3]) for batch in decode_wall_sidecar(
            encode_wall_sidecar(scene, atlas=[atlas]))["batches"] if batch["fly_only"] for v in batch["vertices"]}
        arm = [z for x, z, y in vertices(solid) - vertices(mesh) if y + 50.0 < 1.7]
        self.assertTrue(arm)
        self.assertLessEqual(max(arm), 0.8)

    def test_first_person_surfaces_are_for_the_fly_camera_only(self):
        # Backs and trims go into fly-only batches: what the game's camera
        # draws stays the same.
        scene = self.sample_scene()
        scene["walls"][0].extend([1, 1])
        scene["trims"] = [[0, 1.0, 2.0, 3.0, 2.0, 0.0, 1.0, 0, 0, 1, 1]]
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        plain = decode_wall_sidecar(encode_wall_sidecar(self.sample_scene()))
        def game(batches):
            return [batch for batch in batches if not batch["fly_only"]]
        self.assertEqual(game(decoded["batches"]), game(plain["batches"]))
        fly = [batch for batch in decoded["batches"] if batch["fly_only"]]
        self.assertTrue(fly)
        # The trim: two triangles between its ground points, rows 0..1 of its sprite.
        trim = [v for batch in fly for v in batch["vertices"] if abs(v[2] + 50.0 - 2.0) < 1e-6 and v[0] + 50.0 in (1.0, 3.0)]
        self.assertGreaterEqual(len(trim), 6)

    def test_binary_corruption_is_rejected(self):
        payload = encode_wall_sidecar(self.sample_scene())
        self.assertEqual(payload[:4], WALL_MAGIC)
        bad_version = bytearray(payload)
        struct.pack_into("<H", bad_version, 4, WALL_VERSION + 1)
        with self.assertRaisesRegex(ValueError, "magic/version"):
            decode_wall_sidecar(bad_version)
        with self.assertRaisesRegex(ValueError, "truncated"):
            decode_wall_sidecar(payload[:-1])
        bad_count = bytearray(payload)
        struct.pack_into("<I", bad_count, 16, 999)
        with self.assertRaisesRegex(ValueError, "triangle count mismatch"):
            decode_wall_sidecar(bad_count)
        bad_page = bytearray(payload)
        struct.pack_into("<I", bad_page, WALL_HEADER.size, 1)
        with self.assertRaisesRegex(ValueError, "page batch"):
            decode_wall_sidecar(bad_page)
        bad_uv = bytearray(payload)
        vertex_offset = WALL_HEADER.size + BATCH.size + 12
        struct.pack_into("<f", bad_uv, vertex_offset, float("nan"))
        with self.assertRaisesRegex(ValueError, "non-finite"):
            decode_wall_sidecar(bad_uv)

    def test_a_scene_without_walls_has_no_batches(self):
        scene = self.sample_scene()
        scene["sprites"], scene["walls"], scene["props"], scene["rests"] = [], [], [], []
        data = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertEqual((data["triangles"], data["batches"]), (0, []))

    def test_rejects_other_map_and_out_of_atlas_uv(self):
        scene = self.sample_scene()
        scene["map"] = "../arroyo"
        with self.assertRaisesRegex(ValueError, "one map elevation"):
            encode_wall_sidecar(scene)
        scene = self.sample_scene()
        scene["sprites"][0][1] = 95
        with self.assertRaisesRegex(ValueError, "outside its atlas"):
            encode_wall_sidecar(scene)


class BakedSidecarTests(unittest.TestCase):
    class MapObject:
        def __init__(self, tile, fid):
            self.tile, self.fid = tile, fid

    def sample(self):
        objects = [self.MapObject(100, 0x03000010), self.MapObject(205, 0x02000007),
                   self.MapObject(300, 0x02000099), self.MapObject(400, 0x00000036),
                   self.MapObject(500, 0x00000011)]
        scene = {
            "walls": [[0, 0, 0, 0, 0, 0, 0, 5, 1], [0, 0, 0, 0, 0, 0, 0, 5, 1], [0, 0, 0, 0, 0, 0, 0, 5, 0]],
            # boards: [sprite, u, v, stack, name, object]
            "boards": [[0, 1, 1, 0, 0, 2], [0, 1, 1, 0, 0, 3], [0, 1, 1, 0, 0, 4]],
            "hidden": [0, 1],
            "props": [[0, 1, 1, 0], [0, 1, 1, 1]],
            "lifts": [[2, 0.25, -0.5, 0.75]],
            "forms": [],
        }
        return scene, objects

    def test_walls_and_solids_are_skipped_and_things_on_them_lifted(self):
        scene, objects = self.sample()
        entries = baked_objects(scene, objects)
        self.assertEqual(entries, [(100, 0x03000010, BAKED_SKIP, 0.0, 0.0, 0.0),
                                   (205, 0x02000007, BAKED_SKIP, 0.0, 0.0, 0.0),
                                   (300, 0x02000099, BAKED_SKIP, 0.0, 0.0, 0.0),
                                   (400, 0x00000036, BAKED_SKIP, 0.0, 0.0, 0.0),
                                   (500, 0x00000011, BAKED_LIFT, 0.25, -0.5, 0.75)])
        payload = encode_baked_sidecar(entries)
        self.assertEqual(payload[:4], BAKED_MAGIC)
        self.assertEqual(len(payload), BAKED_HEADER.size + 5 * BAKED_ENTRY.size)
        self.assertEqual(decode_baked_sidecar(payload), entries)

    def test_things_the_donor_lays_flat_are_flat_but_not_critters(self):
        scene, objects = self.sample()
        objects += [self.MapObject(600, 0x02000020), self.MapObject(700, 0x01000030), self.MapObject(800, 0x00000040)]
        # decals: [sprite, u, v, name, object]; a wall's object stays skipped.
        scene["decals"] = [[0, 1, 1, 0, 5], [0, 1, 1, 0, 6], [0, 1, 1, 0, 7], [0, 1, 1, 0, 0]]
        entries = baked_objects(scene, objects)
        self.assertIn((600, 0x02000020, BAKED_FLAT, 0.0, 0.0, 0.0), entries)
        self.assertIn((800, 0x00000040, BAKED_FLAT, 0.0, 0.0, 0.0), entries)
        self.assertNotIn(700, [entry[0] for entry in entries])
        self.assertIn((100, 0x03000010, BAKED_SKIP, 0.0, 0.0, 0.0), entries)
        self.assertEqual(decode_baked_sidecar(encode_baked_sidecar(entries)), entries)

    def test_doors_lie_on_their_wall_and_low_cards_lie_flat_in_flight(self):
        scene, objects = self.sample()
        objects += [self.MapObject(600, 0x02000020), self.MapObject(700, 0x02000030), self.MapObject(800, 0x02000040)]
        scene["sprites"] = [[0, 0, 0, 30, 15], [0, 0, 0, 20, 40]]  # page, x, y, w, h: low, tall
        scene["boards"] += [[0, 1, 1, 0, 0, 5], [1, 1, 1, 0, 0, 6]]
        # doors: [sprite, ua, va, ub, vb, z0, z1, name, object]; GL x, z = u, v less 50
        scene["doors"] = [[0, 51.0, 52.0, 51.0, 54.0, 0.0, 1.0, 0, 7]]
        entries = baked_objects(scene, objects)
        self.assertIn((600, 0x02000020, BAKED_FLY_FLAT, 0.0, 0.0, 0.0), entries)
        self.assertNotIn(700, [entry[0] for entry in entries])
        door = next(entry for entry in entries if entry[0] == 800)
        self.assertEqual(door[2:5], (BAKED_PLANE, 1.0, 2.0))
        self.assertAlmostEqual(door[5], 1.5707963, places=6)
        self.assertIn((500, 0x00000011, BAKED_LIFT, 0.25, -0.5, 0.75), entries)
        decoded = decode_baked_sidecar(encode_baked_sidecar(entries))
        self.assertEqual([entry[:3] for entry in decoded], [entry[:3] for entry in entries])

    def test_a_solid_by_name_hides_its_card_only_in_flight(self):
        scene, objects = self.sample()
        scene["props"][0] = [0, 1, 1, 0] + [0.0] * 10 + [1]  # row[14]: made by name (overrides.json)
        entries = baked_objects(scene, objects)
        self.assertIn((300, 0x02000099, BAKED_FLY_SKIP, 0.0, 0.0, 0.0), entries)
        self.assertIn((400, 0x00000036, BAKED_SKIP, 0.0, 0.0, 0.0), entries)
        self.assertEqual(decode_baked_sidecar(encode_baked_sidecar(entries)), entries)

    def test_a_detail_of_a_joined_thing_hides_its_card_in_flight_beside_forms(self):
        scene, objects = self.sample()
        scene["forms"], scene["props"], scene["flyBoards"] = [[0]], [[0, 1, 1, 0]], [1]  # board 1 names no row
        entries = baked_objects(scene, objects)
        self.assertIn((400, 0x00000036, BAKED_FLY_SKIP, 0.0, 0.0, 0.0), entries)

    def test_a_plant_s_object_casts_no_shadow(self):
        scene, objects = self.sample()
        scene["plants"] = [0]  # board 0 is the object at (300, 0x02000099)
        entries = baked_objects(scene, objects)
        self.assertIn((300, 0x02000099, BAKED_SKIP | BAKED_NOCAST, 0.0, 0.0, 0.0), entries)
        self.assertIn((205, 0x02000007, BAKED_SKIP, 0.0, 0.0, 0.0), entries)
        decoded = decode_baked_sidecar(encode_baked_sidecar(entries))
        self.assertIn((300, 0x02000099, BAKED_SKIP | BAKED_NOCAST, 0.0, 0.0, 0.0), decoded)
        scene["plants"] = [9]
        with self.assertRaisesRegex(ValueError, "outside scene.boards"):
            baked_objects(scene, objects)

    def test_an_animated_solid_stays_live(self):
        scene, objects = self.sample()
        entries = baked_objects(scene, objects, live_boards={1})
        self.assertNotIn(400, [entry[0] for entry in entries])
        self.assertIn(300, [entry[0] for entry in entries])

    def test_rejects_unknown_serial_and_corruption(self):
        with self.assertRaisesRegex(ValueError, "outside the map"):
            baked_objects({"walls": [[0, 0, 0, 0, 0, 0, 0, 5, 3]]}, [self.MapObject(1, 2)])
        payload = encode_baked_sidecar([(1, 2, BAKED_SKIP, 0.0, 0.0, 0.0)])
        with self.assertRaisesRegex(ValueError, "bad length"):
            decode_baked_sidecar(payload[:-1])
        bad_version = bytearray(payload)
        struct.pack_into("<H", bad_version, 4, 1)
        with self.assertRaisesRegex(ValueError, "magic/version"):
            decode_baked_sidecar(bad_version)
        with self.assertRaisesRegex(ValueError, "hex grid"):
            encode_baked_sidecar([(40000, 1, BAKED_SKIP, 0.0, 0.0, 0.0)])


class ProjectionSidecarTests(unittest.TestCase):
    @staticmethod
    def sample_scene():
        return {"map": "kladwtwn", "elevation": 0, "proj": donor_proj.as_json()}

    def test_versioned_projection_has_donor_golden_world_anchor(self):
        payload = encode_projection_sidecar(self.sample_scene())
        projection = decode_projection_sidecar(payload)
        self.assertEqual(payload[:4], PROJECTION_MAGIC)
        self.assertEqual(PROJECTION_HEADER.unpack_from(payload)[:],
                         (PROJECTION_MAGIC, PROJECTION_VERSION, PROJECTION_HEADER.size, 10))
        self.assertEqual(projection["squ"], (-48.0, 12.0))
        self.assertEqual(projection["sqv"], (32.0, 24.0))
        self.assertAlmostEqual(projection["hpx"], 51.06858)
        for actual, expected in zip(hero_anchor(projection, 25075), (-12.5416666667, 12.6875)):
            self.assertAlmostEqual(actual, expected, places=8)

    def test_object_pixel_movement_is_applied_once_to_anchor(self):
        projection = decode_projection_sidecar(encode_projection_sidecar(self.sample_scene()))
        base = hero_anchor(projection, 25075)
        moved = hero_anchor(projection, 25075, 16, -12)
        for actual, expected in zip((moved[0] - base[0], moved[1] - base[1]), (-0.5, -0.25)):
            self.assertAlmostEqual(actual, expected, places=10)
        for actual, expected in zip(moved, (-13.0416666667, 12.4375)):
            self.assertAlmostEqual(actual, expected, places=8)

    def test_art_offsets_define_upright_card_width_and_height(self):
        projection = decode_projection_sidecar(encode_projection_sidecar(self.sample_scene()))
        left, right, up0, up1 = standing_card(projection, 48, 72, -4, 3)
        self.assertAlmostEqual(left, -0.48913987199, places=9)
        self.assertAlmostEqual(right, 0.34938562285, places=9)
        self.assertEqual(up0, 0.0)
        self.assertAlmostEqual(up1, 1.37070582342, places=9)
        odd_left, odd_right, _, _ = standing_card(projection, 47, 72, -4, 3)
        self.assertAlmostEqual(odd_left, -27 / projection["bbPx"], places=12)
        self.assertAlmostEqual(odd_right, 20 / projection["bbPx"], places=12)

    def test_rejects_bad_projection_values_header_and_length(self):
        scene = self.sample_scene()
        scene["proj"]["hpx"] = float("nan")
        with self.assertRaisesRegex(ValueError, "finite"):
            encode_projection_sidecar(scene)
        payload = bytearray(encode_projection_sidecar(self.sample_scene()))
        payload[0] = ord("X")
        with self.assertRaisesRegex(ValueError, "magic/version/header"):
            decode_projection_sidecar(payload)
        with self.assertRaisesRegex(ValueError, "bad length"):
            decode_projection_sidecar(payload[:-1])


class ShadowSidecarTests(unittest.TestCase):
    class _Image:
        def __init__(self, width, height):
            self.shape = (height, width, 4)
            self.pixels = bytes(index % 256 for index in range(width * height * 4))

        def astype(self, kind, copy):
            return self

        def tobytes(self):
            return self.pixels

    class _Object:
        def __init__(self, tile, fid):
            self.tile = tile
            self.fid = fid

    @staticmethod
    def sample_scene():
        return {
            "boards": [[9, 72.5, 37.2, 0, 9, 0]],
            "sprites": [[0, 0, 0, 8, 4, 3, 4]],
            "shadows": [[0, 0, 1, 3, 2]],
        }

    def test_versioned_binary_round_trip(self):
        images = [self._Image(8, 4)]
        objects = [self._Object(1234, 0x0300000B)]
        payload = encode_shadow_sidecar(self.sample_scene(), images, objects)
        self.assertEqual(payload[:4], SHADOW_MAGIC)
        rows = decode_shadow_sidecar(payload)
        self.assertEqual(len(rows), 1)
        row = rows[0]
        self.assertEqual(row["tile"], 1234)
        self.assertEqual(row["fid"], 0x0300000B)
        self.assertEqual(row["frame"], 3)
        self.assertEqual(row["rotation"], 2)
        self.assertEqual((row["width"], row["height"]), (8, 4))
        self.assertEqual((row["ox"], row["oy"]), (3, 4))
        self.assertEqual(row["rgba"], images[0].pixels)

    def test_row_without_a_map_object_of_this_elevation_is_skipped(self):
        payload = encode_shadow_sidecar(self.sample_scene(), [self._Image(8, 4)], [])
        self.assertEqual(decode_shadow_sidecar(payload), [])

    def test_rejects_board_or_sprite_outside_the_scene(self):
        scene = self.sample_scene()
        scene["shadows"][0][0] = 5
        with self.assertRaisesRegex(ValueError, "outside scene.boards"):
            encode_shadow_sidecar(scene, [self._Image(8, 4)], [self._Object(1, 1)])
        scene = self.sample_scene()
        scene["shadows"][0][1] = 5
        with self.assertRaisesRegex(ValueError, "outside scene.sprites"):
            encode_shadow_sidecar(scene, [self._Image(8, 4)], [self._Object(1, 1)])

    def test_rejects_unsupported_version_and_truncation(self):
        payload = encode_shadow_sidecar(self.sample_scene(), [self._Image(8, 4)], [self._Object(1, 1)])
        bad_version = bytearray(payload)
        struct.pack_into("<H", bad_version, 4, SHADOW_VERSION + 1)
        with self.assertRaisesRegex(ValueError, "magic/version"):
            decode_shadow_sidecar(bad_version)
        with self.assertRaisesRegex(ValueError, "truncated"):
            decode_shadow_sidecar(payload[:-1])
        with self.assertRaisesRegex(ValueError, "truncated"):
            decode_shadow_sidecar(payload[:-33])


class ThingShapeTests(unittest.TestCase):
    """Task 075: the pieces of one thing are fitted together, and solids keep out of walls and each other."""

    @staticmethod
    def piece(name, x, y, w, h, tile=1, order=0):
        import numpy as np
        from types import SimpleNamespace
        from vendor.fp import props
        return props.Piece(SimpleNamespace(tile=tile), np.ones((h, w), np.uint8), np.zeros((h, w, 4), np.uint8),
                           x, y, order, name)

    def test_a_detail_inside_a_bigger_piece_of_its_name_joins_it(self):
        from vendor.fp import props
        bed, rail = self.piece("Bed", 0, 0, 80, 60, tile=1), self.piece("Bed", 50, 30, 25, 20, tile=401, order=1)
        crate = self.piece("Crate", 10, 10, 20, 20, tile=3, order=2)
        groups = sorted(props.groups([bed, rail, crate]), key=len)
        self.assertEqual([[crate], [bed, rail]], groups)
        self.assertIs(bed, props.lead(groups[1]))
        apart = self.piece("Bed", 70, 30, 25, 20, tile=402)          # half of it outside the bed: a bed of its own
        self.assertEqual(2, len(props.groups([bed, apart])))

    def scene(self, rows, boxes=()):
        from types import SimpleNamespace
        return SimpleNamespace(props=[list(row) for row in rows], boxes=list(boxes))

    @staticmethod
    def row(round_, cu, cv, a, b, u=None, v=None, z1=0.5):
        return [0, cu if u is None else u, cv if v is None else v, -1, round_, cu, cv, a, b, 0.0, z1, z1, z1, 0, 0, 0]

    def test_two_things_that_overlap_in_plan_are_cut_back_by_half_each(self):
        from vendor.fp import scene
        fake = self.scene([self.row(0, 0.0, 0.0, 1.0, 1.0), self.row(0, 0.8, 0.1, 1.0, 1.0)])
        scene.SceneBuilder.keep_apart(fake, [0, 1])
        game, cut = fake.props[:2], fake.props[2:]
        self.assertEqual([2, 2, 1, 1], [row[15] for row in fake.props])  # the game's camera keeps them as made
        self.assertEqual(1.0, game[0][7])
        first, second = cut
        self.assertAlmostEqual(first[5] + first[7] / 2, second[5] - second[7] / 2, places=3)   # they meet at u = 0.4
        self.assertAlmostEqual(0.4, first[5] + first[7] / 2, places=3)
        self.assertEqual(1.0, first[8])                                  # cut across u only
        slices = self.scene([self.row(0, 0.0, 0.0, 1.0, 1.0), self.row(0, 0.8, 0.1, 1.0, 1.0)])
        scene.SceneBuilder.keep_apart(slices, [0, 0])                          # slices of one thing stay
        held = self.scene([self.row(0, 0.0, 0.0, 2.0, 2.0), self.row(0, 0.1, 0.1, 1.0, 1.0)])
        scene.SceneBuilder.keep_apart(held, [0, 1])                            # one holds the other along u
        big, small = held.props[2:]
        self.assertAlmostEqual(big[5] + big[7] / 2, small[5] - small[7] / 2, places=3)
        self.assertEqual(2, len(slices.props))
        self.assertEqual(1.0, slices.props[0][7])

    def test_a_drum_in_a_wall_keeps_its_hex_side_and_its_near_edge(self):
        from vendor.fp import proj, scene
        radius = 0.5
        fake = self.scene([self.row(1, 0.0, 0.0, radius * proj.BB_PX, radius * proj.BB_PX * 0.5, u=0.0, v=0.4)],
                          boxes=[(-2.0, -0.2, 2.0, 0.1)])                 # a slab along u through the drum's axis
        scene.SceneBuilder.keep_apart(fake, [0])
        cu, cv, a = fake.props[1][5:8]
        r = a / proj.BB_PX
        self.assertAlmostEqual(0.1, cv - r, places=3)                    # on the slab's face
        self.assertAlmostEqual(0.5, cv + r, places=3)                    # its near edge where it was
        self.assertAlmostEqual(0.0, cu, places=3)

    def test_a_cut_solid_is_the_fly_cameras_and_as_made_the_games(self):
        import solids
        faces = [(0, [], False, "both"), (0, [], False, "game"), (0, [], True, "fly")]
        row = self.row(1, 0.0, 0.0, 1.0, 1.0)
        row[15] = 1
        self.assertEqual(["fly", "fly"], [face[3] for face in solids._forced(row, faces)])
        row[15] = 2
        self.assertEqual(["game", "game"], [face[3] for face in solids._forced(row, faces)])
        row[14] = 1                                                      # by name: the game's camera keeps the card
        self.assertEqual([], solids._forced(row, faces))

    def test_a_cut_solid_by_name_keeps_its_axis_for_its_rests(self):
        import solids
        from vendor.fp import scene
        crate = self.row(0, 0.0, 0.0, 1.0, 1.0)
        crate[14] = 1
        fake = self.scene([crate, self.row(0, 0.8, 0.1, 1.0, 1.0)])
        scene.SceneBuilder.keep_apart(fake, [0, 1])
        self.assertEqual([2, 2, 1, 1], [row[15] for row in fake.props])
        self.assertEqual((0.0, 0.0), tuple(fake.props[0][5:7]))       # as made: its rests' axis
        self.assertEqual([], solids._forced(fake.props[0], [(0, [], False, "both")]))
        self.assertAlmostEqual(0.4, fake.props[2][5] + fake.props[2][7] / 2, places=3)

if __name__ == "__main__":
    unittest.main()
