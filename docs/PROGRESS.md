# PROGRESS — Trạng thái tiến độ dự án (UMT-TBS V2)

> Bảng trạng thái chi tiết hơn README. Cập nhật sau mỗi nhiệm vụ (R10).
> Quy trình test: xem `docs/TEST_PROTOCOL.md`.

- **Cập nhật lần cuối**: 2026-10-06
- **Branch**: `main`

## Bảng trạng thái tổng hợp

| ID | Hạng mục | Trạng thái | Bằng chứng / ghi chú |
|----|----------|-----------|-----------------------|
| B0 | Repo UMT-TBS-official, node/PlatformIO | ✅ XONG | init + push (1 remote `origin`) |
| B0b | Tài khoản CoreIoT, 2 device, rule-chain | 🟡 MỘT PHẦN | import rule-chain tập tin có; **chờ token MỚI** (R11) |
| B1 | Cấu hiến opencode (CONSTITUTION/SELFCHECK/agents/commands/skills) | ✅ XONG | commit đầu |
| B2 | Shared contract (`espnow_protocol.h`, `thresholds.h`) | ✅ XONG | 66 + 124 dòng, static_assert R4 |
| B3 | Scaffold sensor-node (sensor/filter/shared_state/buzzer/espnow/coreiot) | ✅ XONG | build 2 env, 10/10 host test |
| B4 | Scaffold waveshare-screen (sensor_model/ui_dashboard/coreiot_client) | ✅ XONG | build OK, RAM 12.8%/Flash 31.2% |
| B5 | Nối ESP-NOW qua shared header | ✅ XONG | grep define ngoài shared = 0 |
| B5f | Fix ESP-NOW NO LINK: `ESPNOW_PEER_MAC` unicast sai (MAC sensor-node) → **broadcast FF:FF:FF:FF:FF:FF**; `espnow_client` bỏ bắt buộc `add_peer`. (2026-09-03) | ✅ XONG (code+build+test) / 🔌 chờ nghiệm thu end-to-end (thiếu sensor-node enum) | `docs/logs/WAVESHARE_SCREEN_ESPNOW_LINK_LOG.md` |
| B9m | Fix MQTT up/down liên tục trên waveshare: tách `set_iot_status` → `set_wifi_status`/`set_mqtt_status` (hết overwrite lẫn nhau) + debounce MQTT DOWN 6s trong `coreiot_client`. (2026-09-03) | ✅ XONG + flash waveshare OK | log boot: MQTT connect OK; drop ~10s vẫn xảy ra do **mạng/broker NAT** (không phải bug fw); debounce giúp UI ổn định 
| B4n | Backlight waveshare: **init CH422G ĐÚNG protocol** (WR-SET→WR-OC→WR-IO=0xFF → LCD_BL/IO2 HIGH; trước đây ghi sai WR-IO=0x1E làm màn tối + kéo USB_SEL/IO5 LOW mất USB). Macro `CONFIG_WAVESHARE_BACKLIGHT_FALLBACK` 1/0. | ✅ XONG + **flash-and-observe: màn SÁNG, UI hiển thị** (2026-09-03) | nguyên nhân gốc từ `esp_io_expander_ch422g.c` chính thức + paulhamsh ref; log `docs/logs/WAVESHARE_SCREEN_BACKLIGHT_CH422G_LOG.md` |
| B6e | Guard tools (`tools/guard/*.py`) | ✅ XONG | scan/gen/check_rulechain/check_size |
| B7 | CI GitHub Actions (build 2 env × 2 fw + test + Gitleaks + size-gate + asserts) | ✅ XONG | CI liên tục xanh (run gần nhất success) |
| B7b | Protected branch `main` (PR phải xanh) | ⏳ CHỜ | cần quyền admin GitHub |
| B9a | Tool `test_mqtt_coreiot.py` V2 + rule-chain snapshot V2 (zone 100/30) | ✅ XONG | gate R11 OK, 16/16 pytest |
| B9b | Nghiệm thu CoreIoT: token MỚI → flash → dashboard `warning_status` | 🟡 MỘT PHẦN (2026-09-03) | token MỚI đã dùng; sensor-node flash env `yolo_uno_coreiot`, **`[NET] MQTT connected`** → device **Active** trên dashboard; còn nghiệm thu `warning_status`/buzzer đầy đủ |
| T5.x | Đo hiệu năng (baseline, latency, soak) | ⏳ CHỜ | sau khi firmware ổn định / có board |
| T4 | Kiểm thử chức năng (host, kịch bản tổng hợp — KHÔNG phải thực địa) | ✅ ĐÃ CHẠY (2026-10-06) | host_sim ctest 13/13 (hazard_core 72 checks, layout 99, settings 113, override 37); `pio test -e native` 19/19; pytest guard 30, recorder 8, latency 8, soak 9, accuracy 8; approach/slam/crossing/normal đúng banner vùng trên sim — `docs/logs/FUNCTIONAL_TEST_LOG.md` |
| T5.1 | Độ chính xác raw vs lọc 6 mốc — DMXT-55 (+ tầm đo/góc búp DMXT-56) | 🟡 CÔNG CỤ XONG, **CHƯA ĐO** (2026-10-06) | env `yolo_uno_accuracy` (dòng `ACC`), `tools/accuracy/measure_accuracy.py`; quy trình `docs/ACCURACY_TEST.md`; log `docs/logs/ACCURACY_DMXT55_LOG.md` |
| T5.2 | Độ trễ ESP-NOW (RTT/2) + MQTT đầu–cuối — DMXT-57 | ✅ **ĐÃ ĐO** (06/10, trên bàn, 1 m): ESP-NOW một chiều p50 2,73 / p95 6,16 ms (Wi-Fi tắt, nhận 99,83 %) và 2,03 / 4,01 ms (Wi-Fi bật, nhận 99,93 %); MQTT đầu–cuối p50 420 / p95 1001 ms (450/450). A2 khoảng cách xa: tuỳ chọn | env `*_latency` 2 board + rule-chain có `seq` (đã import, Root) + `tools/latency/measure_latency.py`; quy trình `docs/LATENCY_TEST.md`; log `docs/logs/LATENCY_DMXT57_LOG.md` |
| T5.3 | Soak 24 h: reset, heap, tỷ lệ nhận ESP-NOW, MQTT reconnect — DMXT-58 | 🟡 CÔNG CỤ XONG, **CHƯA CHẠY** (2026-10-06) | heartbeat `BOOT`/`SOAK` mỗi 60 s trong firmware thường (`firmware/shared/soak_diag.h`), `tools/soak/soak_logger.py`; quy trình `docs/SOAK_TEST.md`; log `docs/logs/SOAK_DMXT58_LOG.md` |
| CD | Release firmware.bin tự động lên GitHub Release khi tag `v*` | ✅ XONG (2026-09-09) | `.github/workflows/release.yml`; tag `v0.1.0-preview` → 3 `.bin` + `SHA256SUMS.txt`, `sha256sum -c` OK (xem `docs/CD_RELEASE.md` + `docs/roadmaps/cd-release.state.md`) |

