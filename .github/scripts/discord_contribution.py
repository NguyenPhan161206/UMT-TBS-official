#!/usr/bin/env python3
import argparse
import datetime as dt
import json
import os
import sys
import urllib.error
import urllib.request

from PIL import Image, ImageDraw, ImageFont

API = "https://api.github.com"
CELL = 16
GAP = 4
PAD_X = 30
PAD_TOP = 76
PAD_BOTTOM = 64
GRID_ROWS = 7

PALETTE = [
    (235, 237, 240),
    (155, 233, 168),
    (64, 196, 99),
    (48, 161, 78),
    (33, 110, 57),
]

DOW = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]

VN_TZ = dt.timezone(dt.timedelta(hours=7))


def fetch_commits(repo, since, until, token):
    commits = []
    page = 1
    while True:
        url = f"{API}/repos/{repo}/commits?since={since}&until={until}&per_page=100&page={page}"
        req = urllib.request.Request(url)
        if token:
            req.add_header("Authorization", f"Bearer {token}")
        req.add_header("Accept", "application/vnd.github+json")
        req.add_header("X-GitHub-Api-Version", "2022-11-28")
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = json.load(resp)
        except urllib.error.HTTPError as err:
            sys.stderr.write(f"GitHub API error: {err.code} {err.reason}\n")
            sys.stderr.write(url + "\n")
            break
        if not data:
            break
        commits.extend(data)
        if len(data) < 100:
            break
        page += 1
    return commits


def bucket_commits(commits):
    counts = {}
    for c in commits:
        raw = c.get("commit", {}).get("author", {}).get("date")
        if not raw:
            continue
        ts = dt.datetime.fromisoformat(raw.replace("Z", "+00:00"))
        local = ts.astimezone(VN_TZ)
        key = local.date().isoformat()
        counts[key] = counts.get(key, 0) + 1
    return counts


def level(count):
    if count <= 0:
        return 0
    if count == 1:
        return 1
    if count <= 3:
        return 2
    if count <= 7:
        return 3
    return 4


def load_font(size, bold=False):
    paths = []
    if bold:
        paths = [
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        ]
    else:
        paths = [
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        ]
    for path in paths:
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            continue
    return ImageFont.load_default()


def text_width(font, text):
    try:
        return font.getlength(text)
    except AttributeError:
        probe = ImageDraw.Draw(Image.new("RGB", (10, 10)))
        return probe.textlength(text, font=font)


