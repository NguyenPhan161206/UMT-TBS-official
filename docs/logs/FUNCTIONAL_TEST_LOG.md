# FUNCTIONAL TEST — Kiểm thử chức năng không cần board (Task 4)

- **Ngày chạy**: 2026-10-06, máy dev Windows 11 (MinGW + Ninja cho host_sim, PlatformIO native cho sensor-node).
- **Phạm vi**: logic phân loại vùng/xe cắt ngang (`hazard_core`), hồ sơ xe, UI LVGL chạy trên PC (host_sim) với
  kịch bản khoảng cách tổng hợp, bộ lọc khoảng cách sensor-node, công cụ Python.
- **Gọi trong bài là "kiểm thử chức năng"** — dữ liệu là kịch bản tổng hợp (`tools/scenarios.py`) và fixture,
  **không** phải thử nghiệm thực địa, không suy ra tỷ lệ phát hiện/báo nhầm/bỏ sót ngoài đời.

## Kết quả

| Bộ test | Lệnh | Kết quả |
|---|---|---|
| host_sim — ctest (4 binary unit + 9 kịch bản UI) | `cmake -S firmware/waveshare-screen/host_sim -B build/host_sim && cmake --build build/host_sim && ctest --test-dir build/host_sim` | **13/13 passed** |
| ↳ `hazard_core_tests` (vùng, worst-zone, xe cắt ngang trên kịch bản sinh từ `tools/scenarios.py`) | `build/host_sim/hazard_core_tests` | **72 checks passed** |
| ↳ `vehicle_layout_tests` | | **99 passed, 0 failed** |
| ↳ `vehicle_settings_tests` | | **113 passed, 0 failed** |
| ↳ `vehicle_override_tests` | | **37 passed, 0 failed** |
| sensor-node DistanceFilter + thresholds (host) | `cd firmware/sensor-node && pio test -e native` | **19/19 passed** |
| Guard + ngữ nghĩa kịch bản (approach/crossing/slam/normal + 11 kịch bản khác) | `PYTHONUTF8=1 python -m pytest tools/guard/test_guard.py -q` | **30 passed** |
| Recorder/replayer | `python -m pytest tools/recorder -q` | **8 passed** |
| Công cụ đo độ trễ | `python -m pytest tools/latency -q` | **8 passed** |
| Công cụ soak | `python -m pytest tools/soak -q` | **9 passed** |
| Công cụ độ chính xác | `python -m pytest tools/accuracy -q` | **8 passed** |
| Guard tĩnh | `scan_secrets.py`, `arch_guard.py`, `check_rulechain_thresholds.py`, `gen_credentials.py --check` | **4/4 OK** |

Không có test nào fail trong các bộ trên.

## 4 kịch bản gốc trên UI (host_sim, khung 100 ms)

Các test ctest có sẵn: `umt_dash_sim_render` (approach), `umt_dash_sim_crossing_banner` (banner "CROSSING TRAFFIC
HAZARD" + "S3 (L-Front)" hiện ở 450 ms, tự tắt trước 4500 ms), `umt_dash_sim_normal_no_crossing` (không banner,
"OVERALL: SAFE"). Chạy thêm thủ công để kiểm banner vùng:

| Kịch bản | Dữ liệu (FRONT, cm) | Kiểm tra | Kết quả |
|---|---|---|---|
| approach | 160 → 20 qua 8 khung (slot khác ≥ 80) | `OVERALL: CAUTION@50`, `OVERALL: DANGER@750` | ok / ok |
| slam | 110 → 60 → 20 | `OVERALL: CAUTION@150`, `OVERALL: DANGER@250` | ok / ok |
| crossing | L-Front 120 → 80 → 40 → 120 | `OVERALL: SAFE@4500` (sau khi vật đi qua) | ok |
| normal | mọi slot 130–165 | không có `CAUTION`/`DANGER` ở 450 ms | ok |

Ghi chú: thử thêm `slam` `OVERALL: SAFE@50` → sim báo FAIL vì lần kiểm thực tế chạy ở 162 ms, lúc khung thứ 2
(60 cm → CAUTION) đã tới. Đây là giới hạn độ phân giải thời gian của cơ chế `--expect-text` trong sim, không phải
lỗi phân loại; không dùng kiểm tra này làm kết quả.

Lệnh:

```bash
cd build/host_sim
./umt_dash_sim.exe --scenario slam --interval 100 --exit-after 2 --expect-text "OVERALL: CAUTION@150" --expect-text "OVERALL: DANGER@250"
./umt_dash_sim.exe --scenario approach --interval 100 --exit-after 2 --expect-text "OVERALL: CAUTION@50" --expect-text "OVERALL: DANGER@750"
./umt_dash_sim.exe --scenario crossing --interval 100 --exit-after 6 --expect-text "OVERALL: SAFE@4500"
./umt_dash_sim.exe --scenario normal --interval 100 --exit-after 2 --expect-no-text "OVERALL: CAUTION@450" --expect-no-text "OVERALL: DANGER@450"
```

## Giới hạn (đưa vào phần Hạn chế của bài)
- Kịch bản là chuỗi khoảng cách tổng hợp, không có nhiễu/vọng thật của JSN-SR04T.
- host_sim chạy UI trên PC (SDL), không đo thời gian thực trên ESP32-S3.
- Chưa có thử nghiệm trên xe/thực địa; không có số liệu tỷ lệ phát hiện, báo nhầm, bỏ sót.
