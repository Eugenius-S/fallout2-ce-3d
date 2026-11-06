"""Focused source-priority regression for the read-only DAT adapter."""

import unittest
import base64
import gzip
import json
import struct
from pathlib import Path
from tempfile import TemporaryDirectory

import numpy as np
from PIL import Image

from baked_sidecar import (ENTRY as BAKED_ENTRY, GL_MAP_OFFSET, HEADER as BAKED_HEADER, LIFT as BAKED_LIFT,
                           MAGIC as BAKED_MAGIC,
                           NOCAST as BAKED_NOCAST, SKIP as BAKED_SKIP, FLAT as BAKED_FLAT, FLY_FLAT as BAKED_FLY_FLAT,
                           FIXED_CARD as BAKED_FIXED_CARD, FLY_SKIP as BAKED_FLY_SKIP, PLANE as BAKED_PLANE,
                           baked_objects, decode_baked_sidecar, encode_baked_sidecar)
from export import ReadOnlyGameFiles, choose_source, main, parse_args, patch_paths, scene_directory, validate_output_contents
from floor_sidecar import HEADER, MAGIC, VERSION, cell_content_rect, decode_floor_sidecar, encode_floor_sidecar
from roof_fringe import (boundary_edges, components_of, donor_tiles, inner_edge, cut_to_walls, own_strip, donate, foot_cover, lay_donor, roof_sides, whole_sides, BELT_LAP, BELT_ROWS, EDGE_ROWS, STRIP_ROWS, belt_art, compose_band, is_fringe, outward, resolve_runs, runs_of,
                         split_tile, tidy_cell, unproject_cell, unroll, wall_under, write_fringe_art)
import roof_edge_check
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
                         "baked.bin", "tiles.png", "light.png", "rock.png", "roof_fringe.png", "sprites10.png"):
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
        self.assertEqual(decoded["version"], 6)  # no water flags given: stays version 6
        self.assertEqual(decoded["grid_size"], (100, 100))
        self.assertEqual(decoded["image_size"], (512, 512))
        self.assertEqual(decoded["cells"], [(0, 0), (89, 0)])
        self.assertEqual(decoded["floor"][:2], (1, 2))
        self.assertEqual(decoded["roof"][:3], (0, 0, 2))
        self.assertEqual(decoded["fringe"], [])
        self.assertEqual(decoded["patches"], [])
        self.assertEqual(len(payload), HEADER.size + 2 * 10000 * 2 + 2 * 4 + 2 + 2)

    def test_roof_fringe_list_round_trips_and_is_validated(self):
        run = (1, 20, 10, 3, 1, 37, 144, 40, 20.125, 1.875, 10.0, 13.0)
        payload = encode_floor_sidecar(self.sample_scene(), [run])
        self.assertEqual(decode_floor_sidecar(payload)["fringe"], [run])
        with self.assertRaisesRegex(ValueError, "roof fringe"):
            encode_floor_sidecar(self.sample_scene(), [(1, 20, 99, 3, 1, 37, 144, 40, 20.125, 1.875, 99.0, 102.0)])
        with self.assertRaisesRegex(ValueError, "truncated"):
            decode_floor_sidecar(payload[:-1])

    def test_roof_patch_list_round_trips_and_is_validated(self):
        patches = [(2, 1, 40)]
        payload = encode_floor_sidecar(self.sample_scene(), (), patches)
        self.assertEqual(decode_floor_sidecar(payload)["patches"], patches)
        with self.assertRaisesRegex(ValueError, "patch"):
            encode_floor_sidecar(self.sample_scene(), (), [(0, 1, 40)])
        with self.assertRaisesRegex(ValueError, "truncated"):
            decode_floor_sidecar(payload[:-1])

    def test_water_flags_round_trip_and_stay_version_6_without_them(self):
        scene = self.sample_scene()
        plain = encode_floor_sidecar(scene)
        self.assertEqual(decode_floor_sidecar(plain)["version"], 6)
        self.assertIsNone(decode_floor_sidecar(plain)["water"])
        water = bytearray(100 * 100)
        water[0] = 1
        water[1] = 2
        water[9999] = 1
        payload = encode_floor_sidecar(scene, water=water, water_color=[0.04, 0.047, 0.045])
        decoded = decode_floor_sidecar(payload)
        self.assertEqual(decoded["version"], VERSION)
        self.assertEqual(decoded["water"][0], 1)
        self.assertEqual(decoded["water"][1], 2)
        self.assertEqual(decoded["water"][2], 0)
        self.assertEqual(decoded["water"][9999], 1)
        for got, want in zip(decoded["waterColor"], (10, 12, 11)):
            self.assertEqual(got, want / 255)
        self.assertEqual(len(payload), len(plain) + 3 + 100 * 100)
        with self.assertRaisesRegex(ValueError, "water flags"):
            encode_floor_sidecar(scene, water=bytearray(99 * 99), water_color=[0.1, 0.1, 0.1])
        with self.assertRaisesRegex(ValueError, "water flags"):
            encode_floor_sidecar(scene, water=b"\x03" + bytearray(100 * 100 - 1), water_color=[0.1, 0.1, 0.1])
        with self.assertRaisesRegex(ValueError, "water colour"):
            encode_floor_sidecar(scene, water=water, water_color=[0.1, 0.1])
        with self.assertRaisesRegex(ValueError, "truncated"):
            decode_floor_sidecar(payload[:-1])

    def test_version_5_floor_lists_still_decode_without_patches(self):
        payload = bytearray(encode_floor_sidecar(self.sample_scene())[:-2])  # no patch count: version 5 had none
        struct.pack_into("<H", payload, 4, 5)
        decoded = decode_floor_sidecar(bytes(payload))
        self.assertEqual((decoded["version"], decoded["fringe"], decoded["patches"]), (5, [], []))

    def test_tileless_elevation_round_trips_with_an_empty_atlas(self):
        # encfite's void boxes: every square is the nothing tile (the lookup's
        # 0), the atlas holds no cell, the sidecar stays valid (171).
        scene = self.sample_scene()
        scene["floor"] = base64.b64encode(bytes(100 * 100 * 2)).decode("ascii")
        scene["roof"] = base64.b64encode(bytes(100 * 100 * 2)).decode("ascii")
        scene["tiles"]["cells"] = []
        payload = encode_floor_sidecar(scene, water=bytearray(100 * 100), water_color=[0.05, 0.05, 0.05])
        decoded = decode_floor_sidecar(payload)
        self.assertEqual(decoded["version"], VERSION)
        self.assertEqual(decoded["cells"], [])
        self.assertEqual(decoded["water"], [0] * 100 * 100)
        # one drawn square over the empty atlas is still a broken scene
        drawn = self.sample_scene()
        drawn["tiles"]["cells"] = []
        with self.assertRaisesRegex(ValueError, "missing tiles.cells entry"):
            encode_floor_sidecar(drawn, water=bytearray(100 * 100), water_color=[0.05, 0.05, 0.05])
        # the plain form without water stays version 6 and round-trips too
        plain = encode_floor_sidecar(scene)
        self.assertEqual(decode_floor_sidecar(plain)["version"], 6)

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


