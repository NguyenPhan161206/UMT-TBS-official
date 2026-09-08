# CHECKLIST — Master Plan (T0–T5) đối chiếu code hiện tại

> **Nguyên tắc chung**: mở được khả năng kiểm thử trước, sửa lỗi sau, rồi mới làm thêm tính năng.
> Đừng vội phát triển tiếp khi những phần hiện tại còn chưa đo được và chưa biết đang chạy đúng đến đâu.
>
> Trạng thái bảng dưới được **audit trực tiếp trên code** (không phải ghi nhớ).
> Cập nhật lần cuối: **2026-09-08** — HEAD `9dcf877` (main).

## Tổng hợp nhanh

| Giai đoạn | Tổng | ✅ XONG | 🟡 MỘT PHẦN | ❌ CHƯA |
|-----------|------|---------|--------------|---------|
| G0 — Tiếp quản & xác minh | 4 | 1 | 2 | 1 |
| G1 — Mở khả năng kiểm thử | 4 | 0 | 2 | 2 |
| G2 — Sửa 4 lỗi P0 | 4 | 3 | 1 | 0 |
| G3 — Hoàn thiện đề xuất | 5 | 1 | 1 | 3 |
| G4 — Tính mới đề tài | 2 | 1 | 0 | 1 |
| G5 — Xuyên suốt | 9 | 3 | 1 | 5 |
| **Tổng** | **28** | **9** | **7** | **12** |

---

## Giai đoạn 0 — Tiếp quản và xác minh (26/08–08/09)

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T0.1 | Dựng môi trường build cho cả hai firmware | ✅ XONG | `platformio.ini` 2 bên; `pio run -e yolo_uno` + `yolo_uno_coreiot` (sensor-node), `yolo_uno` (waveshare) build SUCCESS; host test native 10/10 |
| T0.2 | Chốt nhánh chính thức | ⏳ CHỜ | Chưa chốt đúng như plan: chờ số đo **T5.7** (latency) rồi quyết định — không vội |
| T0.3 | Tái hiện + xác nhận 4 lỗi P0 (mỗi lỗi 1 issue) | 🟡 MỘT PHẦN | Trong code còn 1/4 lỗi P0 **T2.3 chưa sửa**; loạt fix P0 khác đã có (buzzer, banner, link watchdog). **Chưa thấy issue GitHub** cho từng lỗi |
| T0.4 | Kiểm kê & nghiệm thu phần cứng (**làm trước, không để lại sau**) | 🟡 MỘT PHẦN | Đã ghi nhận **1 module sensor hư**: log S5 `Echo bi giu HIGH truoc khi Trigger` (sensor-node serial 2026-09-08), cảm biến S0/S2 vẫn đọc. **Chưa chụp ảnh hiện trạng đấu dây** vào repo |

## Giai đoạn 1 — Mở khả năng kiểm thử (02/09–06/10) — **ƯU TIÊN CAO NHẤT**

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T1.1 | Trình giả lập MQTT đúng schema + tham số scenario | 🟡 MỘT PHẦN | `tools/test_mqtt_coreiot.py` đã chuẩn schema V2 (`d1..d6/nearest_cm`, zone 100/30), gate R11 OK. **THIẾU `--scenario`** (approach/crossing/slam/normal) — đã ghi bước 1 roadmap nhánh tiếp theo |
| T1.2 | Trình mô phỏng LVGL + backend SDL trên PC (**đòn bẩy lớn nhất**) | ❌ CHƯA | Chưa có `host_sim/`; UI chỉ test được trên board — ghi bước 4 roadmap nhánh tiếp theo |
| T1.3 | Unit test DistanceFilter + sensor_model_classify | 🟡 MỘT PHẦN | **DistanceFilter**: có `test_distance_filter.cpp` + `test_thresholds.cpp` (10/10 native). **sensor_model_classify**: chưa có test bên waveshare — thêm bước 2 roadmap |
| T1.4 | Ghi & phát lại dữ liệu thật | ❌ CHƯA | Chưa có `record_telemetry.py`/`replay_telemetry.py` — ghi bước 3 roadmap nhánh tiếp theo |

