# PROGRESS — Trạng thái tiến độ dự án (UMT-TBS V2)

> Bảng trạng thái chi tiết hơn README. Cập nhật sau mỗi nhiệm vụ (R10).
> Quy trình test: xem `docs/TEST_PROTOCOL.md`.

- **Cập nhật lần cuối**: 2026-09-03
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
| B4n | Backlight waveshare: kiến trúc HYBRID fallback (màn lên KHÔNG phụ thuộc CH422G; kiểm soát backlight khi CH422G ACK; macro `CONFIG_WAVESHARE_BACKLIGHT_FALLBACK` 1/0) | 🟢 CODE XONG, build OK cả 2 branch (RAM 12.8%/Flash 31.3%) | ⏳ flash-and-observe chờ board — xem `docs/logs/WAVESHARE_SCREEN_BACKLIGHT_ARCH_FALLBACK_LOG.md` |
| B6e | Guard tools (`tools/guard/*.py`) | ✅ XONG | scan/gen/check_rulechain/check_size |
| B7 | CI GitHub Actions (build 2 env × 2 fw + test + Gitleaks + size-gate + asserts) | ✅ XONG | CI liên tục xanh (run gần nhất success) |
| B7b | Protected branch `main` (PR phải xanh) | ⏳ CHỜ | cần quyền admin GitHub |
| B9a | Tool `test_mqtt_coreiot.py` V2 + rule-chain snapshot V2 (zone 100/30) | ✅ XONG | gate R11 OK, 16/16 pytest |
| B9b | Nghiệm thu CoreIoT: token MỚI → flash → dashboard `warning_status` | ⏳ CHỜ | **cần token MỚI** → điền keys.json → flash |
| T5.x | Đo hiệu năng (baseline, latency, soak) | ⏳ CHỜ | sau khi firmware ổn định / có board |

## Nhóm việc đang xử lý

### Đang triển khai (agent)
- ✅ Tài liệu: `docs/TEST_PROTOCOL.md` + `docs/PROGRESS.md` (mở PR/commit này).
- ✅ B6: hướng dẫn tạo token CoreIoT từng bước (`/devices`) vào README + TEST_PROTOCOL.
- ✅ Báo cáo `report/` (5 chương) + `report-code/` (8 chương chi tiết 4 lớp IoT).

### Chờ user / phần cứng
- 🔑 **B0b/B9b:** tạo token CoreIoT MỚI trên `app.coreiot.io` (**Devices** `/devices`
  → Manage Credentials → Access Token) → điền `config/keys.json` (xem cấp 3
  `docs/TEST_PROTOCOL.md`).
- 🔌 **B5/B9b/T5.x:** flash board thật, quan sát, đo.
- 🛡️ **B7b:** bật protected branch trên GitHub (admin).

## Ngưỡng & quy ước (R3/R4)

- **Zone**: SAFE > 100 cm, CAUTION ≤ 100, DANGER ≤ 30 (nguồn duy nhất
  `firmware/shared/thresholds.h`).
- **Buzzer**: WARNING < 50 cm (3 s) / DANGER < 20 cm (1 s).
- **Số cảm biến**: `SENSOR_COUNT = sizeof(SENSOR_PINS)/...` + static_assert (R4).
- **Không dùng GPIO47/48** (PSRAM Embedded chiếm chân).

## Lịch sử cập nhật

- 2026-09-02: tạo file; đánh dấu B0–B7, B9a XONG; B9b/B0b/B7b/T5.x CHỜ.
