#!/usr/bin/env python3
"""data_recorder.py — Telemetry Data Recorder (T1.4).

Thu thập luồng dữ liệu cảm biến từ:
  1. Cổng Serial (USB CDC / UART từ sensor-node hoặc waveshare-screen)
  2. MQTT CoreIoT (subscribe topic v1/devices/me/telemetry)
  3. Mock generator (sinh kịch bản ảo để test khi chưa cắm board)

Lưu vào file .jsonl (JSON Lines) với timestamp milli-giây để data_replayer.py
phát lại chính xác nhịp đo thực tế.

Tuân thủ R1: Token MQTT chỉ đọc từ config/keys.json hoặc --token, không hardcode.
"""
from __future__ import annotations

import argparse
import datetime
import json
import math
import os
import re
import sys
import time
from pathlib import Path
from typing import Any, Callable, Generator

ROOT = Path(__file__).resolve().parents[2]

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

SENSOR_COUNT = 6
DEFAULT_BAUD = 115200
DEFAULT_INTERVAL_SEC = 0.200  # 200 ms


def create_record_frame(
    distances: list[float],
    valid: list[int] | None = None,
    elapsed_ms: int = 0,
    tag: str = "",
    source: str = "unknown",
) -> dict[str, Any]:
    """Tạo 1 frame dữ liệu chuẩn 6 sensor."""
    dists = [round(float(d), 1) for d in distances[:SENSOR_COUNT]]
    while len(dists) < SENSOR_COUNT:
        dists.append(0.0)

    if valid is None:
        val = [1 if (15.0 <= d <= 500.0) else 0 for d in dists]
    else:
        val = [int(v) for v in valid[:SENSOR_COUNT]]
        while len(val) < SENSOR_COUNT:
            val.append(0)

    # Tìm khoảng cách gần nhất trong các sensor hợp lệ
    valid_dists = [dists[i] for i in range(SENSOR_COUNT) if val[i] == 1 and dists[i] > 0]
    has_nearest = len(valid_dists) > 0
    nearest_cm = min(valid_dists) if has_nearest else 0.0

    return {
        "timestamp_ms": int(time.time() * 1000),
        "elapsed_ms": elapsed_ms,
        "distances": dists,
        "valid": val,
        "nearest_cm": round(nearest_cm, 1),
        "has_nearest": has_nearest,
        "tag": tag,
        "source": source,
    }


def parse_serial_line(line: str) -> tuple[list[float], list[int]] | None:
    """Phân tích dòng text đọc từ Serial thành (distances, valid)."""
    line = line.strip()
    if not line:
        return None

    # Trường hợp 1: Dòng chứa JSON thuần (từ CoreIoT payload hoặc custom telemetry)
    # Ví dụ: {"d1":25.4,"d2":80.0,..., "has_nearest":true}
    if line.startswith("{") and line.endswith("}"):
        try:
            data = json.loads(line)
            dists = [0.0] * SENSOR_COUNT
            val = [0] * SENSOR_COUNT
            for i in range(SENSOR_COUNT):
                key = f"d{i+1}"
                if key in data:
                    dists[i] = float(data[key])
                    val[i] = 1 if (dists[i] > 0 and dists[i] <= 500.0) else 0
            return dists, val
        except (json.JSONDecodeError, ValueError):
            pass

    # Trường hợp 2: Dòng log cảm biến [S0] Raw: 25.40 cm | Stable: 25.40 cm
    # Định dạng mảng: DIST: [25.4, 80.0, ...]
    if "DIST:" in line:
        match = re.search(r"DIST:\s*\[(.*?)\]", line)
        if match:
            try:
                raw_nums = [float(x.strip()) for x in match.group(1).split(",") if x.strip()]
                dists = raw_nums[:SENSOR_COUNT] + [0.0] * max(0, SENSOR_COUNT - len(raw_nums))
                val = [1 if (15.0 <= d <= 500.0) else 0 for d in dists]
                return dists, val
            except ValueError:
                pass

    return None