def draw_grid(counts, days, target, today, week_avg, total, repo, out):
    stride = CELL + GAP
    cols = (days + GRID_ROWS - 1) // GRID_ROWS
    grid_w = cols * stride - GAP
    grid_h = GRID_ROWS * stride - GAP
    legend_x = PAD_X + grid_w + 20

    now = dt.datetime.now(VN_TZ)
    start = (now - dt.timedelta(days=days - 1)).date()
    today_date = now.date()

    font_title = load_font(18, bold=True)
    font_sub = load_font(12)
    font_small = load_font(10)
    font_num = load_font(30, bold=True)

    title_text = f"Commit Dashboard — {repo}"
    date_text = f"{start.strftime('%d %b')} – {today_date.strftime('%d %b %Y')}"
    legend_w = max(
        text_width(font_small, "Avg 7 ngày: 99.9"),
        text_width(font_small, "Target 999/ngày"),
        text_width(font_small, "Total: 99999"),
        text_width(font_small, "commits today"),
        text_width(font_small, "Less") + 42 + len(PALETTE) * 16 + 4 + text_width(font_small, "More"),
    )
    width = max(320, int(PAD_X + text_width(font_title, title_text) + 16), int(PAD_X + text_width(font_sub, date_text) + 16), int(legend_x + legend_w + 12))
    height = int(PAD_TOP + grid_h + PAD_BOTTOM)

    img = Image.new("RGB", (width, height), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    draw.text((PAD_X, 14), title_text, fill=(36, 41, 46), font=font_title)
    draw.text((PAD_X, 42), date_text, fill=(88, 96, 105), font=font_sub)

    grid_top = PAD_TOP
    grid_left = PAD_X

    for r, dow in enumerate(DOW):
        draw.text((8, grid_top + r * stride + 3), dow, fill=(110, 118, 129), font=font_small)

    prev_month = None
    for i in range(days):
        date = start + dt.timedelta(days=i)
        col = i // GRID_ROWS
        row = i % GRID_ROWS
        if date.month != prev_month:
            prev_month = date.month
            draw.text(
                (grid_left + col * stride, grid_top - 16),
                MONTHS[date.month - 1],
                fill=(110, 118, 129),
                font=font_small,
            )
        x = grid_left + col * stride
        y = grid_top + row * stride
        color = PALETTE[level(counts.get(date.isoformat(), 0))]
        draw.rectangle([x, y, x + CELL, y + CELL], fill=color)
        if date == today_date:
            draw.rectangle([x - 2, y - 2, x + CELL + 2, y + CELL + 2], outline=(33, 110, 57), width=2)

    draw.text((legend_x, grid_top), f"{today}", fill=(36, 41, 46), font=font_num)
    draw.text((legend_x, grid_top + 42), "commits today", fill=(88, 96, 105), font=font_small)
    draw.text((legend_x, grid_top + 62), f"Target {target}/ngày", fill=(88, 96, 105), font=font_small)
    draw.text((legend_x, grid_top + 80), f"Avg 7 ngày: {week_avg:.1f}", fill=(88, 96, 105), font=font_small)
    draw.text((legend_x, grid_top + 98), f"Total: {total}", fill=(88, 96, 105), font=font_small)

    scale_y = grid_top + grid_h - 20
    draw.text((legend_x, scale_y), "Less", fill=(88, 96, 105), font=font_small)
    for i, color in enumerate(PALETTE):
        sx = legend_x + 42 + i * 16
        draw.rectangle([sx, scale_y, sx + 12, scale_y + 12], fill=color)
    draw.text((legend_x + 42 + len(PALETTE) * 16 + 4, scale_y), "More", fill=(88, 96, 105), font=font_small)

    bar_x = PAD_X
    bar_y = PAD_TOP + grid_h + 20
    bar_w = width - PAD_X - 16
    bar_h = 18
    fill_frac = min(1.0, today / target) if target > 0 else 0.0
    pct = int(round(fill_frac * 100))
    draw.text((bar_x, bar_y - 16), f"Today {today}/{target} ({pct}%)", fill=(36, 41, 46), font=font_small)
    draw.rounded_rectangle([bar_x, bar_y, bar_x + bar_w, bar_y + bar_h], radius=9, fill=(228, 230, 235))
    fill_w = int(bar_w * fill_frac)
    if fill_w > 0:
        draw.rounded_rectangle([bar_x, bar_y, bar_x + fill_w, bar_y + bar_h], radius=9, fill=(46, 160, 67))

    img.save(out)


def main():
    parser = argparse.ArgumentParser(description="Discord contribution grid")
    parser.add_argument("--repo", required=True, help="owner/repo")
    parser.add_argument("--target", type=int, default=5, help="daily commit target")
    parser.add_argument("--out", required=True, help="output PNG path")
    parser.add_argument("--days", type=int, default=42, help="window in days")
    args = parser.parse_args()

    token = os.environ.get("GITHUB_TOKEN", "")
    now = dt.datetime.now(VN_TZ)
    start = (now - dt.timedelta(days=args.days - 1)).replace(hour=0, minute=0, second=0, microsecond=0)
    until = (now + dt.timedelta(days=1)).replace(hour=0, minute=0, second=0, microsecond=0)

    commits = fetch_commits(args.repo, start.isoformat(), until.isoformat(), token)
    counts = bucket_commits(commits)

    today = counts.get(now.date().isoformat(), 0)
    recent_keys = [k for k in sorted(counts) if k >= start.date().isoformat()]
    total = sum(counts.get(k, 0) for k in recent_keys)
    week_vals = [counts.get((now.date() - dt.timedelta(days=i)).isoformat(), 0) for i in range(6, -1, -1)]
    week_avg = sum(week_vals) / 7.0

    draw_grid(counts, args.days, args.target, today, week_avg, total, args.repo, args.out)

    print(f"TODAY={today}")
    print(f"WEEK_AVG={week_avg:.1f}")
    print(f"TOTAL={total}")


if __name__ == "__main__":
    main()