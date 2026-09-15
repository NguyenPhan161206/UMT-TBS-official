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


def compute_streaks(counts, days, start, today):
    current = 0
    probe = today
    if counts.get(probe.isoformat(), 0) == 0:
        probe -= dt.timedelta(days=1)
    while counts.get(probe.isoformat(), 0) > 0:
        current += 1
        probe -= dt.timedelta(days=1)
    longest = 0
    run = 0
    for i in range(days):
        date = start + dt.timedelta(days=i)
        if counts.get(date.isoformat(), 0) > 0:
            run += 1
            if run > longest:
                longest = run
        else:
            run = 0
    return current, longest


def compute_best_day(counts, days, start):
    best_date = None
    best_count = 0
    for i in range(days):
        date = start + dt.timedelta(days=i)
        cnt = counts.get(date.isoformat(), 0)
        if cnt > best_count:
            best_count = cnt
            best_date = date
    return best_date, best_count


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


def draw_grid(counts, days, target, today, total, streak_cur, streak_long,
              best_day, best_count, pace_avg, pace_pct, need_more, repo, out):
    stride = CELL + GAP
    cols = (days + GRID_ROWS - 1) // GRID_ROWS
    grid_w = cols * stride - GAP
    grid_h = GRID_ROWS * stride - GAP
    card_x = PAD_X + grid_w + 20
    card_h = 52
    card_gap = 10
    n_cards = 4

    now = dt.datetime.now(VN_TZ)
    start = (now - dt.timedelta(days=days - 1)).date()
    today_date = now.date()

    font_title = load_font(18, bold=True)
    font_sub = load_font(12)
    font_small = load_font(10)

    title_text = f"Commit Dashboard — {repo}"
    date_text = f"{start.strftime('%d %b')} – {today_date.strftime('%d %b %Y')}"
    width = max(
        320,
        int(PAD_X + text_width(font_title, title_text) + 16),
        int(PAD_X + text_width(font_sub, date_text) + 16),
        int(card_x + 360),
    )
    card_w = width - card_x - 16

    cards_bottom = PAD_TOP + n_cards * (card_h + card_gap) - card_gap
    bar_h = 18
    bar_y = cards_bottom + 12
    height = int(bar_y + bar_h + 16)

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

    def draw_card(y, label, value, sub):
        draw.rounded_rectangle([card_x, y, card_x + card_w, y + card_h], radius=8,
                               fill=(242, 243, 245), outline=(212, 215, 220))
        draw.text((card_x + 12, y + 8), label, fill=(110, 118, 129), font=font_small)
        draw.text((card_x + 12, y + 24), value, fill=(36, 41, 46), font=font_sub)
        if sub:
            draw.text((card_x + card_w - 12 - text_width(font_small, sub), y + 27), sub,
                      fill=(88, 96, 105), font=font_small)

    pct_today = int(round(min(1.0, today / target) * 100)) if target > 0 else 0
    cy = grid_top
    draw_card(cy, "TODAY", f"{today} / {target}", f"{pct_today}%")
    cy += card_h + card_gap
    draw_card(cy, "STREAK", f"{streak_cur} ngày", f"Dài nhất: {streak_long}")
    cy += card_h + card_gap
    draw_card(cy, "BEST DAY", best_day.strftime("%d %b %Y") if best_day else "—", f"{best_count} commits")
    cy += card_h + card_gap
    draw_card(cy, "PACE", f"{pace_avg:.1f} / ngày", f"{pace_pct}% target · cần +{need_more:.1f}/ngày")
    pace_bar_w = card_w - 24
    bar_frac_pace = min(1.0, pace_pct / 100.0)
    draw.rounded_rectangle([card_x + 12, cy + 42, card_x + 12 + pace_bar_w, cy + 47],
                           radius=3, fill=(228, 230, 235))
    draw.rounded_rectangle([card_x + 12, cy + 42, card_x + 12 + int(pace_bar_w * bar_frac_pace), cy + 47],
                           radius=3, fill=(46, 160, 67))

    bar_x = PAD_X
    bar_w = width - PAD_X - 16
    fill_frac = min(1.0, today / target) if target > 0 else 0.0
    footer_y = bar_y - 16
    draw.text((bar_x, footer_y), f"Today {today}/{target} ({pct_today}%) · Total {total} · 6 tuần",
              fill=(36, 41, 46), font=font_small)
    more_w = text_width(font_small, "More")
    less_w = text_width(font_small, "Less")
    scale_w = less_w + 4 + len(PALETTE) * 13 + 4 + more_w
    scale_x = bar_x + bar_w - scale_w
    draw.text((scale_x, footer_y), "Less", fill=(88, 96, 105), font=font_small)
    for i, color in enumerate(PALETTE):
        sx = scale_x + less_w + 4 + i * 13
        draw.rectangle([sx, footer_y + 2, sx + 11, footer_y + 13], fill=color)
    draw.text((scale_x + less_w + 4 + len(PALETTE) * 13 + 4, footer_y), "More", fill=(88, 96, 105), font=font_small)
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

    target = int(args.target)
    start_day = start.date()
    streak_cur, streak_long = compute_streaks(counts, args.days, start_day, now.date())
    best_day, best_count = compute_best_day(counts, args.days, start_day)
    pace_avg = total / args.days
    pace_pct = min(100, int(round(pace_avg / target * 100))) if target > 0 else 0
    need_more = max(0.0, target - pace_avg)

    draw_grid(counts, args.days, target, today, total, streak_cur, streak_long,
              best_day, best_count, pace_avg, pace_pct, need_more, args.repo, args.out)

    print(f"TODAY={today}")
    print(f"WEEK_AVG={week_avg:.1f}")
    print(f"TOTAL={total}")
    print(f"STREAK={streak_cur}")
    print(f"BEST_DAY={best_day.isoformat() if best_day else ''}")
    print(f"BEST_COUNT={best_count}")
    print(f"PACE_AVG={pace_avg:.1f}")
    print(f"PACE_PCT={pace_pct}")
    print(f"NEED_MORE={need_more:.1f}")


if __name__ == "__main__":
    main()