class RoofFringeTests(unittest.TestCase):
    @staticmethod
    def art(painted, colour=(10, 12, 8)):
        rgba = np.zeros((36, 80, 4), np.uint8)
        for y, x in painted:
            rgba[y, x] = (*colour, 255)
        return rgba

    def test_sparse_dark_ragged_growth_is_a_fringe(self):
        # blades: a few thin strokes of a dark colour on a mostly empty tile
        blades = [(y, x) for x in range(30, 60, 5) for y in range(8, 20 + x % 7)]
        self.assertTrue(is_fringe(self.art(blades)))

    def test_whole_roofs_solid_strips_metal_and_light_growth_are_not(self):
        ys, xs = np.mgrid[0:36, 0:80]
        u, v = donor_proj.delta_to_ground(xs + 0.5 - 48, ys + 0.5)
        diamond = [(y, x) for y, x in zip(*np.nonzero((u >= 0) & (u < 1) & (v >= 0) & (v < 1)))]
        self.assertFalse(is_fringe(self.art(diamond)))                      # a whole roof tile
        strip = [(y, x) for y in range(14, 20) for x in range(10, 70)]
        self.assertFalse(is_fringe(self.art(strip)))                        # a solid trim strip
        blades = [(y, x) for x in range(30, 60, 5) for y in range(8, 20 + x % 7)]
        self.assertFalse(is_fringe(self.art(blades, (120, 130, 110))))      # bright: a metal roof with holes
        self.assertFalse(is_fringe(self.art([])))

    def test_art_image_and_entries(self):
        blades = [(y, x) for x in range(30, 60, 5) for y in range(8, 20 + x % 7)]
        with TemporaryDirectory() as temp:
            png = Path(temp) / "roof_fringe.png"
            # squares 101, 102 (a run of two) and 105 (another): the same tile art on one wall line
            entries, patches = write_fringe_art(
                png, {101: 7, 102: 7, 103: 8, 105: 7}, lambda tile: self.art(blades if tile == 7 else []),
                lambda edge, plane, start, count: (plane - 0.2, plane + 0.1, 1.88, float(start), float(start + count), []))
            self.assertTrue(entries)
            self.assertEqual(patches, [])
            for edge, plane, start, count, x, y, width, height, face, top, lo, hi in entries:
                self.assertIn((start, count), [(1, 2), (5, 1), (1, 1)])  # 103 is no fringe: the runs break there
                self.assertIn(round(face - plane, 3), (-0.2, 0.1))        # the wall's outer face, one of its two sides
                self.assertEqual((top, lo, hi), (1.88, start, start + count))
                self.assertEqual((width, height), (count * (-donor_proj.SQ_U[0] if edge < 2 else donor_proj.SQ_V[0]), BELT_ROWS + EDGE_ROWS))
            self.assertEqual(Image.open(png).height, sum(entry[7] + 1 for entry in entries) + 1)
            self.assertEqual(write_fringe_art(png, {101: 8}, lambda tile: self.art([]), lambda *run: None), ([], []))
            self.assertFalse(png.exists())

    def test_the_belt_continues_the_walls_art_and_the_edge_stands_on_it(self):
        # one wall row along u at v = 3 (edge 0 of squares 3, 4 of row 3), its picture 96 x 60 on a page,
        # a striped picture: column c is grey 10 + c, the top band (6 rows) above the cap is red
        page = np.zeros((60, 96, 4), np.uint8)
        page[..., :3] = (10 + np.arange(96))[None, :, None]
        page[..., 3] = 255
        page[:6, :, :3] = (250, 0, 0)
        walls = [[0, 3.0, 3.0, 5.0, 3.0, 0.0, 2.0, 0, 0, 2.0 - 6 / 96.0, 0.2, -1]]
        sprites = [[0, 0, 0, 96, 60]]
        run = (0, 3, 3, [(303, None), (304, None)])
        belt = belt_art([0], walls, sprites, [page], 96.0, run)
        self.assertEqual(belt.shape, (BELT_ROWS + BELT_LAP, 96, 4))
        self.assertTrue((belt[..., 3] == 255).all())
        # u grows to the left on the screen, so the strip's column 0 is the wall's end (u = 5): the picture's b end
        self.assertEqual(int(belt[-1, 0, 0]), 10 + 95)
        self.assertEqual(int(belt[-1, 95, 0]), 10)
        self.assertEqual(belt_art([], walls, sprites, [page], 96.0, run), None)
        strip = np.zeros((STRIP_ROWS, 96, 4), np.uint8)
        strip[-3:, ::2] = (1, 2, 3, 255)
        band = compose_band(strip, belt)
        self.assertEqual(band.shape, (BELT_ROWS + EDGE_ROWS, 96, 4))
        self.assertTrue((band[EDGE_ROWS:, :, 3] == 255).all())      # the belt is solid
        self.assertEqual(tuple(band[EDGE_ROWS - 1, 0, :3]), (1, 2, 3))  # the strip's outline over the belt's lap rows
        self.assertEqual(band[0, :, 3].max(), 0)                    # nothing above the ragged edge

    def test_a_run_stands_on_the_outer_face_of_the_wall_under_its_roof_edge(self):
        class Slab:
            def __init__(self, axis, plane, side, thick, rows, lo, hi):
                self.axis, self.plane, self.side, self.thick, self.rows, self.lo, self.hi = axis, plane, side, thick, rows, lo, hi

            def band(self):
                return min(self.plane, self.plane + self.side * self.thick), max(self.plane, self.plane + self.side * self.thick)

        # rows: [sprite, ua, va, ub, vb, z0, z1, _, _, zc]; a wall along u on v = 9.875, its body up to 10.09
        walls = [[0, 2.0, 9.875, 6.0, 9.875, 0, 2.0, 0, 0, 1.88]]
        slabs = [Slab(0, 9.875, 1.0, 0.215, [0], 2.0, 6.0), Slab(0, 14.0, 1.0, 0.215, [], 2.0, 6.0)]
        # edge 1 (roof below v = 10): the outer face is the high side
        self.assertEqual(wall_under(slabs, walls, 1, 10, 3, 2), (10.09, 1.88, 3.0, 5.0))
        # edge 0 (roof above v = 10): the outer face is the low side
        self.assertEqual(wall_under(slabs, walls, 0, 10, 3, 2), (9.875, 1.88, 3.0, 5.0))
        # a wall a tile away, one along the other axis, or none under the run: no band
        self.assertIsNone(wall_under(slabs, walls, 1, 11, 3, 2))
        self.assertIsNone(wall_under(slabs, walls, 3, 10, 3, 2))
        self.assertIsNone(wall_under(slabs, walls, 1, 10, 7, 2))

    def test_a_band_that_is_not_genuine_takes_its_roofs_donor_edge(self):
        # a genuine strip: concrete along the bottom rows, a ragged top over it
        def strip_of(count, per_tile, upside_down=False):
            strip = np.zeros((STRIP_ROWS, count * per_tile, 4), np.uint8)
            strip[-3:] = (30, 30, 30, 255)
            strip[-EDGE_ROWS:-3, ::3] = (200, 0, 0, 255)  # rebar
            return strip[::-1].copy() if upside_down else strip

        u_tile, v_tile = int(-donor_proj.SQ_U[0]), int(donor_proj.SQ_V[0])
        self.assertGreaterEqual(foot_cover(strip_of(2, u_tile)), 0.99)
        self.assertLess(foot_cover(strip_of(2, u_tile, upside_down=True)), 0.5)  # the straight edge on top: not genuine
        art = np.zeros((2, 2, 4), np.uint8)
        mk = lambda edge, plane, start, squares: (edge, plane, start, [(i, art) for i in squares])
        runs = [mk(1, 11, 3, [103, 104]),                 # genuine, roof A (squares 103-104 and 203 are one footprint)
                mk(3, 4, 3, [203, 303, 403]),             # slab-cut, roof A: takes A's donor
                mk(0, 51, 7, [5107, 5108])]               # not genuine, roof B: A is far, B has none: the map's donor
        strips = [strip_of(2, u_tile), strip_of(3, v_tile, upside_down=True), strip_of(2, u_tile, upside_down=True)]
        roof = {103: 1, 104: 1, 203: 1, 303: 1, 403: 1, 5107: 1, 5108: 1}
        comp = components_of(roof)
        self.assertEqual(comp[103], comp[403])
        self.assertNotEqual(comp[103], comp[5107])
        pool = [(runs[0], strips[0], comp[103])]
        out = donate(runs, strips, [True, False, False], pool, comp)
        self.assertTrue(np.array_equal(out[0], strips[0]))
        for n, (run, per_tile) in enumerate(((runs[1], v_tile), (runs[2], u_tile)), start=1):
            width = len(run[3]) * per_tile
            self.assertEqual(out[n].shape, (STRIP_ROWS, width, 4))
            self.assertEqual(out[n][:-EDGE_ROWS, :, 3].max(), 0)             # nothing above the donor's height
            self.assertGreaterEqual(foot_cover(out[n]), 0.99)                # ragged on top, concrete along the bottom
            self.assertTrue((out[n][-3:, :, :3] == 30).all())
            self.assertTrue((out[n][-EDGE_ROWS:-3, 0:width:3, 0] == 200).sum() > 0)  # the donor's rebar, repeated along the run
        # a run's strip made of the donor repeats it: shifted by the donor's length it is the same
        repeat = lay_donor((strips[0], 2, 1), 1, 6)
        self.assertTrue((repeat[:, :2 * u_tile] == repeat[:, 2 * u_tile:4 * u_tile]).all())

    def test_genuine_bands_on_one_roof_share_donor_height(self):
        u_tile, v_tile = int(-donor_proj.SQ_U[0]), int(donor_proj.SQ_V[0])

        def strip(count, per_tile, step):
            value = np.zeros((STRIP_ROWS, count * per_tile, 4), np.uint8)
            value[-3:] = (30, 30, 30, 255)
            value[-EDGE_ROWS:-3:step] = (200, 0, 0, 255)
            return value

        art = np.zeros((2, 2, 4), np.uint8)
        long_run = (1, 11, 3, [(103, art), (104, art), (105, art)])
        short_run = (3, 14, 3, [(305, art), (405, art)])
        donor = strip(3, u_tile, 2)
        other = strip(2, v_tile, 5)
        donor_profile = (donor[..., 3] > 0).mean(axis=1)
        other_profile = (other[..., 3] > 0).mean(axis=1)
        self.assertFalse(np.array_equal(donor_profile, other_profile))

        out = donate([long_run, short_run], [donor, other], [True, True],
            [(long_run, donor, 1), (short_run, other, 1)], {103: 1, 305: 1})
        first_profile = (out[0][..., 3] > 0).mean(axis=1)
        second_profile = (out[1][..., 3] > 0).mean(axis=1)
        self.assertTrue(np.array_equal(first_profile, second_profile))
        self.assertGreaterEqual(foot_cover(out[0]), 0.99)
        self.assertGreaterEqual(foot_cover(out[1]), 0.99)

    def test_shared_edge_profile_uses_the_component_median(self):
        u_tile = int(-donor_proj.SQ_U[0])
        art = np.zeros((2, 2, 4), np.uint8)
        runs = [(edge, 10 + edge, 3, [(103 + edge * 100, art), (104 + edge * 100, art)])
                for edge in (0, 1, 2)]

        def strip(dense):
            value = np.zeros((STRIP_ROWS, 2 * u_tile, 4), np.uint8)
            value[-3:] = (30, 30, 30, 255)
            if dense:
                value[-EDGE_ROWS:-3] = (200, 0, 0, 255)
            else:
                value[-EDGE_ROWS:-3:3] = (200, 0, 0, 255)
            return value

        strips = [strip(False), strip(False), strip(True)]
        comp = {index: 1 for run in runs for index, _ in run[3]}
        pool = [(run, value, 1) for run, value in zip(runs, strips)]
        out = donate(runs, strips, [True] * 3, pool, comp)
        sparse_profile = (strips[0][-EDGE_ROWS:, :, 3] > 0).mean(axis=1)
        out_profile = (out[0][-EDGE_ROWS:, :, 3] > 0).mean(axis=1)
        self.assertTrue(np.array_equal(out_profile, sparse_profile))

    def test_a_band_stands_only_over_squares_with_a_wall_under_it(self):
        art = np.zeros((2, 2, 4), np.uint8)
        runs = [(0, 5, 3, [(503 + i, art) for i in range(6)])]
        wall_of = lambda edge, plane, start, count: None if start in (5, 6) else (0.0, 1.0, 1.88, 0.0, 99.0, [])
        out, strips, good = cut_to_walls(runs, [object()], [True], wall_of)
        self.assertEqual([(run[2], len(run[3])) for run in out], [(3, 2), (7, 2)])   # broken where the wall is broken
        self.assertEqual((strips, good), ([None, None], [False, False]))
        # a wall under all of it leaves the run as it is
        out, strips, good = cut_to_walls(runs, ['s'], [True], lambda *run: (0.0, 1.0, 1.88, 0.0, 99.0, []))
        self.assertEqual((out, strips, good), (runs, ['s'], [True]))
        # none under it: no band
        self.assertEqual(cut_to_walls(runs, ['s'], [True], lambda *run: None)[0], [])

    def test_opposite_sides_of_a_one_square_deep_roof_stay_two_runs(self):
        art = np.zeros((2, 2, 4), np.uint8)
        roof = {500 + x: 1 for x in range(3, 7)}
        open_of = {index: [0, 1] for index in roof}
        sides = roof_sides(roof, open_of)
        runs = [(0, 5, 3, [(i, art) for i in (503, 504, 505, 506)]), (1, 6, 3, [(i, art) for i in (503, 504, 505, 506)])]
        out, _, _ = whole_sides(runs, [None, None], [False, False], sides)
        self.assertEqual(sorted((run[0], run[1], run[2], len(run[3])) for run in out), [(0, 5, 3, 4), (1, 6, 3, 4)])

    def test_a_merged_run_keeps_its_art_and_falls_back_on_it_without_a_donor(self):
        art = np.zeros((36, 96, 4), np.uint8)
        art[20:30, 40:60] = 255
        roof = {500 + x: 1 for x in range(3, 6)}
        sides = roof_sides(roof, {index: [0] for index in roof})
        out, strips, _ = whole_sides([(0, 5, 3, [(503, art)])], [None], [False], sides)
        self.assertIs(out[0][3][0][1], art)
        self.assertIsNone(out[0][3][1][1])
        self.assertTrue(own_strip(out[0])[..., 3].any())
        self.assertTrue(donate(out, strips, [False], [], components_of(roof))[0][..., 3].any())

    def test_a_plain_square_a_band_crosses_keeps_its_roof_as_a_patch(self):
        blades = [(y, x) for x in range(30, 60, 5) for y in range(8, 20 + x % 7)]
        with TemporaryDirectory() as temp:
            plain = self.art([(y, x) for y in range(36) for x in range(80)], (120, 130, 110))
            roof = {101: 7, 102: 8, 103: 8, 201: 8, 202: 8, 203: 8, 301: 8, 302: 8, 303: 8}
            entries, patches = write_fringe_art(
                Path(temp) / "roof_fringe.png", roof, lambda tile: self.art(blades) if tile == 7 else plain,
                lambda edge, plane, start, count: (plane - 0.2, plane + 0.1, 1.88, float(start), float(start + count), []))
            crossed = {index for entry in entries for index in roof_edge_check.run_squares(entry)}
            self.assertTrue({201, 301} <= crossed)                              # the plain squares are in a band ...
            self.assertTrue({201, 301} <= {index for index, _, _ in patches})   # ... and keep their flat roof

    def test_a_band_faces_the_roof_across_the_edge_away_from_the_outside(self):
        # the outside past v = face (side 1): the roof is across edge 0 (dz - 1); a ring square open there is no roof
        self.assertEqual([inner_edge(clip) for clip in ((0, 0.9, 1), (0, 0.1, -1), (1, 0.9, 1), (1, 0.1, -1))], [0, 1, 2, 3])

    def test_a_roof_with_no_plain_tile_has_no_donor(self):
        plain = self.art([(y, x) for y in range(36) for x in range(80)], (120, 130, 110))
        blades = self.art([(y, x) for x in range(30, 60, 5) for y in range(8, 20 + x % 7)])
        roof = {101: 8, 102: 8, 201: 8, 202: 8, 105: 7, 106: 7}
        art_of = lambda tile: blades if tile == 7 else plain
        comp = components_of(roof)
        open_of = boundary_edges(roof)
        donors = donor_tiles(roof, open_of, set(), comp, art_of)
        self.assertNotIn(comp[105], donors)   # a ruin borrows no roof from its neighbour
        self.assertEqual(donors[comp[101]], 8)
        self.assertEqual(donor_tiles({105: 7, 106: 7}, boundary_edges({105: 7, 106: 7}), set(), components_of({105: 7, 106: 7}), art_of), {})

    def test_a_roof_whose_runs_are_all_cut_away_keeps_its_tidied_patches(self):
        blades = [(y, x) for x in range(30, 60, 5) for y in range(8, 20 + x % 7)]
        with TemporaryDirectory() as temp:
            png = Path(temp) / "roof_fringe.png"
            plain = self.art([(y, x) for y in range(36) for x in range(80)], (120, 130, 110))
            roof = {101: 7, 102: 8, 103: 8, 201: 8, 202: 8, 203: 8, 301: 8, 302: 8, 303: 8}
            art_of = lambda tile: self.art(blades) if tile == 7 else plain
            with_wall = write_fringe_art(
                png, roof, art_of,
                lambda edge, plane, start, count: (plane - 0.2, plane + 0.1, 1.88, float(start), float(start + count), []))
            self.assertTrue(with_wall[0])
            entries, patches = write_fringe_art(png, roof, art_of, lambda *run: None)   # no wall stands under any run
            self.assertEqual(entries, [])                                               # so no band ...
            self.assertTrue(patches)                                                    # ... but the split roof is still a patch
            self.assertTrue(png.exists())
            sheet = np.asarray(Image.open(png).convert("RGBA"))
            for index, x, y in patches:
                self.assertTrue(x + 100 <= sheet.shape[1] and y + 100 <= sheet.shape[0])
            self.assertEqual(write_fringe_art(png, {}, art_of, lambda *run: None), ([], []))
            self.assertFalse(png.exists())                                              # nothing at all: no image

    def test_a_footprint_side_that_is_not_one_genuine_run_becomes_one_run(self):
        art = np.zeros((2, 2, 4), np.uint8)
        # a roof of squares x 3..6 on row 5: its north side (edge 0, line v = 5) is 4 squares along u
        roof = {5 * 100 + x: 1 for x in range(3, 7)}
        open_of = {index: [0] for index in roof}
        sides = roof_sides(roof, open_of)
        self.assertIn((0, 5, 3, [503, 504, 505, 506]), sides)
        fragments = [(1, 6, 3, [(503, art)]), (0, 5, 5, [(505, art), (506, art)])]
        strips = [np.zeros((STRIP_ROWS, 48, 4), np.uint8)] * 2
        runs, out, good = whole_sides(fragments, strips, [False, False], sides)
        self.assertEqual([(run[0], run[1], run[2], len(run[3])) for run in runs if run[0] == 0], [(0, 5, 3, 4)])
        self.assertEqual(len(runs), 1)
        self.assertEqual((out, good), ([None], [False]))
        # one genuine run over the whole side stays as it is
        whole = [(1, 6, 3, [(i, art) for i in (503, 504, 505, 506)])]
        runs, out, good = whole_sides(whole, [strips[0]], [True], sides)
        self.assertEqual((runs, good), (whole, [True]))

    def test_bands_that_meet_at_a_corner_end_on_each_others_faces_and_share_a_top(self):
        art = np.zeros((2, 2, 4), np.uint8)
        # a band along u on the face v = 10.1 over x 3..6, one along v on the face u = 2.9 over z 10..13: the art
        # names the other edges (1 and 3), the corner is where the faces cross, both ends are short of it
        runs = [(1, 11, 3, [(i, art) for i in range(3)]), (3, 3, 10, [(i, art) for i in range(3)])]
        bodies = [None, None]
        places = resolve_runs(runs, bodies, [None, None], 1.88)
        places = [list(place) for place in places]
        # no wall: the faces are the tile lines 11 and 3: the u band's low end meets the v band's face
        self.assertAlmostEqual(places[0][2], 3.0)
        self.assertEqual(places[0][1], places[1][1])

    def test_pieces_of_one_wall_line_that_follow_each_other_are_one_run(self):
        art = np.zeros((36, 80, 4), np.uint8)
        # edge 1 of squares (3, 4), (4, 4), (6, 4): the line v = 5; edge 0 of (4, 5) lies on it too
        runs = runs_of({403: [(1, art)], 404: [(1, art)], 406: [(1, art)], 504: [(0, art)]})
        self.assertEqual([(edge, plane, start, len(members)) for edge, plane, start, members in runs],
                         [(0, 5, 4, 1), (1, 5, 3, 2), (1, 5, 6, 1)])

    def test_unrolled_strip_lays_the_wall_line_flat(self):
        # a slab of ground (u, v) = (0..1, 0..0.2) on square 0, a wall along u at v = 0 (edge 0)
        ys, xs = np.mgrid[0:36, 0:80]
        u, v = donor_proj.delta_to_ground(xs + 0.5 - 48, ys + 0.5)
        art = np.zeros((36, 80, 4), np.uint8)
        art[(u >= 0) & (u < 1) & (v >= 0) & (v < 0.2)] = (10, 12, 8, 255)
        strip = unroll(0, 0, 0, [(0, art)])
        self.assertEqual(strip.shape, (STRIP_ROWS, 48, 4))
        rows = np.nonzero(strip[..., 3].any(axis=1))[0]
        # the slab is one band along the bottom of the strip, as thick as its depth on the screen
        self.assertEqual(rows[-1], STRIP_ROWS - 1)
        self.assertLessEqual(len(rows), int(0.2 * 32) + 2)
        self.assertEqual(len(rows), rows[-1] - rows[0] + 1)


