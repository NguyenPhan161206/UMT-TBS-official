#!/usr/bin/env python3
"""measure_latency.py — Đo độ trễ ESP-NOW và MQTT (DMXT-57).

Cần firmware build env *_latency trên CẢ HAI board (xem docs/LATENCY_TEST.md):
  sensor-node  in:  "LAT TX seq=N age_min_ms=A age_max_ms=B"   (mỗi gói ESP-NOW)
                    "LAT RTT seq=N us=U"                        (echo từ màn hình về)
                    "LAT MQTT_TX seq=N"                         (trước mỗi publish)
  màn hình     in:  "I (..) LAT: RX seq=N"                      (callback ESP-NOW)
                    "I (..) LAT: MQTT_RX seq=N"                 (shared attribute từ rule-chain)

Hai chế độ:
  record   đọc song song 2 cổng serial, gắn mốc time.perf_counter_ns() của PC cho mỗi dòng LAT,
           ghi CSV rồi in thống kê.
  analyze  tính lại thống kê từ CSV đã ghi.

Phương pháp (ghi đúng như vậy khi báo cáo):
  - ESP-NOW một chiều ≈ RTT/2, RTT đo trên đồng hồ esp_timer của sensor-node (µs).
  - ESP-NOW theo mốc PC (RX màn hình − TX sensor-node) chỉ để đối chiếu: có lẫn jitter USB.
  - MQTT = mốc PC lúc màn hình in MQTT_RX − mốc PC lúc sensor-node in MQTT_TX (sai số cỡ vài ms).

Ví dụ:
  python tools/latency/measure_latency.py record --duration 300 --tag espnow_1m   # cổng tự dò
  python tools/latency/measure_latency.py analyze data/latency/espnow_1m_20261006_101500.csv
"""
from __future__ import annotations

import argparse
import csv
import datetime
import json
import math
import re
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from serial_ports import resolve_port  # noqa: E402
DEFAULT_OUT_DIR = ROOT / "data" / "latency"
DEFAULT_BAUD = 115200

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

# Chấp nhận cả "LAT TX ..." (Serial.printf) và "LAT: RX ..." (ESP_LOGI), có/không mã màu ANSI.
_LAT_RE = re.compile(r"LAT:?\s+(TX|RTT|RX|MQTT_TX|MQTT_RX)\s+seq=(\d+)(.*)")
_KV_RE = re.compile(r"(\w+)=(-?\d+)")

SEQ16 = 1 << 16


@dataclass
class Event:
    t_ns: int     # mốc PC (perf_counter_ns, tính từ lúc bắt đầu ghi)
    source: str   # "sensor" | "screen"
    kind: str     # TX | RTT | RX | MQTT_TX | MQTT_RX
    seq: int
    fields: dict[str, int]


def parse_line(line: str) -> tuple[str, int, dict[str, int]] | None:
    """Tách 1 dòng serial -> (kind, seq, fields). None nếu không phải dòng LAT."""
    m = _LAT_RE.search(line)
    if not m:
        return None
    kind, seq, rest = m.group(1), int(m.group(2)), m.group(3)
    fields = {k: int(v) for k, v in _KV_RE.findall(rest)}
    return kind, seq, fields


def unwrap_seq16(seqs: Iterable[int]) -> list[int]:
    """Mở vòng seq uint16 của ESP-NOW (tràn sau ~1,8 h ở 10 gói/s) thành dãy tăng dần."""
    out: list[int] = []
    offset = 0
    for s in seqs:
        if out and s + offset < out[-1] - SEQ16 // 2:
            offset += SEQ16
        out.append(s + offset)
    return out


def percentile(sorted_vals: list[float], p: float) -> float:
    """Percentile nội suy tuyến tính (giống numpy mặc định). sorted_vals phải đã sắp xếp."""
    if not sorted_vals:
        return math.nan
    if len(sorted_vals) == 1:
        return sorted_vals[0]
    k = (len(sorted_vals) - 1) * p / 100.0
    lo = math.floor(k)
    hi = math.ceil(k)
    if lo == hi:
        return sorted_vals[lo]
    return sorted_vals[lo] + (sorted_vals[hi] - sorted_vals[lo]) * (k - lo)


def describe(values: list[float]) -> dict[str, float]:
    v = sorted(values)
    n = len(v)
    if n == 0:
        return {"n": 0}
    mean = sum(v) / n
    std = math.sqrt(sum((x - mean) ** 2 for x in v) / (n - 1)) if n > 1 else 0.0
    return {
        "n": n,
        "min": v[0],
        "p50": percentile(v, 50),
        "p95": percentile(v, 95),
        "p99": percentile(v, 99),
        "max": v[-1],
        "mean": mean,
        "std": std,
    }


def _by_kind(events: list[Event], kind: str) -> list[Event]:
    return [e for e in events if e.kind == kind]


