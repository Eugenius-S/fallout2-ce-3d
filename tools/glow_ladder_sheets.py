"""Make 073 comparison sheets from the untracked frozen-scene captures."""

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageStat


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--captures', type=Path, default=Path('build/glow073'))
    parser.add_argument('--output', type=Path, default=Path('runs/073-glow-ladder'))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    # Identical crop at every level, never per-frame exposure or color changes.
    specs = [
        ('barrel', 'final/barrel', (1360, 540, 1920, 980)),
        ('campfire', 'final/campfire', (690, 250, 1230, 690)),
        ('monitor', 'final/monitor', (600, 0, 1100, 450)),
        ('slime', 'final/slime', (1140, 480, 1800, 980)),
        ('screens', 'clean/screens', (650, 50, 1250, 550)),
    ]
    strengths = [None, 0.5, 1, 2, 4, 8]
    report = []
    for kind, relative, crop in specs:
        directory = args.captures / relative
        files = sorted(directory.glob('devshot-*.json'))
        if len(files) != 6:
            raise ValueError(f'{kind}: expected exactly six captures, got {len(files)}')
        rows = [json.loads(f.read_text(encoding='utf-8-sig')) for f in files]
        images = []
        evidence = []
        stable = ['map', 'elevation', 'view_tile', 'camera', 'hero', 'resolution', 'exe_sha256']
        for index, (row, strength) in enumerate(zip(rows, strengths)):
            if row['glow']['enabled'] != (index > 0):
                raise ValueError(f'{kind}: incorrect glow state at {index}')
            if strength is not None and row['glow']['strength'] != strength:
                raise ValueError(f'{kind}: incorrect strength at {index}')
            if any(row[key] != rows[0][key] for key in stable):
                raise ValueError(f'{kind}: scene changed between levels')
            path = directory / row['image']
            with Image.open(path) as source:
                image = source.convert('RGB').crop(crop)
            images.append(image)
            evidence.append({'level': index, 'strength': strength,
                             'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                             'mean_rgb': ImageStat.Stat(image).mean})
        sheet = Image.new('RGB', (2160, 354), (18, 18, 18))
        draw = ImageDraw.Draw(sheet)
        draw.text((10, 5), kind, fill='white')
        for index, image in enumerate(images):
            preview = image.copy()
            preview.thumbnail((360, 300))
            sheet.paste(preview, (index * 360, 54))
            label = 'OFF' if index == 0 else f'{index} | glow_strength = {strengths[index]}'
            draw.text((index * 360 + 10, 30), label, fill='white')
        sheet.save(args.output / f'{kind}.jpg', quality=90)
        # Pixel statistics use original crops, before sheet downsampling.
        diff = ImageChops.difference(images[0], images[-1])
        strongest = ImageChops.lighter(ImageChops.lighter(diff.getchannel('R'), diff.getchannel('G')), diff.getchannel('B'))
        changed = sum(strongest.histogram()[9:])
        report.append({'kind': kind, 'captures': relative, 'crop': crop,
                       'map': rows[0]['map'], 'view_tile': rows[0]['view_tile'],
                       'changed_pixels_over_8': changed, 'levels': evidence})
        print(f'{kind}: {changed} pixels change by more than 8/255 at strength 8')
    (args.output / 'measurements.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for name, kinds in [('fires', ['barrel', 'campfire']), ('computers', ['monitor', 'screens'])]:
        combined = Image.new('RGB', (2160, 708), (18, 18, 18))
        for index, kind in enumerate(kinds):
            with Image.open(args.output / f'{kind}.jpg') as source:
                combined.paste(source, (0, index * 354))
        combined.save(args.output / f'{name}.jpg', quality=90)


if __name__ == '__main__':
    main()
