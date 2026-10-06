#!/usr/bin/env python3
"""soak_logger.py — Ghi và tổng kết soak test 24 h (DMXT-58).

Firmware thường (không cần env riêng) in mỗi 60 s (TBS_SOAK_HEARTBEAT_INTERVAL_MS, firmware/shared/soak_diag.h):
  sensor-node: "BOOT node boot=N rr=R"
               "SOAK node up= boot= rr= heap= minheap= blk= tx_ok= tx_fail= mqtt_rc= hwm_sensor= hwm_net= hwm_buzz="
  màn hình:    "BOOT scr boot=N rr=R"
               "SOAK scr up= boot= rr= iheap= iminheap= iblk= psram= psrammin= rx= linkdn= maxgap_ms= mqtt_rc= hwm_lvgl="

Hai lệnh:
  record   đọc song song 2 cổng serial (không kéo DTR/RTS, tự mở lại khi board reset/USB mất),
           chỉ lưu dòng SOAK/BOOT/lỗi + sự kiện cổng vào CSV (flush từng dòng), in tổng kết khi dừng.
  analyze  tổng kết lại từ CSV.

Ví dụ:
  python tools/soak/soak_logger.py record --duration 1800 --tag try30m
  python tools/soak/soak_logger.py record --duration 0 --tag soak24h        # tới khi Ctrl+C
  python tools/soak/soak_logger.py analyze data/soak/soak24h_20261006_210000.csv
"""
from __future__ import annotations

import argparse
import csv
import datetime
import json
import re
import sys
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Iterable

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from serial_ports import resolve_port  # noqa: E402
DEFAULT_OUT_DIR = ROOT / "data" / "soak"
DEFAULT_BAUD = 115200
HEARTBEAT_S = 60          # khớp TBS_SOAK_HEARTBEAT_INTERVAL_MS
WARMUP_UP_S = 600         # bỏ 10 phút đầu sau mỗi boot khi tính độ dốc heap
LINK_GAP_LIMIT_MS = 1500  # khớp ESPNOW_LINK_TIMEOUT_MS
EVENTS_PER_MIN = 30       # chặn dòng lỗi lặp làm phình CSV

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

_HB_RE = re.compile(r"\b(SOAK|BOOT)\s+(node|scr)\b(.*)")
_KV_RE = re.compile(r"(\w+)=(-?\w+)")
_EVENT_RE = re.compile(
    r"Guru Meditation|abort\(\)|Backtrace|rst:0x|[Bb]rownout|[Pp]anic|assert failed|"
    r"[Tt]ask watchdog|task_wdt|[Ss]tack overflow|[Ss]tack canary|"
    r"ESP-NOW link (UP|DOWN)|MQTT Disconnected|MQTT connect failed|Wi-Fi disconnected|"
    r"(?:^|\x1b\[[0-9;]*m)E \(\d+\)"
)

# Lý do reset không do người dùng chủ động (tiêu chí "0 reset ngoài ý muốn").
UNEXPECTED_RESETS = {"PANIC", "INT_WDT", "TASK_WDT", "WDT", "BROWNOUT", "SW", "CPU_LOCKUP",
                     "PWR_GLITCH", "UNKNOWN"}

HEAP_KEYS = {"node": ("heap", "minheap", "blk"), "scr": ("iheap", "iminheap", "iblk")}
BOARD_OF_SOURCE = {"sensor": "node", "screen": "scr"}

CSV_HEADER = ["pc_time", "t_s", "source", "kind", "line"]


# ------------------------------------------------------------------ parse

def classify(line: str) -> str | None:
    """'SOAK' | 'BOOT' | 'EVENT' | None (bỏ qua)."""
    m = _HB_RE.search(line)
    if m:
        return m.group(1)
    if _EVENT_RE.search(line):
        return "EVENT"
    return None


def parse_heartbeat(line: str) -> tuple[str, str, dict[str, int | str]] | None:
    """-> (kind SOAK|BOOT, board node|scr, fields). Số nguyên được đổi sang int, rr giữ chuỗi."""
    m = _HB_RE.search(line)
    if not m:
        return None
    fields: dict[str, int | str] = {}
    for k, v in _KV_RE.findall(m.group(3)):
        try:
            fields[k] = int(v)
        except ValueError:
            fields[k] = v
    return m.group(1), m.group(2), fields


# ------------------------------------------------------------------ record

