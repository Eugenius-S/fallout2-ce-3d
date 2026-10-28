import unittest

from check_public_content import findings, image_metadata


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

    def test_scans_image_metadata_but_not_jpeg_pixels(self):
        username = b"ji" + b"raf"
        marker = b"\xff\xe1\x00\x0c" + username + b"12345"
        sos = b"\xff\xda\x00\x08" + b"header"
        pixel_data = b"encoded-" + username + b"-pixels"
        jpeg = b"\xff\xd8" + marker + sos + pixel_data
        self.assertEqual(image_metadata(jpeg, ".jpg"), [username + b"12345"])
        pixel_only = b"\xff\xd8" + sos + pixel_data
        self.assertEqual(image_metadata(pixel_only, ".jpg"), [])


if __name__ == "__main__":
    unittest.main()
