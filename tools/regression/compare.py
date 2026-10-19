"""Compare decoded RGBA pixels; ignore PNG compression and metadata."""

from pathlib import Path

from PIL import Image, ImageChops, ImageEnhance


def compare_images(reference, actual, output, channel_tolerance=2, max_changed_fraction=0.00001):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    with Image.open(reference) as source:
        expected = source.convert("RGBA")
    with Image.open(actual) as source:
        captured = source.convert("RGBA")
    if expected.size != captured.size:
        return {"status": "fail", "error": "image dimensions differ",
                "reference_size": expected.size, "actual_size": captured.size}

    difference = ImageChops.difference(expected, captured)
    channels = difference.split()
    maximum = channels[0]
    for channel in channels[1:]:
        maximum = ImageChops.lighter(maximum, channel)
    mask = maximum.point(lambda value: 255 if value > channel_tolerance else 0)
    changed = mask.histogram()[255]
    fraction = changed / (captured.width * captured.height)
    result = {
        "status": "pass" if fraction <= max_changed_fraction else "fail",
        "changed_pixels": changed,
        "changed_fraction": fraction,
        "max_channel_delta": maximum.getextrema()[1],
        "difference_bbox": mask.getbbox(),
    }
    if changed:
        # Include alpha-only changes in the difference image as well.
        visible = ImageChops.lighter(difference.convert("RGB"), Image.merge("RGB", (channels[3],) * 3))
        ImageEnhance.Brightness(visible).enhance(8).save(output / "difference.png")
        highlight = Image.blend(captured.convert("RGB"), Image.new("RGB", captured.size, "red"), 0.65)
        Image.composite(highlight, captured.convert("RGB"), mask).save(output / "highlight.png")
    return result