def _unwrap_events(evts: list[Event]) -> dict[int, Event]:
    """seq đã mở vòng -> sự kiện ĐẦU TIÊN có seq đó (bỏ bản lặp)."""
    evts = sorted(evts, key=lambda e: e.t_ns)
    out: dict[int, Event] = {}
    for e, s in zip(evts, unwrap_seq16(e.seq for e in evts)):
        out.setdefault(s, e)
    return out


def analyze(events: list[Event]) -> dict:
    res: dict = {}

    # --- ESP-NOW ---
    tx = _unwrap_events(_by_kind(events, "TX"))
    rx = _unwrap_events(_by_kind(events, "RX"))
    rtt = _unwrap_events(_by_kind(events, "RTT"))
    if tx:
        # Chỉ tính các seq trong khoảng đã TX, bỏ gói đầu/cuối bị cắt bởi lúc mở/đóng cổng.
        lo, hi = min(tx), max(tx)
        rx_in = {s for s in rx if lo <= s <= hi}
        rtt_in = {s for s in rtt if lo <= s <= hi}
        res["espnow"] = {
            "tx": len(tx),
            "rx_screen": len(rx_in),
            "delivery_pct": 100.0 * len(rx_in & set(tx)) / len(tx),
            "echo_back": len(rtt_in),
            "one_way_rtt_half_ms": describe([rtt[s].fields["us"] / 2000.0 for s in rtt_in]),
            "rtt_ms": describe([rtt[s].fields["us"] / 1000.0 for s in rtt_in]),
            "one_way_pc_ms": describe(
                [(rx[s].t_ns - tx[s].t_ns) / 1e6 for s in rx_in if s in tx]
            ),
        }
        ages_min = [e.fields["age_min_ms"] for e in tx.values() if e.fields.get("age_min_ms", -1) >= 0]
        ages_max = [e.fields["age_max_ms"] for e in tx.values() if e.fields.get("age_max_ms", -1) >= 0]
        res["sample_age"] = {"age_min_ms": describe(ages_min), "age_max_ms": describe(ages_max)}

    # --- MQTT (seq uint32, không cần mở vòng) ---
    mtx: dict[int, Event] = {}
    for e in sorted(_by_kind(events, "MQTT_TX"), key=lambda e: e.t_ns):
        mtx.setdefault(e.seq, e)
    mrx: dict[int, Event] = {}
    for e in sorted(_by_kind(events, "MQTT_RX"), key=lambda e: e.t_ns):
        # RX phải xảy ra SAU TX (bỏ bản attribute cũ màn hình nhận lại lúc reconnect).
        if e.seq in mtx and e.t_ns > mtx[e.seq].t_ns:
            mrx.setdefault(e.seq, e)
    if mtx:
        res["mqtt"] = {
            "tx": len(mtx),
            "rx_screen": len(mrx),
            "delivery_pct": 100.0 * len(mrx) / len(mtx),
            "one_way_pc_ms": describe([(mrx[s].t_ns - mtx[s].t_ns) / 1e6 for s in mrx]),
        }
    return res


def _fmt_stats(name: str, d: dict) -> str:
    if not d or d.get("n", 0) == 0:
        return f"| {name} | 0 | – | – | – | – | – | – | – |"
    return (
        f"| {name} | {d['n']} | {d['min']:.2f} | {d['p50']:.2f} | {d['p95']:.2f} | "
        f"{d['p99']:.2f} | {d['max']:.2f} | {d['mean']:.2f} | {d['std']:.2f} |"
    )


def format_report(res: dict) -> str:
    lines = ["| Đại lượng (ms) | n | min | p50 | p95 | p99 | max | mean | std |",
             "|---|---|---|---|---|---|---|---|---|"]
    if "espnow" in res:
        e = res["espnow"]
        lines.append(_fmt_stats("ESP-NOW một chiều (RTT/2)", e["one_way_rtt_half_ms"]))
        lines.append(_fmt_stats("ESP-NOW RTT", e["rtt_ms"]))
        lines.append(_fmt_stats("ESP-NOW một chiều (mốc PC, đối chiếu)", e["one_way_pc_ms"]))
        lines.append(_fmt_stats("Tuổi mẫu lúc gửi — mới nhất", res["sample_age"]["age_min_ms"]))
        lines.append(_fmt_stats("Tuổi mẫu lúc gửi — cũ nhất", res["sample_age"]["age_max_ms"]))
    if "mqtt" in res:
        lines.append(_fmt_stats("MQTT đầu–cuối (mốc PC)", res["mqtt"]["one_way_pc_ms"]))
    lines.append("")
    if "espnow" in res:
        e = res["espnow"]
        lines.append(f"ESP-NOW: TX {e['tx']}, màn hình nhận {e['rx_screen']} "
                     f"({e['delivery_pct']:.2f} %), echo về {e['echo_back']}")
    if "mqtt" in res:
        m = res["mqtt"]
        lines.append(f"MQTT: TX {m['tx']}, màn hình nhận {m['rx_screen']} ({m['delivery_pct']:.2f} %)")
    if not res:
        lines.append("Không có dòng LAT nào — kiểm tra đã flash env *_latency cho cả 2 board chưa.")
    return "\n".join(lines)