class RoofEdgeTests(unittest.TestCase):
    @staticmethod
    def tile(parts):
        """An 80 x 36 iso tile: parts is [(u0, u1, v0, v1, rgb)] of the ground square, painted in order."""
        ys, xs = np.mgrid[0:36, 0:80]
        u, v = donor_proj.delta_to_ground(xs + 0.5 - 48, ys + 0.5)
        rgba = np.zeros((36, 80, 4), np.uint8)
        for u0, u1, v0, v1, rgb in parts:
            rgba[(u >= u0) & (u < u1) & (v >= v0) & (v < v1)] = (*rgb, 255)
        return rgba

    def test_a_whole_shingle_tile_is_a_roof_and_a_grey_top_is_a_wall_top(self):
        pieces, roof = split_tile(self.tile([(0, 1, 0, 1, (120, 80, 50))]))
        self.assertEqual(pieces, [])
        pieces, roof = split_tile(self.tile([(0, 1, 0.6, 1, (120, 80, 50)), (0, 1, 0, 0.3, (14, 14, 14))]))
        self.assertTrue(pieces)                                        # the grey strip stands up
        self.assertTrue(roof is not None and (roof[..., 3] > 0).any())  # the shingle part stays a roof
        grey = (np.abs(roof[..., 0].astype(int) - roof[..., 2]) < 6) & (roof[..., 3] > 0)
        self.assertFalse(grey.any())                                   # none of the dark part is left on the roof
        self.assertEqual(unproject_cell(roof).shape[:2], (100, 100))

    def test_an_unsupported_partial_grey_roof_tile_stays_flat(self):
        partial = self.tile([(0, 1, 0, 0.3, (14, 14, 14))])
        pieces, roof = split_tile(partial, (0, 1, 2, 3), wall_supported=False)
        self.assertEqual(pieces, [])
        self.assertIs(roof, partial)

        with TemporaryDirectory() as temp:
            png = Path(temp) / "roof_fringe.png"
            entries, patches = write_fringe_art(png, {101: 7}, lambda _: partial,
                                                lambda *run: None)
            self.assertEqual(entries, [])
            self.assertEqual(patches, [])
            self.assertFalse(png.exists())

    def test_a_whole_painted_tile_with_a_grey_slab_on_an_open_edge_stands_up_there(self):
        whole = self.tile([(0, 1, 0, 1, (120, 80, 50)), (0, 1, 0, 0.36, (14, 14, 14))])
        self.assertEqual(split_tile(whole)[0], [])                       # no open edge known: a roof square
        self.assertEqual(split_tile(whole, (1, 2))[0], [])               # the slab is not on those edges
        pieces, roof = split_tile(whole, (0, 2))
        self.assertEqual([edge for edge, _ in pieces], [0])              # the slab stands up on its edge
        self.assertTrue(roof is not None and (roof[..., 3] > 0).any())   # the shingle stays a roof
        grey = (np.abs(roof[..., 0].astype(int) - roof[..., 2]) < 6) & (roof[..., 3] > 0)
        self.assertFalse(grey.any())
        plain = self.tile([(0, 1, 0, 1, (120, 80, 50))])
        self.assertEqual(split_tile(plain, (0, 1, 2, 3))[0], [])         # shingle on every open edge: nothing stands

    def test_a_whole_painted_dark_tile_with_a_slab_on_one_open_edge_stays_a_roof_but_for_the_strip(self):
        from roof_fringe import LEFT_ROOF, RIM, SLAB_STRIP
        # painted whole and nearly all dark grey (concrete), a little brown at the far edge: it is a roof square
        dark = self.tile([(0, 1, 0, 1, (14, 14, 14)), (0, 1, 0.9, 1, (120, 80, 50))])
        pieces, roof = split_tile(dark, (0,))
        self.assertEqual([edge for edge, _ in pieces], [0])              # the strip along the open edge stands up
        self.assertIsNotNone(roof)
        from roof_fringe import _grid
        painted, u, v, diamond = _grid(dark)
        kept = ((roof[..., 3] > 0) & diamond).sum() / diamond.sum()
        self.assertGreater(kept, 1 - (SLAB_STRIP + RIM) - 0.15)         # only the strip is gone, the rest is flat roof
        self.assertGreater(kept, 10 * LEFT_ROOF)
        self.assertFalse(((roof[..., 3] > 0) & diamond & (v < SLAB_STRIP)).any())

    def test_the_edge_check_sees_a_flat_slab_without_the_exporters_classifier(self):
        from roof_edge_check import flat_slab
        from floor_sidecar import CELL_OUTER_SIZE
        from roof_fringe import unproject_cell
        whole = self.tile([(0, 1, 0, 1, (120, 80, 50)), (0, 1, 0, 0.4, (14, 14, 14))])
        self.assertEqual(CELL_OUTER_SIZE, unproject_cell(whole).shape[0])
        self.assertEqual(flat_slab(unproject_cell(whole), (0, 1)), [0])
        self.assertEqual(flat_slab(unproject_cell(self.tile([(0, 1, 0, 1, (120, 80, 50))])), (0, 1, 2, 3)), [])

    def test_a_flat_cell_keeps_no_dark_ragged_wall_top_whether_or_not_a_band_stands(self):
        from floor_sidecar import CELL_MARGIN
        shingle = (120, 80, 50)
        cell = np.zeros((100, 100, 4), np.uint8)
        cell[CELL_MARGIN:94, CELL_MARGIN:94] = (*shingle, 255)
        self.assertTrue((tidy_cell(cell, (0, 1)) == cell).all())          # plain shingle keeps its whole art
        ragged = cell.copy()
        ragged[6:40, 6:94] = 0                                            # the cut: clear space along the top
        for x in range(10, 90, 8):                                        # dark fingers stick up from the roof's body
            ragged[20:40, x:x + 3] = (12, 12, 12, 255)
        tidy = tidy_cell(ragged, (0,))
        dark = (tidy[..., 3] > 0) & (tidy[..., :3].astype(float).mean(axis=2) < 40)
        self.assertFalse(dark.any())                                      # no finger is left
        self.assertTrue((tidy[60:90, 20:80, 3] > 0).all())                # the roof's body stays
        grey = cell.copy()
        grey[6:40, 6:94] = (14, 14, 14, 255)                              # a grey slab along the open edge, no hole at all
        self.assertFalse((tidy_cell(grey, (0,))[10:30, 10:90, 3] > 0).any())
        self.assertTrue((tidy_cell(grey, (1,))[10:30, 10:90, 3] > 0).all())  # not on an edge that is not open

    def test_the_edge_check_counts_dark_ragged_texels_of_any_hue_by_shape_alone(self):
        shingle = (120, 80, 50)
        cell = np.zeros((100, 100, 4), np.uint8)
        cell[6:94, 6:94] = (*shingle, 255)
        self.assertEqual(int(roof_edge_check.ragged_thin(cell, [0, 1, 2, 3]).sum()), 0)
        for colour in ((12, 12, 12), (40, 40, 40), (45, 30, 22)):          # black, grey and dark brown fingers
            cell = np.zeros((100, 100, 4), np.uint8)
            cell[50:94, 6:94] = (*shingle, 255)
            for x in range(10, 90, 10):
                cell[20:50, x:x + 3] = (*colour, 255)
            self.assertGreater(int(roof_edge_check.ragged_thin(cell, [0]).sum()), 100, colour)
            self.assertEqual(int(roof_edge_check.ragged_thin(tidy_cell(cell, (0,)), [0]).sum()), 0, colour)
        dither = np.zeros((100, 100, 4), np.uint8)                          # a checkerboard shading is no wall top
        dither[50:94, 6:94] = (*shingle, 255)
        for y in range(20, 50):
            for x in range(10, 90):
                if (x + y) % 2:
                    dither[y, x] = (12, 12, 12, 255)
        self.assertEqual(int(roof_edge_check.ragged_thin(dither, [0]).sum()), 0)
        tidied = tidy_cell(dither, (0,))                                    # tidying keeps the dither: its clear texels are no hole
        shading = np.zeros(dither.shape[:2], bool)
        shading[30:50, 20:80] = dither[30:50, 20:80, 3] > 0
        self.assertGreater(int(shading.sum()), 500)
        self.assertTrue((tidied[..., 3][shading] > 0).all())

    def test_an_all_wall_top_square_is_roofed_up_to_its_band_and_clear_beyond_it(self):
        shingle, grey = (120, 80, 50), (14, 14, 14)
        # a tile that is all grey wall top: a thin strip along its v = 0 edge, rebar above it
        top = self.tile([(0, 1, 0, 0.12, grey), (0.2, 0.25, 0, 0.3, grey), (0.6, 0.65, 0, 0.3, grey)])
        plain = self.tile([(0, 1, 0, 1, shingle)])
        # row 1 is all wall top (tile 7, open edge 0); rows 2 and 3 are plain roof, the donor's
        roof = {101: 7, 102: 7, 103: 7, 201: 8, 202: 8, 203: 8, 301: 8, 302: 8, 303: 8}
        with TemporaryDirectory() as temp:
            png = Path(temp) / "roof_fringe.png"
            # the wall's outer face stands 0.3 into the squares of row 1 (outside is the low side of v)
            entries, patches = write_fringe_art(
                png, roof, lambda tile: top if tile == 7 else plain,
                lambda edge, plane, start, count: (plane + 0.3, plane + 0.4, 1.88, float(start), float(start + count), []))
            sheet = np.asarray(Image.open(png).convert("RGBA"))
            cells = {index: sheet[y:y + 100, x:x + 100] for index, x, y in patches}
            t = (np.arange(100) + 0.5 - 6) / 88
            v, u = np.meshgrid(t, t, indexing="ij")
            for index in (101, 102, 103):
                self.assertIn(index, cells)
                face = [entry[8] for entry in entries if index in roof_edge_check.run_squares(entry) and entry[0] < 2][0]
                self.assertAlmostEqual(face - 1, 0.3, places=2)
                alpha = cells[index][..., 3] > 0
                core = (u > 0.1) & (u < 0.9)
                self.assertTrue(alpha[core & (v > 0.33) & (v < 0.9)].all())       # roof right up to the band's plane
                self.assertFalse(alpha[(v < 0.27) & core].any())                  # nothing beyond it
            lanes, spills, *_ = roof_edge_check.lane_stats(entries, [], set(roof), {i: (x, y) for i, x, y in patches}, sheet)
            self.assertEqual((lanes, spills), (0, 0))

    def test_a_square_a_band_crosses_restores_no_dark_wall_top_flat(self):
        shingle, black = (120, 80, 50), (12, 12, 12)
        # a square whose art is a body with black fingers on its open edge, whole enough for the classifier to call it a roof
        art = self.tile([(0, 1, 0.3, 1, shingle)])
        ys, xs = np.mgrid[0:36, 0:80]
        u, v = donor_proj.delta_to_ground(xs + 0.5 - 48, ys + 0.5)
        for k in range(8):
            art[(u >= 0.1 * k + 0.03) & (u < 0.1 * k + 0.07) & (v >= 0.02) & (v < 0.3)] = (*black, 255)
        plain = self.tile([(0, 1, 0, 1, shingle)])
        with TemporaryDirectory() as temp:
            roof = {101: 7, 102: 8, 103: 8, 201: 8, 202: 8, 203: 8}
            entries, patches = write_fringe_art(
                Path(temp) / "roof_fringe.png", roof, lambda tile: art if tile == 7 else plain,
                lambda edge, plane, start, count: (plane - 0.2, plane + 0.1, 1.88, float(start), float(start + count), []))
            sheet = np.asarray(Image.open(Path(temp) / "roof_fringe.png").convert("RGBA"))
            for index, x, y in patches:
                if index == 101:
                    flat = sheet[y:y + 100, x:x + 100]
                    self.assertEqual(int(roof_edge_check.ragged_thin(flat, [0, 1, 2, 3]).sum()), 0)
                if index in (201, 202):
                    self.assertTrue((sheet[y + 20:y + 80, x + 20:x + 80, 3] > 0).all())  # plain squares keep full art

    def test_outward_is_the_side_with_fewer_roof_squares_beyond_the_wall(self):
        roof = {r * 100 + c: 1 for r in range(10, 20) for c in range(10, 14)}  # a roof of rows 10..19
        self.assertEqual(outward(roof, 0, 9.9, 10.1, 10, 4), -1.0)    # roof beyond the high side only: outside is low
        self.assertEqual(outward(roof, 0, 19.9, 20.1, 10, 4), 1.0)
        self.assertEqual(outward({}, 0, 9.9, 10.1, 10, 4), -1.0)      # a tie goes to the edge's own side
        self.assertEqual(outward({}, 1, 9.9, 10.1, 10, 4), 1.0)

    def test_corner_ends_meet_on_each_others_faces(self):
        art = np.zeros((36, 80, 4), np.uint8)
        runs = [(0, 10, 10, [(1010, art)] * 2), (2, 10, 10, [(1010, art)] * 2)]
        bodies = [(9.8, 10.1, 1.88, 9.0, 13.0, []), (9.8, 10.1, 1.88, 9.0, 13.0, [])]
        placed = resolve_runs(runs, bodies, [-1.0, -1.0], 1.88)
        self.assertEqual([place[0] for place in placed], [9.8, 9.8])  # the outer (low) faces
        self.assertEqual([place[2] for place in placed], [9.8, 9.8])  # each starts on the other's face
        self.assertEqual([place[3] for place in placed], [12.0, 12.0])
        # a wall that ends before the run starts is no wall under it: the band stands on its tile line
        placed = resolve_runs(runs[:1], [(9.8, 10.1, 1.88, 5.0, 8.0, [])], [-1.0], 1.88)
        self.assertEqual(placed[0], [10.0, 1.88, 10.0, 12.0])

    def test_a_corner_snap_never_empties_or_reverses_a_span(self):
        art = np.zeros((36, 80, 4), np.uint8)
        # a one-square run at 10 whose wall covers [10, 10.25], and a band across it with its face at 10.4
        runs = [(0, 5, 10, [(510, art)]), (2, 10, 5, [(510, art)] * 2)]
        bodies = [(4.8, 5.1, 1.88, 10.0, 10.25, []), (10.4, 10.7, 1.88, 4.0, 8.0, [])]
        placed = resolve_runs(runs, bodies, [-1.0, 1.0], 1.88)
        for place in placed:
            self.assertGreater(place[3], place[2])
        self.assertEqual(placed[0][2:], [10.0, 10.7])  # it runs on to the band across it: the corner closes

    def test_check_measures_band_distance_and_ignores_small_blobs(self):
        slab = type("Slab", (), {"axis": 0, "plane": 9.8, "side": 1.0, "thick": 0.3, "lo": 0.0, "hi": 20.0,
                                 "band": lambda self: (9.8, 10.1)})()
        flush = (0, 10, 10, 2, 0, 0, 1, 1, 9.8, 1.88, 10.0, 12.0)
        inset = (0, 10, 12, 2, 0, 0, 1, 1, 10.1, 1.88, 12.0, 14.0)
        footprint = {r * 100 + c for r in range(10, 20) for c in range(10, 16)}
        stats = roof_edge_check.band_stats([flush, inset], [slab], footprint)
        self.assertAlmostEqual(stats["off_face_max"], 0.3, places=3)
        self.assertEqual((stats["gap_max"], stats["overshoot_max"]), (0.0, 0.0))
        cell = np.zeros((100, 100, 4), np.uint8)
        cell[6:94, 6:94] = (120, 80, 50, 255)
        cell[40:50, 40:50] = (10, 10, 10, 255)  # a grey blob in the middle of a whole roof, no hole beside it
        self.assertEqual(int(roof_edge_check.ragged_dark(cell, [0]).sum()), 0)


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

    def test_a_fixed_fly_rest_is_one_fly_only_plane(self):
        scene = self.sample_scene()
        scene["sprites"][0][5:] = [-5, -20]
        scene["walls"] = []
        scene["trims"] = []
        scene["doors"] = []
        scene["rests"] = [[0, 10.0, 10.0, 0.0, 0.0, 3]]

        batches = decode_wall_sidecar(encode_wall_sidecar(scene))["batches"]
        self.assertEqual(len(batches), 1)
        self.assertTrue(batches[0]["fly_only"])
        self.assertFalse(batches[0]["game_only"])
        self.assertEqual(batches[0]["triangles"], 4)

    @staticmethod
    def hull_scene():
        scene = json.loads(json.dumps(WallSidecarTests.sample_scene()))
        scene["pages"] = [{"image": "sprites0.png", "size": [4096, 4096]}]
        scene["sprites"][0][3:] = [2028, 529, -1014, -529]
        scene["walls"] = []
        scene["trims"] = []
        scene["doors"] = []
        scene["rests"] = [[0, 10.0, 10.0, 0.0, 0.0, 3]]
        return scene

    def test_a_hull_sized_fixed_fly_rest_stands_on_its_water_line(self):
        scene = self.hull_scene()
        water = bytearray(100 * 100)          # row 10 water, its north row land
        for col in range(100):
            water[10 * 100 + col] = 1
        batches = decode_wall_sidecar(encode_wall_sidecar(scene, water=water))["batches"]
        self.assertEqual(len(batches), 1)
        self.assertTrue(batches[0]["fly_only"])
        self.assertEqual(batches[0]["triangles"], 4)
        # The plane stands on the water/land boundary (row 9's south edge),
        # feet on the ground, the whole picture above it. A batch vertex
        # stores u - 50, z, v - 50.
        vs = {round(vertex[2], 6) for vertex in batches[0]["vertices"]}
        self.assertEqual(vs, {-40.0})
        zs = sorted({round(vertex[1], 6) for vertex in batches[0]["vertices"]})
        self.assertEqual(zs[0], 0.0)
        self.assertGreater(zs[1], 0.0)

    def test_a_shore_to_the_west_turns_the_hull_along_v(self):
        scene = self.hull_scene()
        water = bytearray(100 * 100)          # column 10 water, land on both sides
        for row in range(100):
            water[row * 100 + 10] = 1
        batches = decode_wall_sidecar(encode_wall_sidecar(scene, water=water))["batches"]
        self.assertEqual(len(batches), 1)
        # The walk finds the west shore first (it steps west before east);
        # the plane stands on the column 9/10 boundary (tile u 10), along v:
        # a batch vertex stores u - 50.
        us = {round(vertex[0], 6) for vertex in batches[0]["vertices"]}
        self.assertEqual(us, {-40.0})

    def test_water_without_a_water_line_leaves_the_sidecar_unchanged(self):
        scene = self.hull_scene()
        self.assertEqual(encode_wall_sidecar(scene),
                         encode_wall_sidecar(scene, water=bytearray(100 * 100)))

    def test_a_fractional_anchor_uses_the_square_it_lands_in(self):
        scene = self.hull_scene()
        scene["rests"] = [[0, 10.75, 10.75, 0.0, 0.0, 3]]
        water = bytearray(100 * 100)          # row 10 water, its north row land
        for col in range(100):
            water[10 * 100 + col] = 1
        batches = decode_wall_sidecar(encode_wall_sidecar(scene, water=water))["batches"]
        self.assertEqual(len(batches), 1)
        # The containing square (10, 10) decides: the hull stands on row 9's
        # south edge, exactly as an integral anchor there would. Rounding read
        # the neighbour (11, 11) — land — and kept the game-screen plane.
        vs = {round(vertex[2], 6) for vertex in batches[0]["vertices"]}
        self.assertEqual(vs, {-40.0})

    def test_an_anchor_past_the_far_edge_reads_no_water(self):
        scene = self.hull_scene()
        scene["rests"] = [[0, 10.0, 99.75, 0.0, 0.0, 3]]
        rows = [[1] * 100 for _ in range(100)]   # all water, no shore in reach
        # floor(99.75) is the containing row 99: the walk finds no shore and
        # the anchor keeps rest_card's plane. Rounding read water[100] of the
        # row-split grid and raised IndexError before the bounds check.
        self.assertEqual(encode_wall_sidecar(scene, water=rows),
                         encode_wall_sidecar(scene))

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

    def test_back_alpha_mirrors_front_below_cap_and_darkens_only_newly_opaque_texels(self):
        import numpy as np
        import wall_sidecar

        scene = self.sample_scene()
        scene["sprites"] = [[0, 10, 10, 4, 4, 0, 0], [0, 30, 30, 4, 3, 0, 0]]
        scene["walls"] = [[0, 1.0, 2.0, 3.0, 2.0, 0.0, 1.0, 0, 1,
                            1.0 - 1.0 / scene["proj"]["hpx"], 0.25, 1]]
        atlas = np.zeros((100, 100, 4), dtype=np.uint8)
        front_alpha = np.array([[255, 0, 255, 255], [255, 255, 0, 255],
                                [255, 0, 0, 255], [0, 255, 255, 0]], dtype=np.uint8)
        atlas[10:14, 10:14, :3] = 120
        atlas[10:14, 10:14, 3] = front_alpha
        atlas[30:33, 30:34, :3] = (90, 80, 70)
        atlas[30:33, 30:34, 3] = 0
        atlas[30, 30, 3] = 255

        generated_pages, row_sprites, generated_sprites = wall_sidecar._mirror_back_atlases(scene, [atlas])
        page, x, y, width, height, *_ = generated_sprites[row_sprites[0]]
        image = generated_pages[page - 1][y:y + height, x:x + width]
        expected = front_alpha[1:] >= 128
        self.assertEqual(image[..., 3].tolist(), (expected.astype(np.uint8) * 255).tolist())
        self.assertEqual(tuple(image[0, 0, :3]), (90, 80, 70))  # existing opaque back pixel
        self.assertEqual(tuple(image[0, 1, :3]), (24, 24, 24))  # transparent back, opaque front
        self.assertEqual(int(image[1, 2, 3]), 0)  # transparent front stays open
        exported_pages = []
        sidecar = decode_wall_sidecar(encode_wall_sidecar(scene, atlas=[atlas], wall_back_pages=exported_pages))
        self.assertEqual(len(exported_pages), 1)
        front_batch = next(batch for batch in sidecar["batches"] if batch["page"] == 0 and not batch["fly_only"])
        back_batch = next(batch for batch in sidecar["batches"] if batch["page"] == 1 and batch["fly_only"])
        # At the same world endpoint (a, z=0), front and back winding/UVs
        # already reverse together. Both therefore address their local left edge.
        front_a = next(v for v in front_batch["vertices"]
                       if abs(v[0] + 50.0 - 1.0) < 1e-5 and abs(v[1]) < 1e-5)
        back_a = next(v for v in back_batch["vertices"]
                      if abs(v[0] + 50.0 - 1.0) < 1e-5 and abs(v[1]) < 1e-5)
        self.assertAlmostEqual(front_a[3] * 100.0 - 10.0, 0.0, places=4)
        self.assertAlmostEqual(back_a[3] * 2048.0 - x, 0.0, places=4)
        self.assertEqual(int(front_alpha[-1, 0]), 0)
        self.assertEqual(int(image[-1, 0, 3]), 0)  # same world endpoint remains open on both faces

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

        # The rest of its sprite, a card on its axis, takes its card's plane,
        # stack and all, not the hex row's at the axis, a hair nearer than the
        # solid: it shows what the solid does not.
        scene["rests"] = [[2, 10.0, 10.1, 0.0, 0.0, 0]]
        homes = [vertex[7] + 50.0 for batch in decode_wall_sidecar(encode_wall_sidecar(scene))["batches"]
                 for vertex in batch["vertices"]]
        self.assertTrue(any(abs(home - (plane + 2e-5)) < 1e-5 for home in homes))
        self.assertFalse(any(abs(home - row_plane(proj, 10.0, 10.1)) < 1e-5 for home in homes))
        scene["rests"] = []

        # A drum: sixteen sides and a top (the sides away from the camera
        # repeat the facing quarter mirrored, so they are not checked here).
        # It stands for several cards (board -1), so it keeps its own depth.
        scene["props"] = [[2, 20.0, 20.0, -1, 1, 20.0, 20.0, 9.0, 6.0, 0.0, 0.4, 0.0, 0.0]]
        drum = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertGreater(drum["triangles"], 16)
        for vertex in drum["batches"][0]["vertices"]:
            self.assertEqual(vertex[5:8], vertex[:3])

    def test_a_back_view_row_points_the_away_faces_at_it_mirrored(self):
        # 124: a row naming a back view (row[17]) sends its away faces to that
        # sprite's page, sampled mirrored about the anchor; without the field
        # they stay on the near sprite's page as they always were.
        scene = self.sample_scene()
        scene["walls"] = []
        scene["pages"].append({"image": "sprites1.png", "size": [100, 100]})
        scene["sprites"].append([0, 40, 40, 50, 50, -25, -40])       # 2: the near view
        scene["sprites"].append([1, 0, 0, 100, 100, -50, -50])       # 3: the back view, the whole page
        scene["props"] = [[2, 10.0, 10.0, 0, 0, 10.0, 10.1, 0.5, 0.4, 0.0, 0.3, 0.0, 0.6,
                           0, 0, 0, [], 3]]
        scene["boards"] = [[2, 10.0, 10.0, 1, 0, 0]]
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertIn(1, [batch["page"] for batch in decoded["batches"]])
        p = scene["proj"]

        def px(u, v, z):
            return (p["origin"][0] + u * p["squ"][0] + v * p["sqv"][0],
                    p["origin"][1] + u * p["squ"][1] + v * p["sqv"][1] - p["hpx"] * z)

        ax_, ay_ = px(10.0, 10.0, 0.0)
        vertices = next(batch["vertices"] for batch in decoded["batches"] if batch["page"] == 1)
        self.assertEqual(len(vertices), 12)                      # the two away faces, unclipped
        for k, (x, y, z, s_, t, *_) in enumerate(vertices):
            u, v, height = x + 50.0, z + 50.0, y
            # the away faces sample the near faces' points: the v0 face from
            # the v1 face, the u0 face from the u1 face (six vertices each)
            su, sv = (u, 10.3) if k < 6 else (10.25, v)
            sx, sy = px(su, sv, height)
            # the back view is read mirrored: the texel of the mirrored
            # world pixel, in the back sprite's own rect
            self.assertAlmostEqual(s_ * 100.0, 0 + (ax_ - sx) + 50, places=3)
            self.assertAlmostEqual(t * 100.0, 0 + (sy - ay_) + 50, places=3)
        scene["props"][0] = scene["props"][0][:16]                   # no back view field
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        self.assertNotIn(1, [batch["page"] for batch in decoded["batches"]])

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

    def test_a_fly_drum_is_sliced_at_the_walls_it_crosses(self):
        # A fly-only drum annotated with a wall plane (scene.keep_apart, 120)
        # keeps its size and ends flat at the plane: the fly mesh loses the part
        # past it and keeps the rest whole. The game's faces are not drawn anyway.
        scene = self.sample_scene()
        scene["walls"] = []
        scene["pages"][0]["size"] = [400, 400]
        scene["sprites"].append([0, 100, 100, 200, 200, -100, -150])
        scene["props"] = [[2, 10.0, 10.0, 0, 1, 10.0, 10.0, 18.0, 8.0, 0.0, 0.5, 0.0, 0.0,
                           0, 0, 1, [[0, 10.0, 1]]]]
        scene["boards"] = [[2, 10.0, 10.0, 0, 0, 0]]
        decoded = decode_wall_sidecar(encode_wall_sidecar(scene))
        fly = [v for b in decoded["batches"] if b["fly_only"] for v in b["vertices"]]
        self.assertTrue(fly)
        us = [v[0] + 50.0 for v in fly]         # the sidecar vertices are ground minus (50, 50)
        self.assertGreaterEqual(min(us), 10.0 - 1e-4)
        # The kept side reaches as far as the whole drum did.
        whole = [2, 10.0, 10.0, 0, 1, 10.0, 10.0, 18.0, 8.0, 0.0, 0.5, 0.0, 0.0, 0, 0, 1]
        scene["props"] = [whole]
        whole_us = [v[0] + 50.0 for b in decode_wall_sidecar(encode_wall_sidecar(scene))["batches"]
                    if b["fly_only"] for v in b["vertices"]]
        self.assertLess(min(whole_us), 10.0)    # the drum did cross the plane
        self.assertAlmostEqual(max(us), max(whole_us), places=3)

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
        slab, out, twin, twin_out, start, cut = corners.pieces[1]
        self.assertAlmostEqual(start, 2.6)
        self.assertAlmostEqual(cut, 3.0)
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

    def test_a_corner_piece_is_cut_at_the_corner_line_and_the_past_stretch_takes_the_other_face(self):
        from unittest import mock
        import wall_sidecar
        walls, sprites, proj, corners = self.decor_corner()
        _, out, _, _, _, cut = corners.pieces[1]
        quads = []
        with mock.patch.object(wall_sidecar, "_quad",
                               lambda batches, page, size, cs: quads.append([list(c) for c in cs])):
            wall_sidecar._wall_front(wall_sidecar._Batches(), proj, sprites, [[100, 100]], walls[1],
                                     proj.toward[:2], (False, False), corner=(cut, out, proj.toward))
        # The picture splits at the corner line (u 3.0): the face's own
        # stretch keeps no depth points, the stretch past it takes the plane
        # of the wall it continues there, its vertices staying where they were.
        self.assertEqual(len(quads), 2, [q[0][:3] for q in quads])
        for quad, past in zip(quads, (False, True)):
            for corner in quad:
                du, dv, dz = corner[5:8] or (0.0, 0.0, 0.0)
                if past:
                    self.assertAlmostEqual(corner[0] + du, 3.0, places=4)
                    self.assertTrue(2.99 <= corner[0] <= 3.41, corner[:3])
                else:
                    self.assertAlmostEqual(du + dv + dz, 0.0, places=6)

    def test_a_thin_fence_row_across_the_other_line_is_cut_at_it(self):
        import wall_sidecar
        # A fence along u ends at a fence along v; the rows of it whose span
        # crosses the other fence's plane are cut there (096), the fence
        # along v is not.
        along_v = [0, 2.0, 2.1, 2.0, 3.0, 0, 0.84, 0, 1, 0.0, 0.06, -1]
        along_u = [1, 2.2, 2.05, 1.0, 2.05, 0, 0.8, 0, 1, 0.0, 0.06, -1]
        sliver = [2, 2.03, 2.05, 1.97, 2.05, 0, 0.84, 0, 1, 0.0, 0.06, -1]
        corners = wall_sidecar._corner_fits([along_v, along_u, sliver], set(), (0.49, 0.74, 0.46),
                                            lambda index: 16, (48.0, 32.0))
        self.assertEqual(corners.thin_cuts, {1: (2.0, 1), 2: (2.0, 1)})

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
        # (The corner's cap piece stands a hair above the top by design, and a
        # clear mesh takes no cap at all: no column of its band stays opaque.)
        scene["walls"][1][6], scene["walls"][1][9] = 0.9, 0.8
        vertices = lambda atlas: {tuple(round(n, 4) for n in v[:3]) for batch in decode_wall_sidecar(
            encode_wall_sidecar(scene, atlas=[atlas]))["batches"] if batch["fly_only"] for v in batch["vertices"]}
        arm = [z for x, z, y in vertices(solid) - vertices(mesh) if y + 50.0 < 1.7 and z < 0.8 + 0.002]
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

    def test_a_flat_part_in_a_composite_is_skipped_only_in_flight(self):
        scene, objects = self.sample()
        objects.append(self.MapObject(600, 0x02000020))
        scene["decals"] = [[0, 1, 1, 0, 5]]
        scene["flyDecals"] = [0]
        entries = baked_objects(scene, objects)
        self.assertIn((600, 0x02000020, BAKED_FLY_SKIP, 0.0, 0.0, 0.0), entries)

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

    def test_a_card_on_the_hand_list_keeps_its_plane(self):
        scene, objects = self.sample()
        objects += [self.MapObject(600, 0x02000020), self.MapObject(700, 0x02000030), self.MapObject(800, 0x03000040)]
        scene["sprites"] = [[0, 0, 0, 30, 15], [0, 0, 0, 20, 40]]  # low, tall
        scene["boards"] += [[1, 1, 1, 0, 0, 5], [1, 1, 1, 0, 0, 6], [1, 1, 1, 0, 0, 7]]
        names = {0x02000020: "NJUNK1.FRM", 0x02000030: "other.frm", 0x03000040: "NJUNK1.FRM"}
        entries = baked_objects(scene, objects, frozenset(), frozenset({"njunk1.frm"}), names.get)
        self.assertIn((600, 0x02000020, BAKED_FIXED_CARD, 0.0, 0.0, 0.0), entries)   # on the list
        self.assertNotIn(700, [entry[0] for entry in entries])                         # not on it
        self.assertNotIn(800, [entry[0] for entry in entries])                         # a wall type
        self.assertNotIn(BAKED_FIXED_CARD, [entry[2] for entry in baked_objects(scene, objects)])
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

    def test_numbered_art_parts_join_by_nearby_tile_cluster(self):
        from vendor.fp import props
        first = [10722, 10723, 11922, 12127, 12316, 12330, 12517, 13323, 13325]
        second = [16522, 16721, 17921, 17926, 18315, 18316, 18329, 19122, 19124]
        pieces = []
        for cluster, tiles in enumerate((first, second)):
            for part, tile in enumerate(tiles):
                p = self.piece("Vertibird", part * 8, cluster * 200, 8, 8,
                               tile=tile, order=len(pieces))
                p.family = ("verti", part + 1)
                p.obj.pid = 1000 + part
                p.stands = part != 0
                pieces.append(p)
        # A short numbered pair is too ambiguous to treat as one object.
        small = [self.piece(f"small-{i}", i * 30, 500, 8, 8, tile=25000 + i * 10, order=18 + i)
                 for i in range(3)]
        for i, p in enumerate(small):
            p.family = ("pair", i + 1)
            p.obj.pid = 2000 + i
        groups = sorted(props.groups(pieces + small), key=len)
        self.assertEqual([1, 1, 1, 9, 9], [len(group) for group in groups])
        self.assertTrue(all(not any(p in small for p in group) for group in groups if len(group) > 1))
        self.assertEqual([9, 9], sorted(len(group) for group in props.multipart_groups(pieces)))

    def test_numbered_art_from_separate_objects_does_not_join_by_family_alone(self):
        from vendor.fp import props
        pieces = []
        for i in range(props.MULTIPART_MIN_PARTS):
            piece = self.piece("Graffiti", i * 100, 0, 20, 20, tile=10000 + i, order=i)
            piece.family = ("gy", i + 1)
            piece.obj.pid = 3000 + i
            pieces.append(piece)
        self.assertEqual([], props.multipart_groups(pieces))

        for i, piece in enumerate(pieces):
            piece.name = "Weed" if i % 2 else "Scrub"
            piece.family = ("weed", i + 1)
        self.assertEqual([], props.multipart_groups(pieces))

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