## Nhóm việc đang xử lý

### Đang triển khai (agent)
- ✅ Tài liệu: `docs/TEST_PROTOCOL.md` + `docs/PROGRESS.md` (mở PR/commit này).
- ✅ B6: hướng dẫn tạo token CoreIoT từng bước (`/devices`) vào README + TEST_PROTOCOL.
- ✅ Báo cáo `report/` (5 chương) + `report-code/` (8 chương chi tiết 4 lớp IoT).

### Chờ user / phần cứng
- 🔑 **B0b/B9b:** token MỚI đã tạo + điền `config/keys.json` (2026-09-03) ✅ → còn **nghiệm thu dashboard `warning_status`/buzzer** (B9b đầy đủ).
- 🔌 **T5.x:** đo hiệu năng (baseline, latency, soak) — sau firmware ổn định.
- 🛡️ **B7b:** bật protected branch trên GitHub (admin).
- 🐛 **Lưu ý:** log cảm biến sensor-node `REJECT` tràn màn hình khi chưa gắn vật — làm khó đọc MQTT; có thể giảm verbose nếu cần.

## Ngưỡng & quy ước (R3/R4)

- **Zone**: SAFE > 100 cm, CAUTION ≤ 100, DANGER ≤ 30 (nguồn duy nhất
  `firmware/shared/thresholds.h`).
- **Buzzer**: kêu khi `nearest_cm <= SENSOR_DANGER_CM` (30), im khi CAUTION/SAFE/invalid (ngưỡng zone dùng chung).
- **Số cảm biến**: `SENSOR_COUNT = sizeof(SENSOR_PINS)/...` + static_assert (R4).
- **Không dùng GPIO47/48** (PSRAM Embedded chiếm chân).

## Lịch sử cập nhật

- 2026-09-02: tạo file; đánh dấu B0–B7, B9a XONG; B9b/B0b/B7b/T5.x CHỜ.
- 2026-09-03: màn waveshare SÁNG (hybrid fallback, commit `42976d8`); token MỚI; sensor-node env coreiot `MQTT connected` → **Active** (B9b một phần).
- 2026-09-03: fix ESP-NOW broadcast (B5f) + tách MQTT label debounce (B9m); build 3 env + 10/10 host test + flash waveshare OK. **Chặn**: sensor-node chưa enum trong kernel → chưa nghiệm thu LINKED.
- 2026-09-09: **CD hoạt động** — `release.yml` build 3 env + upload Release trên tag `v*`; nghiệm thu tag `v0.1.0-preview` (3 `.bin` + `SHA256SUMS.txt`, checksum OK). G1 roadmap đảo theo kiến trúc kiểm thử (`docs/ARCHITECTURE_G1_TESTING.md`).
- 2026-10-06: chuẩn bị số đo cho bài báo — công cụ + firmware đo độ trễ (T5.2), soak heartbeat (T5.3), độ chính xác raw/lọc (T5.1); chạy kiểm thử chức năng host (T4, toàn bộ pass). Chưa có số đo trên board.
