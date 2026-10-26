"""086: validate frozen glow ladders and build complete review sheets."""

import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageOps

ROOT = Path('build/effects086')
OUT = Path('runs/086-weapon-glow-previews')


def sign_crop(before, strongest, row):
    # Isolated mapped signs: locate the principal halo, including its full
    # connected extent, then leave room around it. Identical crop per row.
    delta = np.max(np.abs(np.asarray(strongest).astype(int) - np.asarray(before).astype(int)), axis=2)
    # Ignore unrelated bright floor pixels outside the selected native sign.
    zoom = row['egg']['zoom']
    rect = row['weapon_effect']['screen_rect']
    bounds = [(rect[i] * 2 - (960 if i % 2 == 0 else 540)) * zoom
              + (960 if i % 2 == 0 else 540) for i in range(4)]
    left, top = max(0, int(bounds[0]) - 128), max(0, int(bounds[1]) - 128)
    right, bottom = min(1920, int(bounds[2]) + 128), min(1080, int(bounds[3]) + 128)
    roi = np.zeros_like(delta, dtype=bool)
    roi[top:bottom, left:right] = True
    delta = np.where(roi, delta, 0)
    mask = (delta > 8).reshape(270, 4, 480, 4).any(axis=(1, 3))
    remaining = set(zip(*np.where(mask)))
    largest = []
    while remaining:
        seed = remaining.pop()
        component, pending = [seed], [seed]
        while pending:
            y, x = pending.pop()
            for dy, dx in [(0, 1), (0, -1), (1, 0), (-1, 0)]:
                point = (y + dy, x + dx)
                if point in remaining:
                    remaining.remove(point)
                    component.append(point)
                    pending.append(point)
        if len(component) > len(largest):
            largest = component
    if not largest:
        raise ValueError('no measurable sign halo; inspect the capture')
    yy, xx = zip(*largest)
    return tuple(int(value) for value in (
        max(0, min(xx) * 4 - 64), max(0, min(yy) * 4 - 64),
        min(1920, (max(xx) + 1) * 4 + 64), min(1080, (max(yy) + 1) * 4 + 64)))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    specs = json.loads((ROOT / 'all-specs.json').read_text())
    specs += [dict(name='sign40', art='SIGN40.FRM', fid=33556182, frame=0, existing=True),
              dict(name='sign41', art='SIGN41.FRM', fid=33556183, frame=0, existing=True)]
    cards, report = {}, []
    for spec in specs:
        name = spec['name']
        relative = ('sign-native-small/sign41' if name == 'sign41' else
                    'sign-native/sign40' if name == 'sign40' else 'ladders/' + name)
        directory = ROOT / relative
        files = sorted(directory.glob('*.json'))
        assert len(files) == 6, (name, len(files))
        rows = [json.loads(f.read_text()) for f in files]
        assert [r['glow']['enabled'] for r in rows] == [False, True, True, True, True, True]
        assert [r['glow']['strength'] for r in rows[1:]] == [0.5, 1, 2, 4, 8]
        for row in rows:
            effect = row['weapon_effect']
            assert effect['name'] == name and effect['fid'] == spec['fid'] and effect['frame'] == spec['frame']
            assert bool(effect.get('existing_object')) == bool(spec.get('existing'))
            assert spec.get('existing') or effect['no_save']
            for key in ['map', 'elevation', 'view_tile', 'hero', 'resolution', 'weapon_effect']:
                assert row[key] == rows[0][key], (name, key)
        chosen = [rows[i] for i in [0, 3, 4, 5]]
        paths = [directory / r['image'] for r in chosen]
        images = [Image.open(p).convert('RGB') for p in paths]
        crop = sign_crop(images[0], images[-1], rows[0]) if spec.get('existing') else (600, 200, 1320, 880)
        card = Image.new('RGB', (1280, 344), (18, 18, 18))
        draw = ImageDraw.Draw(card)
        draw.text((8, 4), name + ' | ' + spec['art'], fill='white')
        counts = []
        for index, image in enumerate(images):
            box = image.crop(crop)
            diff = ImageChops.difference(images[0].crop(crop), box)
            delta = np.asarray(diff).max(axis=2)
            counts.append(int((delta > 8).sum()))
            preview = ImageOps.contain(box, (320, 300), Image.Resampling.NEAREST)
            card.paste(preview, (index * 320, 44))
            label = ['OFF', '2 | current', '4 | x2 test', '8 | x4 test'][index]
            draw.text((index * 320 + 8, 25), label, fill='white')
        cards[name] = card
        report.append(dict(source=spec, capture=relative, crop=crop,
                           changed_pixels_over_8=counts,
                           image_sha256=[hashlib.sha256(p.read_bytes()).hexdigest() for p in paths],
                           effect=rows[0]['weapon_effect'], exe_sha256=rows[0]['exe_sha256']))
    groups = {'review-projectiles': ['laser', 'plasma-bolt', 'plasma-pistol', 'plasma-rifle'],
              'review-fire-electric-rocket': ['electric', 'flamethrower', 'rocket'],
              'review-explosions': ['fire-explosion', 'plasma-explosion', 'emp-explosion', 'rocket-explosion'],
              'review-neon-signs': ['sign40', 'sign41']}
    for filename, names in groups.items():
        sheet = Image.new('RGB', (1280, 344 * len(names)))
        for index, name in enumerate(names):
            sheet.paste(cards[name], (0, index * 344))
        sheet.save(OUT / (filename + '.jpg'), quality=88)
    (OUT / 'review-measurements.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Validated 11 effects, 2 mapped neon signs; generated four review sheets.')


if __name__ == '__main__':
    main()
