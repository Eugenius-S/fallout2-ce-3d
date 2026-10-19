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
        self.metadata = {"map": "KLADWTWN.MAP", "elevation": 0, "view_tile": 25075,
                         "still_snapshot": True, "renderer": "scene-gl", "preview_availability": "available",
                         "view": "static-wall-plus-hero-preview", "image": "scene.png", "resolution": "10x10"}
        self.capture = self.root / "one"
        self.capture.mkdir()
        Image.new("RGBA", (10, 10), "gray").save(self.capture / "scene.png")
        self.write_metadata()

    def write_metadata(self):
        (self.capture / "devshot-test.json").write_text(json.dumps(self.metadata), encoding="utf-8")

    def test_fallback_picture_is_rejected(self):
        self.metadata["preview_availability"] = "renderer-inactive"
        self.write_metadata()
        with self.assertRaisesRegex(ValueError, "preview_availability"):
            read_capture(self.capture, self.spot)

    def test_wrong_camera_hex_is_rejected(self):
        self.metadata["view_tile"] = 24481
        self.write_metadata()
        with self.assertRaisesRegex(ValueError, "view_tile"):
            read_capture(self.capture, self.spot)

    def test_broken_png_is_rejected_even_for_update(self):
        (self.capture / "scene.png").write_bytes(b"broken")
        with self.assertRaises(OSError):
            read_capture(self.capture, self.spot)

    def test_missing_capture_does_not_update_other_references(self):
        manifest = self.root / "spots.json"
        second = {**self.spot, "name": "two"}
        manifest.write_text(json.dumps({"spots": [self.spot, second], "channel_tolerance": 2,
                                        "max_changed_fraction": 0.00001}), encoding="utf-8")
        references = self.root / "references"
        self.assertEqual(main(["--spots", str(manifest), "--run", str(self.root),
                               "--reference", str(references), "--update"]), 1)
        self.assertFalse(references.exists())
        with self.assertRaisesRegex(ValueError, "unknown spot"):
            load_spots(manifest, "typo")


if __name__ == "__main__":
    unittest.main()
