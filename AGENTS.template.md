# ESP-IDF Workspace Guidelines & Architecture (Template — V2)

> **Hướng dẫn cấu hình cho máy mới (Cross-Device Setup):**
> 1. Sao chép file này thành `AGENTS.md` ở thư mục gốc: `cp AGENTS.template.md AGENTS.md`
> 2. Tạo key local: `cp config/keys.template.json config/keys.json` và điền **Device Access Token mới**
>    (tự tạo trên tài khoản CoreIoT của chủ dự án — KHÔNG dùng token đã từng commit ở repo cũ).
> 3. Điền/sửa các đường dẫn `<...>` cho khớp máy hiện tại.
> 4. Agent và người dùng chỉ đọc secret từ `config/keys.json` (gitignored).

## Overview
Hệ thống **Cảnh báo va chạm xe tải** — **Hybrid V2**:
1. **`firmware/sensor-node/`**: Cảm biến JSN-SR04T (khoảng cách) → **ESP-NOW (đường chính)** tới
   waveshare-screen; đồng thời **MQTT CoreIoT (đường phụ)** gửi telemetry lên cloud.
   Default Port: **<SENSOR_NODE_PORT, e.g. /dev/ttyACM0>**.
2. **`firmware/waveshare-screen/`**: Màn hình 7" RGB LVGL v9. Nhận cảnh báo qua **ESP-NOW** (ưu tiên,
   độ trễ thấp) và qua **CoreIoT Rule-Chain** (phụ). Default Port: **<WAVESHARE_SCREEN_PORT, e.g. /dev/ttyACM1>**.

## Kiến trúc & Quy tắc bắt buộc (đọc trước khi code)
- **`CONSTITUTION.md` (R1–R12)**: đọc tại `.opencode/docs/CONSTITUTION.md`.
- **R2**: mọi struct/define dùng chung giữa 2 board **chỉ nằm ở `firmware/shared/`** — cấm define trùng lặp.
- **R3**: ngưỡng cảnh báo **chỉ ở `firmware/shared/thresholds.h`** (sensor-node và screen cùng include).
- **R4**: `SENSOR_COUNT = sizeof(SENSOR_PINS)/sizeof(SENSOR_PINS[0])` + `static_assert`.
- **R1/R6**: credential qua `credentials.h` (sinh bởi `tools/guard/gen_credentials.py` từ `config/keys.json`,
  gitignored); module chưa dùng phải được build trong ≥1 env.

## Hardware & Môi trường
- **Target Chip**: `esp32s3` (Dual-Core 240 MHz, Wi-Fi, BT 5 LE, 8MB PSRAM)
- **PlatformIO**: `pio run -e yolo_uno`; `framework = arduino` (sensor-node), `framework = espidf` ≥5.5
  (waveshare-screen, LVGL v9 qua `idf_component.yml`).
- **Python**: `/usr/bin/python3` (3.12+) cho `tools/`.
- GPU/board không có sẵn trên máy dev: build xác minh qua CI (GitHub Actions) hoặc máy Windows.

## Build & Flash Commands

### Sensor Node
```bash
cd firmware/sensor-node
pio run -e yolo_uno                                  # Build (default: USE_COREIOT=0)
pio run -e yolo_uno_coreiot                          # Build có CoreIoT (bắt buộc CI)
pio run -e yolo_uno -t upload --upload-port /dev/ttyACM0
pio device monitor -p /dev/ttyACM0 -b 115200
```

### Waveshare Screen
```bash
cd firmware/waveshare-screen
pio run -e yolo_uno
pio run -e yolo_uno -t upload --upload-port /dev/ttyACM1
pio device monitor -p /dev/ttyACM1 -b 115200
```

> Trên Windows dùng `build_and_flash.bat` tương ứng (nếu có).

## Kiểm thử cục bộ (không cần board)
```bash
python3 tools/guard/scan_secrets.py                 # quét secret trong file tracked
python3 tools/guard/gen_credentials.py --check      # kiểm keys.json đủ field
python3 tools/guard/check_rulechain_thresholds.py   # đối chiếu rule-chain vs thresholds.h (best-effort)
```
Unit test host (DistanceFilter, thresholds) nằm ở `firmware/sensor-node/test/` và
`firmware/waveshare-screen/test/` (làm ở giai đoạn firmware).

## MQTT Testing & CoreIoT Integration
- **Broker Host**: `app.coreiot.io` (Port `1883`) — từ `config/keys.json`
- **Telemetry Topic**: `v1/devices/me/telemetry`
- **Script Test**: `tools/test_mqtt_coreiot.py` (chưa port sang V2 — xem roadmap)
```bash
python3 tools/test_mqtt_coreiot.py --distance 15.5
python3 tools/test_mqtt_coreiot.py --loop --interval 2
```

> **Quy tắc bảo mật:** token/password KHÔNG bao giờ nằm trong file tracked. Chỉ đọc từ
> `config/keys.json` hoặc file sinh từ nó. Token đã từng lộ (repo cũ) KHÔNG được tái sử dụng.

## Git Collaboration Workflow (R8/R9)
Đọc `CONTRIBUTING.md` trước khi commit/push. Tóm tắt:
```bash
git pull origin main --rebase
# ... edit ...
/verify                 # quét secret + selfcheck (opencode command)
git add <file cụ thể>   # CẤM git add -A / .
git commit -m "<type>: <mô tả>"   # feat/fix/docs/refactor/chore
git push origin main
```

## Implementation Logging Requirement
Sau khi hoàn thành một nhiệm vụ (implement/fix/refactor), bắt buộc tạo/cập nhật
`docs/logs/<COMPONENT>_<TASK_NAME>_LOG.md` — ghi rõ mục tiêu, file đã sửa, kết quả kiểm thử,
hướng dẫn vận hành/demo.

## Default workflow: dev-orchestrator
Mọi yêu cầu lớn hơn một sửa đơn file — feature, refactor, migration, "kế hoạch / phân rã /
break this down" — gọi skill `dev-orchestrator` **trước khi viết code**:

```
Skill(skill="dev-orchestrator")
```

- **MODE 1 — Decomposition:** tạo `docs/roadmaps/<slug>.roadmap.json` + bảng tóm tắt atomic steps.
- **MODE 2 — Worker Execution Prompt:** render prompt self-contained, tự verify DoD, cập nhật ledger.
- Bỏ qua chỉ với câu hỏi đọc/kiểm tra đơn giản.