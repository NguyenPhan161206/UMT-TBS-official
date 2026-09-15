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
CELL = 12
GAP = 3
PAD_X = 60
PAD_TOP = 76
PAD_BOTTOM = 54
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


def draw_grid(counts, days, target, today, week_avg, total, repo, out):
    cols = (days + GRID_ROWS - 1) // GRID_ROWS
    width = PAD_X + cols * (CELL + GAP) + 16
    height = PAD_TOP + GRID_ROWS * (CELL + GAP) + PAD_BOTTOM

    img = Image.new("RGB", (width, height), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    font_title = load_font(14, bold=True)
    font_sub = load_font(11)
    font_small = load_font(9)

    start = dt.datetime.now(VN_TZ).date() - dt.timedelta(days=days - 1)
    today_date = dt.datetime.now(VN_TZ).date()

    draw.text((PAD_X, 12), f"Commit Dashboard — {repo}", fill=(36, 41, 46), font=font_title)
    draw.text(
        (PAD_X, 34),
        f"{start.strftime('%d %b')} – {today_date.strftime('%d %b %Y')}  •  Today {today}/{target}  •  Avg {week_avg:.1f}/ngày  •  Total {total}",
        fill=(88, 96, 105),
        font=font_sub,
    )

    grid_top = PAD_TOP
    grid_left = PAD_X

    for r, dow in enumerate(DOW):
        draw.text((8, grid_top + r * (CELL + GAP) + 1), dow, fill=(110, 118, 129), font=font_small)

    prev_month = None
    for i in range(days):
        date = start + dt.timedelta(days=i)
        col = i // GRID_ROWS
        row = i % GRID_ROWS
        if date.month != prev_month:
            prev_month = date.month
            label = MONTHS[date.month - 1]
            draw.text((grid_left + col * (CELL + GAP), grid_top - 16), label, fill=(110, 118, 129), font=font_small)
        x = grid_left + col * (CELL + GAP)
        y = grid_top + row * (CELL + GAP)
        cnt = counts.get(date.isoformat(), 0)
        color = PALETTE[level(cnt)]
        draw.rectangle([x, y, x + CELL, y + CELL], fill=color)
        if date == today_date:
            draw.rectangle([x - 2, y - 2, x + CELL + 2, y + CELL + 2], outline=(33, 110, 57), width=2)

    bar_x = PAD_X
    bar_y = grid_top + GRID_ROWS * (CELL + GAP) + 10
    bar_w = width - PAD_X - 16
    bar_h = 16
    fill_frac = min(1.0, today / target) if target > 0 else 0.0
    draw.rounded_rectangle([bar_x, bar_y, bar_x + bar_w, bar_y + bar_h], radius=8, fill=(228, 230, 235))
    fill_w = int(bar_w * fill_frac)
    if fill_w > 0:
        draw.rounded_rectangle([bar_x, bar_y, bar_x + fill_w, bar_y + bar_h], radius=8, fill=(46, 160, 67))

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