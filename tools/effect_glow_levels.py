"""086: compare the requested common glow levels without changing source gains."""
import hashlib
import json
import argparse
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageOps

from effect_review_sheets import sign_crop

ROOT = Path('build/effects086')
OUT = Path('runs/086-weapon-glow-previews')
LEVELS = [0.7, 0.8, 1.0, 1.3, 1.5, 1.8]


def sign_low():
    references = json.loads((OUT / 'glow-levels-measurements.json').read_text())
    sheet = Image.new('RGB', (1080, 640), (18, 18, 18))
    draw = ImageDraw.Draw(sheet)
    report = []
    for row_index, name in enumerate(['sign40', 'sign41']):
        reference = next(r for r in references if r['source']['name'] == name)
        directory = ROOT / 'sign-low' / name
        files = sorted(directory.glob('*.json'))
        assert len(files) == 4, (name, len(files))
        rows = [json.loads(p.read_text()) for p in files]
        assert [r['glow']['enabled'] for r in rows] == [False, True, True, True]
        assert [r['glow']['strength'] for r in rows[1:]] == [0.3, 0.4, 0.5]
        for row in rows:
            assert row['exe_sha256'] == reference['metadata'][0]['exe_sha256']
            assert row['effect_emission'] and row['light'] == 'enabled'
            for key in ['map', 'elevation', 'hero', 'camera', 'view_tile', 'weapon_effect', 'resolution', 'scale']:
                assert row[key] == reference['metadata'][0][key], (name, key)
        paths = [directory / row['image'] for row in rows]
        images = [Image.open(p).convert('RGB') for p in paths]
        previous = ROOT / 'tuning' / name
        old = json.loads(sorted(previous.glob('*.json'))[0].read_text())
        assert np.array_equal(np.asarray(images[0]), np.asarray(Image.open(previous / old['image']).convert('RGB')))
        crop = reference['crop']
        arrays = [np.asarray(im.crop(crop)).astype(int) for im in images]
        energy = [float(np.maximum(a - arrays[0], 0).mean()) for a in arrays[1:]]
        assert 0 < energy[0] < energy[1] < energy[2], (name, energy)
        y = row_index * 320
        draw.text((8, y + 5), name + ' | ' + reference['source']['art'] + ' | night', fill='white')
        for column, im in enumerate(images[1:]):
            preview = ImageOps.contain(im.crop(crop), (360, 275), Image.Resampling.NEAREST)
            sheet.paste(preview, (column * 360, y + 42))
            draw.text((column * 360 + 8, y + 24), 'Shared glow ' + str([0.3, 0.4, 0.5][column]), fill='white')
        report.append(dict(source=reference['source'], crop=crop, levels=[0.3, 0.4, 0.5],
                           identical_previous_off=True, added_rgb_mean=energy, metadata=rows,
                           image_sha256=[hashlib.sha256(p.read_bytes()).hexdigest() for p in paths]))
    sheet.save(OUT / 'night-signs-low.jpg', quality=88)
    assert (OUT / 'night-signs-low.jpg').stat().st_size < 1_000_000
    (OUT / 'sign-low-measurements.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Validated two OFF/0.3/0.4/0.5 sign sequences; previous build, state and OFF pixels match.')


def main():
    specs = json.loads((ROOT / 'all-specs.json').read_text())
    specs += [dict(name='sign40', art='SIGN40.FRM', fid=33556182, frame=0, existing=True),
              dict(name='sign41', art='SIGN41.FRM', fid=33556183, frame=0, existing=True)]
    cards, report, builds = {}, [], set()
    for spec in specs:
        name = spec['name']
        sequences, source_paths = [], []
        for batch, levels in [('tuning-low', LEVELS[:3]), ('tuning', LEVELS[3:])]:
            directory = ROOT / batch / name
            files = sorted(directory.glob('*.json'))
            assert len(files) == 4, (name, batch, len(files))
            sequence = [json.loads(f.read_text()) for f in files]
            assert [r['glow']['enabled'] for r in sequence] == [False, True, True, True]
            assert [r['glow']['strength'] for r in sequence[1:]] == levels
            sequences.append(sequence)
            source_paths.append([directory / r['image'] for r in sequence])
        # Independent frozen runs must share exactly the same unbloomed pixels.
        controls = [np.asarray(Image.open(paths[0]).convert('RGB')) for paths in source_paths]
        assert np.array_equal(*controls), (name, 'OFF controls differ')
        rows = [sequences[0][0]] + sequences[0][1:] + sequences[1][1:]
        for row in rows:
            builds.add(row['exe_sha256'])
            effect = row['weapon_effect']
            assert effect['fid'] == spec['fid'] and effect['frame'] == spec['frame']
            assert row['light'] == 'enabled' and row['effect_emission']
            assert row['still_snapshot'] and row['resolution'] == '1920x1080' and row['scale'] == 2
            assert not row['sun']['up']
            for key in ['map', 'elevation', 'view_tile', 'hero', 'camera', 'weapon_effect']:
                assert row[key] == rows[0][key], (name, key)
            if not spec.get('existing'):
                assert effect['no_save'] and effect['light_check']
        paths = [source_paths[0][0]] + source_paths[0][1:] + source_paths[1][1:]
        images = [Image.open(p).convert('RGB') for p in paths]
        crop = sign_crop(images[0], images[-1], rows[0]) if spec.get('existing') else (600, 200, 1320, 880)
        arrays = [np.asarray(im.crop(crop)).astype(int) for im in images]
        energy = [float(np.maximum(a - arrays[0], 0).mean()) for a in arrays[1:]]
        assert energy[0] > 0 and all(a < b for a, b in zip(energy, energy[1:])), (name, energy)
        card = Image.new('RGB', (2160, 320), (18, 18, 18))
        draw = ImageDraw.Draw(card)
        draw.text((8, 5), name + ' | ' + spec['art'] + ' | night', fill='white')
        for index, image in enumerate(images[1:]):
            preview = ImageOps.contain(image.crop(crop), (360, 275), Image.Resampling.NEAREST)
            card.paste(preview, (index * 360, 42))
            draw.text((index * 360 + 8, 24), 'Shared glow ' + str(LEVELS[index]), fill='white')
        cards[name] = card
        report.append(dict(source=spec, ambient=16384, crop=crop, levels=LEVELS,
                           identical_off_controls=True,
                           added_rgb_mean=energy,
                           image_sha256=[hashlib.sha256(p.read_bytes()).hexdigest() for p in paths],
                           metadata=rows))
    assert len(builds) == 1, builds
    groups = {'night-projectiles': ['laser', 'plasma-bolt', 'plasma-pistol', 'plasma-rifle'],
              'night-fire-electric-rocket': ['electric', 'flamethrower', 'rocket'],
              'night-explosions': ['fire-explosion', 'plasma-explosion', 'emp-explosion', 'rocket-explosion'],
              'night-signs': ['sign40', 'sign41']}
    for filename, names in groups.items():
        sheet = Image.new('RGB', (2160, 320 * len(names)))
        for index, name in enumerate(names):
            sheet.paste(cards[name], (0, index * 320))
        sheet.save(OUT / (filename + '.jpg'), quality=88)
        assert (OUT / (filename + '.jpg')).stat().st_size < 1_000_000
    (OUT / 'glow-levels-measurements.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Validated 13 six-level comparisons, identical OFF controls, same build and increasing response.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sign-low', action='store_true')
    args = parser.parse_args()
    sign_low() if args.sign_low else main()
