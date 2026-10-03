# TOOLS_SCENARIOS_LOG — G1 T1.1 scenarios.py + `--scenario` (schema V2)

Ngày: 2026-09-09
Nhánh: `nguyen` (cam kết sẽ push origin/nguyen)
Người thực thi: dev-orchestrator (step 2 roadmap next-branch)

## Mục tiêu
- Tạo `tools/scenarios.py` — nguồn duy nhất định nghĩa 4 kịch bản `t → distances[6] cm`
  (`approach/crossing/slam/normal`) để 3 công cụ T1.1/T1.2/T1.4 không duplicate dữ liệu.
- Pin JSONL schema = payload V2 telemetry (`d1..d6`, `nearest_cm`, `has_nearest`,
  `timestamp`, `seq`) để `record_telemetry.py`/`replay_telemetry.py`/`host_sim` khớp nhau.
- `test_mqtt_coreiot.py` thêm `--scenario <name>` duyệt timeline theo `--interval`, giữ
  nguyên hành vi `--distance`/`--loop`/`--dry-run` cũ.

## File đã tạo / sửa
- `tools/scenarios.py` — **mới**: `SCENARIO_NAMES`, `all_scenarios`, `iter_scenario(name)`.
- `tools/test_mqtt_coreiot.py` — sửa: import scenarios qua `TOOLS_DIR` sys.path; thêm
  `--scenario` (choices=SCENARIO_NAMES); thêm `build_payload_from_distances(distances, seq)` +
  `iter_scenario_rows(args, one_round_only)`; nhánh scenario trong `run_publisher` và `main()`
  (dry-run in từng mốc JSON 1 dòng).
- `tools/guard/test_guard.py` — sửa: +7 test (4 tên timeline, semantics 4 kịch bản,
  KeyError, build_payload_from_distances, dry-run timeline, dry-run + distance override).

## Kết quả kiểm thử (verify DoD)
1. `python3 tools/test_mqtt_coreiot.py --scenario approach --dry-run --distance 30`
   → exit 0, payload đủ key `d1..d6/nearest_cm/has_nearest`, mọi slot = 30.0, DANGER.
2. `python3 tools/test_mqtt_coreiot.py --scenario approach --dry-run`
   → exit 0, 8 mốc, d1 giảm 160 → 20.
3. `pytest tools/guard/test_guard.py -q` → **25 passed** (19 cũ + 6 mới).
4. `python3 tools/guard/scan_secrets.py` → **SECRET-SCAN OK**.
5. `python3 tools/guard/arch_guard.py` → **ARCH-GUARD OK** (ngưỡng 100/30 giữ nguyên — B5).

## Cách dùng
```bash
# Bảng / kịch bản trước khi gửi thật
python3 tools/test_mqtt_coreiot.py --scenario approach --dry-run
python3 tools/test_mqtt_coreiot.py --scenario crossing --dry-run --distance 30
# Gửi MQTT thật (cần token trong config/keys.json)
python3 tools/test_mqtt_coreiot.py --scenario slam --loop --interval 1
```
- `--distance` override mọi slot; `--loop` lặp timeline vô hạn (seq tăng).
- Kịch bản thêm mới = sửa `tools/scenarios.py` (thêm tên vào `SCENARIO_NAMES` + timeline).

---

# BỔ SUNG 2026-10-03 — thêm 11 kịch bản (tổng 15), `umt_dash_sim --list`

Roadmap: `docs/roadmaps/more-scenarios.roadmap.json` (4 bước). Yêu cầu: "muốn có thêm nhiều kịch bản".

## Kịch bản mới (11) — nguồn duy nhất vẫn là `tools/scenarios.py`
`overtake_right`, `overtake_left` (xe vượt hai bên từ sau ra trước), `reverse_wall`, `reverse_pedestrian` (lùi vào tường / người đi bộ sau xe),
`pedestrian_front`, `crossing_right` (người/xe đạp qua trước xe), `narrow_lane` (kẹp giữa hai hàng xe), `boxed_in` (bị vây 6 phía),
`threshold_flap` (dao động sát ngưỡng 100/30 cm), `fast_pass` (xe máy vọt qua), `stop_and_go` (vật đứng yên rồi rời, vật thứ hai ở DANGER).
Bảng mô tả đầy đủ: `docs/G1_TESTING_GUIDE.md` mục 3.4. 4 kịch bản gốc không đổi (đã so với HEAD).

## File đã sửa
- `tools/scenarios.py`: helper `_row(...)` (mặc định thoáng = 200 cm), 11 kịch bản mới, `SCENARIO_NAMES` mở rộng (4 tên gốc đứng đầu).
- `tools/guard/test_guard.py`: đổi `test_scenarios_has_4_named_timelines` → `test_scenarios_named_timelines` (tên duy nhất, là định danh C hợp lệ,
  >= 4 mốc, 6 slot, giá trị trong [20, 500]); thêm `test_new_scenarios_semantics` (ngưỡng 100/30 đọc từ `thresholds.h`, không gõ cứng).
- `firmware/waveshare-screen/host_sim/main.c`: cờ `--list` (in tên + số mốc từng kịch bản, thoát 0, không mở cửa sổ).
- `tools/test_mqtt_coreiot.py`: help của `--scenario` không còn liệt kê tên gõ cứng (argparse tự in `choices`).
- `docs/G1_TESTING_GUIDE.md`, `docs/ARCHITECTURE_G1_TESTING.md`, `docs/CHECKLIST.md`: cập nhật danh sách kịch bản.

## Kết quả kiểm thử
- Viết test trước: trước khi thêm dữ liệu đúng 1 test đỏ (`test_new_scenarios_semantics`, thiếu 11 tên); sau đó `pytest tools/guard/test_guard.py` **30 passed**
  (Windows cần `PYTHONUTF8=1`).
- Sim: **15/15 kịch bản replay đủ N/N mốc, exit 0** (`--interval 100 --exit-after 4`). Chụp ảnh kiểm bằng mắt `overtake_right`, `crossing_right`, `boxed_in`,
  `narrow_lane`: chấm/cung đúng slot và màu zone.
- MQTT dry-run (`test_mqtt_coreiot.py --scenario <tên> --dry-run`): **15/15 exit 0**, số payload bằng số mốc.
- `ctest` 3/3 pass; `scan_secrets` và `arch_guard` OK.

## Lưu ý
- CI vẫn chỉ chạy `approach` và `slam` trên sim (không đổi). Kịch bản mới được kiểm ngữ nghĩa bằng pytest nên đã chạy trong CI ở bước guard.
- Sim thoát ngay khi hết `--exit-after` và còn chờ đến hết thời gian đó dù đã feed đủ mốc: đặt `--exit-after` >= số mốc x `--interval` để xem đủ kịch bản.
- Kịch bản chỉ có khoảng cách (6 số cm mỗi mốc); chưa mô tả mất tín hiệu/`DISCONNECTED` hay mẫu ảo. Muốn thử các trường hợp đó cần mở rộng schema.
- Phát hiện sẵn có (không sửa): ở sidebar trái, hậu tố " DANG" bị cắt ("DAN") khi nhãn dài như "S3 (L-Front): 24 cm".
