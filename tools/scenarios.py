#!/usr/bin/env python3
"""scenarios.py — NGUỒN DUY NHẤT định nghĩa kịch bản khoảng cách (G1 T1.1).

Một spec `t → distances[6] (cm)`; 3 công cụ tiêu thụ chung (không duplicate):

- T1.1: `test_mqtt_coreiot.py --scenario X` duyệt chuỗi theo `--interval`.
- T1.4: `record_telemetry.py` / `replay_telemetry.py` (JSONL).
- T1.2: `host_sim --scenario X`.

Thứ tự slot == ESP-NOW wire order (firmware/shared/espnow_protocol.h):
    d1=FRONT, d2=REAR, d3=LEFT_FRONT, d4=LEFT_REAR, d5=RIGHT_FRONT, d6=RIGHT_REAR
Kịch bản được kiểm ngữ nghĩa trong tools/guard/test_guard.py (R10).
Python mirror ngưỡng (CAUTION_CM=100 / DANGER_CM=30) nằm ở
tools/test_mqtt_coreiot.py (arch_guard B5 đối chiếu với thresholds.h).
"""
from __future__ import annotations

from typing import Iterator, Tuple

SCENARIO_NAMES: Tuple[str, ...] = ("approach", "crossing", "slam", "normal")

# Mỗi mốc = tuple 6 số [d1, d2, d3, d4, d5, d6] cm.
all_scenarios: dict[str, Tuple[Tuple[float, ...], ...]] = {
    # approach: vật tiến gần phía FRONT (d1) — mốc đầu > 150 → mốc cuối < 30.
    "approach": (
        (160.0, 80.0, 120.0, 90.0, 100.0, 110.0),
        (140.0, 80.0, 120.0, 90.0, 100.0, 110.0),
        (120.0, 80.0, 120.0, 90.0, 100.0, 110.0),
        (100.0, 80.0, 120.0, 90.0, 100.0, 110.0),
        (75.0, 80.0, 120.0, 90.0, 100.0, 110.0),
        (55.0, 80.0, 120.0, 90.0, 100.0, 110.0),
        (35.0, 80.0, 120.0, 90.0, 100.0, 110.0),
        (20.0, 80.0, 120.0, 90.0, 100.0, 110.0),
    ),
    # crossing: vật cắt ngang phía trước — LEFT_FRONT (d3) đổi nhanh (delta ≥ 40).
    "crossing": (
        (120.0, 120.0, 120.0, 120.0, 120.0, 120.0),
        (120.0, 120.0, 80.0, 120.0, 120.0, 120.0),
        (120.0, 120.0, 40.0, 120.0, 120.0, 120.0),
        (120.0, 120.0, 150.0, 120.0, 120.0, 120.0),
        (120.0, 120.0, 150.0, 120.0, 120.0, 120.0),
    ),
    # slam: dừng gấp — FRONT từ > 100 xuống < 30 trong ≤ 3 mốc (delta tức thời lớn).
    "slam": (
        (110.0, 110.0, 110.0, 110.0, 110.0, 110.0),
        (60.0, 110.0, 110.0, 110.0, 110.0, 110.0),
        (20.0, 110.0, 110.0, 110.0, 110.0, 110.0),
        (20.0, 110.0, 110.0, 110.0, 110.0, 110.0),
        (20.0, 110.0, 110.0, 110.0, 110.0, 110.0),
    ),
    # normal: không có cảnh báo — mọi slot > 100 (không mốc ≤ CAUTION_CM).
    "normal": (
        (160.0, 150.0, 140.0, 130.0, 150.0, 160.0),
        (165.0, 150.0, 140.0, 130.0, 150.0, 160.0),
        (160.0, 145.0, 145.0, 135.0, 150.0, 160.0),
        (170.0, 150.0, 140.0, 130.0, 155.0, 160.0),
        (160.0, 150.0, 140.0, 130.0, 150.0, 165.0),
    ),
}


def iter_scenario(name: str) -> Iterator[Tuple[float, ...]]:
    """Yield từng mốc của kịch bản theo thứ tự thời gian. KeyError nếu tên lạ."""
    try:
        timeline = all_scenarios[name]
    except KeyError:
        raise KeyError(f"Không có kịch bản '{name}'. Có: {list(all_scenarios)}")
    yield from timeline


if __name__ == "__main__":
    for name in SCENARIO_NAMES:
        steps = list(iter_scenario(name))
        print(f"{name}: {len(steps)} mốc, mốc đầu={steps[0]}, mốc cuối={steps[-1]}")