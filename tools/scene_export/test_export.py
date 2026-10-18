"""Focused source-priority regression for the read-only DAT adapter."""

import unittest
import base64
import json
import struct
from pathlib import Path
from tempfile import TemporaryDirectory

from export import ReadOnlyGameFiles, choose_source, main, patch_paths, validate_output_contents
from floor_sidecar import HEADER, MAGIC, VERSION, cell_content_rect, decode_floor_sidecar, encode_floor_sidecar
from projection_sidecar import (HEADER as PROJECTION_HEADER, MAGIC as PROJECTION_MAGIC,
                                VERSION as PROJECTION_VERSION, decode_projection_sidecar,
                                encode_projection_sidecar, hero_anchor, standing_card)
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
                         "tiles.png", "light.png", "rock.png", "sprites10.png"):
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

    def test_encodes_donor_front_cap_back_and_door_geometry(self):
        decoded = decode_wall_sidecar(encode_wall_sidecar(self.sample_scene()))
        self.assertEqual(decoded["version"], WALL_VERSION)
        self.assertEqual(decoded["triangles"], 8)
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
        # Back surface uses back sprite 1 and the donor's reversed endpoint order.
        back = vertices[12]
        for actual, expected in zip(back, (-47.0, 0.0, -47.75, 0.4, 0.6)):
            self.assertAlmostEqual(actual, expected)
        # Door is the final quad, emitted as a static upright sheet.
        door = vertices[18]
        for actual, expected in zip(door, (-45.0, 0.0, -44.0, 0.1, 0.4)):
            self.assertAlmostEqual(actual, expected)

    def test_rejects_invalid_sprite_index_and_bad_page(self):
        scene = self.sample_scene()
        scene["doors"][0][0] = 2
        with self.assertRaisesRegex(ValueError, "outside scene.sprites"):
            encode_wall_sidecar(scene)
        scene = self.sample_scene()
        scene["sprites"][0][0] = 2
        with self.assertRaisesRegex(ValueError, "outside scene.pages"):
            encode_wall_sidecar(scene)

    def test_optional_above_and_above_back_surfaces_are_emitted(self):
        scene = self.sample_scene()
        scene["walls"][0].extend([1, 1])
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertEqual(decoded["triangles"], 12)
        vertices = decoded["batches"][0]["vertices"]
        above = vertices[18]
        self.assertAlmostEqual(above[0], -49.0)
        self.assertAlmostEqual(above[1], scene["walls"][0][9])
        self.assertAlmostEqual(above[3], 0.3)
        above_back = vertices[24]
        self.assertAlmostEqual(above_back[0], -47.0)
        self.assertAlmostEqual(above_back[2], -47.75)
        self.assertAlmostEqual(above_back[3], 0.4)

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
