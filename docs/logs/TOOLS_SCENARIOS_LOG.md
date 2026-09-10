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