class FillHolesTests(unittest.TestCase):
    """Task 115: a solid's texture has no enclosed holes; the open border stays."""

    @staticmethod
    def rgba(pattern):
        import numpy as np
        out = np.zeros((len(pattern), len(pattern[0]), 4), np.uint8)
        for y, row in enumerate(pattern):
            for x, cell in enumerate(row):
                if cell == 'x':
                    out[y, x] = (200, 100, 50, 255)
                elif cell == 'o':
                    out[y, x] = (10, 200, 30, 255)
        return out

    def test_an_enclosed_hole_takes_the_average_of_its_neighbours(self):
        import numpy as np
        from vendor.fp import props
        rgba = self.rgba([
            'xxxxx',
            'xooox',
            'xo.ox',
            'xooox',
            'xxxxx',
        ])
        filled = props.fill_holes(rgba)
        self.assertEqual(255, filled[2, 2, 3])
        # the ring around the hole is two 'o' columns and two 'o' rows: the
        # average of the four neighbours of the centre texel
        self.assertEqual(int(rgba[1, 2, 0] + rgba[3, 2, 0] + rgba[2, 1, 0] + rgba[2, 3, 0]) // 4,
                         int(filled[2, 2, 0]))

    def test_the_open_border_stays_transparent(self):
        import numpy as np
        from vendor.fp import props
        rgba = self.rgba([
            'xxx.x',
            'x....',
            'xxx.x',
        ])
        filled = props.fill_holes(rgba)
        self.assertEqual(0, filled[1, 3, 3])   # the notch open to the border
        self.assertEqual(0, filled[1, 2, 3])
        self.assertEqual(0, filled[1, 1, 3])

    def test_a_fully_opaque_or_empty_texture_is_returned_as_is(self):
        import numpy as np
        from vendor.fp import props
        full = self.rgba(['xxx', 'xxx'])
        empty = self.rgba(['...', '...'])
        self.assertTrue((props.fill_holes(full) == full).all())
        self.assertTrue((props.fill_holes(empty) == empty).all())

