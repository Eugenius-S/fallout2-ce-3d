"""Reject personal paths and account identifiers in lines added by a PR."""
import re
import subprocess
import sys
import zlib


RULES = (
    ("Windows user name", re.compile("j" + "iraf", re.IGNORECASE)),
    ("drive path", re.compile(r"\b[A-Z]:\\", re.IGNORECASE)),
    ("personal MCP name", re.compile("[i]" + "cons8", re.IGNORECASE)),
    ("local storage provider", re.compile("[Y]" + "andexDisk", re.IGNORECASE)),
    (
        "email address",
        re.compile(r"\b[A-Za-z0-9._%+-]+[@][A-Za-z0-9.-]+\.[A-Za-z]{2,}\b"),
    ),
)
ALLOWED_EMAIL = "noreply@" + "anthropic.com"


def findings(patch):
    results = []
    path = ""
    for line_number, line in enumerate(patch.splitlines(), 1):
        if line.startswith("+++ b/"):
            path = line[6:]
        if not line.startswith("+") or line.startswith("+++"):
            continue
        added = line[1:]
        for label, pattern in RULES:
            if label == "email address" and email_exempt_path(path):
                continue
            for match in pattern.finditer(added):
                if label == "email address" and match.group(0).lower() == ALLOWED_EMAIL:
                    continue
                results.append((path, line_number, label))
    return results


def email_exempt_path(path):
    parts = path.lower().split("/")
    name = parts[-1]
    return (
        bool(
            {"third_party", "third-party", "thirdparty", "vendor", "vendors", "vendored"}
            & set(parts)
        )
        or "license" in parts
        or "licenses" in parts
        or name.startswith(("license", "copying", "notice"))
    )


def _inflate(data):
    try:
        return zlib.decompressobj().decompress(data, 1024 * 1024)
    except zlib.error:
        return b""


def image_metadata(data, suffix):
    """Return metadata payloads, excluding encoded image pixels."""
    suffix = suffix.lower()
    chunks = []
    if suffix in {".jpg", ".jpeg"} and data.startswith(b"\xff\xd8"):
        offset = 2
        while offset + 4 <= len(data) and data[offset] == 0xFF:
            while offset < len(data) and data[offset] == 0xFF:
                offset += 1
            if offset >= len(data):
                break
            marker = data[offset]
            offset += 1
            if marker in {0xD9, 0xDA}:
                break
            if marker in {0x01} or 0xD0 <= marker <= 0xD7:
                continue
            if offset + 2 > len(data):
                break
            size = int.from_bytes(data[offset : offset + 2], "big")
            if size < 2 or offset + size > len(data):
                break
            payload = data[offset + 2 : offset + size]
            if 0xE0 <= marker <= 0xEF or marker == 0xFE:
                chunks.append(payload)
            offset += size
    elif suffix == ".png" and data.startswith(b"\x89PNG\r\n\x1a\n"):
        offset = 8
        while offset + 12 <= len(data):
            size = int.from_bytes(data[offset : offset + 4], "big")
            kind = data[offset + 4 : offset + 8]
            end = offset + 8 + size
            if end + 4 > len(data):
                break
            payload = data[offset + 8 : end]
            if kind == b"tEXt":
                chunks.append(payload)
            elif kind == b"zTXt":
                _, separator, rest = payload.partition(b"\0")
                if separator and len(rest) >= 2 and rest[0] == 0:
                    chunks.append(_inflate(rest[1:]))
            elif kind == b"iTXt":
                _, separator, rest = payload.partition(b"\0")
                if separator and len(rest) >= 2:
                    compressed, method, rest = rest[0], rest[1], rest[2:]
                    _, separator, rest = rest.partition(b"\0")
                    if separator:
                        _, separator, text = rest.partition(b"\0")
                        if separator:
                            if compressed == 1 and method == 0:
                                chunks.append(_inflate(text))
                            elif compressed == 0:
                                chunks.append(text)
            elif kind == b"eXIf":
                chunks.append(payload)
            offset = end + 4
            if kind == b"IEND":
                break
    elif suffix == ".webp" and data.startswith(b"RIFF") and data[8:12] == b"WEBP":
        offset = 12
        while offset + 8 <= len(data):
            kind = data[offset : offset + 4]
            size = int.from_bytes(data[offset + 4 : offset + 8], "little")
            start = offset + 8
            end = start + size
            if end > len(data):
                break
            if kind in {b"EXIF", b"XMP ", b"ICCP"}:
                chunks.append(data[start:end])
            offset = end + (size & 1)
    return chunks


def metadata_findings(path, payloads):
    problems = []
    for payload in payloads:
        text = payload.replace(b"\0", b"").decode("latin1", "ignore")
        for label, pattern in RULES:
            if label == "email address" and email_exempt_path(path):
                continue
            for match in pattern.finditer(text):
                if label == "email address" and match.group(0).lower() == ALLOWED_EMAIL:
                    continue
                problems.append(label)
    return problems


def main():
    if len(sys.argv) != 2:
        print("usage: check_public_content.py <base-ref>")
        return 2
    base = sys.argv[1]
    proc = subprocess.run(
        ["git", "diff", "--no-ext-diff", "--unified=0", f"{base}...HEAD", "--", "."],
        check=True,
        capture_output=True,
        text=True,
        errors="replace",
    )
    problems = findings(proc.stdout)
    names = subprocess.run(
        ["git", "diff", "--name-only", "--diff-filter=AM", f"{base}...HEAD", "--", "."],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.splitlines()
    for name in names:
        if not name.lower().endswith((".jpg", ".jpeg", ".png", ".webp")):
            continue
        blob = subprocess.run(
            ["git", "show", f"HEAD:{name}"],
            check=True,
            capture_output=True,
        ).stdout
        for payload in image_metadata(blob, "." + name.rsplit(".", 1)[-1]):
            for label in metadata_findings(name, [payload]):
                problems.append((name, 0, f"image metadata: {label}"))
    for path, line, label in problems:
        print(f"{path}: added line {line}: {label}")
    if problems:
        print("Remove personal paths and account identifiers from added lines.")
        return 1
    print("No personal paths or account identifiers in added lines.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
