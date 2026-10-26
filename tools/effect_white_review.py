"""086: compare the chosen source looks before/after neutral bloom suppression."""
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageOps

ROOT = Path('build/effects086')
OUT = Path('runs/086-weapon-glow-previews')
PICKS = {'laser': 0.7, 'plasma-bolt': 1.5, 'plasma-pistol': 0.7,
         'plasma-rifle': 0.7, 'electric': 0.7, 'flamethrower': 1.0,
         'rocket': 1.0, 'fire-explosion': 0.7, 'plasma-explosion': 0.7,
         'emp-explosion': 0.7, 'rocket-explosion': 0.7,
         'sign40': 0.4, 'sign41': 0.4}


def read(directory):
    return [(json.loads(p.read_text()), directory) for p in sorted(directory.glob('*.json'))]


def main():
    prior = json.loads((OUT / 'glow-levels-measurements.json').read_text())
    cards, report, builds = {}, [], set()
    for reference in prior:
        source = reference['source']
        name = source['name']
        chosen = PICKS[name]
        sign = source.get('existing', False)
        batch = 'sign-low' if sign else ('tuning' if chosen == 1.5 else 'tuning-low')
        old = read(ROOT / batch / name)
        new = read(ROOT / 'white-balanced' / name)
        assert len(old) == 4 and len(new) == 2, name
        old_off = next(pair for pair in old if not pair[0]['glow']['enabled'])
        before = next(pair for pair in old if pair[0]['glow']['enabled'] and pair[0]['glow']['strength'] == chosen)
        new_off, after = new
        assert not new_off[0]['glow']['enabled'] and after[0]['glow']['enabled']
        assert after[0]['glow']['strength'] == (chosen if sign else 2)
        builds.add(after[0]['exe_sha256'])
        for row, _ in new:
            for key in ['map', 'elevation', 'view_tile', 'hero', 'camera', 'weapon_effect', 'resolution', 'scale']:
                assert row[key] == before[0][key], (name, key)
        if not sign:
            assert after[0]['weapon_effect']['light_check']
        pairs = [old_off, before, new_off, after]
        paths = [directory / row['image'] for row, directory in pairs]
        images = [Image.open(p).convert('RGB') for p in paths]
        assert np.array_equal(np.asarray(images[0]), np.asarray(images[2])), (name, 'source or surface lighting changed')
        crop = reference['crop']
        boxes = [im.crop(crop) for im in images]
        arrays = [np.asarray(im).astype(int) for im in boxes]
        additions = [np.maximum(arrays[i] - arrays[0], 0) for i in [1, 3]]
        neutral = [float(a.min(axis=2).mean()) for a in additions]
        assert neutral[1] < neutral[0], (name, 'white bloom did not decrease', neutral)
        white_pixels = [int((a.min(axis=2) >= 245).sum()) for a in [arrays[1], arrays[3]]]
        card = Image.new('RGB', (720, 320), (18, 18, 18))
        draw = ImageDraw.Draw(card)
        draw.text((8, 5), name + ' | ' + source['art'] + ' | night', fill='white')
        for column, index in enumerate([1, 3]):
            preview = ImageOps.contain(boxes[index], (360, 275), Image.Resampling.NEAREST)
            card.paste(preview, (column * 360, 42))
            label = ('Before | chosen ' + str(chosen)) if column == 0 else ('White reduced | common ' + str(chosen if sign else 2))
            draw.text((column * 360 + 8, 24), label, fill='white')
        cards[name] = card
        report.append(dict(source=source, previous_strength=chosen,
                           common_strength=chosen if sign else 2,
                           sign_strength_provisional=sign, neutral_share=0.15,
                           identical_source_and_surface_pixels=True, crop=crop,
                           mean_added_neutral=neutral, white_pixels_245=white_pixels,
                           image_sha256=[hashlib.sha256(p.read_bytes()).hexdigest() for p in paths],
                           old_exe=before[0]['exe_sha256'], new_exe=after[0]['exe_sha256']))
    assert len(builds) == 1, builds
    groups = {'white-projectiles': ['laser', 'plasma-bolt', 'plasma-pistol', 'plasma-rifle'],
              'white-fire-electric-rocket': ['electric', 'flamethrower', 'rocket'],
              'white-explosions': ['fire-explosion', 'plasma-explosion', 'emp-explosion', 'rocket-explosion'],
              'white-signs': ['sign40', 'sign41']}
    for filename, names in groups.items():
        sheet = Image.new('RGB', (720, 320 * len(names)))
        for index, name in enumerate(names):
            sheet.paste(cards[name], (0, index * 320))
        sheet.save(OUT / (filename + '.jpg'), quality=88)
        assert (OUT / (filename + '.jpg')).stat().st_size < 1_000_000
    (OUT / 'white-measurements.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Validated 13 before/after comparisons: unchanged source/surface pixels, reduced neutral bloom.')


if __name__ == '__main__':
    main()
