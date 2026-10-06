#!/usr/bin/env python3
"""measure_accuracy.py — Đo độ chính xác/ổn định (DMXT-55) và tầm đo/góc búp (DMXT-56).

Cần sensor-node flash env `yolo_uno_accuracy` (TBS_ACCURACY_PROBE=1), mỗi lần đọc in:
  hợp lệ:  "ACC S<i> raw=<cm> out=<cm|nan> has=<0/1> status=<FilterResult.status> n=<clusterCount> pulse=<us>"
  reject:  "ACC S<i> raw=nan pulse=<us> calc=<cm|nan> rej=<lý do>"
Quy trình: docs/ACCURACY_TEST.md.

  record   ghi 1 mốc (1 cảm biến, 1 khoảng cách chuẩn, tuỳ chọn góc/độ cao) ra CSV.
  analyze  gộp nhiều CSV -> bảng theo mốc (raw vs đã lọc), hoặc --beam: tỷ lệ phát hiện theo (độ cao, góc).

Ví dụ:
  python tools/accuracy/measure_accuracy.py record --sensor 0 --true-cm 100 --duration 30 --tag acc
  python tools/accuracy/measure_accuracy.py analyze data/accuracy/acc_*.csv
  python tools/accuracy/measure_accuracy.py record --sensor 0 --true-cm 150 --angle-deg 20 --height-cm 50 --tag beam
  python tools/accuracy/measure_accuracy.py analyze --beam data/accuracy/beam_*.csv
"""
from __future__ import annotations

import argparse
import csv
import datetime
import glob
import json
import math
import re
import sys
import time
from collections import defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from serial_ports import resolve_port  # noqa: E402
DEFAULT_OUT_DIR = ROOT / "data" / "accuracy"
DEFAULT_BAUD = 115200
DEFAULT_TOL_CM = 15.0  # beam: mẫu "phát hiện vật" nếu |raw - true| <= tol

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

_ACC_RE = re.compile(r"\bACC S(\d+)\s+(.*)")
_KV_RE = re.compile(r"(\w+)=(\S+)")

CSV_FIELDS = ["t_s", "sensor", "true_cm", "angle_deg", "height_cm", "tag",
              "raw", "out", "has", "status", "n", "pulse", "calc", "rej"]


@dataclass
class Sample:
    t_s: float
    sensor: int
    true_cm: float
    angle_deg: float | None
    height_cm: float | None
    tag: str
    raw: float          # nan = reject
    out: float          # nan = bộ lọc chưa có đầu ra / reject
    has: int            # 1 = bộ lọc có đầu ra
    status: str
    n: int
    pulse: int
    calc: float         # khoảng cách tính từ xung khi reject (nan nếu không có Echo)
    rej: str


def _num(v: str | None) -> float:
    try:
        return float(v) if v is not None else math.nan
    except ValueError:
        return math.nan


def parse_acc_line(line: str) -> tuple[int, dict[str, str]] | None:
    """-> (sensor index, fields). `rej` lấy phần còn lại của dòng (lý do có dấu cách)."""
    m = _ACC_RE.search(line)
    if not m:
        return None
    rest = m.group(2)
    fields: dict[str, str] = {}
    if " rej=" in f" {rest}":
        head, _, rej = f" {rest}".partition(" rej=")
        fields["rej"] = rej.strip()
        rest = head
    fields.update(dict(_KV_RE.findall(rest)))
    return int(m.group(1)), fields


def to_sample(t_s: float, sensor: int, fields: dict[str, str], true_cm: float, tag: str,
              angle_deg: float | None = None, height_cm: float | None = None) -> Sample:
    return Sample(t_s, sensor, true_cm, angle_deg, height_cm, tag,
                  raw=_num(fields.get("raw")), out=_num(fields.get("out")),
                  has=int(fields.get("has", 0) or 0), status=fields.get("status", ""),
                  n=int(fields.get("n", 0) or 0), pulse=int(fields.get("pulse", 0) or 0),
                  calc=_num(fields.get("calc")), rej=fields.get("rej", ""))


# ------------------------------------------------------------------ stats

def _mean(v: list[float]) -> float:
    return sum(v) / len(v) if v else math.nan


def _std(v: list[float]) -> float:
    if len(v) < 2:
        return math.nan
    m = _mean(v)
    return math.sqrt(sum((x - m) ** 2 for x in v) / (len(v) - 1))


def error_stats(values: list[float], true_cm: float) -> dict[str, float]:
    if not values:
        return {"n": 0, "mean": math.nan, "bias": math.nan, "std": math.nan, "mae": math.nan,
                "max_abs_err": math.nan}
    errs = [x - true_cm for x in values]
    return {"n": len(values), "mean": _mean(values), "bias": _mean(errs), "std": _std(values),
            "mae": _mean([abs(e) for e in errs]), "max_abs_err": max(abs(e) for e in errs)}


