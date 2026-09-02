#!/usr/bin/env python3
"""test_mqtt_coreiot.py — CoreIoT (ThingsBoard) MQTT test publisher (V2).

Nghiệm thu B9: gửi telemetry đúng format sensor-node V2 (d1..d6 + nearest_cm +
has_nearest) lên app.coreiot.io để rule-chain xử lý thành warning_status trên
dashboard waveshare-screen.

Secret (R1): token CHỈ đọc từ config/keys.json (gitignored) hoặc env
COREIOT_TOKEN / --token. Không hardcode token nào trong source.
Không cần token: --help, --dry-run (in payload, không kết nối MQTT).

Usage:
  python3 tools/test_mqtt_coreiot.py --dry-run --distance 25
  python3 tools/test_mqtt_coreiot.py --distance 15.5
  python3 tools/test_mqtt_coreiot.py --loop --interval 2
"""
from __future__ import annotations

import argparse
import json
import os
import random
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

DEFAULT_BROKER = "app.coreiot.io"
DEFAULT_PORT = 1883
DEFAULT_TOPIC = "v1/devices/me/telemetry"

# Format telemetry sensor-node V2 (firmware/sensor-node/src/main.cpp).
SENSOR_KEYS = ("d1", "d2", "d3", "d4", "d5", "d6")

# Ngưỡng zone — mirror firmware/shared/thresholds.h (R3/R11): 100 / 30 (cm).
CAUTION_CM = 100.0
DANGER_CM = 30.0
RANDOM_MIN_CM = 10.0
RANDOM_MAX_CM = 180.0


def load_config_keys() -> dict:
    """Đọc config/keys.json (gitignored). Lỗi/thiếu => {} (chạy được --dry-run)."""
    try:
        return json.loads((ROOT / "config" / "keys.json").read_text(encoding="utf-8"))
    except Exception:
        return {}


def classify(distance: float) -> str:
    """Phân loại zone theo thresholds.h: DANGER <= 30, CAUTION <= 100, NORMAL."""
    if distance <= DANGER_CM:
        return "DANGER"
    if distance <= CAUTION_CM:
        return "CAUTION"
    return "NORMAL"


def build_payload(distance: float, seq: int | None = None) -> dict:
    """Telemetry đúng format sensor-node V2: 6 slot + nearest_cm + has_nearest."""
    dist = round(float(distance), 1)
    payload = {key: dist for key in SENSOR_KEYS}
    payload["nearest_cm"] = dist
    payload["has_nearest"] = True
    payload["warning_status"] = classify(dist)
    payload["vehicle_detected"] = dist <= CAUTION_CM
    payload["timestamp"] = int(time.time() * 1000)
    if seq is not None:
        payload["seq"] = seq
    return payload


def resolve_token(args: argparse.Namespace, cfg: dict) -> str:
    return (
        args.token
        or os.environ.get("COREIOT_TOKEN", "")
        or cfg.get("SENSOR_NODE_DEVICE_TOKEN", "")
    )


def mask_token(token: str) -> str:
    return token[:3] + "..." + token[-3:] if len(token) > 6 else "***"


def parse_args() -> argparse.Namespace:
    cfg = load_config_keys()
    parser = argparse.ArgumentParser(
        description="CoreIoT (ThingsBoard) MQTT test publisher (V2)"
    )
    parser.add_argument(
        "--broker", default=cfg.get("COREIOT_BROKER", DEFAULT_BROKER),
        help="MQTT broker host (default: app.coreiot.io)",
    )
    parser.add_argument(
        "--port", type=int, default=cfg.get("COREIOT_PORT", DEFAULT_PORT),
        help="MQTT broker port (default: 1883)",
    )
    parser.add_argument(
        "--token", default=None,
        help="Device access token (ưu tiên --token > env COREIOT_TOKEN > config/keys.json)",
    )
    parser.add_argument(
        "--topic", default=cfg.get("COREIOT_TELEMETRY_TOPIC", DEFAULT_TOPIC),
        help="Telemetry topic (default: v1/devices/me/telemetry)",
    )
    parser.add_argument(
        "--distance", type=float, default=None,
        help="Khoảng cách cố định (cm); mặc định random 10–180",
    )
    parser.add_argument(
        "--loop", action="store_true", help="Gửi dữ liệu liên tục (Ctrl+C để dừng)"
    )
    parser.add_argument(
        "--interval", type=float, default=2.0,
        help="Chu kỳ loop (giây; mặc định 2.0s)",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Chỉ in payload, không kết nối MQTT (không cần token)",
    )
    return parser.parse_args()


def publish_payload(client, topic: str, payload: dict) -> bool:
    client.publish(topic, json.dumps(payload), qos=1)


def run_publisher(args: argparse.Namespace) -> int:
    """Kết nối MQTT và gửi payload theo chế độ. Trả về exit code."""
    try:
        import paho.mqtt.client as mqtt  # lazy: test/dry-run không cần paho
    except ImportError:
        print(
            "[ERROR] Thiếu paho-mqtt. Cài: pip install -r tools/requirements.txt",
            file=sys.stderr,
        )
        return 2

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

    print("=" * 60)
    print("      CoreIoT (ThingsBoard) MQTT Test Publisher — V2")
    print("=" * 60)
    print(f"Broker Host:   {args.broker}:{args.port}")
    print(f"Access Token:  {mask_token(token)}")
    print(f"Topic:         {args.topic}")
    print(f"Loop Mode:     {args.loop} (Interval: {args.interval}s)")
    print("=" * 60)

    client = mqtt.Client()
    client.username_pw_set(token)
    try:
        client.connect(args.broker, args.port, keepalive=60)
    except Exception as exc:  # broker unreachable / sai broker
        print(f"[ERROR] Không thể kết nối tới MQTT broker: {exc}", file=sys.stderr)
        return 1
    client.loop_start()
    time.sleep(1)

    try:
        counter = 1
        while True:
            distance = (
                args.distance
                if args.distance is not None
                else round(random.uniform(RANDOM_MIN_CM, RANDOM_MAX_CM), 1)
            )
            payload = build_payload(distance, seq=counter if args.loop else None)
            label = f"#{counter}" if args.loop else ""
            print(f"[SEND {label}] Payload: {json.dumps(payload)}")
            publish_payload(client, args.topic, payload)
            if not args.loop:
                break
            counter += 1
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\n[INFO] Đã dừng bởi người dùng.")
    finally:
        client.loop_stop()
        client.disconnect()
    print("[MQTT] Đã ngắt kết nối thành công.")
    return 0


def main() -> int:
    args = parse_args()

    if args.dry_run:
        distance = (
            args.distance
            if args.distance is not None
            else round(random.uniform(RANDOM_MIN_CM, RANDOM_MAX_CM), 1)
        )
        payload = build_payload(distance)
        print("[DRY-RUN] Payload mẫu (chưa gửi MQTT):")
        print(json.dumps(payload, indent=2))
        print(f"[DRY-RUN] warning_status = {payload['warning_status']}")
        return 0

    return run_publisher(args)


if __name__ == "__main__":
    sys.exit(main())