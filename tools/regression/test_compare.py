import tempfile
import unittest
from pathlib import Path

from PIL import Image

from compare import compare_images


class PixelComparisonTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.image = Image.new("RGBA", (10, 10), (30, 40, 50, 255))
        self.image.save(self.root / "reference.png")

    def compare(self):
        self.image.save(self.root / "actual.png")
        return compare_images(self.root / "reference.png", self.root / "actual.png", self.root / "diff")

    def test_identical_pixels_pass(self):
        self.assertEqual(self.compare()["changed_pixels"], 0)

    def test_small_channel_noise_is_ignored(self):
        self.image.putpixel((5, 6), (32, 40, 50, 255))
        self.assertEqual(self.compare()["status"], "pass")

    def test_visible_change_fails_and_is_located(self):
        self.image.putpixel((5, 6), (90, 40, 50, 255))
        result = self.compare()
        self.assertEqual(result["status"], "fail")
        self.assertEqual(result["difference_bbox"], (5, 6, 6, 7))
        self.assertTrue((self.root / "diff/difference.png").is_file())

    def test_alpha_change_is_not_lost(self):
        self.image.putpixel((5, 6), (30, 40, 50, 0))
        self.assertEqual(self.compare()["status"], "fail")

    def test_resolution_change_fails(self):
        self.image = self.image.resize((20, 20))
        self.assertEqual(self.compare()["error"], "image dimensions differ")


if __name__ == "__main__":
    unittest.main()
