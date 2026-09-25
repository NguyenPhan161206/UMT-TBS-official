#!/usr/bin/env python3
"""data_replayer.py — Telemetry Data Replayer (T1.4).

Đọc các file .jsonl (đã ghi bởi data_recorder.py) và phát lại theo đúng nhịp
thời gian thực (elapsed_ms).

Hỗ trợ các đích nhận (--target):
  1. udp     : Bơm JSON qua UDP socket tới 127.0.0.1:9090 (kết nối T1.2 LVGL Simulator)
  2. console : Hiển thị bảng radar màu ANSI trực quan trên Terminal
  3. mqtt    : Publish telemetry lên CoreIoT MQTT broker (cho B9/cloud acceptance)

Hỗ trợ tốc độ (--speed), phát lặp (--loop).
"""
from __future__ import annotations

import argparse
import json
import socket
import sys
import time
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

# ANSI colors
RED = "\033[91m"
YELLOW = "\033[93m"
GREEN = "\033[92m"
CYAN = "\033[96m"
GRAY = "\033[90m"
RESET = "\033[0m"
BOLD = "\033[1m"

# Thresholds (mirror thresholds.h: 100/30)
CAUTION_CM = 100.0
DANGER_CM = 30.0


def format_zone_str(distance_cm: float, valid: int) -> str:
    """Định dạng màu sắc hiển thị theo zone cảnh báo."""
    if valid == 0 or distance_cm <= 0:
        return f"{GRAY}  -- cm (NO DATA) {RESET}"
    if distance_cm <= DANGER_CM:
        return f"{RED}{BOLD}{distance_cm:5.1f} cm [DANGER ]{RESET}"
    if distance_cm <= CAUTION_CM:
        return f"{YELLOW}{BOLD}{distance_cm:5.1f} cm [CAUTION]{RESET}"
    return f"{GREEN}{distance_cm:5.1f} cm [ SAFE  ]{RESET}"


def print_console_frame(frame: dict[str, Any], frame_idx: int, total_frames: int):
    """Vẽ bảng hiển thị trực quan thông số cảm biến."""
    dists = frame.get("distances", [0.0] * 6)
    valid = frame.get("valid", [0] * 6)
    nearest = frame.get("nearest_cm", 0.0)
    elapsed_ms = frame.get("elapsed_ms", 0)
    tag = frame.get("tag", "")

    labels = ["S0 FRONT      ", "S1 REAR       ", "S2 LEFT_FRONT ",
              "S3 LEFT_REAR  ", "S4 RIGHT_FRONT", "S5 RIGHT_REAR "]

    # Header
    print(f"\n{CYAN}--- [FRAME {frame_idx + 1}/{total_frames}] Elapsed: {elapsed_ms}ms | Tag: '{tag}' ---{RESET}")

    # 6 Slots
    for i in range(6):
        d = dists[i] if i < len(dists) else 0.0
        v = valid[i] if i < len(valid) else 0
        zone_str = format_zone_str(d, v)
        print(f"  {labels[i]}: {zone_str}")

    # Nearest summary
    if nearest > 0:
        near_color = RED if nearest <= DANGER_CM else YELLOW if nearest <= CAUTION_CM else GREEN
        print(f"  {BOLD}NEAREST OBJECT : {near_color}{nearest:.1f} cm{RESET}")
    else:
        print(f"  {BOLD}NEAREST OBJECT : {GRAY}None{RESET}")


class UdpSender:
    """Gửi frame JSON qua UDP socket tới localhost."""
    def __init__(self, host: str = "127.0.0.1", port: int = 9090):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def send(self, frame: dict[str, Any]):
        data = json.dumps(frame).encode("utf-8")
        self.sock.sendto(data, (self.host, self.port))

    def close(self):
        self.sock.close()


class MqttSender:
    """Gửi telemetry lên CoreIoT MQTT broker."""
    def __init__(self, broker: str, port: int, topic: str, token: str):
        import paho.mqtt.client as mqtt
        self.broker = broker
        self.port = port
        self.topic = topic
        self.client = mqtt.Client(client_id="t14_data_replayer")
        if token:
            self.client.username_pw_set(token, "")
        self.client.connect(broker, port, keepalive=60)
        self.client.loop_start()

    def send(self, frame: dict[str, Any]):
        dists = frame.get("distances", [0.0] * 6)
        payload = {
            f"d{i+1}": dists[i] for i in range(min(6, len(dists)))
        }
        payload["nearest_cm"] = frame.get("nearest_cm", 0.0)
        payload["has_nearest"] = frame.get("has_nearest", False)
        self.client.publish(self.topic, json.dumps(payload))

    def close(self):
        self.client.loop_stop()
        self.client.disconnect()