## Giai đoạn 2 — Sửa 4 lỗi P0 (16/09–20/10, song song G1)

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T2.1 | Tạo buzzerTask (còi trước đây không bao giờ kêu) | ✅ XONG | `buzzer.cpp` có `buzzerTask()`; **DANGER kêu liên tục, CAUTION 1 lần/s** (commit `9dcf877`); build 2 env + flash sensor-node OK |
| T2.2 | Sửa banner kẹt vĩnh viễn DANGER | ✅ XONG | `ui_dashboard.c:145` — `if (readings[i].is_stale) continue;` skip slot chưa báo → hết kẹt banner |
| T2.3 | Sửa heuristic cảnh báo xe cắt ngang bị vô hiệu | ❌ CHƯA | **Dead path còn nguyên**: rule-chain `0` lần xuất `crossing_hazard`; firmware vẫn đọc field tại `main.c:84` + `s_forced_crossing_warning` luôn false. Local heuristic (`front_close<150 && side delta>=40`) vẫn chạy nhưng nguồn ép từ cloud chết |
| T2.4 | Phát hiện mất kết nối & dữ liệu cũ (**quan trọng nhất về an toàn**) | ✅ XONG | `main.c:142 espnow_link_watchdog_cb` (LVGL timer 500ms) + `espnow_receiver.c` clear slot quá `ESPNOW_LINK_TIMEOUT_MS` + `is_stale` |

## Giai đoạn 3 — Hoàn thiện các yêu cầu trong đề xuất (14/10–01/12)

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T3.1 | Biểu tượng phương tiện/vật thể tại vị trí phát hiện | ❌ CHƯA | UI chỉ có text/số per-sensor (`ui_dashboard.c:213 " DANG"`), chưa có icon theo toạ độ |
| T3.2 | Vẽ sơ đồ xe tải EX8 theo hồ sơ (không gắn cứng toạ độ) | ❌ CHƯA | Chưa có sơ đồ EX8; chưa có cấu trúc vehicle profile |
| T3.3 | Cảnh báo âm thanh trong cabin | 🟡 MỘT PHẦN | Buzzer đã hoạt động (T2.1). **Xung đột GPIO47/48 đã giải quyết**: `BUZZER_PIN=11` (progress cũ ghi 48 — hiện code không còn). Còn thiếu: mạch khuếch đại 5V (transistor) + chụp tài liệu lắp đặt |
| T3.4 | Thống nhất các ngưỡng cảnh báo (**R3**) | ✅ XONG | Buzzer giờ dùng chung `SENSOR_CAUTION_CM=100`/`SENSOR_DANGER_CM=30` từ `firmware/shared/thresholds.h`; đã **xoá** `BUZZER_WARNING_DISTANCE_CM`/`BUZZER_DANGER_DISTANCE_CM`/`BUZZER_DANGER_PERIOD_MS`; grep firmware = 0; check_rulechain OK {100,30} |
| T3.5 | Cân chỉnh độ trễ bộ lọc cho vật chuyển động | ❌ CHƯA | Vẫn đang cho bài toán "đo mực nước" (5 mẫu + 3 confirm jump ≈ 0.5–0.8s). **Cần T1.4** để đo dữ liệu thật trước khi đổi |

## Giai đoạn 4 — Làm rõ tính mới của đề tài (11/11–12/01/2027)

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T4.1a | Cấu trúc hồ sơ xe + nhập cứng hồ sơ EX8 | ❌ CHƯA | Chưa có struct vehicle profile |
| T4.1b | 2 hồ sơ nữa + màn chọn + lưu NVS | ❌ CHƯA | Chưa có màn Setup hồ sơ (sidebar chỉ có tab Collision/System) |
| T4.1c | Chỉnh tay từng cảm biến ghi đè profile | ❌ CHƯA | Chưa có |
| T4.1d | Người dùng tự tạo hồ sơ mới | ❌ CHƯA | Chưa có (để dành giai đoạn sau) |
| T4.2 | Gom bản đồ vị trí cảm biến 1 nguồn duy nhất (**SENSOR_COUNT từ sizeof**) | ✅ XONG | `thresholds.h:112` `#define SENSOR_COUNT (sizeof(SENSOR_PINS)/sizeof(SENSOR_PINS[0]))` + `static_assert(SENSOR_COUNT==6)` (R4) |

## Giai đoạn 5 — Việc làm xuyên suốt