class LidDrumTests(unittest.TestCase):
    """Task 116: a round body under a lid cap fits a drum whose top face takes
    the lid; blobs without a neck and plain drums do not."""

    @staticmethod
    def capsule(r, lift, foot, width=None):
        """A drum silhouette in the game's screen slopes: columns 0..2r, its
        lower outline on the floor ellipse, its top `lift` px above the floor."""
        import numpy as np
        from vendor.fp import props
        cols = 2 * int(r)
        half = props.K * r * np.sqrt(np.clip(1 - ((np.arange(cols) + 0.5 - r) / r) ** 2, 0, 1))
        height = int(foot + half.max() + 1)
        alpha = np.zeros((height, cols), bool)
        for x in range(cols):
            top = max(0, int(foot - lift - half[x]))
            alpha[top:int(foot + half[x]), x] = True
        return alpha

    @staticmethod
    def stacked(parts, gap):
        """Sprite of parts stacked top to bottom, `gap` empty rows between, centred."""
        import numpy as np
        width = max(p.shape[1] for p in parts)
        height = sum(p.shape[0] for p in parts) + gap * (len(parts) - 1)
        alpha = np.zeros((height, width), bool)
        at = 0
        for p in parts:
            x0 = (width - p.shape[1]) // 2
            alpha[at:at + p.shape[0], x0:x0 + p.shape[1]] |= p
            at += p.shape[0] + gap
        return alpha

    def test_a_plain_drum_is_no_lid(self):
        import numpy as np
        from vendor.fp import props
        res = props.fit(self.capsule(12, 25, 40), 0, 0)
        self.assertIsNotNone(res)
        shape, found = res
        self.assertEqual(found.kind, "drum")
        self.assertFalse(found.lid)

    def test_a_stove_is_a_drum_whose_top_takes_the_lid(self):
        import numpy as np
        from vendor.fp import props
        body = self.capsule(13, 26, 46)
        cap = self.capsule(11, 6, 22)
        alpha = self.stacked([cap, body], 3)
        shape = props.Shape(alpha, 0, 0)
        found = props._lid_drum(shape)
        self.assertIsNotNone(found)
        error, fit = found
        self.assertEqual(fit.kind, "drum")
        self.assertTrue(fit.lid)
        self.assertGreater(fit.z1, 1.0)        # the raise climbed onto the cap

    def test_blobs_without_a_neck_are_no_lid(self):
        import numpy as np
        from vendor.fp import props
        top = np.zeros((12, 14), bool)
        top[:] = True
        bottom = np.zeros((26, 20), bool)
        bottom[:] = True                       # a pile: the seam stays as wide as the top
        alpha = self.stacked([top, bottom], 0)
        res = props.fit(alpha, 0, 0)
        self.assertTrue(res is None or not res[1].lid)   # a pile is never a drum with a lid

    def test_a_footed_drum_never_reads_as_a_lid(self):
        import numpy as np
        from vendor.fp import props
        body = self.capsule(12, 22, 38)
        alpha = self.stacked([body, self.capsule(4, 2, 8)], 2)   # small feet under the belly
        res = props.fit(alpha, 0, 0)
        self.assertIsNotNone(res)
        self.assertFalse(res[1].lid)   # the feet are not a cap: no lid drum, whatever wins


