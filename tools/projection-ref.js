// Upstream reference for the C++ projection port (src/projection.cpp).
// Prints groundToPx / hexPx / rowPlane numbers for the same inputs the C++
// selftest uses. Run: node tools/projection-ref.js
// The C++ side must match to 1e-9.
const fs = require('fs');
const path = require('path');
const { Proj, card, hexPx, rowPlane } = require(process.argv[2] || '../viewer/geom.js');

const scene = JSON.parse(fs.readFileSync(process.argv[3] ||
  path.join(__dirname, '../../vendor/fallout2-first-person/viewer/data/kladwtwn/scene.json')));
const proj = new Proj(scene.proj);

const pts = [[0, 0, 0], [10, 5, 0], [37, 21, 0]];
for (const [u, v, z] of pts) {
  const [x, y] = proj.groundToPx(u, v, z);
  console.log(`groundToPx ${u},${v},${z} -> ${x.toFixed(9)} ${y.toFixed(9)}`);
}
for (const [x, y] of [[600, 400], [1234, 567]]) {
  console.log(`hexFromPx ${x},${y} -> ${(() => { const v3 = Math.floor(y / 12); const v4 = x - 16 * v3; const v5 = y - 12 * v3; const v6 = Math.floor(v4 / 64); let v10 = v6 + v3; let v8 = v4 - v6 * 64; let v11 = 2 * v6; if (v8 >= 32) { v8 -= 32; v11 += 1; } const MASK = []; for (let row = 0; row < 4; row++) for (let i = 0; i < 16; i++) { if (64 - 4 * i > 16 * row) MASK[32 * row + i] = 1; if (4 * i > 16 * row) MASK[32 * row + 16 + i] = 2; } const corner = MASK[32 * v5 + v8] || 0; if (corner === 2) { v11 += 1; if (v11 & 1) v10 -= 1; } else if (corner === 1) v10 -= 1; const hx = 200 - 1 - v11; return hx >= 0 && hx < 200 && v10 >= 0 && v10 < 200 ? v10 * 200 + hx : -1; })()}`);
}
console.log(`hexPx 25075 -> ${hexPx(25075).join(' ')}`);
console.log(`rowPlane 10,5 -> ${rowPlane(proj, 10, 5, 0).toFixed(9)}`);
if (scene.sprites && scene.sprites.length) {
  const s = scene.sprites[0];
  const c = card(proj, s);
  console.log(`card sprite0 [${s.join(',')}] -> ${['shift','lean','z0','z1','up0','up1'].map(k => c[k].toFixed(9)).join(' ')}`);
}
console.log(`roofZ ${proj.roofZ.toFixed(9)} groundPerPx ${proj.groundPerPx.toFixed(9)}`);