def summarize_point(samples: list[Sample]) -> dict:
    """1 mốc khoảng cách: tỷ lệ reject, thống kê raw và đã lọc."""
    true_cm = samples[0].true_cm
    raw = [s.raw for s in samples if not math.isnan(s.raw)]
    out = [s.out for s in samples if s.has == 1 and not math.isnan(s.out)]
    total = len(samples)
    return {
        "true_cm": true_cm,
        "sensor": samples[0].sensor,
        "attempts": total,
        "reject_pct": 100.0 * (total - len(raw)) / total if total else math.nan,
        "raw": error_stats(raw, true_cm),
        "filtered": error_stats(out, true_cm),
        "has_output_pct": 100.0 * len(out) / len(raw) if raw else math.nan,
    }


def summarize_beam(samples: list[Sample], tol_cm: float = DEFAULT_TOL_CM) -> dict:
    """1 ô (độ cao, góc, khoảng cách): % đọc hợp lệ và % phát hiện đúng vật (|raw-true| <= tol)."""
    true_cm = samples[0].true_cm
    raw = [s.raw for s in samples if not math.isnan(s.raw)]
    hit = [x for x in raw if abs(x - true_cm) <= tol_cm]
    total = len(samples)
    return {
        "height_cm": samples[0].height_cm, "angle_deg": samples[0].angle_deg, "true_cm": true_cm,
        "sensor": samples[0].sensor, "attempts": total,
        "valid_pct": 100.0 * len(raw) / total if total else math.nan,
        "detect_pct": 100.0 * len(hit) / total if total else math.nan,
        "raw_mean_hit": _mean(hit),
    }


def group(samples: Iterable[Sample], beam: bool) -> dict[tuple, list[Sample]]:
    g: dict[tuple, list[Sample]] = defaultdict(list)
    for s in samples:
        key = (s.sensor, s.height_cm, s.angle_deg, s.true_cm) if beam else (s.sensor, s.true_cm)
        g[key].append(s)
    return dict(sorted(g.items(), key=lambda kv: tuple(-1e9 if x is None else x for x in kv[0])))


def _f(v: float, fmt: str = "{:.2f}") -> str:
    return "–" if v is None or (isinstance(v, float) and math.isnan(v)) else fmt.format(v)


def format_points(points: list[dict]) -> str:
    lines = [
        "| Cảm biến | Mốc (cm) | n | % reject | raw mean | raw bias | raw σ | raw MAE | raw |err| max "
        "| lọc n | lọc mean | lọc bias | lọc σ | lọc MAE | % có đầu ra |",
        "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|",
    ]
    for p in points:
        r, f = p["raw"], p["filtered"]
        lines.append(
            f"| S{p['sensor']} | {p['true_cm']:g} | {p['attempts']} | {_f(p['reject_pct'], '{:.1f}')} | "
            f"{_f(r['mean'])} | {_f(r['bias'], '{:+.2f}')} | {_f(r['std'])} | {_f(r['mae'])} | "
            f"{_f(r['max_abs_err'])} | {f['n']} | {_f(f['mean'])} | {_f(f['bias'], '{:+.2f}')} | "
            f"{_f(f['std'])} | {_f(f['mae'])} | {_f(p['has_output_pct'], '{:.1f}')} |")
    return "\n".join(lines)


def format_beam(cells: list[dict], tol_cm: float) -> str:
    lines = [f"Phát hiện = |raw − mốc| ≤ {tol_cm:g} cm, tính trên mọi lần đọc.",
             "| Cảm biến | Độ cao (cm) | Góc (°) | Mốc (cm) | n | % hợp lệ | % phát hiện | raw mean (khi phát hiện) |",
             "|---|---|---|---|---|---|---|---|"]
    for c in cells:
        lines.append(f"| S{c['sensor']} | {_f(c['height_cm'], '{:g}')} | {_f(c['angle_deg'], '{:g}')} | "
                     f"{c['true_cm']:g} | {c['attempts']} | {_f(c['valid_pct'], '{:.1f}')} | "
                     f"{_f(c['detect_pct'], '{:.1f}')} | {_f(c['raw_mean_hit'])} |")
    return "\n".join(lines)


# ------------------------------------------------------------------ CSV I/O

def _opt(v: str) -> float | None:
    return None if v in ("", "None") else float(v)


def load_csv(paths: Iterable[Path]) -> list[Sample]:
    out: list[Sample] = []
    for p in paths:
        with Path(p).open(encoding="utf-8", newline="") as f:
            for r in csv.DictReader(f):
                out.append(Sample(float(r["t_s"]), int(r["sensor"]), float(r["true_cm"]),
                                  _opt(r["angle_deg"]), _opt(r["height_cm"]), r["tag"],
                                  _num(r["raw"]), _num(r["out"]), int(r["has"]), r["status"],
                                  int(r["n"]), int(r["pulse"]), _num(r["calc"]), r["rej"]))
    return out


