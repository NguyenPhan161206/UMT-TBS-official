#!/usr/bin/env python3
"""fetch_telemetry_api.py — Kéo dữ liệu telemetry từ CoreIoT qua REST API (không cần dây).

Sử dụng ThingsBoard REST API để tải lịch sử đo đạc của cảm biến và lưu thành file .jsonl,
sẵn sàng để data_replayer.py phát lại trên Console hoặc LVGL Simulator.

Tuân thủ R1: Secret đọc từ config/keys.json hoặc CLI params, không hardcode.
Tuân thủ R7: Độ dài file <= 400 dòng.
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from collections import defaultdict
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SENSOR_COUNT = 6
SENSOR_KEYS = ["d1", "d2", "d3", "d4", "d5", "d6"]
DEFAULT_KEYS = ",".join(SENSOR_KEYS + ["nearest_cm", "has_nearest"])

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")


def load_config_keys() -> dict[str, Any]:
    """Đọc config/keys.json nếu có."""
    path = ROOT / "config" / "keys.json"
    if path.exists():
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except Exception:
            pass
    return {}


def api_request(url: str, method: str = "GET", data: dict | None = None, token: str | None = None) -> Any:
    """Thực hiện HTTP request tới ThingsBoard / CoreIoT REST API."""
    headers = {"Accept": "application/json"}
    body = None
    if data is not None:
        headers["Content-Type"] = "application/json"
        body = json.dumps(data).encode("utf-8")

    if token:
        headers["X-Authorization"] = f"Bearer {token}"

    req = urllib.request.Request(url, data=body, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            content = resp.read().decode("utf-8")
            if content:
                return json.loads(content)
            return {}
    except urllib.error.HTTPError as err:
        err_msg = err.read().decode("utf-8", errors="ignore")
        raise RuntimeError(f"HTTP {err.code}: {err.reason} - {err_msg}") from err
    except Exception as err:
        raise RuntimeError(f"Kết nối thất bại: {err}") from err


def login_coreiot(base_url: str, username: str, password: str) -> str:
    """Đăng nhập CoreIoT lấy JWT token."""
    login_url = f"{base_url.rstrip('/')}/api/auth/login"
    payload = {"username": username, "password": password}
    resp = api_request(login_url, method="POST", data=payload)
    token = resp.get("token")
    if not token:
        raise ValueError("Không nhận được token từ CoreIoT login API.")
    return token


def find_device_id(base_url: str, token: str, device_name: str = "") -> str:
    """Tìm Device ID (UUID) theo tên thiết bị hoặc lấy thiết bị đầu tiên."""
    url = f"{base_url.rstrip('/')}/api/tenant/devices?pageSize=50&page=0"
    resp = api_request(url, token=token)
    devices = resp.get("data", [])
    if not devices:
        raise ValueError("Tài khoản không có thiết bị nào trong danh sách Devices.")

    if device_name:
        for dev in devices:
            if dev.get("name", "").lower() == device_name.lower():
                return dev["id"]["id"]

    # Tìm thiết bị có tên chứa 'sensor'
    for dev in devices:
        if "sensor" in dev.get("name", "").lower():
            return dev["id"]["id"]

    # Mặc định lấy thiết bị đầu tiên
    first = devices[0]
    print(f"[*] Tự động chọn thiết bị: '{first.get('name')}' (ID: {first['id']['id']})")
    return first["id"]["id"]


def fetch_timeseries(
    base_url: str,
    token: str,
    device_id: str,
    keys: str = DEFAULT_KEYS,
    limit: int = 500,
    start_ts: int = 0,
    end_ts: int = 0,
) -> dict[str, list[dict[str, Any]]]:
    """Tải lịch sử timeseries telemetry của thiết bị."""
    params = {"keys": keys, "limit": str(limit)}
    if start_ts > 0:
        params["startTs"] = str(start_ts)
    if end_ts > 0:
        params["endTs"] = str(end_ts)

    qs = urllib.parse.urlencode(params)
    url = f"{base_url.rstrip('/')}/api/plugins/telemetry/DEVICE/{device_id}/values/timeseries?{qs}"
    return api_request(url, token=token)


def convert_telemetry_to_jsonl(raw_telemetry: dict[str, list[dict[str, Any]]], output_file: Path) -> int:
    """Chuyển đổi dữ liệu timeseries từ CoreIoT sang format .jsonl chuẩn."""
    # Gom dữ liệu theo timestamp (ms)
    frames_by_ts: dict[int, dict[str, Any]] = defaultdict(dict)

    for key, entries in raw_telemetry.items():
        for entry in entries:
            ts = int(entry.get("ts", 0))
            val_str = entry.get("value")
            try:
                if key in SENSOR_KEYS or key == "nearest_cm":
                    frames_by_ts[ts][key] = float(val_str)
                elif key == "has_nearest":
                    frames_by_ts[ts][key] = str(val_str).lower() in ("true", "1")
                else:
                    frames_by_ts[ts][key] = val_str
            except (ValueError, TypeError):
                frames_by_ts[ts][key] = 0.0

    if not frames_by_ts:
        return 0

    # Sắp xếp theo thứ tự thời gian tăng dần
    sorted_ts = sorted(frames_by_ts.keys())
    first_ts = sorted_ts[0]

    output_file.parent.mkdir(parents=True, exist_ok=True)
    count = 0

    with open(output_file, "w", encoding="utf-8") as f:
        for ts in sorted_ts:
            fields = frames_by_ts[ts]
            dists = [round(fields.get(f"d{i+1}", 0.0), 1) for i in range(SENSOR_COUNT)]
            valid = [1 if (15.0 <= d <= 500.0) else 0 for d in dists]

            # Tính nearest nếu chưa có
            valid_dists = [dists[i] for i in range(SENSOR_COUNT) if valid[i] == 1 and dists[i] > 0]
            has_nearest = len(valid_dists) > 0
            nearest_cm = min(valid_dists) if has_nearest else 0.0
            if "nearest_cm" in fields and fields["nearest_cm"] > 0:
                nearest_cm = fields["nearest_cm"]
                has_nearest = True

            frame = {
                "timestamp_ms": ts,
                "elapsed_ms": ts - first_ts,
                "distances": dists,
                "valid": valid,
                "nearest_cm": round(nearest_cm, 1),
                "has_nearest": has_nearest,
                "tag": "coreiot_rest",
                "source": "rest:app.coreiot.io",
            }
            f.write(json.dumps(frame) + "\n")
            count += 1

    return count


def parse_args() -> argparse.Namespace:
    cfg = load_config_keys()
    parser = argparse.ArgumentParser(
        description="Tải dữ liệu Telemetry từ CoreIoT qua REST API (không cần dây)"
    )
    parser.add_argument(
        "--url", default="https://app.coreiot.io",
        help="CoreIoT API Host (mặc định: https://app.coreiot.io)"
    )
    parser.add_argument(
        "-u", "--user", default=cfg.get("COREIOT_USERNAME"),
        help="Email tài khoản CoreIoT (hoặc điền vào config/keys.json)"
    )
    parser.add_argument(
        "-p", "--password", default=cfg.get("COREIOT_PASSWORD"),
        help="Mật khẩu tài khoản CoreIoT"
    )
    parser.add_argument(
        "--jwt", default=cfg.get("COREIOT_JWT"),
        help="JWT token đăng nhập (nếu đã có, không cần user/password)"
    )
    parser.add_argument(
        "--device-name", default=cfg.get("COREIOT_DEVICE_NAME", "sensor-node"),
        help="Tên thiết bị trên CoreIoT (mặc định: sensor-node)"
    )
    parser.add_argument(
        "--device-id", default=cfg.get("COREIOT_DEVICE_ID"),
        help="Device UUID trực tiếp (nếu có)"
    )
    parser.add_argument(
        "--limit", type=int, default=500,
        help="Số lượng bản ghi tối đa cần tải (mặc định: 500)"
    )
    parser.add_argument(
        "-s", "--seconds", type=float, default=None,
        help="Lấy dữ liệu trong N giây vừa qua tính từ thời điểm gõ lệnh (ví dụ: --seconds 30)"
    )
    parser.add_argument(
        "--live", type=float, default=None,
        help="Ghi dữ liệu LIVE trong N giây TỚI tính từ lúc bấm Enter (ví dụ: --live 30)"
    )
    parser.add_argument(
        "--hours", type=float, default=None,
        help="Khoảng thời gian cần kéo dữ liệu tính bằng giờ (mặc định: 24 giờ qua nếu không dùng --seconds/--live)"
    )
    parser.add_argument(
        "-o", "--output", default="data/recordings/coreiot_download.jsonl",
        help="Đường dẫn file .jsonl lưu dữ liệu (mặc định: data/recordings/coreiot_download.jsonl)"
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    print("=" * 55)
    print("   COREIOT REST API TELEMETRY DOWNLOADER (WIRELESS)")
    print("=" * 55)

    token = args.jwt
    if not token:
        if not args.user or not args.password:
            print("[LỖI] Cần tài khoản CoreIoT để tải dữ liệu không dây!", file=sys.stderr)
            print("Cách 1: Truyền tham số: --user <email> --password <pass>", file=sys.stderr)
            print("Cách 2: Hoặc điền 'COREIOT_USERNAME' và 'COREIOT_PASSWORD' vào config/keys.json", file=sys.stderr)
            return 1
        print(f"[*] Đang đăng nhập tài khoản '{args.user}' trên {args.url}...")
        try:
            token = login_coreiot(args.url, args.user, args.password)
            print("[+] Đăng nhập thành công! Đã lấy được JWT session token.")
        except Exception as err:
            print(f"[LỖI] Đăng nhập thất bại: {err}", file=sys.stderr)
            return 1

    device_id = args.device_id
    if not device_id:
        print(f"[*] Đang tìm thiết bị '{args.device_name}'...")
        try:
            device_id = find_device_id(args.url, token, args.device_name)
            print(f"[+] Tìm thấy Device ID: {device_id}")
        except Exception as err:
            print(f"[LỖI] Không tìm thấy thiết bị: {err}", file=sys.stderr)
            return 1

    limit = args.limit
    if args.live is not None and args.live > 0:
        start_ts = int(time.time() * 1000)
        print(f"[*] Bắt đầu ghi LIVE trong {args.live:.0f} giây tính từ bây giờ...")
        total_sec = int(args.live)
        for rem in range(total_sec, 0, -1):
            print(f"\r  -> Đang theo dõi thiết bị... còn {rem}s ", end="", flush=True)
            time.sleep(1.0)
        time.sleep(0.5)
        now_ts = int(time.time() * 1000)
        print(f"\r[+] Hoàn thành đợt ghi LIVE {args.live:.0f}s! Đang kéo dữ liệu về...          ")
    elif args.seconds is not None and args.seconds > 0:
        now_ts = int(time.time() * 1000)
        start_ts = now_ts - int(args.seconds * 1000)
        print(f"[*] Đang tải dữ liệu trong {args.seconds:.0f} giây vừa qua...")
    else:
        hours = args.hours if args.hours is not None else 24.0
        now_ts = int(time.time() * 1000)
        start_ts = now_ts - int(hours * 3600 * 1000)
        print(f"[*] Đang tải tối đa {limit} bản ghi trong {hours} giờ qua...")

    try:
        raw_telemetry = fetch_timeseries(
            args.url, token, device_id, limit=limit, start_ts=start_ts, end_ts=now_ts
        )
    except Exception as err:
        print(f"[LỖI] Tải dữ liệu thất bại: {err}", file=sys.stderr)
        return 1

    out_path = Path(args.output)
    frames_count = convert_telemetry_to_jsonl(raw_telemetry, out_path)

    print("=" * 55)
    if frames_count > 0:
        print(f"[THÀNH CÔNG] Đã lưu {frames_count} frames vào: {out_path}")
        print("\nĐể xem lại dữ liệu dạng radar trực quan:")
        print(f"  python tools/recorder/data_replayer.py --in {out_path} --target console")
    else:
        print(f"[THÔNG BÁO] Không có dữ liệu telemetry nào được tìm thấy trên CoreIoT.")
        print("Hãy chắc chắn Sensor Node đã được cấp nguồn và kết nối Wi-Fi thành công.")
    print("=" * 55)
    return 0


if __name__ == "__main__":
    sys.exit(main())
