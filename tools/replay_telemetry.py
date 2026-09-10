#!/usr/bin/env python3
"""replay_telemetry.py — phát lại telemetry từ JSONL đúng nhịp (G1 T1.4).

Đọc JSONL do tools/record_telemetry.py tạo (hoặc fixture cùng schema), publish
lên MQTT theo đúng khoảng cách timestamp giữa các dòng để tái hiện cảnh gốc
không cần board — T5.5/T5.7/T5.9 test phân loại/UI.

Format dòng == payload V2 (d1..d6/nearest_cm/has_nearest/timestamp/seq, thêm
recv_epoch_ms khi do record sinh) — schema nguồn duy nhất: tools/scenarios.py.

Secret (R1): token CHỈ đọc từ config/keys.json (gitignored) / env COREIOT_TOKEN /
--token. Không hardcode. Không cần token khi --dry-run.

Usage:
  python3 tools/replay_telemetry.py --in /tmp/tb.jsonl --dry-run
  python3 tools/replay_telemetry.py --in /tmp/tb.jsonl --topic v1/devices/me/telemetry
  python3 tools/replay_telemetry.py --in /tmp/tb.jsonl --scale 0.5   (phát nhanh 2x)
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

SENSOR_KEYS = ("d1", "d2", "d3", "d4", "d5", "d6")
REQUIRED_KEYS = SENSOR_KEYS + ("nearest_cm", "has_nearest")


def load_config_keys() -> dict:
    """Đọc config/keys.json (gitignored). Lỗi/thiếu => {} (dry-run vẫn chạy)."""
    try:
        return json.loads((ROOT / "config" / "keys.json").read_text(encoding="utf-8"))
    except Exception:
        return {}


def mask_token(token: str) -> str:
    return token[:3] + "..." + token[-3:] if len(token) > 6 else "***"


def resolve_token(args: argparse.Namespace, cfg: dict) -> str:
    return (
        args.token
        or os.environ.get("COREIOT_TOKEN", "")
        or cfg.get("SENSOR_NODE_DEVICE_TOKEN", "")
    )


def row_errors(row: dict) -> list[str]:
    """Validate 1 dòng so với schema V2. Trả list lỗi (rỗng = hợp lệ)."""
    errs = []
    for key in REQUIRED_KEYS:
        if key not in row:
            errs.append(f"thiếu '{key}'")
    # d1..d6 phải là số dương.
    for key in SENSOR_KEYS:
        if key in row:
            val = row[key]
            if isinstance(val, bool) or not isinstance(val, (int, float)) or val < 0:
                errs.append(f"'{key}' không phải số không âm: {val!r}")
    return errs


def rows_from_file(path: str) -> list[dict]:
    if not os.path.isfile(path):
        print(f"[ERROR] Không tìm thấy file: {path}", file=sys.stderr)
        return None  # type: ignore[return-value]
    with open(path, encoding="utf-8") as f:
        parsed = []
        bad = 0
        for lineno, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError as exc:
                bad += 1
                print(f"[WARN] dòng {lineno} không phải JSON: {exc}", file=sys.stderr)
                continue
            if not isinstance(obj, dict):
                bad += 1
                print(f"[WARN] dòng {lineno} không phải object", file=sys.stderr)
                continue
            parsed.append(obj)
        if bad:
            print(f"[WARN] {bad} dòng bị bỏ.", file=sys.stderr)
        return parsed


def parse_args() -> argparse.Namespace:
    cfg = load_config_keys()
    parser = argparse.ArgumentParser(
        description="CoreIoT (ThingsBoard) MQTT telemetry replay (V2, JSONL)"
    )
    parser.add_argument("--in", dest="in_path", required=True, help="File JSONL đầu vào")
    parser.add_argument(
        "--broker", default=cfg.get("COREIOT_BROKER", "app.coreiot.io"),
        help="MQTT broker host (default: từ config/keys.json)",
    )
    parser.add_argument(
        "--port", type=int, default=cfg.get("COREIOT_PORT", 1883),
        help="MQTT broker port (default: 1883)",
    )
    parser.add_argument(
        "--token", default=None,
        help="Device access token (ưu tiên --token > env COREIOT_TOKEN > config/keys.json)",
    )
    parser.add_argument(
        "--topic", default=cfg.get("COREIOT_TELEMETRY_TOPIC", "v1/devices/me/telemetry"),
        help="Telemetry topic (default: v1/devices/me/telemetry)",
    )
    parser.add_argument(
        "--scale", type=float, default=1.0,
        help="Tỷ lệ thời gian phát lại: 1.0 = đúng nhịp timestamp, 0.5 = nhanh 2x",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Chỉ validate schema + in từng dòng, không kết nối MQTT",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    rows = rows_from_file(args.in_path)
    if rows is None:
        return 1

    errors = {idx: errs for idx, row in enumerate(rows) if (errs := row_errors(row))}
    if errors:
        for idx, errs in errors.items():
            print(f"[ERROR] dòng {idx + 1}: {', '.join(errs)}", file=sys.stderr)
        print(f"[SCHEMA] {len(errors)}/{len(rows)} dòng lỗi schema (exit 1).", file=sys.stderr)
        return 1

    if args.dry_run:
        print(f"[DRY-RUN] {args.in_path}: {len(rows)} dòng, schema V2 hợp lệ.")
        for idx, row in enumerate(rows, start=1):
            print(f"[ROW #{idx}] {json.dumps(row, ensure_ascii=False)}")
        return 0

    cfg = load_config_keys()
    token = resolve_token(args, cfg)
    if not token.strip() or token.startswith("<"):
        print("[ERROR] Chưa có Device Access Token hợp lệ!", file=sys.stderr)
        print(
            "Tạo token MỚI trên app.coreiot.io (R11), điền vào config/keys.json "
            "(field SENSOR_NODE_DEVICE_TOKEN) hoặc truyền --token.",
            file=sys.stderr,
        )
        return 1

    try:
        import paho.mqtt.client as mqtt  # lazy: --dry-run không cần paho
    except ImportError:
        print(
            "[ERROR] Thiếu paho-mqtt. Cài: pip install -r tools/requirements.txt",
            file=sys.stderr,
        )
        return 2

    print("=" * 60)
    print("      CoreIoT Telemetry Replay — V2")
    print("=" * 60)
    print(f"Input:         {args.in_path} ({len(rows)} dòng)")
    print(f"Broker:        {args.broker}:{args.port}")
    print(f"Access Token:  {mask_token(token)}")
    print(f"Topic:         {args.topic}")
    print(f"Time scale:    {args.scale}")
    print("=" * 60)

    client = mqtt.Client()
    client.username_pw_set(token)
    try:
        client.connect(args.broker, args.port, keepalive=60)
    except Exception as exc:
        print(f"[ERROR] Không thể kết nối tới MQTT broker: {exc}", file=sys.stderr)
        return 1
    client.loop_start()

    # Nhịp = timestamp giữa các dòng (ưu tiên recv_epoch_ms — ghi chính xác thời
    # điểm nhận; fallback timestamp của thiết bị).
    prev_ts = None
    sent = 0
    try:
        for idx, row in enumerate(rows, start=1):
            row_ts = row.get("recv_epoch_ms") or row.get("timestamp")
            if prev_ts is not None and row_ts is not None and row_ts > prev_ts:
                delay = (row_ts - prev_ts) / 1000.0 * args.scale
                if delay > 0:
                    if delay > 5.0:
                        print(f"[INFO] gap {delay:.1f}s giữa dòng {idx - 1}->{idx} (giữ nguyên nhịp).")
                    time.sleep(min(delay, 10.0))
            seq = row.get("seq", idx)
            payload = {k: v for k, v in row.items() if k != "recv_epoch_ms"}
            client.publish(args.topic, json.dumps(payload, ensure_ascii=False), qos=1)
            sent += 1
            print(f"[SEND #{seq}] {json.dumps(payload, ensure_ascii=False)}")
            prev_ts = row_ts or prev_ts
        time.sleep(0.5)
    except KeyboardInterrupt:
        print("\n[INFO] Đã dừng bởi người dùng.")
    finally:
        client.loop_stop()
        client.disconnect()

    print(f"[DONE] Phát lại {sent}/{len(rows)} dòng.")
    return 0


if __name__ == "__main__":
    sys.exit(main())