"""086: compare the requested common glow levels without changing source gains."""
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageOps

from effect_review_sheets import sign_crop

ROOT = Path('build/effects086')
OUT = Path('runs/086-weapon-glow-previews')
LEVELS = [0.7, 0.8, 1.0, 1.3, 1.5, 1.8]


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
    main()
