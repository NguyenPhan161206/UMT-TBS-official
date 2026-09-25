"""test_recorder_replayer.py — Unit tests cho T1.4 Data Recorder & Replayer.

Chạy với:
  $env:PYTHONUTF8="1"; $env:PYTHONIOENCODING="utf-8"; python -m pytest tools/recorder/test_recorder_replayer.py -v
"""
from __future__ import annotations

import json
import socket
import sys
import tempfile
import time
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "recorder"))

from data_recorder import (
    create_record_frame,
    parse_serial_line,
    generate_mock_stream,
    SENSOR_COUNT,
)
from data_replayer import (
    replay_file,
    format_zone_str,
    UdpSender,
    CAUTION_CM,
    DANGER_CM,
)


def test_create_record_frame_padding_and_nearest():
    # Cung cấp 3 cảm biến, phải tự pad đủ 6
    frame = create_record_frame([25.4, 120.0, 300.0], elapsed_ms=100, tag="test")
    assert len(frame["distances"]) == SENSOR_COUNT
    assert len(frame["valid"]) == SENSOR_COUNT
    assert frame["distances"][0] == 25.4
    assert frame["distances"][1] == 120.0
    assert frame["distances"][2] == 300.0
    assert frame["distances"][3] == 0.0
    assert frame["valid"][0] == 1
    assert frame["valid"][1] == 1
    assert frame["valid"][2] == 1
    assert frame["valid"][3] == 0
    assert frame["nearest_cm"] == 25.4
    assert frame["has_nearest"] is True
    assert frame["elapsed_ms"] == 100
    assert frame["tag"] == "test"


def test_create_record_frame_no_valid_sensors():
    # Tất cả đều 0 hoặc ngoài dải (15-500cm)
    frame = create_record_frame([0.0, 5.0, 600.0, 0.0, 0.0, 0.0])
    assert frame["has_nearest"] is False
    assert frame["nearest_cm"] == 0.0
    assert all(v == 0 for v in frame["valid"])


def test_parse_serial_line_json():
    line = '{"d1":35.2,"d2":88.0,"d3":150.5,"d4":0,"d5":0,"d6":220.0,"nearest_cm":35.2}'
    parsed = parse_serial_line(line)
    assert parsed is not None
    dists, val = parsed
    assert dists[0] == 35.2
    assert dists[1] == 88.0
    assert dists[2] == 150.5
    assert dists[5] == 220.0
    assert val[0] == 1
    assert val[3] == 0


def test_parse_serial_line_dist_prefix():
    line = "LOG: DIST: [20.5, 45.0, 95.0, 150.0, 200.0, 350.0] OK"
    parsed = parse_serial_line(line)
    assert parsed is not None
    dists, val = parsed
    assert len(dists) == SENSOR_COUNT
    assert dists[0] == 20.5
    assert dists[1] == 45.0
    assert all(v == 1 for v in val)


def test_parse_serial_line_invalid():
    assert parse_serial_line("") is None
    assert parse_serial_line("Random debug log without numbers") is None
    assert parse_serial_line("{invalid json syntax") is None


def test_mock_stream_generation():
    frames = list(generate_mock_stream(scenario="approach", duration_sec=0.5, interval_sec=0.1))
    # Trong 0.5s với interval 0.1s phải sinh khoảng 5 frames
    assert len(frames) >= 4
    for f in frames:
        assert len(f["distances"]) == SENSOR_COUNT
        assert f["tag"] == "approach"
        assert f["source"] == "mock"


def test_format_zone_str():
    s_safe = format_zone_str(200.0, 1)
    assert "SAFE" in s_safe
    s_caution = format_zone_str(60.0, 1)
    assert "CAUTION" in s_caution
    s_danger = format_zone_str(25.0, 1)
    assert "DANGER" in s_danger
    s_nodata = format_zone_str(0.0, 0)
    assert "NO DATA" in s_nodata


def test_end_to_end_record_and_replay_udp(tmp_path):
    # 1. Ghi một file .jsonl giả lập
    test_jsonl = tmp_path / "test_session.jsonl"
    frames_written = 5
    with open(test_jsonl, "w", encoding="utf-8") as f:
        for i in range(frames_written):
            frame = create_record_frame(
                distances=[50.0 + i * 10, 150.0, 200.0, 300.0, 400.0, 450.0],
                elapsed_ms=i * 50,
                tag="test_udp",
            )
            f.write(json.dumps(frame) + "\n")

    # 2. Tạo một UDP server socket để đón gói
    udp_port = 19095  # dùng port riêng cho test
    receiver_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    receiver_sock.bind(("127.0.0.1", udp_port))
    receiver_sock.settimeout(2.0)

    try:
        # 3. Chạy replay_file bắn vào UDP
        sent = replay_file(
            file_path=test_jsonl,
            target="udp",
            speed=50.0,
            udp_host="127.0.0.1",
            udp_port=udp_port,
        )
        assert sent == frames_written

        # 4. Nhận và verify gói đầu tiên
        data, addr = receiver_sock.recvfrom(4096)
        recv_frame = json.loads(data.decode("utf-8"))
        assert recv_frame["tag"] == "test_udp"
        assert recv_frame["distances"][0] == 50.0
        assert recv_frame["has_nearest"] is True
    finally:
        receiver_sock.close()
