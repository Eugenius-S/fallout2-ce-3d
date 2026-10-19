import json
import tempfile
import unittest
from pathlib import Path

from PIL import Image

from check import load_spots, main, read_capture


class CaptureValidationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.spot = {"name": "one", "map": "kladwtwn", "elevation": 0, "tile": 25075}
        self.screen = {"width": 640, "height": 480, "scale": 1}
        self.metadata = {"map": "KLADWTWN.MAP", "elevation": 0, "view_tile": 25075,
                         "still_snapshot": True, "renderer": "scene-gl", "preview_availability": "available",
                         "view": "static-wall-plus-hero-preview", "image": "scene.png", "resolution": "640x480",
                         "logical_resolution": "640x480", "scale": 1, "windowed": True}
        self.capture = self.root / "one"
        self.capture.mkdir()
        Image.new("RGBA", (640, 480), "gray").save(self.capture / "scene.png")
        self.write_metadata()

    def write_metadata(self):
        (self.capture / "devshot-test.json").write_text(json.dumps(self.metadata), encoding="utf-8")

    def test_fallback_picture_is_rejected(self):
        self.metadata["preview_availability"] = "renderer-inactive"
        self.write_metadata()
        with self.assertRaisesRegex(ValueError, "preview_availability"):
            read_capture(self.capture, self.spot, self.screen)

    def test_wrong_camera_hex_is_rejected(self):
        self.metadata["view_tile"] = 24481
        self.write_metadata()
        with self.assertRaisesRegex(ValueError, "view_tile"):
            read_capture(self.capture, self.spot, self.screen)

    def test_wrong_scale_is_rejected(self):
        self.metadata["scale"] = 2
        self.write_metadata()
        with self.assertRaisesRegex(ValueError, "capture scale"):
            read_capture(self.capture, self.spot, self.screen)

    def test_incompatible_manifest_scale_is_rejected(self):
        manifest = self.root / "spots.json"
        manifest.write_text(json.dumps({"spots": [self.spot], "screen": {"width": 1920, "height": 1080, "scale": 4},
                                        "channel_tolerance": 2, "max_changed_fraction": 0.00001}), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "fixed screen"):
            load_spots(manifest)

    def test_wrong_resolution_is_rejected_even_for_update(self):
        self.metadata["resolution"] = "640x480"
        self.write_metadata()
        manifest = self.root / "spots.json"
        manifest.write_text(json.dumps({"spots": [self.spot], "screen": {"width": 1920, "height": 1080, "scale": 2},
                                        "channel_tolerance": 2, "max_changed_fraction": 0.00001}), encoding="utf-8")
        references = self.root / "references"
        self.assertEqual(main(["--spots", str(manifest), "--run", str(self.root),
                               "--reference", str(references), "--update"]), 1)
        self.assertFalse(references.exists())

    def test_broken_png_is_rejected_even_for_update(self):
        (self.capture / "scene.png").write_bytes(b"broken")
        with self.assertRaises(OSError):
            read_capture(self.capture, self.spot, self.screen)

    def test_missing_capture_does_not_update_other_references(self):
        manifest = self.root / "spots.json"
        second = {**self.spot, "name": "two"}
        manifest.write_text(json.dumps({"spots": [self.spot, second], "screen": self.screen, "channel_tolerance": 2,
                                        "max_changed_fraction": 0.00001}), encoding="utf-8")
        references = self.root / "references"
        self.assertEqual(main(["--spots", str(manifest), "--run", str(self.root),
                               "--reference", str(references), "--update"]), 1)
        self.assertFalse(references.exists())
        with self.assertRaisesRegex(ValueError, "unknown spot"):
            load_spots(manifest, "typo")


if __name__ == "__main__":
    unittest.main()
