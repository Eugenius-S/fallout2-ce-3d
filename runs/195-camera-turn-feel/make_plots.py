import csv
import json
import math
import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
RATE = 144.0
DT = 1.0 / RATE
ACCEL = 640.0
MAX_SPEED = 110.0


def step(at, target, speed, dt):
    count = max(1, math.ceil(dt / 0.001))
    h = dt / count
    for _ in range(count):
        distance = target - at
        if abs(distance) < 1e-9:
            at = target
            return target, target, 0.0
        direction = 1.0 if distance > 0.0 else -1.0
        stop_speed = math.sqrt(2.0 * ACCEL * abs(distance))
        desired = direction * min(MAX_SPEED, max(0.0, stop_speed - ACCEL * h))
        max_delta = ACCEL * h
        next_speed = speed + max(-max_delta, min(max_delta, desired - speed))
        movement = (speed + next_speed) * 0.5 * h
        if movement * direction >= abs(distance):
            return target, target, 0.0
        at += movement
        speed = next_speed
    return at, target, speed


def turn_run(press_count, held=False):
    at = target = speed = 0.0
    samples = [(0.0, at, speed)]
    presses = 0
    for frame in range(2000):
        if frame == 0 and not held:
            target += 10.0
        elif held and frame % 12 == 0 and presses < press_count:
            target += 10.0
            presses += 1
        at, target, speed = step(at, target, speed, DT)
        samples.append(((frame + 1) * DT, at, speed))
        if held and presses == press_count and at == target:
            break
        if not held and at == target:
            break
    return samples


def zoom_samples(ease):
    duration = 0.2
    start, end = 1.0, 1.25
    samples = [(0.0, start)]
    for frame in range(1, math.ceil(duration * RATE) + 1):
        t_seconds = min(frame * DT, duration)
        t = t_seconds / duration
        if ease == "old ease-out":
            value = start + (end - start) * (1.0 - (1.0 - t) ** 3)
        else:
            value = start + (end - start) * (3.0 * t * t - 2.0 * t * t * t)
        samples.append((t_seconds, value))
    return samples


def font(size):
    return ImageFont.truetype(str(Path(os.environ["WINDIR"]) / "Fonts" / "arial.ttf"), size)


def plot_panel(draw, box, title, series, x_max, y_min, y_max, unit):
    left, top, right, bottom = box
    draw.text((left, top), title, fill=(30, 30, 30), font=font(18))
    plot = (left + 74, top + 34, right - 16, bottom - 34)
    x0, y0, x1, y1 = plot
    for i in range(5):
        y = y0 + (y1 - y0) * i / 4
        value = y_max - (y_max - y_min) * i / 4
        draw.line((x0, y, x1, y), fill=(225, 225, 225), width=1)
        draw.text((left, y - 8), f"{value:.2f}{unit}" if unit == "" else f"{value:.0f}{unit}", fill=(80, 80, 80), font=font(12))
    for i in range(5):
        x = x0 + (x1 - x0) * i / 4
        value = x_max * i / 4
        draw.line((x, y0, x, y1), fill=(238, 238, 238), width=1)
        draw.text((x - 15, y1 + 8), f"{value:.2f}s", fill=(80, 80, 80), font=font(12))
    draw.rectangle(plot, outline=(80, 80, 80), width=1)
    for label, points, color in series:
        xy = []
        for time, value in points:
            px = x0 + max(0.0, min(1.0, time / x_max)) * (x1 - x0)
            py = y1 - max(0.0, min(1.0, (value - y_min) / (y_max - y_min))) * (y1 - y0)
            xy.append((px, py))
        if len(xy) > 1:
            draw.line(xy, fill=color, width=3)
        for x, y in xy:
            draw.ellipse((x - 2, y - 2, x + 2, y + 2), fill=color)
    legend_y = bottom - 22
    legend_x = x0
    for label, _, color in series:
        draw.line((legend_x, legend_y + 8, legend_x + 20, legend_y + 8), fill=color, width=3)
        draw.text((legend_x + 25, legend_y), label, fill=(30, 30, 30), font=font(13))
        legend_x += 170 if len(label) < 18 else 210


def save_csv(filename, samples, columns):
    with (ROOT / filename).open("w", newline="", encoding="utf-8") as out:
        writer = csv.writer(out)
        writer.writerow(columns)
        writer.writerows(samples)


