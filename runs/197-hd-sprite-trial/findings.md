# Findings

- The tool returned the requested 14 occupied frames in the same 4x4 arrangement: six standing directions and eight walk frames. It preserved the blue suit and yellow Vault 13 emblem.
- It did not keep the original pixel-art look. The x4 result reads as smooth, semi-realistic illustration; x2 is more pixel-like after resampling but still has smooth shading and edges. Treat both as rejected style experiments, not usable game art.
- Shape checks at x1 show the six standing frames' bottom bounds align with the source. Across the walk frames, generated width ranges from 1.038x to 1.227x source width, height from 0.916x to 1.063x, and the foot/bottom bound shifts upward by 0 to 7 px. This suggests possible scale and anchor jitter; it is not a playback test. Full per-frame measurements and source offsets are in `metrics.json` and `frame-layout.json`.
- Each image generation took about 50 s (x4) and 48 s (x2), about 3.6 s and 3.4 s per occupied frame if divided evenly. The tool generates a sheet in one call, not separate frame calls. It did not report its backend model variant or any billed amount, so Luna/low-effort selection and monetary cost cannot be verified.
- The generated images are only under `runs/197-hd-sprite-trial/`; no game data was changed.