class CarveTests(unittest.TestCase):
    """Task 120: the carve fallback - a multi-view art the plain fit refused
    stands as the forced box raised to the art's own height bands."""

    @staticmethod
    def box_sprite(a, b, lifts):
        """The true projection of a row of floor-standing boxes (the carve's
        body and cabin): the near corner right of centre, the boxes marching
        along the long axis, each `lift` px tall."""
        import numpy as np
        from vendor.fp import props
        ones = np.ones((150, 280), bool)
        geom = props.Shape(ones, 0, 0)
        corner = (190.0, 100.0)
        alpha = np.zeros_like(ones)
        y = geom.top + np.arange(geom.h)[:, None] + 0.5
        for n, (run, lift) in enumerate(lifts):
            block = props.Block(geom, (corner[0] - n * run * props.proj.SQ_U[0], corner[1] - n * run * props.proj.SQ_U[1]),
                                run, b, 0)
            alpha |= (y >= block.far[None, :] - lift) & (y < block.near[None, :]) & block.on[None, :]
        return alpha

    def test_a_car_profile_splits_into_body_and_cabin(self):
        from vendor.fp import props
        alpha = self.box_sprite(1.3, 0.9, [(0.8, 22), (0.5, 36)])
        shape, fit = props.fit(alpha, 0, 0, force="box")
        carved = props.carved(shape, fit)
        lifts = sorted(block.lift for block in carved.blocks)
        self.assertGreaterEqual(len(lifts), 2)                  # the bands split
        self.assertLess(lifts[0], 22 + 7)                       # the body's own height
        self.assertGreater(lifts[-1], 36 - 7)                   # the cabin's own height
        self.assertAlmostEqual(carved.z1, lifts[-1] / props.proj.HPX, places=3)

    def test_a_flat_profile_keeps_one_block_at_the_sprites_height(self):
        from vendor.fp import props
        alpha = self.box_sprite(1.4, 0.9, [(1.4, 28)])
        shape, fit = props.fit(alpha, 0, 0, force="box")
        carved = props.carved(shape, fit)
        self.assertEqual(len(carved.blocks), 1)
        self.assertAlmostEqual(carved.blocks[0].lift, 28.0, delta=7)

    def test_placeholder_cards_are_no_directions(self):
        from vendor.fp import props
        import numpy as np
        view = np.zeros((20, 30), bool)
        view[4:16, 3:26] = True                                 # a silhouette: open canvas around it
        card = np.ones((22, 46), bool)                          # the UNUSED ART card: edge to edge
        self.assertEqual(props.real_directions([view, view, card, card]), 2)
        self.assertEqual(props.real_directions([card]), 0)
        self.assertEqual(props.real_directions([view]), 1)

    def test_the_carve_takes_long_low_arts_only(self):
        # 123: the carve's ground boxes would lie a tall leaning frame down;
        # the vehicles' profile is long and low.
        import numpy as np
        from vendor.fp import props
        wide = np.zeros((40, 130), bool)
        wide[10:32, 4:126] = True                               # a car's side view
        tall = np.zeros((140, 100), bool)
        tall[6:134, 20:80] = True                               # a leaning burnt frame
        self.assertGreaterEqual(props.real_aspect([wide, wide]), props.CARVE_LONG)
        self.assertLess(props.real_aspect([tall, tall]), props.CARVE_LONG)
        # the placeholder cards count in neither measure
        card = np.ones((22, 46), bool)
        self.assertLess(props.real_aspect([tall, tall, card, card]), props.CARVE_LONG)