def replay_file(
    file_path: Path,
    target: str = "console",
    speed: float = 1.0,
    loop: bool = False,
    limit: int = 0,
    udp_host: str = "127.0.0.1",
    udp_port: int = 9090,
    keys_path: Path | None = None,
) -> int:
    """Đọc và phát lại dữ liệu từ file .jsonl."""
    if not file_path.exists():
        print(f"ERROR: File không tồn tại: {file_path}", file=sys.stderr)
        return 1

    # Đọc toàn bộ frames vào memory
    frames: list[dict[str, Any]] = []
    with open(file_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    frames.append(json.loads(line))
                except json.JSONDecodeError:
                    pass

    total_frames = len(frames)
    if total_frames == 0:
        print(f"ERROR: File {file_path} không có frame hợp lệ nào!", file=sys.stderr)
        return 1

    print(f"[REPLAYER] Loaded {total_frames} frames from {file_path}")
    print(f"           Target: {target} | Speed: {speed}x | Loop: {loop}")

    # Khởi tạo sender theo target
    udp_sender = None
    mqtt_sender = None

    if target == "udp":
        udp_sender = UdpSender(udp_host, udp_port)
        print(f"[REPLAYER] Sending UDP packets to {udp_host}:{udp_port}")
    elif target == "mqtt":
        broker = "app.coreiot.io"
        port = 1883
        topic = "v1/devices/me/telemetry"
        token = ""
        if keys_path and keys_path.exists():
            try:
                kdata = json.loads(keys_path.read_text(encoding="utf-8"))
                broker = kdata.get("COREIOT_BROKER", broker)
                port = int(kdata.get("COREIOT_PORT", port))
                token = kdata.get("SENSOR_NODE_DEVICE_TOKEN", "")
            except Exception as e:
                print(f"WARN: Error reading keys.json: {e}", file=sys.stderr)
        mqtt_sender = MqttSender(broker, port, topic, token)
        print(f"[REPLAYER] Publishing to CoreIoT {broker}:{port}, topic {topic}")

    sent_count = 0
    try:
        while True:
            first_frame_time = None
            sim_start_time = time.time()

            for idx, frame in enumerate(frames):
                if limit > 0 and sent_count >= limit:
                    return sent_count

                elapsed_ms = frame.get("elapsed_ms", 0)
                if first_frame_time is None:
                    first_frame_time = elapsed_ms
                    sim_start_time = time.time()
                else:
                    # Pacing nhịp thời gian thực
                    target_time_sec = (elapsed_ms - first_frame_time) / (1000.0 * speed)
                    actual_time_sec = time.time() - sim_start_time
                    delay = target_time_sec - actual_time_sec
                    if delay > 0:
                        time.sleep(delay)

                # Dispatching
                if target == "console":
                    print_console_frame(frame, idx, total_frames)
                elif target == "udp" and udp_sender:
                    udp_sender.send(frame)
                elif target == "mqtt" and mqtt_sender:
                    mqtt_sender.send(frame)

                sent_count += 1

            if not loop:
                break
            print(f"\n{CYAN}--- [LOOP] Restarting playback from beginning... ---{RESET}")

    except KeyboardInterrupt:
        print("\n[REPLAYER] Bị ngắt bởi người dùng.")
    finally:
        if udp_sender:
            udp_sender.close()
        if mqtt_sender:
            mqtt_sender.close()

    print(f"\n[REPLAYER] Hoàn thành. Đã phát {sent_count} frames.")
    return sent_count


def main() -> int:
    parser = argparse.ArgumentParser(description="T1.4 Telemetry Data Replayer (Console / UDP / MQTT)")
    parser.add_argument("--input", "-i", type=Path, required=True,
                        help="Đường dẫn file .jsonl cần phát lại.")
    parser.add_argument("--target", choices=["console", "udp", "mqtt"], default="console",
                        help="Đích phát: console (màn hình text), udp (cho T1.2 simulator), mqtt (CoreIoT).")
    parser.add_argument("--speed", type=float, default=1.0,
                        help="Hệ số tốc độ phát lại (1.0 = chuẩn, 2.0 = gấp đôi, 0.5 = chậm một nửa).")
    parser.add_argument("--loop", action="store_true", help="Lặp lại vô tận khi phát hết file.")
    parser.add_argument("--limit", type=int, default=0, help="Giới hạn số frame tối đa cần phát (0 = hết file).")
    parser.add_argument("--udp-host", default="127.0.0.1", help="Host UDP (mặc định: 127.0.0.1).")
    parser.add_argument("--udp-port", type=int, default=9090, help="Port UDP (mặc định: 9090).")
    parser.add_argument("--keys", type=Path, default=ROOT / "config" / "keys.json",
                        help="Đường dẫn keys.json khi dùng target mqtt.")

    args = parser.parse_args()
    res = replay_file(
        file_path=args.input,
        target=args.target,
        speed=args.speed,
        loop=args.loop,
        limit=args.limit,
        udp_host=args.udp_host,
        udp_port=args.udp_port,
        keys_path=args.keys,
    )
    return 0 if res > 0 else 1


if __name__ == "__main__":
    sys.exit(main())
