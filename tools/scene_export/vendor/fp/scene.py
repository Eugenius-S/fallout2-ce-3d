"""Map -> first-person scene (scene.json + atlas PNGs) for viewer.

What each kind of map content becomes:

    floor / roof squares   square textures un-projected from the 80x36 tile art
    wall objects           slabs: their picture on an upright plane (walls.py), a thickness, a top and
                           a far side (boxes.py); sprite cards when unrecognised
    doors                  upright quads in the wall they close
    scenery, items         camera-facing sprite cards anchored like the engine anchors the sprite
                           (the viewer decides how a card stands, viewer/render.js); in first
                           person furniture and other deep things are boxes and drums instead (props.py)
    signs, posters, ...    flat scenery in a wall plane: quads like walls
    FLAT objects           decals lying on the floor (rugs, blood, exit grids, corpses)
    critters               billboards with all six facings and the standing animation
    light emitters         a baked light image (light.py) and a glow at each visible flame or lamp

First person only, in place of some of the above (the game's views keep the map's own pieces):

    tents                  hide walls under a patch of roof: one form with a ridge roof (tents.py)
    set pieces             the stone head, the Temple of Trials: shapes written down in sets.py
    underground            the rock round the open ground and over it, and no pictures of its top (rock.py)
    plants                 two crossed sheets where they grow, not a card that turns
"""
import base64
import json
import math
import os
from collections import Counter
from pathlib import Path

import numpy as np

from f2lib import MapFile, geometry, ids, mapstxt, pro
from f2lib import map as mapfile

from . import boxes, light, proj, props, rock, sets, tents, things, walls
from .atlas import Atlas, save_png

TILE_RES = 88       # texels per square side. > 80 makes the un-projection lossless: every
                    # screen pixel of a tile owns at least one texel (48/N + 32/N < 1).
TILE_MARGIN = 6     # texels around the square: tile art overhangs its rhombus by a pixel or two
MAX_IDLE_FRAMES = 24
WALL_BLOCKER_REACH = 0.3    # an invisible blocker this close to a wall face is part of that wall (units)
NO_TILE = 1
DARK_TILE = 40      # mean brightness (0..255) below which the commonest floor tile is rock or blackness,
                    # which makes a map without any roof an underground one
SET_LIGHT_LEVEL = b"\x80\xE9"       # script opcode (interpreter_extra.cc); its argument is pushed just before
PUSH_INT = b"\xC0\x01"              # ... as this tag and a big-endian int32, when it is a literal
GLOW_FIRE, GLOW_LAMP = (1.0, 0.5, 0.18), (1.0, 0.86, 0.6)
SMALL_AREA, TALL = 0.2, 0.9     # a solid with a smaller footprint (units^2) that is also lower (units) is stepped over
SMALL_CARD = (48, 64)           # px: a card narrower and lower than this does not stop the player either (chairs)
WALK_THROUGH = 160  # px: a thing that stops no one is a solid all the same if it is this wide (a gateway)
# FRMs whose solid is set by name, whatever the fit says (the shapes design allows such
# exceptions, at most a tenth of a map group's distinct FRMs).
_OVERRIDES = json.loads((Path(__file__).resolve().parents[2] / "shapes" / "overrides.json").read_text(encoding="utf-8"))
FORCED = {name: kind for kind in ("box", "drum") for name in _OVERRIDES.get(kind, {})}
PLAIN_OBJECT_FLAGS = mapfile.OBJECT_LIGHT_THRU | mapfile.OBJECT_LIGHTING    # no structure: loose clutter (build_props)
MIN_DRUM = 0.1      # units: the radius a drum keeps when a wall or another thing cuts it back
APART = 0.02        # units: solids of two things that overlap less in plan are left as they are
MIN_BOX = 0.1       # units: what is left of a box that is cut back at a wall, at least
ON_TOP = 0.05       # units outside a solid's footprint that still count as on it
AT_SOLID = 0.3      # an invisible blocker this close to a solid is part of it (units)
PLANT = (0.33, 0.45, 0.5)   # a plant: this share of its pixels on its outline at least, this share of its rectangle
                            # filled at most, and this share of its pixels covered by its mirror image


def _b64(array):
    return base64.b64encode(np.ascontiguousarray(array).tobytes()).decode("ascii")


def _round(values, digits=4):
    return [round(float(v), digits) for v in values]