def write_csv(path: Path, samples: list[Sample]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=CSV_FIELDS)
        w.writeheader()
        for s in samples:
            w.writerow({k: ("" if v is None else v) for k, v in asdict(s).items()})


# ------------------------------------------------------------------ record

def _open_serial(port: str, baud: int):
    import serial  # lazy

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0.5
    ser.dtr = False  # không reset board khi mở cổng
    ser.rts = False
    ser.open()
    return ser


def record(port: str, baud: int, sensor: int, true_cm: float, duration: float, skip: float, tag: str,
           angle_deg: float | None, height_cm: float | None) -> list[Sample]:
    ser = _open_serial(port, baud)
    samples: list[Sample] = []
    t0 = time.monotonic()
    buf = b""
    print(f"[ACC] {port} S{sensor} mốc {true_cm:g} cm — bỏ {skip:g} s đầu, ghi {duration:g} s ...")
    try:
        while time.monotonic() - t0 < skip + duration:
            chunk = ser.read(ser.in_waiting or 1)
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                t = time.monotonic() - t0
                p = parse_acc_line(raw.decode("utf-8", errors="replace"))
                if not p or p[0] != sensor or t < skip:
                    continue
                samples.append(to_sample(t - skip, sensor, p[1], true_cm, tag, angle_deg, height_cm))
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
    return samples


def main(argv: Iterable[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Độ chính xác / góc búp JSN-SR04T (DMXT-55/56)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("record", help="ghi 1 mốc")
    r.add_argument("--port", default="auto", help="mặc định dò sensor-node theo VID:PID 303A:1001")
    r.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    r.add_argument("--sensor", type=int, default=0, help="chỉ số S<i> trong log (0..5)")
    r.add_argument("--true-cm", type=float, required=True, help="khoảng cách chuẩn đo bằng thước")
    r.add_argument("--duration", type=float, default=30, help="giây ghi (10 mẫu/s)")
    r.add_argument("--skip", type=float, default=2, help="giây bỏ đầu (WARMUP bộ lọc)")
    r.add_argument("--tag", default="acc")
    r.add_argument("--angle-deg", type=float)
    r.add_argument("--height-cm", type=float)
    r.add_argument("--out", type=Path)
    a = sub.add_parser("analyze", help="gộp nhiều CSV")
    a.add_argument("csv", nargs="+", help="file CSV (glob được)")
    a.add_argument("--beam", action="store_true", help="bảng tỷ lệ phát hiện theo (độ cao, góc)")
    a.add_argument("--tol-cm", type=float, default=DEFAULT_TOL_CM)
    a.add_argument("--json", type=Path)
    args = ap.parse_args(list(argv) if argv is not None else None)

    if args.cmd == "record":
        parts = [args.tag, f"S{args.sensor}", f"{args.true_cm:g}cm"]
        if args.height_cm is not None:
            parts.append(f"h{args.height_cm:g}")
        if args.angle_deg is not None:
            parts.append(f"a{args.angle_deg:g}")
        out = args.out or DEFAULT_OUT_DIR / ("_".join(parts) + f"_{datetime.datetime.now():%Y%m%d_%H%M%S}.csv")
        samples = record(resolve_port(args.port, "sensor"), args.baud, args.sensor, args.true_cm, args.duration, args.skip,
                         args.tag, args.angle_deg, args.height_cm)
        write_csv(out, samples)
        print(f"[ACC] {len(samples)} mẫu -> {out}")
        if not samples:
            print("[ACC] Không có dòng ACC — đã flash env yolo_uno_accuracy và đúng --sensor chưa?",
                  file=sys.stderr)
            return 1
        beam = args.angle_deg is not None
        groups = group(samples, beam)
    else:
        paths = [Path(p) for pat in args.csv for p in (glob.glob(pat) or [pat])]
        missing = [str(p) for p in paths if not p.is_file()]
        if missing:
            print(f"[ACC] Không tìm thấy file: {', '.join(missing)} — kiểm tra --tag/--sensor đã dùng khi record "
                  "(xem thư mục data/accuracy/).", file=sys.stderr)
            return 1
        samples = load_csv(paths)
        beam = args.beam
        groups = group(samples, beam)

    tol = getattr(args, "tol_cm", DEFAULT_TOL_CM)
    if beam:
        res = [summarize_beam(g, tol) for g in groups.values()]
        print(format_beam(res, tol))
    else:
        res = [summarize_point(g) for g in groups.values()]
        print(format_points(res))
    if getattr(args, "json", None):
        args.json.write_text(json.dumps(res, ensure_ascii=False, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