class WallCardTests(unittest.TestCase):
    """123: a burnt rebar piece joins its wall - the baked sidecar's PLANE."""

    def test_a_wall_card_row_names_the_run_s_plane(self):
        import math
        row = [51.5339, 20.9883, 51.5339, 24.7695, 1]           # a run along v, one object
        scene = {"walls": [], "boards": [], "wallCards": [row]}
        objects = [BakedSidecarTests.MapObject(9302, 0x2000505), BakedSidecarTests.MapObject(9303, 0x2000506)]
        entries = baked_objects(scene, objects)
        self.assertEqual(len(entries), 1)
        tile, fid, mode, du, dv, dz = entries[0]
        self.assertEqual((tile, fid), (9303, 0x2000506))
        self.assertEqual(mode & 0x7FFFFFFF, BAKED_PLANE)
        self.assertAlmostEqual(du, row[0] - GL_MAP_OFFSET)
        self.assertAlmostEqual(dv, row[1] - GL_MAP_OFFSET)
        self.assertAlmostEqual(dz, math.atan2(row[3] - row[1], row[2] - row[0]))


class BackSpriteTests(unittest.TestCase):
    """Task 124: the carve's back view - the art's own opposite direction
    composed into the near view's pixel frame, and the gates that keep the
    plain mirror."""

    palette_rgba = None                                        # built once, below

    @classmethod
    def setUpClass(cls):
        import numpy as np
        cls.palette_rgba = np.zeros((4, 4), np.uint8)
        cls.palette_rgba[1] = (200, 30, 30, 255)               # the near view's colour
        cls.palette_rgba[2] = (30, 30, 200, 255)               # the opposite view's colour

    @staticmethod
    def stub_frm(views, x_offsets):
        """A directions-wide Frm stub over index-array views, the real
        placement math, one frame per direction."""
        import types

        class StubFrm:
            direction_count, frame_count = len(views), 1
            y_offsets = [0] * len(views)

            def __init__(self):
                self.x_offsets = x_offsets

            def frame(self, direction, index=0):
                return types.SimpleNamespace(array=lambda: views[direction])

            def placement(self, direction, index=0, obj_x=0, obj_y=0):
                height, width = views[direction].shape
                return (self.x_offsets[direction] + obj_x - width // 2, self.y_offsets[direction] + obj_y - (height - 1),
                        width, height)

            def rgba(self, palette, direction, index=0):
                return palette.rgba[views[direction]]

        return StubFrm()

    def builder(self, stub):
        import types
        from vendor.fp.atlas import Atlas
        from vendor.fp.scene import SceneBuilder
        box = SceneBuilder.__new__(SceneBuilder)
        box.atlas = Atlas()
        box.sprites = []
        box._sprite_ids = {}
        palette = types.SimpleNamespace(rgba=self.palette_rgba)
        box.gf = types.SimpleNamespace(art=types.SimpleNamespace(load=lambda fid: stub),
                                       palette=types.SimpleNamespace(glow_sprite=palette))
        return box

    def views(self, opposite):
        """Six views: the near view (direction 0) a red 23x15 blob on a 20x30
        canvas, direction `opposite` a blue 27x17 blob covering it at the
        world alignment (its x_offset sits 4 px round), the rest red."""
        import numpy as np
        near = np.zeros((20, 30), np.uint8)
        near[3:18, 4:27] = 1
        views = [near.copy() for _ in range(6)]
        if opposite is not None:
            back = np.zeros((20, 30), np.uint8)
            back[2:19, 2:29] = 2
            views[opposite] = back
        return views

    def piece(self):
        import types
        return types.SimpleNamespace(
            obj=types.SimpleNamespace(fid=0x2000500, rotation=0, frame=0, x=0, y=0))

    def test_the_opposite_view_composes_over_the_near_body(self):
        import numpy as np
        from vendor.fp.scene import SceneBuilder
        views = self.views(3)
        stub = self.stub_frm(views, [0, 0, 0, 4, 0, 0])
        box = self.builder(stub)
        frames = [view != 0 for view in views]
        left, top, _, _ = stub.placement(0, 0)
        back = SceneBuilder.back_sprite(box, self.piece(), self.palette_rgba[views[0]], frames, left, top)
        self.assertGreaterEqual(back, 0)
        image, ox, oy = box.sprites[back]
        got = box.atlas.images[image]
        col0, row0 = ox - left, oy - top
        # where only the opposite is opaque, its blue; where only the near is,
        # the near's red as the fallback; and the near silhouette stays whole
        self.assertEqual(tuple(got[10 - row0, 28 - col0]), (30, 30, 200, 255))
        self.assertEqual(tuple(got[10 - row0, 5 - col0]), (200, 30, 30, 255))
        near_alpha = views[0] != 0
        for i, j in zip(*np.nonzero(near_alpha)):
            self.assertTrue(got[i - row0, j - col0, 3] > 0, f"hole over the near body at {i}, {j}")

    def test_a_placeholder_opposite_keeps_the_mirror(self):
        import numpy as np
        from vendor.fp.scene import SceneBuilder
        views = self.views(3)
        views[3] = np.ones((22, 46), np.uint8)                  # the UNUSED ART card
        stub = self.stub_frm(views, [0, 0, 0, 4, 0, 0])
        back = SceneBuilder.back_sprite(self.builder(stub), self.piece(), self.palette_rgba[views[0]],
                                        [v != 0 for v in views], *stub.placement(0, 0)[:2])
        self.assertEqual(back, -1)

    def test_an_opposite_too_small_against_the_near_body_keeps_the_mirror(self):
        import numpy as np
        from vendor.fp.scene import SceneBuilder
        views = self.views(3)
        views[3] = np.zeros((20, 30), np.uint8)
        views[3][8:12, 12:16] = 2                               # 16 texels of 345: under BACK_COVER
        stub = self.stub_frm(views, [0, 0, 0, 4, 0, 0])
        back = SceneBuilder.back_sprite(self.builder(stub), self.piece(), self.palette_rgba[views[0]],
                                        [v != 0 for v in views], *stub.placement(0, 0)[:2])
        self.assertEqual(back, -1)

    def test_an_art_without_opposite_directions_keeps_the_mirror(self):
        from vendor.fp.scene import SceneBuilder
        views = self.views(3)[:3]                               # three stored directions
        stub = self.stub_frm(views, [0, 0, 0])
        back = SceneBuilder.back_sprite(self.builder(stub), self.piece(), self.palette_rgba[views[0]],
                                        [v != 0 for v in views], *stub.placement(0, 0)[:2])
        self.assertEqual(back, -1)


