"""Checks shared by the sidecar encoders."""

import re


def one_map_elevation(scene):
    """The scene is of one map elevation: a plain map name and elevation 0-2."""
    name = scene.get("map")
    return isinstance(name, str) and re.fullmatch(r"[a-z0-9_]+", name) is not None and scene.get("elevation") in (0, 1, 2)
