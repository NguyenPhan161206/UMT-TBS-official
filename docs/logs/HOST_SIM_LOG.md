# HOST_SIM_LOG — G1 T1.2 LVGL SDL Simulator

Ngày: 2026-09-10
Nhánh: `nguyen` (chưa merge main)

## Mục tiêu
Tạo executable host `umt_dash_sim` — LVGL v9.1.0 + SDL2 trên máy tính, compile các
file UI THẬT của waveshare-screen (ui_dashboard.c/layout.c/system.c, sensor_model.c,
hazard_core.c) qua stub mỏng, để test UI không cần board. Headless cho CI bằng
`--exit-after N` (xvfb-run hoặc SDL dummy driver).

## File đã tạo/sửa
- `firmware/waveshare-screen/host_sim/main.c` (mới, 221 dòng) — CLI `--scenario`
  `<name>` / `--replay <jsonl>` / `--exit-after <sec>` / `--interval <ms>`; khởi tạo
  SDL2 + LVGL (lv_sdl_window_create 800x480); feed 6 slot theo mốc scenario;
  SDL_VIDEODRIVER=dummy headless OK.
- `firmware/waveshare-screen/host_sim/lv_conf.h` (mới) — LVGL 9.1: LV_USE_SDL,
  LV_COLOR_DEPTH 16, LV_MEM_SIZE 64KB, LV_TICK_CUSTOM = SDL_GetTicks, font 14/16,
  widget cần (arc/btn/label/line/img), LV_USE_LOG WARN.
- `firmware/waveshare-screen/host_sim/gen_scenarios.py` (mới) — đọc
  `tools/scenarios.py` (nguồn duy nhất, step 2) → sinh C header
  `scenarios_gen.h` tại build dir.
- `firmware/waveshare-screen/host_sim/stub/` (mới, 10 file) — freertos/
  {FreeRTOS.h, semphr.h} (mutex no-op), esp_log/esp_system/esp_wifi/esp_timer/
  esp_chip_info/esp_flash/esp_app_desc/esp_err/coreiot_client (giá trị MASKED giả,
  không lộ token thật — R1).
- `firmware/waveshare-screen/host_sim/CMakeLists.txt` (sửa) — thêm FetchContent
  lvgl v9.1.0 (khớp src/idf_component.yml), find_package(SDL2), target
  `umt_dash_sim` (+ custom command gen_scenarios.py), add_test
  `umt_dash_sim_render` (SDL dummy driver).
- `.github/workflows/ci.yml` (sửa) — cài libsdl2-dev + xvfb; build host_sim; chạy
  hazard_core_tests + umt_dash_sim (xvfb-run, scenario approach + slam).

## Kết quả kiểm thử (đã chạy 2026-09-10)
- `cmake -S ... -B /tmp/host_sim && cmake --build` — SUCCESS (gcc 13.3.0, lvgl fetch).
- `ctest` tại build dir: 2/2 PASSED (hazard_core_tests 25 checks; umt_dash_sim_render).
- `SDL_VIDEODRIVER=dummy ./umt_dash_sim --scenario approach --exit-after 3 --interval 300`
  → init UI, 8/8 mốc, exit 0.
- `--replay /tmp/replay_fixture.jsonl` (2 dòng d1..d6) → 2 row(s), exit 0.
- `xvfb-run` không có máy dev (không sudo) → CI sẽ chạy đường xvfb-run đúng DoD.
- `pio run -e yolo_uno` (waveshare) — không ảnh hưởng build firmware (chạy tại
  sensor-node OK; waveshare build là mục CI).
- pytest tools/guard/test_guard.py: 25 passed; arch_guard.py OK; scan_secrets.py OK.

## Hướng dẫn vận hành / demo
```bash
export PATH="/home/binhnguyen/.venv-pio/bin:$PATH"
cmake -S firmware/waveshare-screen/host_sim -B /tmp/host_sim
cmake --build /tmp/host_sim -j$(nproc)
# headless (không cần X):
SDL_VIDEODRIVER=dummy /tmp/host_sim/umt_dash_sim --scenario approach --exit-after 3
# cửa sổ thật (máy có display):
/tmp/host_sim/umt_dash_sim --scenario slam --exit-after 10
/tmp/host_sim/umt_dash_sim --replay /tmp/tb.jsonl --exit-after 5
# CI chạy đúng DoD bằng xvfb:
xvfb-run -a /tmp/host_sim/umt_dash_sim --exit-after 3 --scenario approach
```

## Deviation so với prompt gốc step 3
- Keep target `hazard_core_tests` + test `umt_dash_sim_render` với
  `SDL_VIDEODRIVER=dummy` (CI vẫn dùng xvfb-run theo DoD).
- Không include `ui_dashboard.h` trong target_files — file đã có sẵn API đúng, không
  cần sửa bước này (roadmap liệt kê vì là contract, không phải change).
- LV_COLOR_DEPTH 16 (khớp RGB565 màn hình 7", PSRAM tiết kiệm).

## Ghi nhận cho bước sau (T3.1/T3.2, step 8/9)
- Sim hiện feed cả 6 slot qua `ui_dashboard_update_sensor`; khi thêm biểu tượng
  vị trí vật thể (T3.1) và sơ đồ EX8 (T3.2) sẽ cần thêm snapshot trạng thái LVGL
  để assert layout — hoặc screenshot cơ bản (lv_snapshot) nếu cần CI visual.