if __name__ == "__main__":
    unittest.main()


class FillHolesBandTests(unittest.TestCase):
    """121: the band pass - a transparent gap the shape straddles fills."""

    def test_a_lid_gap_over_a_body_fills(self):
        from vendor.fp.props import fill_holes
        import numpy as np
        rgba = np.zeros((12, 10, 4), np.uint8)
        rgba[..., :3] = 200
        rgba[1:4, 2:8, 3] = 255          # the lid
        rgba[7:11, 1:9, 3] = 255         # the body
        out = fill_holes(rgba)
        self.assertTrue((out[4:7, 3:7, 3] == 255).all())   # the gap between them filled
        self.assertTrue((out[0, :, 3] == 0).all())          # the open border stays

    def test_an_open_sky_cutout_stays_open(self):
        from vendor.fp.props import fill_holes
        import numpy as np
        rgba = np.zeros((10, 10, 4), np.uint8)
        rgba[..., :3] = 200
        rgba[1:9, 1:3, 3] = 255           # a left post
        rgba[1:9, 7:9, 3] = 255           # a right post
        out = fill_holes(rgba)
        self.assertTrue((out[1:9, 4:6, 3] == 0).all())      # between the posts: open to the top

    def test_a_tall_gap_between_rails_stays_open(self):
        from vendor.fp.props import fill_holes
        import numpy as np
        rgba = np.zeros((20, 10, 4), np.uint8)
        rgba[..., :3] = 200
        rgba[1:4, 1:9, 3] = 255           # the upper rail
        rgba[12:15, 1:9, 3] = 255         # the lower rail: 8 open rows between
        out = fill_holes(rgba)
        self.assertTrue((out[4:12, 3:7, 3] == 0).all())     # a real gap the game's view shows too
