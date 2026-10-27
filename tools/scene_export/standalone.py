#!/usr/bin/env python3
"""Entry point for the standalone scene exporter (task 091).

PyInstaller packs this file together with the whole tools/scene_export tree
as data.  At runtime the tree is extracted under sys._MEIPASS/scene_export
and imported from there, so every Path(__file__) lookup inside it (vendor/fp
and vendor/f2lib reaching two levels up for shapes/) behaves exactly as in
the repository.  The imports below exist for PyInstaller's static analysis:
without them the stdlib modules and the numpy/Pillow binaries used by the
data-only tree would not be bundled.
"""

from __future__ import annotations

import argparse  # noqa: F401
import base64  # noqa: F401
import collections  # noqa: F401
import contextlib  # noqa: F401
import fnmatch  # noqa: F401
import gzip  # noqa: F401
import hashlib  # noqa: F401
import json  # noqa: F401
import math  # noqa: F401
import os  # noqa: F401
import re  # noqa: F401
import shutil  # noqa: F401
import struct  # noqa: F401
import sys  # noqa: F401
import tempfile  # noqa: F401
import zlib  # noqa: F401
from pathlib import Path  # noqa: F401

import numpy  # noqa: F401
import PIL.Image  # noqa: F401
import PIL.ImageDraw  # noqa: F401
import PIL.ImageFilter  # noqa: F401

if getattr(sys, "frozen", False):
    here = Path(sys._MEIPASS) / "scene_export"
    sys.path.insert(0, str(here))
    sys.path.insert(0, str(here / "vendor"))

from export import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main())
