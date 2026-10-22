# Minimum-width frame review

- At 640x480 logical, the decorative frame spans x=16..623; the scrollbar
  spans x=628..639, leaving a 4-pixel gap. The content and legacy control axes
  remain centered at x=320, and original control hit regions are unchanged.
- The official Top and Bottom captures are `minimum-top-final/scr00030.png`
  and `minimum-bottom-final/scr00031.png` at 1280x960 physical, scale 2.
- Isolated Save changed zoom 1.0 to 10.0, enabled 1 to 0, and combat
  difficulty 1 to 2. The config SHA changed from
  `ABD92A2EB71C32C2F02C21FCD5FE52BFCC489886D48D5056DAEA9C00FA5ADBFD` to
  `5CA04FBC615795A0217B6CFAA5DD8708BB2B9DA910EAF5AC188469209F4EA4C2`.
- Cancel restored the saved values and preserved that config SHA exactly.
  Both probes observed the C++ completion marker before the deadline and
  stopped only the isolated helper through the harness.
- Probe JSON and stdout are in `save-1280x960/` and `cancel-1280x960/`.
  The screenshot JSON sidecars are beside their PNGs.
