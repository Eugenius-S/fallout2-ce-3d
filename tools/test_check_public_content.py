import unittest
import zlib

from check_public_content import findings, image_metadata, metadata_findings


class PublicContentTests(unittest.TestCase):
    def test_clean_lines_and_allowed_email(self):
        patch = (
            "+++ b/README.md\n"
            "+Game files live in <game>; builds live in <worktree>.\n"
            "+Contact " + "noreply@" + "anthropic.com for upstream notices.\n"
        )
        self.assertEqual(findings(patch), [])

    def test_rejects_personal_identifiers_and_paths(self):
        username = "j" + "iraf"
        path = "E:" + "\\" + "private"
        server = "i" + "cons8" + "mcp"
        provider = "Y" + "andexDisk"
        address = "owner" + "@example.com"
        patch = "+++ b/private.txt\n" + "\n".join(
            "+" + value for value in (username, path, server, provider, address)
        )
        labels = {finding[2] for finding in findings(patch)}
        self.assertEqual(
            labels,
            {
                "Windows user name",
                "drive path",
                "personal MCP name",
                "local storage provider",
                "email address",
            },
        )

    def test_email_scan_excludes_vendor_and_license_notices(self):
        address = "owner" + "@example.com"
        patch = (
            "+++ b/ce/third_party/demo/source.c\n+" + address + "\n"
            "+++ b/docs/LICENSE-demo.txt\n+" + address + "\n"
            "+++ b/tasks/notes.md\n+" + address + "\n"
        )
        results = findings(patch)
        self.assertEqual(len(results), 1)
        self.assertEqual(results[0][0], "tasks/notes.md")
        self.assertEqual(results[0][2], "email address")

    def test_scans_image_metadata_but_not_jpeg_pixels(self):
        username = b"ji" + b"raf"
        marker = b"\xff\xe1\x00\x0c" + username + b"12345"
        sos = b"\xff\xda\x00\x08" + b"header"
        pixel_data = b"encoded-" + username + b"-pixels"
        jpeg = b"\xff\xd8" + marker + sos + pixel_data
        self.assertEqual(image_metadata(jpeg, ".jpg"), [username + b"12345"])
        pixel_only = b"\xff\xd8" + sos + pixel_data
        self.assertEqual(image_metadata(pixel_only, ".jpg"), [])

    def test_scans_compressed_png_text_metadata(self):
        path = "E:" + "\\" + "private"
        compressed_text = zlib.compress(path.encode())
        ztxt = b"key\0\0" + compressed_text
        itxt = b"key\0\1\0\0\0" + zlib.compress(path.encode())

        def chunk(kind, payload):
            return (
                len(payload).to_bytes(4, "big")
                + kind
                + payload
                + b"\0\0\0\0"
            )

        png = (
            b"\x89PNG\r\n\x1a\n"
            + chunk(b"zTXt", ztxt)
            + chunk(b"iTXt", itxt)
            + chunk(b"IEND", b"")
        )
        payloads = image_metadata(png, ".png")
        self.assertEqual(len(payloads), 2)
        self.assertEqual(metadata_findings("runs/capture.png", payloads), ["drive path", "drive path"])

    def test_metadata_email_exemption_covers_vendored_and_license_paths(self):
        address = "author" + "@example.com"
        self.assertEqual(
            metadata_findings("ce/third_party/example/image.png", [address.encode()]),
            [],
        )
        self.assertEqual(
            metadata_findings("tools/LICENSE-demo.txt", [address.encode()]),
            [],
        )


if __name__ == "__main__":
    unittest.main()