# ----------------------------------------------------------------- CSV I/O

CSV_HEADER = ["t_ns", "source", "kind", "seq", "fields", "raw"]


def load_csv(path: Path) -> list[Event]:
    events: list[Event] = []
    with path.open(encoding="utf-8", newline="") as f:
        for row in csv.DictReader(f):
            events.append(Event(int(row["t_ns"]), row["source"], row["kind"], int(row["seq"]),
                                json.loads(row["fields"] or "{}")))
    return events


# ----------------------------------------------------------------- record

def _open_serial(port: str, baud: int):
    import serial  # lazy: test/analyze không cần pyserial

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0.5
    # Không kéo DTR/RTS khi mở cổng: USB-CDC của ESP32-S3 có thể reset board.
    ser.dtr = False
    ser.rts = False
    ser.open()
    return ser


def record(sensor_port: str, screen_port: str, baud: int, duration: float, out: Path) -> list[Event]:
    out.parent.mkdir(parents=True, exist_ok=True)
    lock = threading.Lock()
    stop = threading.Event()
    events: list[Event] = []
    counts = {"TX": 0, "RTT": 0, "RX": 0, "MQTT_TX": 0, "MQTT_RX": 0}
    t0 = time.perf_counter_ns()

    f = out.open("w", encoding="utf-8", newline="")
    writer = csv.writer(f)
    writer.writerow(CSV_HEADER)

    def reader(port: str, source: str) -> None:
        ser = _open_serial(port, baud)
        buf = b""
        try:
            while not stop.is_set():
                chunk = ser.read(ser.in_waiting or 1)
                if not chunk:
                    continue
                t = time.perf_counter_ns() - t0  # mốc khi byte tới PC
                buf += chunk
                while b"\n" in buf:
                    raw, buf = buf.split(b"\n", 1)
                    line = raw.decode("utf-8", errors="replace").strip()
                    parsed = parse_line(line)
                    if not parsed:
                        continue
                    kind, seq, fields = parsed
                    ev = Event(t, source, kind, seq, fields)
                    with lock:
                        events.append(ev)
                        counts[kind] += 1
                        writer.writerow([t, source, kind, seq, json.dumps(fields), line])
        finally:
            ser.close()

    threads = [threading.Thread(target=reader, args=(sensor_port, "sensor"), daemon=True),
               threading.Thread(target=reader, args=(screen_port, "screen"), daemon=True)]
    for th in threads:
        th.start()
    print(f"[LAT] Ghi {sensor_port} (sensor-node) + {screen_port} (màn hình) -> {out}")
    print("[LAT] Ctrl+C để dừng sớm." if duration <= 0 else f"[LAT] Thời lượng {duration:.0f} s.")
    start = time.monotonic()
    try:
        while duration <= 0 or time.monotonic() - start < duration:
            time.sleep(5)
            if not all(th.is_alive() for th in threads):
                print("[LAT] Một luồng serial đã dừng (mất cổng?) — kết thúc.", file=sys.stderr)
                break
            with lock:
                print(f"  [{time.monotonic() - start:5.0f}s] " +
                      " ".join(f"{k}={v}" for k, v in counts.items()))
    except KeyboardInterrupt:
        pass
    finally:
        stop.set()
        for th in threads:
            th.join(timeout=2)
        with lock:
            f.close()
    return events


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Đo độ trễ ESP-NOW / MQTT (DMXT-57)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("record", help="ghi 2 cổng serial rồi phân tích")
    r.add_argument("--sensor-port", default="auto", help="mặc định dò theo VID:PID 303A:1001")
    r.add_argument("--screen-port", default="auto", help="mặc định dò theo VID:PID 1A86:55D3 (CH343)")
    r.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    r.add_argument("--duration", type=float, default=300, help="giây; 0 = tới khi Ctrl+C")
    r.add_argument("--tag", default="run", help="nhãn kịch bản, vd espnow_1m, hybrid_wifi")
    r.add_argument("--out", type=Path, help="file CSV (mặc định data/latency/<tag>_<giờ>.csv)")
    a = sub.add_parser("analyze", help="phân tích CSV đã ghi")
    a.add_argument("csv", type=Path)
    for p in (r, a):
        p.add_argument("--json", type=Path, help="ghi thêm kết quả dạng JSON")
    args = ap.parse_args(argv)

    if args.cmd == "record":
        out = args.out or DEFAULT_OUT_DIR / (
            f"{args.tag}_{datetime.datetime.now():%Y%m%d_%H%M%S}.csv")
        events = record(resolve_port(args.sensor_port, "sensor"), resolve_port(args.screen_port, "screen"),
                        args.baud, args.duration, out)
    else:
        events = load_csv(args.csv)

    res = analyze(events)
    print()
    print(format_report(res))
    if args.json:
        args.json.write_text(json.dumps(res, ensure_ascii=False, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