def generate_mock_stream(
    scenario: str = "approach",
    duration_sec: float = 10.0,
    interval_sec: float = DEFAULT_INTERVAL_SEC,
) -> Generator[dict[str, Any], None, None]:
    """Bộ sinh dữ liệu giả lập có biến thiên thời gian thực để test không cần board."""
    start_time = time.time()
    elapsed_ms = 0

    while True:
        now = time.time()
        elapsed_sec = now - start_time
        if duration_sec > 0 and elapsed_sec >= duration_sec:
            break

        elapsed_ms = int(elapsed_sec * 1000)

        # Kịch bản 1: approach (vật cản tiếp cận từ xa 250cm -> 20cm DANGER rồi lùi ra)
        if scenario == "approach":
            # Chu kỳ 8 giây
            phase = (elapsed_sec % 8.0) / 8.0
            # Hình sin từ 250 cm xuống 20 cm
            dist_right_front = 135.0 + 115.0 * math.cos(phase * 2 * math.pi)
            noise = (math.sin(elapsed_sec * 17) * 1.5)
            dist_right_front = max(18.0, min(260.0, dist_right_front + noise))

            distances = [350.0, 320.0, dist_right_front, 280.0, 300.0, 400.0]
            valid = [1, 1, 1, 1, 1, 1]

        # Kịch bản 2: multi_hazard (nhiều hướng cùng có nguy hiểm)
        elif scenario == "multi_hazard":
            d_front = max(20.0, 80.0 + 50.0 * math.sin(elapsed_sec * 1.2))
            d_rear = max(25.0, 60.0 + 40.0 * math.cos(elapsed_sec * 0.9))
            distances = [d_front, 150.0, 200.0, 180.0, 220.0, d_rear]
            valid = [1, 1, 1, 1, 1, 1]

        # Kịch bản 3: safe (thông thoáng)
        else:
            distances = [420.0, 400.0, 380.0, 450.0, 430.0, 410.0]
            valid = [1, 1, 1, 1, 1, 1]

        yield create_record_frame(
            distances=distances,
            valid=valid,
            elapsed_ms=elapsed_ms,
            tag=scenario,
            source="mock",
        )
        time.sleep(interval_sec)


def record_from_serial(
    port: str,
    baud: int,
    writer_cb: Callable[[dict[str, Any]], None],
    duration_sec: float = 0.0,
    tag: str = "",
) -> int:
    """Thu thập dữ liệu từ cổng Serial."""
    try:
        import serial
    except ImportError:
        print("ERROR: Chưa cài đặt pyserial. Hãy chạy: pip install pyserial", file=sys.stderr)
        return 1

    print(f"[RECORDER] Opening serial port {port} at {baud} baud...")
    try:
        ser = serial.Serial(port, baud, timeout=1.0)
    except Exception as e:
        print(f"ERROR: Không thể mở cổng serial {port}: {e}", file=sys.stderr)
        return 1

    start_time = time.time()
    frame_count = 0
    current_dists = [0.0] * SENSOR_COUNT
    current_val = [0] * SENSOR_COUNT

    try:
        while True:
            now = time.time()
            elapsed_sec = now - start_time
            if duration_sec > 0 and elapsed_sec >= duration_sec:
                break

            line = ser.readline().decode("utf-8", errors="replace").strip()
            if not line:
                continue

            parsed = parse_serial_line(line)
            if parsed:
                current_dists, current_val = parsed
                frame = create_record_frame(
                    distances=current_dists,
                    valid=current_val,
                    elapsed_ms=int(elapsed_sec * 1000),
                    tag=tag,
                    source=f"serial:{port}",
                )
                writer_cb(frame)
                frame_count += 1
                if frame_count % 10 == 0:
                    print(f"  [REC] Captured {frame_count} frames... Nearest: {frame['nearest_cm']} cm")
    finally:
        ser.close()

    return frame_count


def record_from_mqtt(
    broker: str,
    port: int,
    topic: str,
    token: str,
    writer_cb: Callable[[dict[str, Any]], None],
    duration_sec: float = 0.0,
    tag: str = "",
) -> int:
    """Thu thập dữ liệu từ CoreIoT MQTT broker."""
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        print("ERROR: Chưa cài đặt paho-mqtt. Hãy chạy: pip install paho-mqtt", file=sys.stderr)
        return 1

    start_time = time.time()
    frame_count = 0

    def on_message(client, userdata, msg):
        nonlocal frame_count
        now = time.time()
        elapsed_sec = now - start_time
        try:
            payload_str = msg.payload.decode("utf-8")
            data = json.loads(payload_str)
            dists = [float(data.get(f"d{i+1}", 0.0)) for i in range(SENSOR_COUNT)]
            val = [1 if (d > 0 and d <= 500.0) else 0 for d in dists]
            frame = create_record_frame(
                distances=dists,
                valid=val,
                elapsed_ms=int(elapsed_sec * 1000),
                tag=tag,
                source=f"mqtt:{broker}",
            )
            writer_cb(frame)
            frame_count += 1
            if frame_count % 10 == 0:
                print(f"  [REC] Captured {frame_count} MQTT frames... Nearest: {frame['nearest_cm']} cm")
        except Exception as err:
            print(f"WARN: Error parsing MQTT message: {err}", file=sys.stderr)

    client = mqtt.Client(client_id="t14_data_recorder")
    if token:
        client.username_pw_set(token, "")

    client.on_message = on_message
    print(f"[RECORDER] Connecting to MQTT broker {broker}:{port}, topic: {topic}...")
    client.connect(broker, port, keepalive=60)
    client.subscribe(topic)
    client.loop_start()

    try:
        while True:
            now = time.time()
            elapsed_sec = now - start_time
            if duration_sec > 0 and elapsed_sec >= duration_sec:
                break
            time.sleep(0.1)
    finally:
        client.loop_stop()
        client.disconnect()

    return frame_count