class RateLimiter:
    """Tối đa `limit` dòng EVENT mỗi phút cho mỗi nguồn; trả về số dòng bị bỏ khi sang phút mới."""

    def __init__(self, limit: int = EVENTS_PER_MIN):
        self.limit = limit
        self.minute = None
        self.count = 0
        self.dropped = 0

    def allow(self, t_s: float) -> tuple[bool, int]:
        minute = int(t_s // 60)
        flushed = 0
        if minute != self.minute:
            flushed, self.dropped = self.dropped, 0
            self.minute, self.count = minute, 0
        if self.count < self.limit:
            self.count += 1
            return True, flushed
        self.dropped += 1
        return False, flushed


def open_serial(port: str, baud: int):
    import serial  # lazy: analyze/test không cần pyserial

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0.5
    # Không kéo DTR/RTS khi mở cổng: USB-CDC của ESP32-S3 có thể reset board.
    ser.dtr = False
    ser.rts = False
    ser.open()
    return ser


def read_port(open_fn: Callable[[], object], source: str, emit: Callable[[str, str, str], None],
              stop: threading.Event, retry_s: float = 1.0) -> None:
    """Đọc 1 cổng tới khi `stop`; mất cổng (board reset, USB-CDC biến mất) thì tự mở lại.

    emit(source, kind, line) với kind thuộc SOAK/BOOT/EVENT/PORT/RAW (RAW = mọi dòng, cho --raw).
    """
    lost_reported = False
    while not stop.is_set():
        try:
            ser = open_fn()
        except Exception as e:  # noqa: BLE001 — cổng chưa xuất hiện lại
            if not lost_reported:
                emit(source, "PORT", f"PORT_OPEN_FAIL {e}")
                lost_reported = True
            stop.wait(retry_s)
            continue
        emit(source, "PORT", "PORT_OPEN")
        lost_reported = False
        buf = b""
        try:
            while not stop.is_set():
                chunk = ser.read(getattr(ser, "in_waiting", 0) or 1)
                if not chunk:
                    continue
                buf += chunk
                while b"\n" in buf:
                    raw, buf = buf.split(b"\n", 1)
                    line = raw.decode("utf-8", errors="replace").strip()
                    if not line:
                        continue
                    emit(source, "RAW", line)
                    kind = classify(line)
                    if kind:
                        emit(source, kind, line)
        except Exception as e:  # noqa: BLE001 — SerialException/OSError khi cổng biến mất
            emit(source, "PORT", f"PORT_LOST {e}")
            lost_reported = True
        finally:
            try:
                ser.close()
            except Exception:  # noqa: BLE001
                pass
        stop.wait(retry_s)


@dataclass
class Row:
    t_s: float
    source: str
    kind: str
    line: str
    pc_time: str = ""


def record(sensor_port: str, screen_port: str, baud: int, duration: float, out: Path,
           raw_dir: Path | None = None) -> list[Row]:
    out.parent.mkdir(parents=True, exist_ok=True)
    lock = threading.Lock()
    stop = threading.Event()
    rows: list[Row] = []
    limiters = {"sensor": RateLimiter(), "screen": RateLimiter()}
    t0 = time.monotonic()

    f = out.open("w", encoding="utf-8", newline="")
    writer = csv.writer(f)
    writer.writerow(CSV_HEADER)
    raw_files = {}
    if raw_dir:
        raw_dir.mkdir(parents=True, exist_ok=True)
        for src in ("sensor", "screen"):
            raw_files[src] = (raw_dir / f"{out.stem}_{src}.log").open("w", encoding="utf-8")

    def write_row(t: float, source: str, kind: str, line: str) -> None:
        pc = datetime.datetime.now().isoformat(timespec="milliseconds")
        rows.append(Row(t, source, kind, line, pc))
        writer.writerow([pc, f"{t:.3f}", source, kind, line])
        f.flush()

    def emit(source: str, kind: str, line: str) -> None:
        t = time.monotonic() - t0
        with lock:
            if kind == "RAW":
                if source in raw_files:
                    raw_files[source].write(f"{t:.3f} {line}\n")
                return
            if kind == "EVENT":
                ok, flushed = limiters[source].allow(t)
                if flushed:
                    write_row(t, source, "EVENT", f"SUPPRESSED {flushed} event lines")
                if not ok:
                    return
            write_row(t, source, kind, line)
            if kind in ("SOAK", "BOOT", "PORT"):
                print(f"  [{t / 3600:6.2f} h] {source:6s} {line}")

    threads = [
        threading.Thread(target=read_port, daemon=True,
                         args=(lambda: open_serial(sensor_port, baud), "sensor", emit, stop)),
        threading.Thread(target=read_port, daemon=True,
                         args=(lambda: open_serial(screen_port, baud), "screen", emit, stop)),
    ]
    for th in threads:
        th.start()
    print(f"[SOAK] {sensor_port} (sensor-node) + {screen_port} (màn hình) -> {out}")
    print("[SOAK] Ctrl+C để dừng." if duration <= 0 else f"[SOAK] Thời lượng {duration:.0f} s.")
    try:
        while duration <= 0 or time.monotonic() - t0 < duration:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        stop.set()
        for th in threads:
            th.join(timeout=3)
        with lock:
            f.close()
            for rf in raw_files.values():
                rf.close()
    return rows


def load_csv(path: Path) -> list[Row]:
    with path.open(encoding="utf-8", newline="") as f:
        return [Row(float(r["t_s"]), r["source"], r["kind"], r["line"], r["pc_time"])
                for r in csv.DictReader(f)]


# ------------------------------------------------------------------ analyze

@dataclass
class Beat:
    t_s: float
    fields: dict[str, int | str] = field(default_factory=dict)


def sum_deltas(beats: list[Beat], key: str) -> int:
    """Tổng mức tăng của bộ đếm tích luỹ kể từ boot, chịu được reset (bộ đếm về 0)."""
    total = 0
    prev: Beat | None = None
    for b in beats:
        v = b.fields.get(key)
        if not isinstance(v, int) or v < 0:
            continue
        if prev is not None:
            pv = prev.fields[key]
            same_boot = b.fields.get("boot") == prev.fields.get("boot")
            total += (v - pv) if (same_boot and v >= pv) else v
        prev = b
    return total


def linear_slope(xs: list[float], ys: list[float]) -> float | None:
    n = len(xs)
    if n < 2:
        return None
    mx, my = sum(xs) / n, sum(ys) / n
    sxx = sum((x - mx) ** 2 for x in xs)
    if sxx == 0:
        return None
    return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sxx


def _resets(beats: list[Beat], boots: list[Beat]) -> dict:
    """Reset = bộ đếm boot tăng. Đếm cả lần boot không bắt được dòng BOOT (USB chưa mở lại kịp)."""
    obs = sorted(beats + boots, key=lambda b: b.t_s)
    seen: dict[int, Beat] = {}
    for b in obs:
        boot = b.fields.get("boot")
        if isinstance(boot, int) and boot not in seen:
            seen[boot] = b
    if not seen:
        return {"count": 0, "unexpected": 0, "events": []}
    first = min(seen)
    events = []
    for boot in sorted(seen)[1:]:
        rr = str(seen[boot].fields.get("rr", "UNKNOWN"))
        events.append({"t_h": round(seen[boot].t_s / 3600, 3), "boot": boot, "rr": rr})
    count = max(seen) - first
    missed = count - len(events)  # boot tăng >1 giữa 2 lần quan sát -> không rõ lý do
    unexpected = sum(1 for e in events if e["rr"] in UNEXPECTED_RESETS) + max(missed, 0)
    return {"count": count, "unexpected": unexpected, "missed_reason": max(missed, 0),
            "events": events}


def analyze(rows: list[Row]) -> dict:
    res: dict = {}
    beats: dict[str, list[Beat]] = {"node": [], "scr": []}
    boots: dict[str, list[Beat]] = {"node": [], "scr": []}
    for r in rows:
        if r.kind not in ("SOAK", "BOOT"):
            continue
        p = parse_heartbeat(r.line)
        if not p:
            continue
        kind, board, fields = p
        (beats if kind == "SOAK" else boots)[board].append(Beat(r.t_s, fields))

    for board, bl in beats.items():
        bl.sort(key=lambda b: b.t_s)
        free_k, min_k, blk_k = HEAP_KEYS[board]
        src = "sensor" if board == "node" else "screen"
        d: dict = {
            "beats": len(bl),
            "resets": _resets(bl, boots[board]),
            "port_lost": sum(1 for r in rows if r.source == src and r.kind == "PORT"
                             and r.line.startswith("PORT_LOST")),
            "events": sum(1 for r in rows if r.source == src and r.kind == "EVENT"),
        }
        if bl:
            d["duration_h"] = (bl[-1].t_s - bl[0].t_s) / 3600
            gaps = [b.t_s - a.t_s for a, b in zip(bl, bl[1:])]
            d["missing_beats"] = sum(1 for g in gaps if g > 1.5 * HEARTBEAT_S)
            free = [b.fields[free_k] for b in bl if isinstance(b.fields.get(free_k), int)]
            mins = [b.fields[min_k] for b in bl if isinstance(b.fields.get(min_k), int)]
            blks = [b.fields[blk_k] for b in bl if isinstance(b.fields.get(blk_k), int)]
            steady = [b for b in bl if isinstance(b.fields.get("up"), int)
                      and b.fields["up"] >= WARMUP_UP_S and isinstance(b.fields.get(free_k), int)]
            slope = linear_slope([b.t_s / 3600 for b in steady],
                                 [b.fields[free_k] / 1024 for b in steady])
            d["heap"] = {
                "free_first_kb": free[0] / 1024 if free else None,
                "free_last_kb": free[-1] / 1024 if free else None,
                "free_min_kb": min(free) / 1024 if free else None,
                "minheap_min_kb": min(mins) / 1024 if mins else None,
                "minheap_steady_first_kb": steady[0].fields[min_k] / 1024 if steady else None,
                "minheap_last_kb": mins[-1] / 1024 if mins else None,
                "blk_min_kb": min(blks) / 1024 if blks else None,
                "slope_kb_per_h": slope,
                "steady_points": len(steady),
            }
            if board == "scr":
                ps = [b.fields["psram"] for b in bl if isinstance(b.fields.get("psram"), int)]
                pm = [b.fields["psrammin"] for b in bl if isinstance(b.fields.get("psrammin"), int)]
                d["heap"]["psram_first_kb"] = ps[0] / 1024 if ps else None
                d["heap"]["psram_last_kb"] = ps[-1] / 1024 if ps else None
                d["heap"]["psram_min_kb"] = min(pm) / 1024 if pm else None
            d["hwm_min"] = {k: min(b.fields[k] for b in bl if isinstance(b.fields.get(k), int))
                            for k in sorted({k for b in bl for k in b.fields if k.startswith("hwm_")})}
            mqtt = [b for b in bl if isinstance(b.fields.get("mqtt_rc"), int) and b.fields["mqtt_rc"] >= 0]
            d["mqtt_reconnects"] = sum_deltas(mqtt, "mqtt_rc") if mqtt else None
        res[board] = d

    node, scr = beats["node"], beats["scr"]
    if node or scr:
        tx = sum_deltas(node, "tx_ok")
        rx = sum_deltas(scr, "rx")
        gaps = [b.fields["maxgap_ms"] for b in scr if isinstance(b.fields.get("maxgap_ms"), int)]
        res["espnow"] = {
            "tx_ok": tx,
            "tx_fail": sum_deltas(node, "tx_fail"),
            "rx": rx,
            "rx_over_tx_pct": 100.0 * rx / tx if tx else None,
            "link_down": sum_deltas(scr, "linkdn"),
            "maxgap_ms": max(gaps) if gaps else None,
            "windows_gap_over_limit": sum(1 for g in gaps if g > LINK_GAP_LIMIT_MS),
        }
    return res


def _f(v, fmt="{:.1f}") -> str:
    return "–" if v is None else fmt.format(v)


def format_report(res: dict) -> str:
    out: list[str] = []
    names = {"node": "sensor-node", "scr": "màn hình"}
    for board in ("node", "scr"):
        d = res.get(board)
        if not d or not d["beats"]:
            out.append(f"## {names[board]}: không có dòng SOAK nào")
            continue
        h, r = d["heap"], d["resets"]
        out.append(f"## {names[board]}")
        out.append(f"- Thời lượng: {d['duration_h']:.2f} h, {d['beats']} dòng SOAK, "
                   f"thiếu {d['missing_beats']} nhịp (>90 s), mất cổng {d['port_lost']} lần, "
                   f"{d['events']} dòng lỗi/sự kiện")
        reasons = ", ".join(f"{e['rr']}@{e['t_h']}h" for e in r["events"]) or "—"
        out.append(f"- Reset: {r['count']} (ngoài ý muốn {r['unexpected']}); lý do: {reasons}")
        out.append(f"- Heap trống (KB): đầu {_f(h['free_first_kb'])}, cuối {_f(h['free_last_kb'])}, "
                   f"thấp nhất {_f(h['free_min_kb'])}; min-ever thấp nhất {_f(h['minheap_min_kb'])} "
                   f"(sau 10 phút đầu {_f(h['minheap_steady_first_kb'])} → cuối {_f(h['minheap_last_kb'])}); "
                   f"block lớn nhất thấp nhất {_f(h['blk_min_kb'])}")
        out.append(f"- Độ dốc heap trống: {_f(h['slope_kb_per_h'], '{:+.3f}')} KB/h "
                   f"({h['steady_points']} điểm, bỏ 10 phút đầu sau boot)")
        if board == "scr":
            out.append(f"- PSRAM trống (KB): đầu {_f(h['psram_first_kb'])}, cuối {_f(h['psram_last_kb'])}, "
                       f"min-ever {_f(h['psram_min_kb'])}")
        out.append("- Stack HWM thấp nhất (byte): " +
                   (", ".join(f"{k}={v}" for k, v in d["hwm_min"].items()) or "—"))
        out.append(f"- MQTT reconnect: {_f(d['mqtt_reconnects'], '{}')}")
    e = res.get("espnow")
    if e:
        out.append("## ESP-NOW")
        out.append(f"- Gửi {e['tx_ok']} (lỗi {e['tx_fail']}), màn hình nhận {e['rx']} → "
                   f"{_f(e['rx_over_tx_pct'], '{:.2f}')} % (xấp xỉ: 2 board lệch cửa sổ ≤ 60 s)")
        out.append(f"- Mất link: {e['link_down']} lần; khoảng hở lớn nhất {_f(e['maxgap_ms'], '{}')} ms; "
                   f"{e['windows_gap_over_limit']} cửa sổ có khoảng hở > {LINK_GAP_LIMIT_MS} ms")
    out.append("## Tiêu chí đề xuất (docs/SOAK_TEST.md)")
    out.append(f"- 0 reset ngoài ý muốn: " + " / ".join(
        f"{names[b]} {'ĐẠT' if res.get(b, {}).get('resets', {}).get('unexpected', 1) == 0 else 'KHÔNG ĐẠT'}"
        for b in ("node", "scr") if res.get(b, {}).get("beats")))
    if e:
        out.append(f"- Không lần mất link nào > {LINK_GAP_LIMIT_MS} ms: "
                   f"{'ĐẠT' if e['windows_gap_over_limit'] == 0 else 'KHÔNG ĐẠT'}")
    out.append("- Min heap đi ngang: xem 'sau 10 phút đầu → cuối' và độ dốc ở trên (người đọc tự kết luận)")
    return "\n".join(out)


def main(argv: Iterable[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Soak test 24 h (DMXT-58)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("record", help="ghi 2 cổng serial")
    r.add_argument("--sensor-port", default="auto", help="mặc định dò theo VID:PID 303A:1001")
    r.add_argument("--screen-port", default="auto", help="mặc định dò theo VID:PID 1A86:55D3 (CH343)")
    r.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    r.add_argument("--duration", type=float, default=0, help="giây; 0 = tới khi Ctrl+C")
    r.add_argument("--tag", default="soak")
    r.add_argument("--out", type=Path, help="CSV (mặc định data/soak/<tag>_<giờ>.csv)")
    r.add_argument("--raw", action="store_true", help="ghi thêm toàn bộ log thô (file lớn)")
    a = sub.add_parser("analyze", help="tổng kết CSV đã ghi")
    a.add_argument("csv", type=Path)
    for p in (r, a):
        p.add_argument("--json", type=Path, help="ghi thêm kết quả dạng JSON")
    args = ap.parse_args(list(argv) if argv is not None else None)

    if args.cmd == "record":
        out = args.out or DEFAULT_OUT_DIR / f"{args.tag}_{datetime.datetime.now():%Y%m%d_%H%M%S}.csv"
        rows = record(resolve_port(args.sensor_port, "sensor"), resolve_port(args.screen_port, "screen"),
                      args.baud, args.duration, out,
                      out.parent if args.raw else None)
    else:
        rows = load_csv(args.csv)
    res = analyze(rows)
    print()
    print(format_report(res))
    if args.json:
        args.json.write_text(json.dumps(res, ensure_ascii=False, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
