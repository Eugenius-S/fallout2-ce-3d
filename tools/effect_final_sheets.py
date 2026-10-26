"""086: final merged-main OFF/default-2 native effect evidence."""
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageOps

ROOT = Path('build/effects086/final-main')
OUT = Path('runs/086-weapon-glow-previews')


def main():
    references = json.loads((OUT / 'glow-levels-measurements.json').read_text())
    cards, report, builds = {}, [], set()
    for reference in references:
        source = reference['source']
        name = source['name']
        directory = ROOT / name
        files = sorted(directory.glob('*.json'))
        assert len(files) == 2, (name, len(files))
        rows = [json.loads(p.read_text()) for p in files]
        assert [r['glow']['enabled'] for r in rows] == [False, True]
        assert rows[1]['glow']['strength'] == 2
        for row in rows:
            builds.add(row['exe_sha256'])
            assert row['light'] == 'enabled' and row['effect_emission']
            assert row['weapon_effect']['fid'] == source['fid'] and row['weapon_effect']['frame'] == source['frame']
            assert row['resolution'] == '1920x1080' and row['scale'] == 2 and not row['sun']['up']
            for key in ['map', 'elevation', 'view_tile', 'hero', 'camera', 'weapon_effect']:
                assert row[key] == rows[0][key], (name, key)
            if not source.get('existing'):
                assert row['weapon_effect']['light_check'] and row['weapon_effect']['no_save']
        paths = [directory / r['image'] for r in rows]
        crop = reference['crop']
        images = [Image.open(p).convert('RGB').crop(crop) for p in paths]
        arrays = [np.asarray(im).astype(int) for im in images]
        changed = int((np.max(abs(arrays[1] - arrays[0]), axis=2) > 8).sum())
        assert changed > 0, name
        card = Image.new('RGB', (720, 320), (18, 18, 18))
        draw = ImageDraw.Draw(card)
        draw.text((8, 5), name + ' | ' + source['art'] + ' | night', fill='white')
        for column, im in enumerate(images):
            preview = ImageOps.contain(im, (360, 275), Image.Resampling.NEAREST)
            card.paste(preview, (column * 360, 42))
            draw.text((column * 360 + 8, 24), ['Glow OFF | local light ON', 'Common glow 2 | accepted calibration'][column], fill='white')
        cards[name] = card
        report.append(dict(source=source, crop=crop, changed_pixels_over_8=changed,
                           image_sha256=[hashlib.sha256(p.read_bytes()).hexdigest() for p in paths], metadata=rows))
    assert len(builds) == 1, builds
    groups = {'final-projectiles': ['laser', 'plasma-bolt', 'plasma-pistol', 'plasma-rifle'],
              'final-fire-electric-rocket': ['electric', 'flamethrower', 'rocket'],
              'final-explosions': ['fire-explosion', 'plasma-explosion', 'emp-explosion', 'rocket-explosion'],
              'final-signs': ['sign40', 'sign41']}
    for filename, names in groups.items():
        sheet = Image.new('RGB', (720, 320 * len(names)))
        for index, name in enumerate(names):
            sheet.paste(cards[name], (0, index * 320))
        sheet.save(OUT / (filename + '.jpg'), quality=88)
        assert (OUT / (filename + '.jpg')).stat().st_size < 1_000_000
    (OUT / 'final-measurements.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Validated 13 final OFF/default-2 pairs from the integrated build.')


if __name__ == '__main__':
    main()
