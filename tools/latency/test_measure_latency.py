"""Unit test cho tools/latency/measure_latency.py — chạy không cần board.

    python -m pytest tools/latency -q
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import measure_latency as ml  # noqa: E402
from measure_latency import Event  # noqa: E402

MS = 1_000_000  # ns


def test_parse_sensor_lines():
    assert ml.parse_line("LAT TX seq=12 age_min_ms=3 age_max_ms=-1") == (
        "TX", 12, {"age_min_ms": 3, "age_max_ms": -1})
    assert ml.parse_line("LAT RTT seq=12 us=2450") == ("RTT", 12, {"us": 2450})
    assert ml.parse_line("LAT MQTT_TX seq=7") == ("MQTT_TX", 7, {})


def test_parse_screen_esp_log_with_ansi():
    assert ml.parse_line("\x1b[0;32mI (123456) LAT: RX seq=65535\x1b[0m") == ("RX", 65535, {})
    assert ml.parse_line("I (99) LAT: MQTT_RX seq=7") == ("MQTT_RX", 7, {})


def test_parse_ignores_other_lines():
    assert ml.parse_line("DIST: [12.0, 0.0, 0.0, 0.0, 0.0, 0.0]") is None
    assert ml.parse_line("I (10) coreiot: MQTT DATA received ... \"seq\":5") is None


def test_unwrap_seq16():
    assert ml.unwrap_seq16([65534, 65535, 0, 1]) == [65534, 65535, 65536, 65537]
    assert ml.unwrap_seq16([5, 6, 6, 7]) == [5, 6, 6, 7]


def test_percentile_matches_linear_interpolation():
    v = [1.0, 2.0, 3.0, 4.0]
    assert ml.percentile(v, 50) == 2.5
    assert ml.percentile(v, 0) == 1.0
    assert ml.percentile(v, 100) == 4.0


def test_analyze_espnow_and_mqtt():
    ev = []
    for s in range(1, 11):  # 10 gói, gói 5 mất phía màn hình, gói 6 mất echo
        t = s * 100 * MS
        ev.append(Event(t, "sensor", "TX", s, {"age_min_ms": 10, "age_max_ms": 50}))
        if s != 5:
            ev.append(Event(t + 3 * MS, "screen", "RX", s, {}))
        if s not in (5, 6):
            ev.append(Event(t + 5 * MS, "sensor", "RTT", s, {"us": 4000}))
    # MQTT: seq 1 nhận sau 200 ms; seq 2 chỉ có bản RX cũ TRƯỚC TX (bỏ) -> coi như mất
    ev += [Event(0, "sensor", "MQTT_TX", 1, {}), Event(200 * MS, "screen", "MQTT_RX", 1, {}),
           Event(50 * MS, "screen", "MQTT_RX", 2, {}), Event(60 * MS, "sensor", "MQTT_TX", 2, {})]

    res = ml.analyze(ev)
    e = res["espnow"]
    assert e["tx"] == 10 and e["rx_screen"] == 9 and e["echo_back"] == 8
    assert abs(e["delivery_pct"] - 90.0) < 1e-9
    assert e["one_way_rtt_half_ms"]["p50"] == 2.0
    assert e["one_way_pc_ms"]["p50"] == 3.0
    assert res["sample_age"]["age_max_ms"]["max"] == 50
    m = res["mqtt"]
    assert m["tx"] == 2 and m["rx_screen"] == 1
    assert m["one_way_pc_ms"]["p50"] == 200.0
    assert "ESP-NOW một chiều (RTT/2)" in ml.format_report(res)


def test_analyze_empty_reports_hint():
    assert "env *_latency" in ml.format_report(ml.analyze([]))


def test_csv_roundtrip(tmp_path):
    path = tmp_path / "run.csv"
    rows = [[1000, "sensor", "RTT", 3, '{"us": 1800}', "LAT RTT seq=3 us=1800"]]
    import csv
    with path.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(ml.CSV_HEADER)
        w.writerows(rows)
    (ev,) = ml.load_csv(path)
    assert ev.kind == "RTT" and ev.seq == 3 and ev.fields == {"us": 1800}