def main() -> int:
    parser = argparse.ArgumentParser(description="T1.4 Telemetry Data Recorder (Serial / MQTT / Mock)")
    parser.add_argument("--source", choices=["serial", "mqtt", "mock"], default="mock",
                        help="Nguồn thu dữ liệu: serial (board), mqtt (CoreIoT), mock (ảo).")
    parser.add_argument("--port", default="COM3", help="Cổng Serial (khi dùng --source serial).")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="Baud rate Serial.")
    parser.add_argument("--duration", type=float, default=10.0,
                        help="Thời gian ghi (giây). 0 = ghi liên tục tới khi bấm Ctrl+C.")
    parser.add_argument("--scenario", choices=["approach", "multi_hazard", "safe"], default="approach",
                        help="Kịch bản giả lập (khi dùng --source mock).")
    parser.add_argument("--output", "-o", type=Path, default=None,
                        help="Đường dẫn file .jsonl xuất ra. Mặc định tự sinh trong data/recordings/.")
    parser.add_argument("--tag", default="", help="Nhãn kịch bản ghi chú cho phiên đo.")
    parser.add_argument("--keys", type=Path, default=ROOT / "config" / "keys.json",
                        help="File keys.json để lấy token MQTT (khi dùng --source mqtt).")

    args = parser.parse_args()

    # Xác định đường dẫn file xuất
    if args.output is None:
        timestamp_str = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        tag_suffix = f"_{args.tag}" if args.tag else f"_{args.scenario}" if args.source == "mock" else ""
        out_dir = ROOT / "data" / "recordings"
        out_dir.mkdir(parents=True, exist_ok=True)
        out_file = out_dir / f"rec_{args.source}_{timestamp_str}{tag_suffix}.jsonl"
    else:
        out_file = args.output
        out_file.parent.mkdir(parents=True, exist_ok=True)

    print("==================================================")
    print("      T1.4 SENSOR DATA RECORDER — VEHICLE V2     ")
    print("==================================================")
    print(f"Source   : {args.source}")
    print(f"Output   : {out_file}")
    print(f"Duration : {args.duration}s (0 = vô tận, Ctrl+C để dừng)")
    print("==================================================")

    records_written = 0
    with open(out_file, "w", encoding="utf-8") as f:
        def write_frame(frame: dict[str, Any]):
            nonlocal records_written
            f.write(json.dumps(frame, ensure_ascii=False) + "\n")
            f.flush()
            records_written += 1

        try:
            if args.source == "mock":
                print(f"[RECORDER] Running mock generator scenario: {args.scenario}...")
                for frame in generate_mock_stream(scenario=args.scenario, duration_sec=args.duration):
                    write_frame(frame)
                    print(f"  Frame {records_written}: elapsed={frame['elapsed_ms']}ms "
                          f"nearest={frame['nearest_cm']}cm distances={frame['distances']}")

            elif args.source == "serial":
                records_written = record_from_serial(
                    port=args.port,
                    baud=args.baud,
                    writer_cb=write_frame,
                    duration_sec=args.duration,
                    tag=args.tag,
                )

            elif args.source == "mqtt":
                broker = "app.coreiot.io"
                port = 1883
                topic = "v1/devices/me/telemetry"
                token = ""
                if args.keys.exists():
                    try:
                        kdata = json.loads(args.keys.read_text(encoding="utf-8"))
                        broker = kdata.get("COREIOT_BROKER", broker)
                        port = int(kdata.get("COREIOT_PORT", port))
                        token = kdata.get("SENSOR_NODE_DEVICE_TOKEN", "")
                    except Exception as err:
                        print(f"WARN: Không đọc được keys.json: {err}", file=sys.stderr)

                records_written = record_from_mqtt(
                    broker=broker,
                    port=port,
                    topic=topic,
                    token=token,
                    writer_cb=write_frame,
                    duration_sec=args.duration,
                    tag=args.tag,
                )

        except KeyboardInterrupt:
            print("\n[RECORDER] Bị ngắt bởi người dùng (Ctrl+C). Đang lưu file...")

    print("==================================================")
    print(f"Hoàn thành! Đã ghi {records_written} frames vào: {out_file}")
    print("==================================================")
    return 0


if __name__ == "__main__":
    sys.exit(main())