### Vệ sinh kỹ thuật

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T5.1 | Wi-Fi password & CoreIoT token ra khỏi header được commit | ✅ XONG | `credentials.h` sinh bởi `tools/guard/gen_credentials.py` từ `config/keys.json` (gitignored); `scan_secrets.py` sạch; CI chạy Gitleaks |
| T5.2 | Nút Calibrate chưa có callback | ❌ CHƯA | `ui_dashboard_layout.c:177-181` tạo `calib_btn` + label nhưng **không `lv_obj_add_event_cb`** → làm cho hoạt động hoặc bỏ nút |
| T5.3 | Sửa comment sai ID cảm biến (`ui_dashboard.h`) | 🟡 MỘT PHẦN | `sensor_model.h:10` đã ghi đúng `FRONT=0, REAR=1, LEFT_FRONT=2` khớp `espnow_slot_t`; còn comment phần label UI + lướt lại toàn bộ để chắc chắn |
| T5.4 | CI tự động build cả 2 firmware | ✅ XONG | `.github/workflows/ci.yml`: 2 env sensor-node + waveshare, host test, pytest, Gitleaks, scan_secrets, size-gate; push/PR chạy |

### Đo lường & đánh giá hiệu năng (thiếu nhiều nhất)

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T5.5 | Baseline độ ổn định/chính xác 6 khoảng cách (raw + filtered) | ❌ CHƯA | `docs/PROGRESS.md` chưa có số đo; report/ chưa có số thực tế |
| T5.6 | Tầm thực tế + phản xạ mặt đường ở 3 chiều cao + góc búp | ❌ CHƯA | Chưa đo (mẫu đo trong `docs/TEST_PROTOCOL.md`) |
| T5.7 | Độ trễ đầu–cuối 2 nhánh (**dùng chốt T0.2**) | ❌ CHƯA | Chưa đo |
| T5.8 | Soak 24h → 72h (heap + số lần reset) | ❌ CHƯA | Chưa chạy |
| T5.9 | Tỷ lệ báo nhầm/spot sót trên tập replay đã gán nhãn | ❌ CHƯA | **Cần T1.4** trước |

---

## Nhận xét chính từ audit

1. **G2 P0 đã gần xong**: 3/4 lỗi P0 có fix trong code (buzzer, banner, mất kết nối). Duy nhất **T2.3 (crossing hazard) vẫn còn dead-path** — đây là P0 còn sót duy nhất.
2. **G1 vẫn là lỗ hổng ưu tiên số 1**: T1.1 thiếu scenario, T1.2 (LVGL SDL sim) và T1.4 (record/replay) chưa làm → toàn bộ T3.5, T5.7, T5.9 đang bị chặn. Không nên làm G3/G4 trước khi có T1.2 + T1.4.
3. **T3.4 đã xong sớm hơn dự kiến** nhờ sửa buzzer (commit `9dcf877`): ngưỡng còi nay đồng bộ 100/30 với zone chung — đúng yêu cầu "thống nhất ngưỡng", không phải chỉnh giao diện.
4. **T0.4 phần cứng còn nợ**: chưa có ảnh chụp hiện trạng đấu dây trong repo; theo plan đây là việc "làm đầu tiên, không để lại sau".
5. **Báo cáo chưa có số đo**: report/ mới có thiết kế + lập luận khả thi; muốn nói trước hội đồng phải đổ dữ liệu T5.x vào `docs/PROGRESS.md`.

## Thứ tự ưu tiên đề xuất cho phiên sau

1. **G1 trước** — T1.1 (scenario), T1.4 (record/replay sớm vì cần board), T1.2 (LVGL SDL sim), T1.3 (classify test) → theo `docs/roadmaps/next-branch.roadmap.json` (15 bước, 4 giai đoạn).
2. **T2.3** (P0 sót) khi đang làm G2.
3. Nghiệm thu T0.4: chụp ảnh hiện trạng + ghi log bằng chứng **trước khi tháo bất cứ thứ gì**.
4. Sau khi có T1.2 → làm T3.1 (icon), T3.2 (EX8) trên simulator.
5. Sau khi có T1.4 → T3.5 (cân lọc), T5.9 (báo nhầm), rồi T5.5–T5.8 (đo + soak) → số liệu vào PROGRESS.