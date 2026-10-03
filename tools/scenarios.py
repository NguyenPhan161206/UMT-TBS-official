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

# 4 kịch bản gốc đứng đầu (không đổi), tiếp theo là các tình huống quanh xe tải.
# Tên phải là định danh C hợp lệ (host_sim sinh k_sim_<tên>).
SCENARIO_NAMES: Tuple[str, ...] = (
    "approach", "crossing", "slam", "normal",
    "overtake_right", "overtake_left", "reverse_wall", "reverse_pedestrian",
    "pedestrian_front", "crossing_right", "narrow_lane", "boxed_in",
    "threshold_flap", "fast_pass", "stop_and_go",
)

CLEAR = 200.0  # cm — "thoáng": cao hơn ngưỡng CAUTION nhưng vẫn trong dải đo


def _row(front: float = CLEAR, rear: float = CLEAR, lf: float = CLEAR,
         lr: float = CLEAR, rf: float = CLEAR, rr: float = CLEAR) -> Tuple[float, ...]:
    """Một mốc theo thứ tự slot d1..d6; slot không nêu = thoáng (CLEAR)."""
    return (float(front), float(rear), float(lf), float(lr), float(rf), float(rr))


# boxed_in: mỗi slot đi theo cùng đường cong nhưng trễ nhịp khác nhau (không đồng bộ tuyệt đối).
_BOX_RAMP = (200.0, 150.0, 110.0, 90.0, 70.0, 55.0, 40.0, 28.0, 24.0, 22.0, 22.0, 22.0)
_BOX_LAG = (0, 1, 1, 2, 1, 2)

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
    # overtake_right: xe vượt bên PHẢI từ sau ra trước (~50 cm bên hông): RIGHT_REAR (d6) rồi RIGHT_FRONT (d5).
    "overtake_right": (
        _row(), _row(rr=180), _row(rr=110), _row(rr=60), _row(rr=50, rf=150), _row(rr=70, rf=80),
        _row(rr=130, rf=55), _row(rf=50), _row(rf=75), _row(rf=140), _row(rf=190), _row(),
    ),
    # overtake_left: đối xứng bên TRÁI: LEFT_REAR (d4) rồi LEFT_FRONT (d3).
    "overtake_left": (
        _row(), _row(lr=180), _row(lr=110), _row(lr=60), _row(lr=50, lf=150), _row(lr=70, lf=80),
        _row(lr=130, lf=55), _row(lf=50), _row(lf=75), _row(lf=140), _row(lf=190), _row(),
    ),
    # reverse_wall: lùi vào tường — REAR (d2) giảm đều từ 300 xuống DANGER rồi đứng yên.
    "reverse_wall": tuple(
        _row(rear=v) for v in (300, 260, 220, 180, 140, 110, 90, 70, 55, 40, 28, 22, 22)
    ),
    # reverse_pedestrian: người đi bộ phía sau xe — vào CAUTION (min 45 cm, không tới DANGER) rồi đi ra.
    "reverse_pedestrian": tuple(
        _row(rear=v) for v in (350, 300, 250, 200, 150, 110, 80, 60, 45, 45, 60, 100, 160, 220)
    ),
    # pedestrian_front: người đi bộ tiến vào trước xe rồi lách sang trái: FRONT (d1) rồi LEFT_FRONT (d3).
    "pedestrian_front": (
        _row(front=300), _row(front=250), _row(front=200), _row(front=150), _row(front=110),
        _row(front=80), _row(front=65), _row(front=90, lf=120), _row(front=140, lf=80),
        _row(lf=60), _row(lf=55), _row(lf=90), _row(lf=150), _row(),
    ),
    # crossing_right: xe đạp cắt ngang trước xe từ phải sang trái: RIGHT_FRONT -> FRONT -> LEFT_FRONT.
    "crossing_right": (
        _row(), _row(rf=150), _row(rf=70), _row(rf=45, front=160), _row(rf=120, front=60),
        _row(front=45, lf=170), _row(front=110, lf=70), _row(lf=45), _row(lf=110), _row(lf=190), _row(),
    ),
    # narrow_lane: chạy giữa hai hàng xe/tường: 4 slot bên luôn 50–95 cm (CAUTION kéo dài), trước/sau thoáng.
    "narrow_lane": tuple(
        _row(lf=a, lr=b, rf=c, rr=d)
        for a, b, c, d in (
            (80, 85, 90, 95), (70, 75, 80, 85), (60, 65, 70, 75), (55, 60, 60, 65), (50, 55, 50, 55),
            (55, 60, 55, 60), (60, 65, 65, 70), (65, 70, 75, 80), (70, 75, 85, 90), (80, 85, 90, 95),
        )
    ),
    # boxed_in: bị vây 6 phía — mọi slot giảm dần từ thoáng xuống DANGER (báo động toàn bộ).
    "boxed_in": tuple(
        tuple(_BOX_RAMP[max(t - lag, 0)] for lag in _BOX_LAG) for t in range(len(_BOX_RAMP))
    ),
    # threshold_flap: dao động sát ngưỡng — FRONT quanh CAUTION (100 cm), LEFT_FRONT quanh DANGER (30 cm).
    "threshold_flap": tuple(
        _row(front=f, lf=l)
        for f, l in (
            (104, 34), (97, 27), (103, 33), (96, 26), (105, 35),
            (98, 28), (102, 32), (95, 25), (101, 31), (99, 29),
        )
    ),
    # fast_pass: xe máy vọt qua bên trái-trước (~10 m/s): thoáng -> 40 cm -> thoáng chỉ trong vài mốc.
    "fast_pass": (
        _row(), _row(), _row(lf=95), _row(lf=40), _row(lf=130), _row(), _row(),
    ),
    # stop_and_go: vật đứng yên ở CAUTION (~80 cm) 6 mốc rồi rời đi; sau đó vật thứ hai đứng ở DANGER (~25 cm) 4 mốc.
    "stop_and_go": tuple(
        _row(front=v)
        for v in (200, 200, 80, 78, 81, 79, 80, 82, 200, 200, 25, 24, 26, 25, 200, 200)
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