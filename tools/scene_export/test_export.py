"""Focused source-priority regression for the read-only DAT adapter."""

import unittest
import base64
import json
import struct
from pathlib import Path
from tempfile import TemporaryDirectory

from baked_sidecar import (ENTRY as BAKED_ENTRY, HEADER as BAKED_HEADER, LIFT as BAKED_LIFT, MAGIC as BAKED_MAGIC,
                           SKIP as BAKED_SKIP, baked_objects, decode_baked_sidecar, encode_baked_sidecar)
from export import ReadOnlyGameFiles, choose_source, main, patch_paths, validate_output_contents
from floor_sidecar import HEADER, MAGIC, VERSION, cell_content_rect, decode_floor_sidecar, encode_floor_sidecar
from projection_sidecar import (HEADER as PROJECTION_HEADER, MAGIC as PROJECTION_MAGIC,
                                VERSION as PROJECTION_VERSION, decode_projection_sidecar,
                                encode_projection_sidecar, hero_anchor, standing_card)
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
            out = Path(temp)
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
        return {
            "format": 6,
            "map": "kladwtwn",
            "elevation": 0,
            "floor": base64.b64encode(floor).decode("ascii"),
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
        self.assertEqual(len(payload), HEADER.size + 10000 * 2 + 2 * 4)

    def test_cell_coordinates_select_inner_square_and_validate_outer_cell(self):
        self.assertEqual(cell_content_rect((0, 0), (100, 100)), (6, 6, 94, 94))
        scene = self.sample_scene()
        scene["tiles"]["cells"] = [[420, 0], [89, 0]]
        with self.assertRaisesRegex(ValueError, "tile cell 0 is outside"):
            encode_floor_sidecar(scene)

    def test_rejects_scene_for_wrong_map_or_elevation(self):
        scene = self.sample_scene()
        scene["elevation"] = 1
        with self.assertRaisesRegex(ValueError, "kladwtwn elevation 0"):
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
        self.assertEqual(decoded["triangles"], 4)
        batch = decoded["batches"][0]
        self.assertEqual(batch["page"], 0)
        self.assertEqual(batch["size"], (100, 100))
        vertices = batch["vertices"]

        # First front triangle: donor upright() lower crop, with GL x/z offset.
        for actual, expected in zip(vertices[0], (-49.0, 0.0, -48.0, 0.1, 0.4)):
            self.assertAlmostEqual(actual, expected)
        for actual, expected in zip(vertices[1], (-47.0, 0.0, -48.0, 0.2, 0.4)):
            self.assertAlmostEqual(actual, expected)
        # First cap triangle starts after the two front triangles. It is laid back
        # by the exact donor projection ray and samples the top sprite rows.
        cap = vertices[6]
        self.assertAlmostEqual(cap[0], -49.0)
        hpx = self.sample_scene()["proj"]["hpx"]
        self.assertAlmostEqual(cap[1], 2.0 - 10.0 / hpx)
        self.assertAlmostEqual(cap[2], -48.0)
        self.assertAlmostEqual(cap[3], 0.1)
        self.assertAlmostEqual(cap[4], 0.3)
        far = vertices[8]
        projection = self.sample_scene()["proj"]
        ray = [projection["sqv"][0], -projection["squ"][0],
               (projection["sqv"][0] * projection["squ"][1] -
                projection["squ"][0] * projection["sqv"][1]) / hpx]
        length = sum(component * component for component in ray) ** 0.5
        toward = [component / length for component in ray]
        far_distance = (2.0 - (2.0 - 10.0 / hpx)) / toward[2]
        self.assertAlmostEqual(far[0], 3.0 - toward[0] * far_distance - 50.0, places=5)
        self.assertAlmostEqual(far[2], 2.0 - toward[1] * far_distance - 50.0, places=5)
        # A front vertex takes its own depth; the far edge of the top takes that of
        # the top edge of the front plane it was painted on (donor wallFront home).
        self.assertEqual(vertices[0][5:], vertices[0][:3])
        for actual, expected in zip(vertices[8][5:], (-47.0, 2.0, -48.0)):
            self.assertAlmostEqual(actual, expected, places=5)
        for actual, expected in zip(vertices[11][5:], (-49.0, 2.0, -48.0)):
            self.assertAlmostEqual(actual, expected, places=5)
        # No back face (sprite 1), slab end or door: the game's camera does not see
        # backs and ends, and the engine draws doors live so they open.
        self.assertEqual(len(vertices), 12)

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
        for vertex in vertices:
            x, y, z, s_, t = vertex[:5]
            sx, sy = px(x + 50.0, z + 50.0, y)
            self.assertAlmostEqual(s_ * 100.0, 40 + sx - ax + 25, places=3)
            self.assertAlmostEqual(t * 100.0, 40 + sy - ay + 40, places=3)
            # Depth: the card's hex row plane, a hair nearer, along the game's ray.
            dx, dy, dz = (vertex[5 + k] - vertex[k] for k in range(3))
            self.assertAlmostEqual(vertex[7] + 50.0, plane, places=4)
            self.assertAlmostEqual(dx * toward[1] - dz * toward[0], 0.0, places=4)
            self.assertAlmostEqual(dy * toward[1] - dz * toward[2], 0.0, places=4)
        # Triangles face the game's camera: counter-clockwise on its screen, y up.
        for index in range(0, len(vertices), 3):
            a, b, c = (px(v[0] + 50.0, v[2] + 50.0, v[1]) for v in vertices[index:index + 3])
            area = (b[0] - a[0]) * (-(c[1] - a[1])) - (c[0] - a[0]) * (-(b[1] - a[1]))
            self.assertGreater(area, 0.0)

        # A drum: sixteen sides and a top (the sides away from the camera
        # repeat the facing quarter mirrored, so they are not checked here).
        # It stands for several cards (board -1), so it keeps its own depth.
        scene["props"] = [[2, 20.0, 20.0, -1, 1, 20.0, 20.0, 9.0, 6.0, 0.0, 0.4, 0.0, 0.0]]
        drum = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertGreater(drum["triangles"], 16)
        for vertex in drum["batches"][0]["vertices"]:
            self.assertEqual(vertex[5:], vertex[:3])

    def test_first_person_surfaces_are_left_out(self):
        scene = self.sample_scene()
        scene["walls"][0].extend([1, 1])
        scene["trims"] = [[0, 1.0, 2.0, 3.0, 2.0, 0.0, 1.0, 0, 0, 1, 1]]
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertEqual(decoded["triangles"], 4)

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
        struct.pack_into("<I", bad_count, 16, 9)
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

    def test_rejects_other_map_and_out_of_atlas_uv(self):
        scene = self.sample_scene()
        scene["map"] = "arroyo"
        with self.assertRaisesRegex(ValueError, "kladwtwn elevation 0"):
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


if __name__ == "__main__":
    unittest.main()
