# Ctrl+T across midnight

`night-to-morning.jpg` is one isolated test run at KLADWTWN tile 25075.
The fixture starts at 22:00 and calls the same Ctrl+T handler eleven times;
the scene is captured at 22:00, 02:00, 06:00 and 09:00. It does not change
the owner's game or saves.

Before the fix the same handler wrapped 23:00 to the start of the same day
and the map ambient stayed at 28426/65536 through 09:00; the outdoor daylight
flag stayed true. The sun's direction advanced, but its strength at 09:00 was
only 0.245 because it still received the night ambient. With the fix, the day
advances across midnight, the native map update refreshes ambient, and the
four captures have ambient 28426, 28426, 65536 and 65536. At 09:00 the sun
is up with strength 1.0. The screenshots are frozen test frames, not a video.