tap = turn_run(1)
held = turn_run(8, held=True)
old_zoom = zoom_samples("old ease-out")
new_zoom = zoom_samples("smooth")
old_rate = [(t, (old_zoom[i][1] - old_zoom[i - 1][1]) / (old_zoom[i][0] - old_zoom[i - 1][0])) for i, (t, _) in enumerate(old_zoom) if i]
new_rate = [(t, (new_zoom[i][1] - new_zoom[i - 1][1]) / (new_zoom[i][0] - new_zoom[i - 1][0])) for i, (t, _) in enumerate(new_zoom) if i]

save_csv("camera-turn-tap.csv", [(f"{t:.9f}", f"{a:.9f}", f"{v:.9f}") for t, a, v in tap], ["seconds", "angle_degrees", "speed_degrees_per_second"])
save_csv("camera-turn-held.csv", [(f"{t:.9f}", f"{a:.9f}", f"{v:.9f}") for t, a, v in held], ["seconds", "angle_degrees", "speed_degrees_per_second"])
zoom_rows = [(0.0, 1.0, 0.0)] + [
    (new_zoom[i][0], new_zoom[i][1], (new_zoom[i][1] - new_zoom[i - 1][1]) / (new_zoom[i][0] - new_zoom[i - 1][0]))
    for i in range(1, len(new_zoom))
]
save_csv("wheel-zoom.csv", [(f"{t:.9f}", f"{a:.9f}", f"{v:.9f}") for t, a, v in zoom_rows], ["seconds", "zoom", "frame_average_zoom_rate"])

metrics = {
    "source": "camera_turn.h simulation at 144 Hz, 1 ms integration slices",
    "turn_acceleration_degrees_per_second_squared": ACCEL,
    "held_repeat_frames": 12,
    "held_repeat_interval_seconds": 12 * DT,
    "tap_duration_seconds": tap[-1][0],
    "tap_peak_speed_degrees_per_second": max(abs(v) for _, _, v in tap),
    "tap_largest_frame_speed_delta": max(abs(tap[i][2] - tap[i - 1][2]) for i in range(1, len(tap))),
    "held_duration_seconds": held[-1][0],
    "held_peak_speed_degrees_per_second": max(abs(v) for _, _, v in held),
    "held_largest_frame_speed_delta": max(abs(held[i][2] - held[i - 1][2]) for i in range(1, len(held))),
    "wheel_zoom_from_to": [1.0, 1.25],
    "wheel_zoom_duration_seconds": 0.2,
    "old_ease_out_first_frame_average_rate": old_rate[0][1],
    "new_hermite_first_frame_average_rate": new_rate[0][1],
}
(ROOT / "metrics.json").write_text(json.dumps(metrics, indent=2) + "\n", encoding="utf-8")

image = Image.new("RGB", (1440, 1110), "white")
draw = ImageDraw.Draw(image)
draw.text((40, 20), "Camera motion, sampled every 6.94 ms (144 Hz)", fill=(20, 20, 20), font=font(25))
plot_panel(draw, (40, 70, 1400, 320), "Turn angle", [("tap 10°", [(t, a) for t, a, _ in tap], (29, 98, 170)), ("held 8 steps", [(t, a) for t, a, _ in held], (193, 70, 56))], 0.9, 0, 90, "°")
plot_panel(draw, (40, 330, 1400, 570), "Turn angular speed", [("tap", [(t, v) for t, _, v in tap], (29, 98, 170)), ("held", [(t, v) for t, _, v in held], (193, 70, 56))], 0.9, 0, 160, "°/s")
plot_panel(draw, (40, 580, 1400, 820), "One wheel zoom step: 1.00 to 1.25", [("smooth Hermite", new_zoom, (29, 98, 170)), ("old ease-out", old_zoom, (193, 70, 56))], 0.25, 1.0, 1.27, "")
plot_panel(draw, (40, 830, 1400, 1070), "Zoom rate: old ease-out starts with a speed jump", [("new Hermite", new_rate, (29, 98, 170)), ("old ease-out", old_rate, (193, 70, 56))], 0.25, 0, 4.2, "/s")
image.save(ROOT / "camera-motion-plots.jpg", quality=88, optimize=True)
