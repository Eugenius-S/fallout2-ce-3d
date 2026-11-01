"""Embed the owner-supplied settings frame PNGs without changing the originals."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = root / 'assets/options-frame-owner-133'
names = ('topLeft', 'top', 'topRight', 'left', 'right', 'bottomLeft', 'bottom', 'bottomRight')
lines = ['// Generated from the owner supplied PNG files in assets/options-frame-owner-133.',
         '// Regenerate with tools/generate_options_frame.py.',
         '#ifndef FALLOUT_SCENE_OPTIONS_FRAME_OWNER_H',
         '#define FALLOUT_SCENE_OPTIONS_FRAME_OWNER_H',
         'namespace fallout { namespace scene { namespace owner_frame {']
for name in names:
    data = (source / (name + '.png')).read_bytes()
    lines.append('static constexpr unsigned char ' + name + '[] = {')
    lines.extend('    ' + ','.join(str(x) for x in data[i:i + 24]) + ',' for i in range(0, len(data), 24))
    lines.append('};')
lines.extend(['} } }', '#endif'])
(root / 'ce/src/scene/options_frame_owner.h').write_text('\n'.join(lines) + '\n', encoding='utf-8')