class SceneBuilder:
    def __init__(self, gf, game_map, elevation=None):
        """game_map: a MapFile, or the name of a map in the game tree."""
        self.gf = gf
        self.map = MapFile.load(game_map, gf) if isinstance(game_map, str) else game_map
        map_name = self.map.base_name
        default = self.map.entering_elevation
        if default not in self.map.tiles:               # two retail maps enter on an elevation they do not have
            default = min(self.map.tiles)
        self.default_elevation = default
        self.elevation = default if elevation is None else elevation
        if self.elevation not in self.map.tiles:
            raise SystemExit(f"{map_name}: elevation {self.elevation} has no squares "
                             f"(present: {sorted(self.map.tiles)})")
        everything = self.map.objects[self.elevation]
        self.serial = {id(o): n for n, o in enumerate(everything)}     # position in the map file: names an object
        self.objects = [o for o in everything if not o.flags & mapfile.OBJECT_HIDDEN]
        self.atlas = Atlas()
        self.sprites = []               # [atlas id, ox, oy]: offset of the top-left pixel from the anchor
        self._sprite_ids = {}
        self.names = []                 # what the crosshair names ...
        self.descriptions = []          # ... and what a closer look says (the proto's description)
        self._name_ids = {}
        self._wall_cache = {}
        self._sprite_cache = {}
        self.wall_stats = Counter()     # per FRM
        self.wall_use = Counter()       # per object

    # ------------------------------------------------------------------ helpers
    def name_id(self, pid):
        name = self.gf.protos.name(pid) or ids.TYPE_NAMES[ids.pid_type(pid)].capitalize()
        key = (name, self.gf.protos.description(pid) or "")
        index = self._name_ids.get(key)
        if index is None:
            index = self._name_ids[key] = len(self.names)
            self.names.append(name)
            self.descriptions.append(key[1])
        return index

    def sprite(self, rgba, ox=0, oy=0):
        key = (self.atlas.add(rgba), ox, oy)
        index = self._sprite_ids.get(key)
        if index is None:
            index = self._sprite_ids[key] = len(self.sprites)
            self.sprites.append(key)
        return index

    def frame_sprite(self, frm, rotation, frame, dx=0, dy=0):
        """Sprite of one FRM frame, offset so that the anchor is the hex centre (+ object x / y)."""
        left, top, _, _ = frm.placement(rotation, frame, dx, dy)
        return self.sprite(frm.rgba(self.gf.palette.glow_sprite, rotation, frame), left, top)

    def load_art(self, obj):
        try:
            return self.gf.art.load(obj.fid)
        except (FileNotFoundError, ValueError, KeyError):
            return None

    @staticmethod
    def anchor(obj):
        """Ground point of the object's hex centre. The engine draws the sprite relative to it
        (shifted by the FRM's offsets and the object's own x / y, which go into the sprite)."""
        return proj.px_to_ground(*proj.hex_px(obj.tile))

    # -------------------------------------------------------------------- tiles
    def build_tiles(self):
        """Un-project every used tile into a (res + 2 margin)^2 cell of the tile atlas."""
        squares = self.map.tiles[self.elevation]
        floor_raw = squares & 0xFFFF                    # flag 0x1000: floor square not drawn
        layers = [np.where((floor_raw & 0x1000) != 0, NO_TILE, floor_raw & 0xFFF),
                  (squares >> 16) & 0xFFF]              # roofs: the loader clears their hidden flag
        used = sorted((set(layers[0].tolist()) | set(layers[1].tolist())) - {NO_TILE})

        cell = TILE_RES + 2 * TILE_MARGIN
        # Texel centre -> ground (u, v) inside the square -> pixel of the tile sprite.
        t = (np.arange(cell) + 0.5 - TILE_MARGIN) / TILE_RES
        v, u = np.meshgrid(t, t, indexing="ij")         # rows = v, columns = u
        src_x = np.floor(48 + u * proj.SQ_U[0] + v * proj.SQ_V[0]).astype(int)
        src_y = np.floor(u * proj.SQ_U[1] + v * proj.SQ_V[1]).astype(int)

        per_row = max(1, 2048 // (cell + 1))
        rows = (len(used) + per_row - 1) // per_row
        sheet = np.zeros((max(rows, 1) * (cell + 1), per_row * (cell + 1), 4), np.uint8)
        assert sheet.shape[0] <= 4096, "too many tiles for one atlas"   # retail maximum is 381 tiles = 1843 px
        cells = []
        index_of = {}
        bright = np.zeros(0x1000, bool)                 # tiles that are floor, not the black top of the rock (rock.py)
        self.clipped_tiles = []
        for n, tile_id in enumerate(used):
            frm = self.gf.art.load(ids.make_fid(ids.OBJ_TYPE_TILE, tile_id))
            rgba = frm.rgba(self.gf.palette.glow_tile)
            h, w = rgba.shape[:2]
            inside = (src_x >= 0) & (src_x < w) & (src_y >= 0) & (src_y < h)
            texture = np.zeros((cell, cell, 4), np.uint8)
            texture[inside] = rgba[src_y[inside], src_x[inside]]
            if not self._tile_lossless(rgba, texture):  # art outside the cell (a few odd-sized tiles) is clipped
                self.clipped_tiles.append(self.gf.art.name(ids.make_fid(ids.OBJ_TYPE_TILE, tile_id)))
            x, y = (n % per_row) * (cell + 1), (n // per_row) * (cell + 1)
            sheet[y:y + cell, x:x + cell] = texture
            cells.append([x, y])
            index_of[tile_id] = n + 1
            bright[tile_id] = rgba[..., :3][rgba[..., 3] > 0].mean() >= rock.DARK if rgba[..., 3].any() else False
        index_of[NO_TILE] = 0
        lookup = np.zeros(0x1000, np.uint16)
        for tile_id, n in index_of.items():
            lookup[tile_id] = n
        self.floor, self.roof = lookup[layers[0]], lookup[layers[1]]
        self.floor_ids, self.roof_ids = layers
        self.lit = bright[layers[0]].reshape(100, 100)
        self.tile_sheet, self.tile_cells = sheet, cells
        self.build_ground(sheet)

    def build_ground(self, sheet):
        """The commonest floor tile says what lies around the map and whether we are underground."""
        counts = np.bincount(self.floor.ravel(), minlength=len(self.tile_cells) + 1)[1:]
        self.ground = {"cell": -1, "color": [0.1, 0.09, 0.08]}
        self.indoor = False
        if counts.any():
            cell = int(counts.argmax())
            x, y = self.tile_cells[cell]
            texture = sheet[y + TILE_MARGIN:y + TILE_MARGIN + TILE_RES, x + TILE_MARGIN:x + TILE_MARGIN + TILE_RES]
            colour = texture[texture[..., 3] > 0][:, :3].mean(axis=0) if texture[..., 3].any() else np.zeros(3)
            self.ground = {"cell": cell, "color": _round(colour / 255)}
            self.indoor = bool(colour.mean() < DARK_TILE and not self.roof.any())

    def title(self):
        """The map's name in the game's own map table (data/maps.txt), else its file name."""
        name = self.map.base_name.lower()
        try:
            table = mapstxt.maps(self.gf.read("data/maps.txt"))
        except (FileNotFoundError, KeyError):
            table = []
        return next((row["lookup_name"] for row in table if row.get("map_name", "").lower() == name), name.upper())

    def game_messages(self):
        """UI phrases supplied by the user's game; absent entries use viewer defaults."""
        try:
            messages = self.gf.msg("game/proto.msg")
        except (FileNotFoundError, ValueError):
            return {}
        return {key: text for key, number in (("nothing", 493), ("see", 490))
                if (text := messages.text(number))}

    def script_light(self):
        """Ambient light 0.25..1 if the map script sets one fixed level, else None.

        Maps under the sky call set_light_level with several literals (their day and
        night levels), caves and vaults with a single one. The script is not run:
        this only reads the literal arguments out of its byte code.
        """
        name = self.gf.scripts.name(self.map.script_index - 1)
        try:
            code = self.gf.read(f"scripts/{name}.int") if name else b""
        except (FileNotFoundError, KeyError):
            return None
        levels = set()
        at = code.find(SET_LIGHT_LEVEL)
        while at >= 0:
            if at >= 6 and code[at - 6:at - 4] == PUSH_INT:
                levels.add(int.from_bytes(code[at - 4:at], "big", signed=True))
            at = code.find(SET_LIGHT_LEVEL, at + 2)
        if len(levels) != 1:
            return None
        level = min(max(levels.pop(), 0), 100)         # opSetLightLevel: 0 / 50 / 100 % = quarter / five eighths / full
        return round(0.25 + 0.0075 * level if level <= 50 else 0.625 + 0.0075 * (level - 50), 4)

    @staticmethod
    def _tile_lossless(rgba, texture):
        """Iso mode samples the texel under each screen pixel centre: is it always that pixel?"""
        ys, xs = np.nonzero(rgba[..., 3])
        u, v = proj.delta_to_ground(xs + 0.5 - 48, ys + 0.5)
        a = np.floor(u * TILE_RES).astype(int) + TILE_MARGIN
        b = np.floor(v * TILE_RES).astype(int) + TILE_MARGIN
        inside = (a >= 0) & (b >= 0) & (a < texture.shape[1]) & (b < texture.shape[0])
        return bool(inside.all() and (texture[b, a] == rgba[ys, xs]).all())

    # -------------------------------------------------------------------- walls
    def art_of(self, obj, frm):
        """-> (art key, sprite) of the frame the object shows; sprite = (opaque pixels, left, top)."""
        frame = min(max(obj.frame, 0), frm.frame_count - 1)
        art = (obj.fid, obj.rotation, frame)
        if art not in self._sprite_cache:
            left, top, _, _ = frm.placement(obj.rotation, frame)
            self._sprite_cache[art] = (frm.frame(obj.rotation, frame).array() != 0, left, top)
        return art, self._sprite_cache[art]

    def faces_of(self, obj, art, sprite, faces, trusted, kind, order):
        x, y = proj.hex_px(obj.tile)
        solid = kind == "wall" and not obj.flags & mapfile.OBJECT_NO_BLOCK
        return [walls.Face(obj, art, (x + obj.x, y + obj.y), slope, c, x_from, x_to, trusted, solid, kind, order, sprite)
                for slope, c, x_from, x_to in faces]

    def add_wall(self, obj, frm, order):
        """Queue a wall object's faces; False when it is not wall-shaped (the caller makes a sprite card)."""
        art, sprite = self.art_of(obj, frm)
        ext_flags = self.gf.protos.get(obj.pid).flags_ext
        key = art + (ext_flags,)
        hit = self._wall_cache.get(key)
        if hit is None:
            hit = self._wall_cache[key] = walls.classify(*sprite, ext_flags)
            self.wall_stats[hit[0] if hit[1] == "fit" else f"{hit[0]} (by proto flag)"] += 1
        kind, source, faces = hit
        self.wall_use[kind] += 1
        if kind == "board":
            return False
        self.pieces.append(self.faces_of(obj, art, sprite, faces, source == "fit", "wall", order))
        return True

    def join_odd_walls(self, runs_by_tile):
        """Wall objects whose picture says nothing (corner posts, tent poles): if a wall line
        passes through the hexes beside them they are part of it and take its plane."""
        rest = []
        for obj, frm, order, entry in self.things:
            run = obj.obj_type == ids.OBJ_TYPE_WALL and walls.line_through(obj.tile, runs_by_tile)
            if not run:
                rest.append((obj, frm, order, entry))
                continue
            art, sprite = self.art_of(obj, frm)
            face, = self.faces_of(obj, art, sprite, [walls.floor_line(*sprite, "u" if run.along_u else "v")], False, "wall", order)
            face.move_to(run.plane)
            face.run = run
            run.faces.append(face)
            self.pieces.append([face])
        self.things = rest

    def place(self, obj, frm, order, entry, index, runs_by_tile):
        """A door, scenery object, item or critter: a face in a wall plane, a sprite card or a
        decal (things.py). entry: the end of its scene row; a critter's starts with its set."""
        art, sprite = self.art_of(obj, frm)
        x, y = proj.hex_px(obj.tile)
        centre = (x + obj.x, y + obj.y)                 # where the engine anchors the sprite
        proto = self.gf.protos.get(obj.pid)
        if len(entry) > 4:                              # a standing critter is always a card
            stack = self.stacked[obj.tile]
            index.add(things.Card(obj.tile, sprite[0], sprite[1] + obj.x, sprite[2] + obj.y, order, stack))
            self.critters.append(entry[:4] + [stack] + entry[4:])
            self.stacked[obj.tile] += 1
            return
        if obj.obj_type == ids.OBJ_TYPE_SCENERY and proto.subtype_name == "door":
            axis = walls.door_axis(obj.tile, self.wall_tiles, proto.flags_ext, *sprite)
            shape = things.door(obj, art, centre, sprite, axis, order, index, runs_by_tile)
        else:
            shape = things.stand(obj, art, centre, sprite, order, index, self.stacked[obj.tile])
        if shape == "flat":
            self.shape_use["flat"] += 1
            self.decals.append([self.frame_sprite(frm, obj.rotation, art[2], obj.x, obj.y)] + entry)
            return
        index.add(shape)                                # later things must agree with this one too
        if shape.kind == "card":
            self.add_card(obj, frm, shape, entry)
        else:
            self.sheets.append(shape)
            if shape.kind == "panel":
                self.panels.append((shape, frm, entry))

    def add_card(self, obj, frm, card, entry):
        frame = min(max(obj.frame, 0), frm.frame_count - 1)
        self.boards.append([self.frame_sprite(frm, obj.rotation, frame, obj.x, obj.y)] + entry[:2] + [card.stack] + entry[2:])
        self.card_info.append((obj, frm, card.order))
        self.stacked[obj.tile] += 1
        # A shadow painted into the sprite: for first person the card without it, and it alone to lie on the floor.
        left, top, _, _ = frm.placement(obj.rotation, frame, obj.x, obj.y)
        shadow = light.painted_shadow(frm.frame(obj.rotation, frame).array(), top)
        if shadow is not None:
            rgba = frm.rgba(self.gf.palette.glow_sprite, obj.rotation, frame)
            parts = [self.cropped(rgba * keep[..., None], left, top) for keep in (~shadow, shadow)]
            if None not in parts:
                self.shadows.append([len(self.boards) - 1] + parts + [frame, obj.rotation])

    def cropped(self, rgba, left, top):
        """Sprite of the opaque part of an image whose top-left pixel is (left, top) from the anchor, or None."""
        rows, columns = np.flatnonzero(rgba[..., 3].any(axis=1)), np.flatnonzero(rgba[..., 3].any(axis=0))
        if not len(rows):
            return None
        return self.sprite(rgba[rows[0]:rows[-1] + 1, columns[0]:columns[-1] + 1], left + int(columns[0]), top + int(rows[0]))

    def build_walls(self):
        """Give the queued faces their final planes and pixels, un-shear them, make the walls solid
        (boxes.py) and emit a row each."""
        runs = walls.align(self.pieces)
        runs_by_tile = {(run.along_u, face.obj.tile): run for run in runs for face in run.faces}
        self.join_odd_walls(runs_by_tile)
        wall_faces = [face for piece in self.pieces for face in piece]
        wall_faces += walls.fold_ends(runs)
        index = things.Index(wall_faces)
        for thing in self.things:
            self.place(*thing, index, runs_by_tile)
        for face, frm, entry in self.panels:            # sheets of scenery again, now that everything stands
            shape = things.reconsider(face, index, self.stacked[face.obj.tile])
            if shape is not face:
                index.remove(face)
                index.add(shape)
                self.sheets.remove(face)
                self.add_card(face.obj, frm, shape, entry)
        self.shape_use.update(["card"] * len(self.boards))
        self.shape_use.update(face.kind + (" on a wall" if face.run else "") for face in self.sheets)
        walls.resolve_overlaps(wall_faces)
        for face in self.sheets:
            if face.run is not None:
                things.trim(face, index)
        textures, cuts = {}, []
        for face in wall_faces + self.sheets:
            fid, rotation, frame = face.art
            shown = None if face.mask is None else face.mask.tobytes()
            key = face.art + (face.slope, face.x_from, face.x_to, shown)
            if key not in textures:
                frm = self.gf.art.load(fid)
                cut = walls.unshear(frm.rgba(self.gf.palette.glow_tile, rotation, frame), face.left, face.top,
                                    face.slope, face.x_from, face.x_to, face.mask)
                # A wall's face carries no holes: the art's enclosed transparent
                # texels fill with the edge colours, so a wall does not sparkle
                # with the room behind it (115). The backs and ends take their
                # material from this picture, and the openings of doors stay:
                # they open at the picture's border.
                cut = cut and (props.fill_holes(cut[0]),) + cut[1:]
                textures[key] = cut and (self.sprite(cut[0]),) + cut[1:]
            if textures[key] is not None:
                sprite, x0, x1, y_top = textures[key]           # the floor line is un-sheared y = c
                cuts.append(boxes.Cut(face, sprite, self.atlas.images[self.sprites[sprite][0]], x0, x1, y_top))
        for cut in cuts:
            if self.is_hide(cut.face.obj):              # a tent wall is a skin: what its picture shows above it is no top face
                cut.clean = False
        trims, self.boxes = boxes.thicken(cuts, self.roof.reshape(100, 100) != 0, self.sprite)
        self.trims = [[sprite] + _round(a + b + (z0, z1)) + texels for a, b, sprite, *texels, z0, z1 in trims]
        self.boxes = [_round(box) for box in self.boxes]
        for cut in cuts:
            face = cut.face
            quad = [cut.sprite] + _round(cut.a + cut.b + (cut.z0, cut.z1)) + [self.name_id(face.obj.pid), self.serial[id(face.obj)]]
            if face.kind == "door":
                self.door_quads.append(quad)
            else:
                self.sheet_rows[id(face)] = len(self.walls)
                self.walls.append(quad + _round((cut.zc, cut.thick)) + [cut.back] + (cut.above if max(cut.above) >= 0 else []))
            if face.solid:
                self.faced.add(id(face.obj))
                self.colliders += [_round(cut.at(i) + cut.at(j)) for i, j in cut.solid]
        self.cuts = cuts
        self.build_forms(cuts)
        self.build_props()

    # -------------------------------------------------------------------- forms
    def is_hide(self, obj):
        """Is a wall piece made of hide (its proto's material)? Tents are."""
        return obj.obj_type == ids.OBJ_TYPE_WALL and self.gf.protos.get(obj.pid).material == pro.MATERIAL_NAMES.index("leather")

    def add_form(self, rgba, x, y, faces):
        """A picture whose top-left corner is world pixel (x, y), and the faces that show it by projection."""
        self.forms.append([self.sprite(rgba)] + _round(proj.px_to_ground(x, y)) + [faces])

    def floor_picture(self, x0, y0, x1, y1):
        """The floor tiles of a rectangle of world pixels as the game paints them: rgba (y1 - y0, x1 - x0)."""
        out = np.zeros((y1 - y0, x1 - x0, 4), np.uint8)
        corners = [proj.px_to_ground(x, y) for x in (x0, x1) for y in (y0, y1)]
        for v in range(max(0, int(min(c[1] for c in corners)) - 1), min(100, int(max(c[1] for c in corners)) + 2)):
            for u in range(max(0, int(min(c[0] for c in corners)) - 1), min(100, int(max(c[0] for c in corners)) + 2)):
                if self.floor_ids[v * 100 + u] == NO_TILE:
                    continue
                rgba = self.gf.art.load(ids.make_fid(ids.OBJ_TYPE_TILE, int(self.floor_ids[v * 100 + u]))).rgba(self.gf.palette.glow_tile)
                x, y = (int(c) for c in proj.ground_to_px(u, v))        # the top corner of the square: the tile's pixel (48, 0)
                ax, ay, bx, by = max(x - 48, x0), max(y, y0), min(x - 48 + rgba.shape[1], x1), min(y + rgba.shape[0], y1)
                if ax < bx and ay < by:
                    part = rgba[ay - y:by - y, ax - x + 48:bx - x + 48]
                    out[ay - y0:by - y0, ax - x0:bx - x0][part[..., 3] > 0] = part[part[..., 3] > 0]
        return out

    def build_sets(self):
        """Set pieces (sets.py): the form that is written down for a sprite, in place of it and of its fellows."""
        self.set_boards = set()                         # the cards they stand for: no solids of those
        shown = {id(obj): ("board", board) for board, (obj, _, _) in enumerate(self.card_info)}
        shown.update({id(face.obj): ("wall", self.sheet_rows[id(face)]) for face in self.sheets if id(face) in self.sheet_rows})
        by_name = {}
        for obj, frm, order, _ in self.things:
            if id(obj) in shown:
                by_name.setdefault((self.gf.art.name(obj.fid) or "").lower(), []).append((order, obj, frm))
        for name, entry in sets.SHAPES.items():
            for first in by_name.get(name, ()):
                near = lambda other: geometry.distance(first[1].tile, other[1].tile) <= sets.REACH
                members = [first] + [m for fellow in entry["with"] for m in by_name.get(fellow, ()) if near(m)]
                sprites = []
                for _, part, art in sorted(members, key=lambda m: m[0]):
                    frame = min(max(part.frame, 0), art.frame_count - 1)
                    left, top, _, _ = art.placement(part.rotation, frame, part.x, part.y)
                    x, y = (int(round(c)) for c in proj.hex_px(part.tile))
                    sprites.append((art.rgba(self.gf.palette.glow_sprite, part.rotation, frame), x + left, y + top))
                form = sets.build(name, proj.hex_px(first[1].tile), self.floor_picture, sprites)
                if form:
                    self.add_form(*form)
                    self.shape_use["set piece"] += 1
                    for _, part, _ in members:
                        kind, index = shown[id(part)]
                        (self.set_boards.add if kind == "board" else self.hidden_walls.append)(index)

    def build_forms(self, cuts):
        """What first person draws in place of walls, roof squares and cards that no box does justice to:
        tents (tents.py) and set pieces (sets.py). The game's views keep the map's own pieces."""
        self.forms, self.hidden_walls, self.hidden_roof = [], [], []
        self.build_sets()
        rgba_of = lambda face: self.gf.art.load(face.art[0]).rgba(self.gf.palette.glow_tile, *face.art[1:])
        tile_rgba = lambda square: self.gf.art.load(ids.make_fid(ids.OBJ_TYPE_TILE, int(self.roof_ids[square]))).rgba(self.gf.palette.glow_tile)
        for squares, far, near, foot in tents.find(cuts, self.roof.reshape(100, 100) != 0, self.is_hide):
            pictures = tents.build(squares, far, near, foot, rgba_of, tile_rgba)
            if pictures:
                for picture in pictures:
                    self.add_form(*picture)
                self.hidden_walls += [self.sheet_rows[id(cut.face)] for cut in far + near]
                self.hidden_roof += squares
                self.shape_use["tent"] += 1

    # -------------------------------------------------------------------- props
    def piece(self, obj, frm, order, board=None, sheet=None):
        """The object's sprite on the game's screen (props.Piece), and whether it stands on the floor."""
        frame = min(max(obj.frame, 0), frm.frame_count - 1)
        left, top, _, height = frm.placement(obj.rotation, frame, obj.x, obj.y)
        x, y = (int(round(c)) for c in proj.hex_px(obj.tile))
        piece = props.Piece(obj, frm.frame(obj.rotation, frame).array(), frm.rgba(self.gf.palette.glow_sprite, obj.rotation, frame),
                            x + left, y + top, order, self.gf.protos.name(obj.pid), board, sheet)
        piece.stands = top + height > 0                 # else it hangs over its hex: on a wall
        return piece

    def out_of_walls(self, cu, cv, a, b):
        """A box a x b around (cu, cv), cut back where it reaches into a slab of wall: -> ((cu, cv), a, b).

        A box is the hull of what its sprite shows, a little larger than the thing;
        against a wall that would come out on the other side.
        """
        low, high = [cu - a / 2, cv - b / 2], [cu + a / 2, cv + b / 2]
        for u0, v0, u1, v1 in self.boxes:
            if low[0] < u1 and high[0] > u0 and low[1] < v1 and high[1] > v0:
                axis = int(u1 - u0 > v1 - v0)               # across the slab
                slab = ((u0, u1), (v0, v1))[axis]
                if (low[axis] + high[axis]) / 2 > (slab[0] + slab[1]) / 2:
                    low[axis] = max(low[axis], min(slab[1], high[axis] - MIN_BOX))
                else:
                    high[axis] = min(high[axis], max(slab[0], low[axis] + MIN_BOX))
        return ((low[0] + high[0]) / 2, (low[1] + high[1]) / 2), high[0] - low[0], high[1] - low[1]

    def keep_apart(self, things):
        """Solids that reach into a wall or into another thing are cut back (things: the thing of
        every row of self.props; rows of one thing are slices of one solid and stay as they are).

        A solid is the hull of what its sprite shows, a little larger than the thing. A drum
        beside a wall came out through it (a round stove in a corner), and two things side by
        side overlapped in plan and showed through each other from the air (a dresser and the
        crates against it). A box is cut back at the slabs of the walls as it is made
        (out_of_walls); a drum here, on the side of its own hex. Two solids whose heights meet
        are each cut back by half of their overlap, across the axis where they overlap less. A
        drum is cut as its square and becomes the largest drum within what is left: it keeps
        its near side and moves off the cut, so it still shows the sprite it was made of.
        """
        rows = [list(row) for row in self.props]        # the fly camera's; the game's camera keeps self.props
        fly = [row[15] != 2 for row in rows]

        def extent(row):
            cu, cv, a, b = row[5:9]
            half = (a / proj.BB_PX, a / proj.BB_PX) if row[4] else (a / 2, b / 2)
            return [cu - half[0], cv - half[1], cu + half[0], cv + half[1]]

        def shrink(row, cut):
            """Cut a row back to the extent `cut`."""
            if row[4]:
                row[5:7] = _round(((cut[0] + cut[2]) / 2, (cut[1] + cut[3]) / 2))
                row[7] = round(min(cut[2] - cut[0], cut[3] - cut[1]) / 2 * proj.BB_PX, 4)
                row[8] = round(row[7] * props.K, 4)
            else:
                row[5:9] = _round(((cut[0] + cut[2]) / 2, (cut[1] + cut[3]) / 2, cut[2] - cut[0], cut[3] - cut[1]))

        for row, seen in zip(rows, fly):
            if not row[4] or not seen:
                continue
            if row[15] == 1:
                continue        # a fly-only drum keeps its size: the game's own views are its
                                # card's, and in flight a little wall overlap beats a stump
            for u0, v0, u1, v1 in self.boxes:
                radius = row[7] / proj.BB_PX
                if math.hypot(row[5] - min(max(row[5], u0), u1), row[6] - min(max(row[6], v0), v1)) >= radius:
                    continue
                axis = int(u1 - u0 > v1 - v0)                   # across the slab
                slab, home, cut = ((u0, u1), (v0, v1))[axis], row[1 + axis], extent(row)
                if home > (slab[0] + slab[1]) / 2:              # its hex is past the slab: keep that side
                    cut[axis] = min(max(cut[axis], slab[1]), cut[axis + 2] - 2 * MIN_DRUM)
                else:
                    cut[axis + 2] = max(min(cut[axis + 2], slab[0]), cut[axis] + 2 * MIN_DRUM)
                shrink(row, cut)
        for i, row in enumerate(rows):
            for j in range(i + 1, len(rows)):
                other = rows[j]
                if not fly[i] or not fly[j] or things[i] == things[j] or row[9] >= other[10] or other[9] >= row[10]:
                    continue
                p, q = extent(row), extent(other)
                overlap = [min(p[2], q[2]) - max(p[0], q[0]), min(p[3], q[3]) - max(p[1], q[1])]
                if min(overlap) <= APART:
                    continue
                axis = int(overlap[1] < overlap[0])
                # Each gives way on the side that faces the other, to the plane halfway between
                # those sides (half the overlap, or more when one holds the other along the axis).
                (near, near_row), (far, far_row) = sorted(((p, row), (q, other)), key=lambda pair: pair[0][axis] + pair[0][axis + 2])
                plane = (near[axis + 2] + far[axis]) / 2

                def side(r):
                    return 2 * MIN_DRUM if r[4] else MIN_BOX

                near[axis + 2] = max(min(near[axis + 2], plane), near[axis] + side(near_row))
                far[axis] = min(max(far[axis], plane), far[axis + 2] - side(far_row))
                shrink(row, p)
                shrink(other, q)
        # The game's camera sees the card, and a solid only over it: one cut back would change
        # its view. So a solid that was cut stays as made for that camera, and the cut copy
        # is the fly camera's (row[15]: 0 both cameras, 1 the fly camera, 2 the game's). A
        # solid by name keeps its card in the game's view: its copy as made draws nowhere
        # (solids._forced) and only names the axis its rests stand on.
        for n, (row, cut) in enumerate(zip(list(self.props), rows)):
            if cut != row and row[15] == 1:
                self.props[n] = cut
            elif cut != row:
                row[15], cut[15] = 2, 1
                self.props.append(cut)

    def solid_of(self, index, left, top, force=False):
        """-> (the floor's shade in a sprite, props.fit of the rest); cached, sprites repeat."""
        key = (index.shape, index.tobytes(), left, top, force)
        if key not in self._solids:
            shade = light.outer_dark(index)
            painted = light.painted_shadow(index, top)
            if painted is not None:
                shade |= painted
            self._solids[key] = shade, props.fit((index != 0) & ~shade, left, top, force)
        return self._solids[key]

    def build_props(self):
        """What first person shows as solids instead of cards and sheets (props.py).

        Every standing card and sheet of scenery is tried, slices of one picture
        together. A solid takes the sprite without the shade painted on the floor
        around the object, which goes onto the floor (`casts`); what the solid
        does not account for becomes upright cards on it (`rests`), and loose
        things whose hex it covers are lifted onto its top (`lifts`). The game's
        views keep the cards: there a solid is only drawn over its own card.
        """
        self.props, self.rests, self.lifts, self.casts = [], [], [], []
        self.hidden, self.hidden_sheets = [], []        # boards / walls that first person does not draw
        self.blocks, self.posts, self.prop_blobs = [], [], []
        self.forced_boards = set()                      # boards the fly camera alone draws as solids: their cards shade the light
        self.propped = set()                            # objects that stop the player as solids, or not at all
        pieces = [self.piece(obj, frm, order, board=n) for n, (obj, frm, order) in enumerate(self.card_info) if n not in self.set_boards]
        formed = set(self.hidden_walls)                 # ... nor of the sheets that a form stands for
        pieces += [self.piece(face.obj, frm, face.order, sheet=self.sheet_rows[id(face)])       # not what is painted on walls
                   for face, frm, _ in self.panels if id(face) in self.sheet_rows and not face.obj.flags & mapfile.OBJECT_NO_BLOCK
                   and self.sheet_rows[id(face)] not in formed]
        solids = []
        things = []                                     # the thing (group) of each row of self.props
        # A detail joined to a bigger piece of its name (props.groups) changes what the game's camera
        # sees of both: for it they stay as they were made apart (seen 2), and the thing they make up
        # is the fly camera's (seen 1). A thing that is the same either way is both cameras' (0), and
        # so are the pieces of a joined thing that made no solid.
        standing = [piece for piece in pieces if piece.stands]
        joined = props.groups(standing)
        apart = props.groups(standing, details=False)
        keys = {frozenset(map(id, group)) for group in joined}
        alone = {frozenset(map(id, group)) for group in apart}
        whole = {id(piece): frozenset(map(id, group)) for group in joined for piece in group}
        todo = [(group, 0 if frozenset(map(id, group)) in alone else 1) for group in joined] \
            + [(group, None) for group in apart if frozenset(map(id, group)) not in keys]
        made = set()                                    # joined things that became the fly camera's solids
        fly_hidden = []                                 # boards that only a fly camera's solid stands for
        for thing, (group, seen) in enumerate(todo):
            first = group[0]
            if seen is None:
                seen = 2 if whole[id(first)] in made else 0
            # The mapper's loose clutter: its flag word carries nothing but light-through and its
            # own light, no bit that says the game treats it as a thing of the world (usable, flat,
            # walk-through, shoot-through). Such a pile keeps the card everywhere: stood up, its box
            # read as junk standing in an alley the game's own view keeps hidden behind a wall
            # (the bathhouse's junk piles, the pot-belly stoves: 2026-10-28). The lid-drum fit and
            # the standing rules are built and tested (props._lid_drum, tasks 116/119) but stand
            # down here: stood up, the neighbour groups re-compose, their fits and light models
            # flip, and the game's own views change on eight regression spots (87..1153 px a
            # view). Task 119 carries the group-safe version.
            if all(not piece.obj.flags & ~PLAIN_OBJECT_FLAGS & 0xFFFFFFFF for piece in group):
                continue
            blocking = any(not piece.obj.flags & mapfile.OBJECT_NO_BLOCK for piece in group)
            # A box by name (tools/scene_export/shapes/overrides.json): chairs and
            # crates the fit takes for plants.
            forced = FORCED.get((self.gf.art.name(first.obj.fid) or "").lower()) if len(group) == 1 else None
            if len(group) == 1 and not blocking and first.index.shape[1] < WALK_THROUGH and not forced:
                continue                                # loose things: cards
            index, rgba, x, y = props.compose(group) if len(group) > 1 else (first.index, first.rgba, first.x, first.y)
            ax, ay = (int(round(c)) for c in proj.hex_px(first.obj.tile))       # the anchor: the first piece's hex
            left, top = x - ax, y - ay
            shade, found = self.solid_of(index, left, top, forced)
            if not found:
                continue
            shape, fit = found
            if fit.side_fill >= props.BULK:             # no legs to see through: the black on it is its own
                shade = shade.copy()
                shade[shape.top - top:shape.top - top + shape.h, shape.left - left:shape.left - left + shape.w] &= ~fit.covers(shape)
            u, v = self.anchor(first.obj)
            du, dv = fit.ground()
            cu, cv = u + du, v + dv
            body = props.fill_holes(rgba * ~shade[..., None])
            sprite = self.cropped(body, left, top)
            if shade.any() and not forced and seen != 1:   # a solid by name: its card keeps the shade
                self.casts.append([self.cropped(rgba * shade[..., None], left, top)] + _round((u, v)))
            above, below = fit.rest(shape)
            window = body[shape.top - top:shape.top - top + shape.h, shape.left - left:shape.left - left + shape.w]

            def rest(mask, base, z, top=0.0):
                """The pixels `mask` as a card standing on the solid's axis, its screen point `base` at height
                z; top: the height its upper edge is drawn out to (0: as high as the pixels are)."""
                bx, by = int(round(base[0])), int(round(base[1]))
                self.rests.append([self.cropped(window * mask[..., None], shape.left - bx, shape.top - by)] + _round((cu, cv, z, top)) + [seen])

            round_ = fit.kind == "drum"
            if round_:
                radius = fit.a / proj.BB_PX
                parts = [((du, dv), fit.a, fit.a * props.K, fit.z0, fit.z1, fit.z1, fit.z1)]
                if above.sum() >= props.MIN_REST:
                    rest(above, (fit.centre[0], shape.top + np.flatnonzero(above.any(axis=1))[-1] + 1), fit.z1)
            else:
                parts = fit.boxes(shape)
            if below.sum() >= props.MIN_REST:           # a stem: the game's camera saw its upper end hidden by the
                rest(below, (fit.centre[0], fit.centre[1] + fit.z1 * proj.HPX), 0.0, fit.z0)    # solid; it reaches it
            # A thing of several pieces keeps its own depth (board -1), unless all but one are details
            # of that one (a bed's rail): then it is that piece's thing and takes its card's depth.
            main = first if len(group) == 1 else props.lead(group)
            board = main.board if main is not None and main.board is not None else -1
            for (pu, pv), a, b, z0, z1, wall_u, wall_v in parts:
                if not round_:
                    (pu, pv), a, b = self.out_of_walls(u + pu, v + pv, a, b)
                    pu, pv = pu - u, pv - v
                self.props.append([sprite] + _round((u, v)) + [board, int(round_)]
                                  + _round((u + pu, v + pv, a, b, z0, z1, wall_u, wall_v)) + [self.name_id(first.obj.pid)]
                                  + [int(bool(forced)), seen])
                things.append(thing)
                half = (radius, radius) if round_ else (a / 2, b / 2)
                area = math.pi * radius ** 2 if round_ else a * b
                if seen == 1:                           # the game's view and the player go by the pieces apart
                    continue
                if not forced:                          # a solid by name lifts nothing: the game's view keeps its cards
                    solids.append((round_, u + pu, v + pv, half, z1))
                if forced:
                    self.forced_boards.add(first.board)
                else:
                    self.prop_blobs.append((u + pu, v + pv, 1.15 * max(half), 1.15 * min(half)))
                if blocking and (area >= SMALL_AREA or z1 >= TALL):
                    if round_:
                        self.posts.append(_round((u + pu, v + pv, radius)))
                    else:
                        self.blocks.append(_round((u + pu - half[0], v + pv - half[1], u + pu + half[0], v + pv + half[1])))
            if seen == 1:
                made.add(frozenset(map(id, group)))
                fly_hidden += [piece.board for piece in group if piece.board is not None]
                for piece in group:                     # a stood-up panel hides its sheet
                    if piece.board is None and piece.sheet is not None:
                        self.hidden_sheets.append(piece.sheet)
                continue
            self.shape_use["drum" if round_ else "box"] += 1
            for piece in group:
                self.propped.add(id(piece.obj))
                if piece.board is not None:
                    self.hidden.append(piece.board)
                else:
                    self.hidden_sheets.append(piece.sheet)
        fly_hidden = set(fly_hidden) - set(self.hidden)
        self.lift(pieces, solids, set(self.hidden) | set(self.set_boards), fly_hidden)
        self.hidden += sorted(fly_hidden)
        self.forced_boards |= fly_hidden
        self.keep_apart(things)
        # A card shows the sprite without the shade a solid's card gets; and what was a painted
        # shadow of a card that is a solid now is among the casts.
        self.hidden += sorted(self.set_boards)
        hidden = set(self.hidden)
        # Plants stay cards, but a card that turns with the eye is no tree: first person stands such
        # a sprite up twice, crosswise, where it grows (scene.crossed). A plant is of wood (its proto's
        # material), ragged and airy, and looks much the same from the side: its sprite is its own
        # mirror image about the upright line through its hex. A plant baked as a solid is marked all
        # the same (scene.plants): its crown throws no real shadow (task 045's follow-up).
        self.plants = []
        self.crossed = []
        wood = pro.MATERIAL_NAMES.index("wood")
        for piece in pieces:
            obj = piece.obj
            if piece.board is None or not piece.stands or obj.obj_type != ids.OBJ_TYPE_SCENERY \
                    or self.gf.protos.get(obj.pid).material != wood:
                continue
            opaque = piece.index != 0
            shape = props.Shape(opaque, 0, 0)
            if shape.ragged >= PLANT[0] and shape.pixels <= PLANT[1] * shape.w * shape.h:
                axis = int(round(proj.hex_px(obj.tile)[0])) - piece.x      # the column of the hex centre
                twin = 2 * axis - 1 - np.arange(opaque.shape[1])
                there = (twin >= 0) & (twin < opaque.shape[1])
                if (opaque[:, there] & opaque[:, twin[there]]).sum() >= PLANT[2] * opaque.sum():
                    self.plants.append(piece.board)
                    if piece.board not in hidden:
                        self.crossed.append(piece.board)
        self.shadows = [row for row in self.shadows if row[0] not in hidden]
        self.solids = solids

    def lift(self, pieces, solids, hidden, fly_hidden):
        """Loose things whose hex a solid covers go onto its top (self.lifts), as the game's view has
        them: hidden, the boards its solids stand for. A baked card is lifted or skipped, not both:
        one of fly_hidden that is lifted leaves it and stays a lifted card in flight too."""
        for piece in pieces:
            obj = piece.obj
            if piece.board is None or piece.board in hidden or not piece.stands or not obj.flags & mapfile.OBJECT_NO_BLOCK:
                continue
            u, v = self.anchor(obj)
            for round_, cu, cv, half, z1 in solids:
                on = math.hypot(u - cu, v - cv) <= half[0] if round_ else \
                    abs(u - cu) <= half[0] + ON_TOP and abs(v - cv) <= half[1] + ON_TOP
                if on:                                  # the same pixels, but z1 up: that much nearer to the camera
                    self.lifts.append([piece.board] + _round(proj.delta_to_ground(0, z1 * proj.HPX) + (z1,)))
                    fly_hidden.discard(piece.board)
                    break

    # ------------------------------------------------------------------ objects
    def build_objects(self):
        self.walls, self.boards, self.decals, self.critters, self.sets = [], [], [], [], []
        self.pieces = []                # wall faces per object, until build_walls()
        self.things = []                # doors, scenery and items: placed once the walls stand
        self.stacked = Counter()        # cards per hex so far (proj.row_plane)
        self.sheets = []                # their faces, where they are wall-like
        self.panels = []                # (face, frm, scene row) of those that are not doors
        self.door_quads = []
        self.shadows = []               # [board, sprite without its painted shadow, the shadow, frame, rotation]
        self.plants = []                # boards of plants: their crowns throw no real shadow
        self.exits = []                 # which decals are exit grid hatching
        self.shape_use = Counter()
        self.colliders = []             # [ua, va, ub, vb]: the fronts of walls that stop the player (and light)
        self.card_info = []             # (object, frm, draw order) of every board
        self.sheet_rows = {}            # id(face) -> its row in self.walls
        self._solids = {}
        self.faced = set()              # objects whose collision is their wall face
        set_ids = {}
        self.wall_tiles = {o.tile for o in self.objects if o.obj_type == ids.OBJ_TYPE_WALL}
        # The engine draws hex by hex in ascending tile order, file order within a hex.
        for order, obj in enumerate(sorted(self.objects, key=lambda o: o.tile)):
            if not geometry.is_valid(obj.tile):
                continue
            frm = self.load_art(obj)
            if frm is None:
                continue
            frame = min(max(obj.frame, 0), frm.frame_count - 1)
            if not frm.frame(obj.rotation, frame).array().any():
                continue                                        # invisible helpers (block.frm)
            entry = _round(self.anchor(obj)) + [self.name_id(obj.pid), self.serial[id(obj)]]
            kind = obj.obj_type
            if obj.flags & mapfile.OBJECT_FLAT:
                if ids.is_exit_grid(obj.pid):
                    self.exits.append(len(self.decals))
                self.decals.append([self.frame_sprite(frm, obj.rotation, frame, obj.x, obj.y)] + entry)
            elif kind == ids.OBJ_TYPE_WALL:
                if not self.add_wall(obj, frm, order):
                    self.things.append((obj, frm, order, entry))
            elif kind == ids.OBJ_TYPE_CRITTER and frm.direction_count == 6 and ids.fid_anim(obj.fid) == 0:
                set_id = set_ids.get(obj.fid)
                if set_id is None:
                    set_id = set_ids[obj.fid] = len(self.sets)
                    self.sets.append(self.critter_set(frm))
                x, y = proj.hex_px(obj.tile)                    # frames are shared: the object's x / y move the anchor
                self.things.append((obj, frm, order, [set_id] + _round(proj.px_to_ground(x + obj.x, y + obj.y))
                                    + [obj.rotation % 6] + entry[2:]))
            else:
                self.things.append((obj, frm, order, entry))
        self.build_walls()

    def critter_set(self, frm):
        """Standing animation, six directions; frame offsets accumulate as the engine's animation does."""
        dirs = []
        for rotation in range(6):
            dx = dy = 0
            frames = []
            for n, frame in enumerate(frm.frames(rotation)[:MAX_IDLE_FRAMES]):
                if n:
                    dx, dy = dx + frame.x, dy + frame.y
                frames.append(self.frame_sprite(frm, rotation, n, dx, dy))
            dirs.append(frames)
        return {"fps": frm.fps or 10, "dirs": dirs}

    # ----------------------------------------------------------------- blocking
    def build_blocking(self):
        """Two views of what stops movement.

        blocked  the engine's rule, hex by hex (_obj_blocking_at, research/04 section 8)
        solid    hexes the viewer treats as pillars: the same minus doors (nothing can
                 open one yet), minus walls that collide as slabs (`boxes`), scenery that
                 collides as a solid (`blocks`, `posts`) and the invisible blockers
                 mappers put along both to square off the hex grid, minus cards too
                 small to be in anyone's way (chairs), plus every hex without floor art
                 under it, which fences in the map
        """
        blocked = np.zeros(geometry.HEX_COUNT, bool)
        solid = np.zeros(geometry.HEX_COUNT, bool)
        self.door_tiles = set()
        slabs = np.array(self.boxes, float).reshape(-1, 4)
        blocks = np.array(self.blocks + [[u - r, v - r, u + r, v + r] for u, v, r in self.posts], float).reshape(-1, 4)
        sizes = [(obj, frm.size(obj.rotation, min(max(obj.frame, 0), frm.frame_count - 1))) for obj, frm, _ in self.card_info]
        small = {id(obj) for obj, (width, height) in sizes if width < SMALL_CARD[0] and height < SMALL_CARD[1]}

        def near(tile, rects, reach):
            """Is the hex centre in one of the rectangles, or within reach of one?"""
            if not len(rects):
                return False
            p = np.array(proj.px_to_ground(*proj.hex_px(tile)))
            return np.hypot(*(np.clip(p, rects[:, :2], rects[:, 2:]) - p).T).min() < reach

        for obj in self.objects:
            if obj.flags & mapfile.OBJECT_NO_BLOCK or not geometry.is_valid(obj.tile):
                continue
            if ids.fid_type(obj.fid) not in (ids.OBJ_TYPE_CRITTER, ids.OBJ_TYPE_SCENERY, ids.OBJ_TYPE_WALL):
                continue
            tiles = [obj.tile]
            if obj.flags & mapfile.OBJECT_MULTIHEX:
                tiles += [tile for tile in geometry.neighbors(obj.tile) if tile != -1]
            blocked[tiles] = True
            door = obj.obj_type == ids.OBJ_TYPE_SCENERY and self.gf.protos.get(obj.pid).subtype_name == "door"
            if door:
                self.door_tiles.update(tiles)
            frm = self.load_art(obj)
            invisible = frm is None or not frm.frame(obj.rotation, min(max(obj.frame, 0), frm.frame_count - 1)).array().any()
            if door or id(obj) in self.faced or id(obj) in self.propped or id(obj) in small:
                continue
            if invisible and (near(obj.tile, slabs, WALL_BLOCKER_REACH) or near(obj.tile, blocks, AT_SOLID)):
                continue
            solid[tiles] = True
        for tile in range(geometry.HEX_COUNT):
            square = geometry.floor_square(tile)
            if square == -1 or self.floor[square] == 0:
                solid[tile] = True
        self.blocked, self.solid = blocked, solid

    # --------------------------------------------------------------------- rock
    def build_rock(self):
        """Underground, the rock around the open ground (rock.py): its faces and its material."""
        self.rock, self.rock_texture = None, None
        self.tiled = np.zeros((100, 100), bool)         # squares whose ceiling is their floor tile
        if not self.indoor:
            return
        # Where one can walk to from where the map is entered: doors open, and no further than an exit grid.
        free = ~self.blocked
        free[list(self.door_tiles)] = True
        exits = {obj.tile for obj in self.objects if ids.is_exit_grid(obj.pid)}
        todo, walked = [self.map.entering_tile], set()
        while todo:
            tile = todo.pop()
            if tile not in walked and geometry.is_valid(tile) and free[tile]:
                walked.add(tile)
                if tile not in exits:
                    todo += [near for near in geometry.neighbors(tile) if near != -1]
        tops = [cut for cut in self.cuts if cut.face.kind == "wall" and rock.is_top(cut.image)]
        self.hidden_walls += [self.sheet_rows[id(cut.face)] for cut in tops]
        shown = [cut for cut in self.cuts if cut.face.kind in ("wall", "door") and cut not in tops]
        # Nor is there rock where something stands that is to be seen.
        things = [self.anchor(obj) for obj, *_ in self.things]
        things += [(u, v) for _, cu, cv, (a, b), _ in self.solids
                   for u in np.arange(cu - a, cu + a + 0.25, 0.25) for v in np.arange(cv - b, cv + b + 0.25, 0.25)]
        mass, clear = rock.solid(self.lit, [self.anchor_of(tile) for tile in walked] + things,
                                 [cut.a + cut.b + (cut.thick,) for cut in shown], [cut.a + cut.b + (0.0,) for cut in tops])
        # Its material: the pictures of rock with rock behind them (not masonry: that stands free).
        stone = pro.MATERIAL_NAMES.index("stone")
        behind = lambda cut: np.add(cut.a, cut.b) / 2 - np.array((0, 1) if cut.face.along_u else (1, 0)) * (cut.thick + 0.25)
        cave = [cut for cut in shown if cut.face.kind == "wall" and self.gf.protos.get(cut.face.obj.pid).material == stone
                and mass[rock.cell(behind(cut))]]
        pictures = list({id(cut.image): cut.image for cut in cave}.values())
        samples = [image[image[..., 3] > 0][::7, :3] for image in pictures]
        if not samples:                                 # no rock in sight: the filler tile, lighter
            x, y = self.tile_cells[self.ground["cell"]]
            tile = self.tile_sheet[y + TILE_MARGIN:y + TILE_MARGIN + TILE_RES, x + TILE_MARGIN:x + TILE_MARGIN + TILE_RES]
            samples = [np.minimum(tile[tile[..., 3] > 0][:, :3].astype(int) * 2 + 24, 255)]
        self.rock = rock.loops(mass)
        self.rock_texture = rock.texture(pictures, np.concatenate(samples))
        # The ceiling is rock too, except in rooms that are built: where the nearest wall is no rock
        # face, the floor's own tiles repeat overhead.
        middles = np.array([np.add(cut.a, cut.b) / 2 for cut in shown]).reshape(-1, 2)
        built = np.array([cut not in cave for cut in shown], bool)
        v, u = np.nonzero(self.lit)
        if len(middles) and len(u):
            nearest = np.hypot(u[:, None] + 0.5 - middles[:, 0], v[:, None] + 0.5 - middles[:, 1]).argmin(axis=1)
            self.tiled[v, u] = built[nearest]

    @staticmethod
    def anchor_of(tile):
        return proj.px_to_ground(*proj.hex_px(tile))

    # ----------------------------------------------------------------- lighting
    def sprite_pixels(self, index):
        """(opaque pixels, left, top) of an entry of self.sprites."""
        image, ox, oy = self.sprites[index]
        return self.atlas.images[image][..., 3] > 0, ox, oy

    def build_light(self):
        """The baked light image (light.py) and the glows of visible flames and lamps."""
        cards = [(row[0], row[1], row[2], row[-1]) for row in self.boards]
        for board, body, _, _, _ in self.shadows:       # what stands there is the sprite without its painted shadow
            cards[board] = (body,) + cards[board][1:]
        cards += [(self.sets[row[0]]["dirs"][row[3]][0], row[1], row[2], row[-1]) for row in self.critters]
        solid = set(self.hidden) - self.forced_boards  # these throw the shade of their solid
        blobs = [light.blob(*self.sprite_pixels(sprite), u, v) for n, (sprite, u, v, _) in enumerate(cards) if n not in solid]
        blobs += self.prop_blobs
        stops = self.colliders + [door[1:5] for door in self.door_quads]
        everything = self.map.objects[self.elevation]
        sides = [side for u0, v0, u1, v1 in self.boxes for side in
                 (([u0, v0, u1, v0], [u0, v1, u1, v1]) if u1 - u0 > v1 - v0 else ([u0, v0, u0, v1], [u1, v0, u1, v1]))]
        doors = [door[1:5] for door in self.door_quads]         # they stop lamp light, but let the day in (light.daylight)
        self.light_image = light.bake(light.emitters(self.objects), stops, self.roof.reshape(100, 100) != 0, self.colliders,
                                      doors, sides, [b for b in blobs if b], self.indoor)
        # A glow sits on the hot pixels of a light emitter's sprite: embers are red, lamps bright.
        self.glows = []
        for sprite, u, v, serial in cards:
            obj = everything[serial]
            if obj.light_intensity <= 0 or not obj.flags & mapfile.OBJECT_LIGHTING:
                continue
            image, ox, oy = self.sprites[sprite]
            rgb = self.atlas.images[image][..., :3].astype(float)
            fire = np.clip(rgb[..., 0] - rgb[..., 1:].mean(axis=2) - 40, 0, None)
            lamp = np.clip(rgb.mean(axis=2) - 150, 0, None)
            hot = fire if fire.sum() > lamp.sum() else lamp
            if not hot.any():
                continue
            ys, xs = np.nonzero(hot)
            x, y = ox + np.average(xs, weights=hot[ys, xs]) + 0.5, oy + np.average(ys, weights=hot[ys, xs]) + 0.5
            # [u, v, px right of the anchor, height, r, g, b]
            self.glows.append(_round([u, v, x, max(-y / proj.HPX, 0.08)]) + list(GLOW_FIRE if hot is fire else GLOW_LAMP))

    # ------------------------------------------------------------------- output
    def export(self, out_dir):
        self.build_tiles()
        self.build_objects()
        self.build_blocking()
        self.build_rock()
        self.build_light()
        os.makedirs(out_dir, exist_ok=True)
        for name in os.listdir(out_dir):                # stale pages from an earlier export
            if name.endswith(".png"):
                os.remove(os.path.join(out_dir, name))
        from PIL import Image
        tile_bytes = save_png(os.path.join(out_dir, "tiles.png"), self.tile_sheet)
        Image.fromarray(self.light_image).save(os.path.join(out_dir, "light.png"), optimize=True)
        light_bytes = os.path.getsize(os.path.join(out_dir, "light.png"))
        rock_bytes = save_png(os.path.join(out_dir, "rock.png"), self.rock_texture) if self.rock else 0
        self.atlas.pack()
        pages = self.atlas.save(out_dir, "sprites")
        sprites = [list(self.atlas.rect(a)) + [ox, oy] for a, ox, oy in self.sprites]
        m = self.map
        ambient = self.script_light()
        scene = {
            "format": 6,                                        # 6: forms (tents.py)
            "map": m.base_name,
            "title": self.title(),
            "messages": self.game_messages(),
            "elevation": self.elevation,
            "proj": proj.as_json(),
            "spawn": {"tile": m.entering_tile, "rot": m.entering_rotation},
            # mapLoad resets the ambient light to full; the map script then sets it (script_light).
            "ambient": 1.0 if ambient is None else ambient,
            "daylight": ambient is None and not self.indoor,    # under the sky: the viewer may pick the time of day
            "indoor": self.indoor,                              # underground: no sky, the floor repeats as ceiling
            "ground": self.ground,                              # {cell, color}: the commonest floor tile
            # "bytes" of an image: its file size, for the viewer's progress bar.
            "light": {"image": "light.png", "res": light.RES, "bytes": light_bytes},    # R lamps, G daylight, B floor shade
            "glows": self.glows,                                # [u, v, px right, height, r, g, b]
            "tiles": {"image": "tiles.png", "size": [self.tile_sheet.shape[1], self.tile_sheet.shape[0]],
                      "bytes": tile_bytes, "res": TILE_RES, "margin": TILE_MARGIN, "cells": self.tile_cells},
            "floor": _b64(self.floor.astype("<u2")),        # 100x100 uint16: cell index + 1, 0 = none
            "roof": _b64(self.roof.astype("<u2")),
            "pages": pages,
            "sprites": sprites,                             # [page, x, y, w, h, ox, oy]
            # The last number of every row is the object's position in the map file (for tools).
            # Upright quads stand between ground points a and b, a on the left as one looks at them.
            # A wall is its picture on its front plane (boxes.py): above zc the picture is the top of
            # the slab, which is `thick` deep; back = the sprite of its far side, -1 = the same picture.
            # A wall drawn cut open has two more: the sprites that go on above zc on its front and back.
            "walls": self.walls,                            # [sprite, ua, va, ub, vb, z0, z1, name, object, zc, thick, back]
            "trims": self.trims,                            # [sprite, ua, va, ub, vb, z0, z1, tx, ty, tw, th]: ends of slabs
            "doors": self.door_quads,                       # [sprite, ua, va, ub, vb, z0, z1, name, object]; they open
            "boards": self.boards,                          # [sprite, u, v, stack, name, object]: stack = see proj.row_plane
            "shadows": self.shadows,                        # [board, sprite without painted shadow, shadow sprite, frame, rotation]
            # First person only (props.py). A solid shows `sprite`, whose anchor is ground point
            # (u, v), by projection from the game's camera. board: the card it replaces (several
            # boxes may share one), -1 = it replaces other or several things. A box (round 0) is a x b
            # units along u and v around (cu, cv), a drum (round 1) the cylinder over the floor
            # ellipse that is a x b px on screen. Both reach from height z0 to z1; a box's far
            # sides go on up: the one along u to wallU, the one along v to wallV.
            "props": self.props,                            # [sprite, u, v, board, round, cu, cv, a, b, z0, z1, wallU, wallV, name, forced, seen]
            "rests": self.rests,                            # [sprite, u, v, z, top, seen]: upright cards standing at height z
            "lifts": self.lifts,                            # [board, du, dv, z]: boards that stand on a solid: moved, and z up
            "crossed": self.crossed,                        # boards that stand as two crossed sheets (plants)
            "plants": self.plants,                          # boards of plants: cast no real shadow (baked.bin's flag)
            "casts": self.casts,                            # [sprite, u, v]: shade to lie on the floor, like `shadows`
            "hidden": sorted(self.hidden),                  # boards ...
            "flyBoards": sorted(self.forced_boards),        # (of them, those the game's view keeps as cards)
            "hiddenWalls": sorted(self.hidden_sheets + self.hidden_walls),      # ... walls ...
            "hiddenRoof": sorted(self.hidden_roof),         # ... and roof squares that a solid or a form stands for there
            # A form shows `sprite`, whose top-left pixel is ground point (u, v), by projection from the
            # game's camera. A face = [dim, normal, corners, from]: dim 0 = lit like a wall .. 1 = black;
            # corners = x, y, z of each, counter-clockwise seen from outside; from = the points whose
            # pixels the corners show if not their own, else 0.
            "forms": self.forms,                            # [sprite, u, v, faces]
            # Underground (rock.py): the faces of the rock round the open ground, as loops of corners
            # [u, v at the floor, half way up, at the height of the roofs], rock on the left of the way
            # round, in an image that repeats, `texels` of it to the unit.
            # tiled: bit per square, set where the ceiling is not rock but the square's floor tile.
            "rock": self.rock and {"image": "rock.png", "bytes": rock_bytes, "texels": rock.TEXELS, "loops": self.rock,
                                   "tiled": _b64(np.packbits(self.tiled, bitorder="little"))},
            "blocks": self.blocks,                          # [u0, v0, u1, v1]: boxes that stop the player
            "posts": self.posts,                            # [u, v, radius]: drums that do
            "decals": self.decals,                          # [sprite, u, v, name, object]
            "exits": self.exits,                            # indices into decals: the exit grid (drawn faintly in first person)
            "sets": self.sets,                              # {fps, dirs: 6 x [sprite per frame]}
            "critters": self.critters,                      # [set, u, v, rotation, stack, name, object]
            "blocked": _b64(np.packbits(self.blocked, bitorder="little")),   # bit per hex: the engine's rule
            "solid": _b64(np.packbits(self.solid, bitorder="little")),       # bit per hex: pillars for the viewer
            "boxes": self.boxes,                            # [u0, v0, u1, v1]: the slabs of wall that stop the player
            "names": self.names,
            "descriptions": self.descriptions,              # same order as names
            "wallClasses": {"frms": dict(self.wall_stats), "objects": dict(self.wall_use)},
            "shapes": dict(self.shape_use),                 # how doors, scenery and items were stood up
        }
        with open(os.path.join(out_dir, "scene.json"), "w") as f:
            json.dump(scene, f, separators=(",", ":"))
        return scene
