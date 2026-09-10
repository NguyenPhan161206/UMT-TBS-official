#!/usr/bin/env python3
"""record_telemetry.py — ghi telemetry CoreIoT/ThingsBoard thật ra JSONL (G1 T1.4).

Subscribe topic `v1/devices/me/telemetry` của SENSOR-NODE (board flash
`yolo_uno_coreiot`), ghi mỗi message thành 1 dòng JSONL kèm timestamp nhận.
Format dòng == payload V2 (d1..d6/nearest_cm/has_nearest/timestamp/seq) — schema
nguồn duy nhất: tools/scenarios.py (được test_mqtt_coreiot.py pin; xem
docs/ARCHITECTURE_G1_TESTING.md). Purpose: thu thập dữ liệu thật để replay
(tools/replay_telemetry.py) tái hiện cảnh trong test/UI không cần board — T5.5/T5.7/T5.9.

Secret (R1): token CHỈ đọc từ config/keys.json (gitignored) / env COREIOT_TOKEN /
--token. Không hardcode.

Usage:
  python3 tools/record_telemetry.py --seconds 10 --out /tmp/tb.jsonl
  python3 tools/record_telemetry.py --out /tmp/tb.jsonl         # Ctrl+C để dừng
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Format telemetry sensor-node V2 — khớp tools/scenarios.py + test_mqtt_coreiot.py.
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


def schema_partial(payload: dict) -> bool:
    """Kiểm tra dòng có đủ key V2? Không đủ => cảnh báo, vẫn ghi raw (để khảo sát)."""
    missing = [k for k in REQUIRED_KEYS if k not in payload]
    return len(missing) == 0


def parse_args() -> argparse.Namespace:
    cfg = load_config_keys()
    parser = argparse.ArgumentParser(
        description="CoreIoT (ThingsBoard) MQTT telemetry recorder (V2)"
    )
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
        "--seconds", type=float, default=None,
        help="Số giây ghi (mặc định: chạy tới khi Ctrl+C). Ví dụ --seconds 10",
    )
    parser.add_argument(
        "--out", default=None,
        help="File JSONL đầu ra (mặc định: in ra stdout)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
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
        import paho.mqtt.client as mqtt  # lazy: --help/không dùng thì không cần paho
    except ImportError:
        print(
            "[ERROR] Thiếu paho-mqtt. Cài: pip install -r tools/requirements.txt",
            file=sys.stderr,
        )
        return 2

    print("=" * 60)
    print("      CoreIoT Telemetry Recorder — V2 (JSONL)")
    print("=" * 60)
    print(f"Broker:        {args.broker}:{args.port}")
    print(f"Access Token:  {mask_token(token)}")
    print(f"Topic:         {args.topic}")
    print(f"Output:        {args.out or '<stdout>'}")
    print(f"Duration:      {args.seconds}s" if args.seconds else "Duration:      until Ctrl+C")
    print("=" * 60)

    out_handle = None
    close_out = False
    if args.out:
        out_handle = open(args.out, "w", encoding="utf-8")
        close_out = True
    else:
        out_handle = sys.stdout

    rows = 0
    failed_rows = 0

    client = mqtt.Client()
    client.username_pw_set(token)

    def on_connect(c, userdata, flags, reason_code, properties=None):
        # paho 2.x: reason_code; paho 1.x: rc int. Xử lý linh hoạt.
        rc = reason_code if not isinstance(reason_code, int) else reason_code
        print(f"[MQTT] Connected (rc={rc}), subscribing {args.topic}")
        c.subscribe(args.topic, qos=1)

    def on_message(c, userdata, msg):
        nonlocal rows, failed_rows
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
            if not isinstance(payload, dict):
                raise ValueError("payload không phải JSON object")
        except Exception as exc:
            failed_rows += 1
            print(f"[WARN] Bỏ dòng không parse được: {exc}", file=sys.stderr)
            return
        payload.setdefault("timestamp", int(time.time() * 1000))
        payload["recv_epoch_ms"] = int(time.time() * 1000)
        if not schema_partial(payload):
            missing = [k for k in REQUIRED_KEYS if k not in payload]
            print(f"[WARN] Payload thiếu schema V2 {missing}: {payload}", file=sys.stderr)
        row = json.dumps(payload, ensure_ascii=False)
        out_handle.write(row + "\n")
        out_handle.flush()
        rows += 1
        print(f"[REC #{rows}] {row}")

    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(args.broker, args.port, keepalive=60)
    except Exception as exc:
        print(f"[ERROR] Không thể kết nối tới MQTT broker: {exc}", file=sys.stderr)
        if close_out:
            out_handle.close()
        return 1
    client.loop_start()

    try:
        if args.seconds:
            time.sleep(args.seconds)
        else:
            while True:
                time.sleep(0.5)
    except KeyboardInterrupt:
        print("\n[INFO] Đã dừng bởi người dùng.")
    finally:
        client.loop_stop()
        client.disconnect()
        if close_out:
            out_handle.close()

    print(f"[DONE] Ghi {rows} dòng" + (f", {failed_rows} dòng lỗi bị bỏ." if failed_rows else "."))
    return 0


if __name__ == "__main__":
    sys.exit(main())