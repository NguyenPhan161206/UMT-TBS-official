# CHECKLIST — Master Plan (T0–T5) đối chiếu code hiện tại

> **Nguyên tắc chung**: mở được khả năng kiểm thử trước, sửa lỗi sau, rồi mới làm thêm tính năng.
> Đừng vội phát triển tiếp khi những phần hiện tại còn chưa đo được và chưa biết đang chạy đúng đến đâu.
>
> Trạng thái bảng dưới được **audit trực tiếp trên code** (không phải ghi nhớ).
> Cập nhật lần cuối: **2026-10-05** (nhánh `khoa`, HEAD `9bba9c2` + thay đổi chưa commit). Các dòng **T2.3, T3.1, T3.2, T4.1a–T4.1c, T5.2, T5.3** được đối chiếu lại ngày này
> (log: `docs/logs/WAVESHARE_SCREEN_VEHICLE_PROFILE_LOG.md`, `docs/logs/WAVESHARE_SCREEN_MAIN_FEATURES_LOG.md`); **T1.1–T1.4** từ 2026-10-03; các dòng còn lại
> giữ nguyên từ audit 2026-09-08 (HEAD `9dcf877`).

## Tổng hợp nhanh

| Giai đoạn | Tổng | ✅ XONG | 🟡 MỘT PHẦN | ❌ CHƯA |
|-----------|------|---------|--------------|---------|
| G0 — Tiếp quản & xác minh | 4 | 1 | 2 | 1 |
| G1 — Mở khả năng kiểm thử | 4 | 4 | 0 | 0 |
| G2 — Sửa 4 lỗi P0 | 4 | 4 | 0 | 0 |
| G3 — Hoàn thiện đề xuất | 5 | 2 | 3 | 0 |
| G4 — Tính mới đề tài | 2 | 1 | 1 | 0 |
| G5 — Xuyên suốt | 9 | 4 | 0 | 5 |
| **Tổng** | **28** | **16** | **6** | **6** |

> Số liệu trên được **đếm lại từ các dòng chi tiết bên dưới** ngày 2026-10-05 (⏳ tính vào ❌; T4.1a–d gộp thành 1 mục T4.1, vẫn 🟡).
> Dòng G2 và G5 của bản cũ đã lệch so với bảng chi tiết (T2.3 là ❌, G5 chỉ có 2 ✅) nên được sửa theo bảng chi tiết.

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
| T1.1 | Trình giả lập MQTT đúng schema + tham số scenario | ✅ XONG | `tools/test_mqtt_coreiot.py` chuẩn schema V2 + `--scenario` (15 kịch bản: 4 gốc approach/crossing/slam/normal + 11 tình huống quanh xe, xem `tools/scenarios.py`) |
| T1.2 | Trình mô phỏng LVGL + backend SDL trên PC (**đòn bẩy lớn nhất**) | ✅ XONG | `firmware/waveshare-screen/host_sim` chạy LVGL v9 + SDL2 (desktop lẫn headless CI) |
| T1.3 | Unit test DistanceFilter + sensor_model_classify | ✅ XONG | `test_distance_filter.cpp` + `test_thresholds.cpp` + `hazard_core_tests` (host_sim, 30 check) |
| T1.4 | Ghi & phát lại dữ liệu thật | ✅ XONG | `tools/record_telemetry.py` và `tools/replay_telemetry.py` chuẩn schema V2 |

## Giai đoạn 2 — Sửa 4 lỗi P0 (16/09–20/10, song song G1)

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T2.1 | Tạo buzzerTask (còi trước đây không bao giờ kêu) | ✅ XONG | `buzzer.cpp` có `buzzerTask()`; **DANGER kêu liên tục, CAUTION 1 lần/s** (commit `9dcf877`); build 2 env + flash sensor-node OK |
| T2.2 | Sửa banner kẹt vĩnh viễn DANGER | ✅ XONG | `ui_dashboard.c:145` — `if (readings[i].is_stale) continue;` skip slot chưa báo → hết kẹt banner |
| T2.3 | Sửa heuristic cảnh báo xe cắt ngang bị vô hiệu | ✅ XONG (host) | 2026-10-05: tính tại màn hình cho cả ESP-NOW lẫn MQTT; đã xoá đường ép từ cloud chết (`crossing_hazard`, `ui_dashboard_set_hazard_warning`). `hazard_eval_crossing` có trạng thái: bỏ slot mất kết nối (trước đây **mất link là báo giả**), so với mốc tham chiếu `CROSSING_WINDOW_MS`=500 (trước đây ở 10 khung/s vật 1 m/s không bao giờ được báo), giữ `CROSSING_HOLD_MS`=3000, chỉ xét 2 góc trước. `hazard_core_tests` 72 check (gồm dữ liệu kịch bản), ctest `umt_dash_sim_crossing_banner`. **Chưa quan sát trên board/xe; các ngưỡng là ước lượng** |
| T2.4 | Phát hiện mất kết nối & dữ liệu cũ (**quan trọng nhất về an toàn**) | ✅ XONG | `main.c:142 espnow_link_watchdog_cb` (LVGL timer 500ms) + `espnow_receiver.c` clear slot quá `ESPNOW_LINK_TIMEOUT_MS` + `is_stale` |

## Giai đoạn 3 — Hoàn thiện các yêu cầu trong đề xuất (14/10–01/12)

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T3.1 | Biểu tượng phương tiện/vật thể tại vị trí phát hiện | ✅ XONG (host) | 2026-10-05: `ui_dashboard_marker.c` vẽ nhãn vật thể bo tròn **hiện khoảng cách (cm)** trên trục búp cảm biến ở đúng khoảng cách đo (cùng tỷ lệ với thân xe), nền màu zone, chỉ hiện khi ≤ `SENSOR_CAUTION_CM`, ẩn khi mất dữ liệu; cùng số với thanh trái. Ảnh sim approach/boxed_in. Giới hạn: siêu âm không phân loại được phương tiện nên không có silhouette; vị trí là chỉ báo tầm (vật có thể lệch trong góc quét 75°) |
| T3.2 | Vẽ sơ đồ xe tải EX8 theo hồ sơ (không gắn cứng toạ độ) | 🟡 MỘT PHẦN | `vehicle_profile` + `vehicle_layout_compute()` + `build_truck_body()`: kích thước thân xe/cabin và vị trí + góc 6 cảm biến lấy từ hồ sơ, quy đổi mm→px theo tỷ lệ (host test 40 check, đã thử phá cố ý để chắc test bắt được). Còn hằng thẩm mỹ trong UI: canvas 440×440 + lề 50 ở `build_center_canvas`, vị trí trục bánh 70%/80%, cabin 90% bề ngang. Từ 2026-10-05 vị trí bánh lấy từ hồ sơ (`front_axle_mm` + `wheelbase_mm`, không còn hằng 70%/80%) và EX8 dùng **số hãng** cho dài 7370 / rộng 2028 / cơ sở 3850; **cabin, phần nhô trước và vị trí cảm biến vẫn là placeholder** |
| T3.3 | Cảnh báo âm thanh trong cabin | 🟡 MỘT PHẦN | Buzzer đã hoạt động (T2.1). **Xung đột GPIO47/48 đã giải quyết**: `BUZZER_PIN=11` (progress cũ ghi 48 — hiện code không còn). Còn thiếu: mạch khuếch đại 5V (transistor) + chụp tài liệu lắp đặt |
| T3.4 | Thống nhất các ngưỡng cảnh báo (**R3**) | ✅ XONG | Buzzer giờ dùng chung `SENSOR_CAUTION_CM=100`/`SENSOR_DANGER_CM=30` từ `firmware/shared/thresholds.h`; đã **xoá** `BUZZER_WARNING_DISTANCE_CM`/`BUZZER_DANGER_DISTANCE_CM`/`BUZZER_DANGER_PERIOD_MS`; grep firmware = 0; check_rulechain OK {100,30} |
| T3.5 | Cân chỉnh độ trễ bộ lọc cho vật chuyển động | 🟡 MỘT PHẦN | Từ 21/9 `distance_filter.cpp` có fast-track; 03/10 sửa thành **"nhanh vào – chậm ra"**: VÀO cần 2 mẫu sát nhau (≈ 0,1–0,2 s), RA cần `FILTER_RELEASE_CONFIRM_SAMPLES`=5 mẫu xa liên tiếp (≈ 0,5 s, nhả về mẫu gần nhất) — bản 21/9 nhả chỉ với 1 mẫu nên đầu ra nhảy theo echo ảo (log thật: 48% mẫu báo "xa" khi vật ở 22,6 cm). Có test native 19/19 và mô phỏng; **chưa cân chỉnh bằng dữ liệu thật** (cần mẫu thô — T1.4/T5.9) và chưa đo lại trên thiết bị với đúng cảnh cũ |

## Giai đoạn 4 — Làm rõ tính mới của đề tài (11/11–12/01/2027)

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T4.1a | Cấu trúc hồ sơ xe + nhập cứng hồ sơ EX8 | 🟡 MỘT PHẦN | Cấu trúc xong: `vehicle_profile_t` có `id`, `front_axle_mm`, `wheelbase_mm`; `validate`, registry, `set_active`; bánh xe vẽ theo hồ sơ. EX8: dài 7370 / rộng 2028 / cơ sở 3850 là **số hãng** (bảng thông số người dùng gửi 2026-10-05); **cabin, phần nhô trước, vị trí + góc 6 cảm biến vẫn placeholder** — cần đo trên xe thật. Host test 99/99 |
| T4.1b | 2 hồ sơ nữa + màn chọn + lưu NVS | 🟡 MỘT PHẦN | Đã có 3 hồ sơ (2 hồ sơ phụ là placeholder), tab SETUP chọn hồ sơ, `vehicle_settings` (blob + CRC) + adapter NVS `vehicle_store`, nạp trước khi dựng UI; sơ đồ xe dựng lại tại chỗ khi đổi hồ sơ. Host: `vehicle_settings_tests` 113/113, stress 120 lần không rò, test UI chuột ảo; firmware build xem log. **Chưa kiểm chứng NVS trên board** (không có board): chưa thấy log boot / giữ hồ sơ sau reset |
| T4.1c | Chỉnh tay từng cảm biến ghi đè profile | 🟡 MỘT PHẦN | Bộ chỉnh trong tab SETUP (chọn slot, −/+ x, y, góc; Apply/Reset; từ chối vị trí ngoài khung xe), API `vehicle_settings_set_override/clear_override`, lưu NVS cùng blob. Host: `vehicle_override_tests` 37/37; ảnh sim: FRONT lùi 200 mm → dịch đúng 9 px, Reset trả về đúng từng pixel. **Chưa kiểm chứng trên board** |
| T4.1d | Người dùng tự tạo hồ sơ mới | ❌ CHƯA | Chưa có (để dành giai đoạn sau; blob NVS có trường `version` để nâng cấp) |
| T4.2 | Gom bản đồ vị trí cảm biến 1 nguồn duy nhất (**SENSOR_COUNT từ sizeof**) | ✅ XONG | `thresholds.h:112` `#define SENSOR_COUNT (sizeof(SENSOR_PINS)/sizeof(SENSOR_PINS[0]))` + `static_assert(SENSOR_COUNT==6)` (R4) |

## Giai đoạn 5 — Việc làm xuyên suốt

### Vệ sinh kỹ thuật

| ID | Nhiệm vụ | Trạng thái | Bằng chứng trên code hiện tại |
|----|----------|-----------|-------------------------------|
| T5.1 | Wi-Fi password & CoreIoT token ra khỏi header được commit | ✅ XONG | `credentials.h` sinh bởi `tools/guard/gen_credentials.py` từ `config/keys.json` (gitignored); `scan_secrets.py` sạch; CI chạy Gitleaks |
| T5.2 | Nút Calibrate chưa có callback | ✅ XONG | 2026-10-05: `calib_btn` mở trang SETUP (chỉnh vị trí/hướng cảm biến, T4.1c); ctest `umt_dash_sim_calibrate_button` (chuột ảo) |
| T5.3 | Sửa comment sai ID cảm biến (`ui_dashboard.h`) | ✅ XONG | 2026-10-05: rà toàn firmware; sửa `espnow_protocol.h` (ghi sai "thứ tự slot == SENSOR_PINS"; nay ghi rõ ánh xạ qua `SENSOR_ESPNOW_SLOT[]` và nhãn S1..S6 = slot 0..5) và `main.c` (chu kỳ gửi 100 ms, không phải 500 ms). `sensor_model.h`, `ui_dashboard_theme.h` đã đúng |
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

1. **G2 P0 đã xong trên code** *(cập nhật 2026-10-05)*: 4/4 lỗi P0 có fix (buzzer, banner, mất kết nối, xe cắt ngang). T2.3 mới được kiểm chứng trên host (unit + sim), chưa trên board.
2. **G1 đã mở khả năng kiểm thử** *(cập nhật 2026-10-03)*: đã có `--scenario`, host_sim LVGL + SDL, record/replay. T3.5, T5.7, T5.9 hết bị chặn về công cụ nhưng vẫn cần **dữ liệu thật** để đo. (Audit 2026-09-08 ghi G1 là lỗ hổng số 1 vì khi đó chưa có các công cụ này.)
3. **T3.4 đã xong sớm hơn dự kiến** nhờ sửa buzzer (commit `9dcf877`): ngưỡng còi nay đồng bộ 100/30 với zone chung — đúng yêu cầu "thống nhất ngưỡng", không phải chỉnh giao diện.
4. **T0.4 phần cứng còn nợ**: chưa có ảnh chụp hiện trạng đấu dây trong repo; theo plan đây là việc "làm đầu tiên, không để lại sau".
5. **Báo cáo chưa có số đo**: report/ mới có thiết kế + lập luận khả thi; muốn nói trước hội đồng phải đổ dữ liệu T5.x vào `docs/PROGRESS.md`.

## Thứ tự ưu tiên đề xuất cho phiên sau

1. **G1 trước** — T1.1 (scenario), T1.4 (record/replay sớm vì cần board), T1.2 (LVGL SDL sim), T1.3 (classify test) → theo `docs/roadmaps/next-branch.roadmap.json` (15 bước, 4 giai đoạn). **Đã xong** *(cập nhật 2026-10-03)*.
2. ~~**T2.3** (P0 sót) khi đang làm G2.~~ **Đã xong (host) 2026-10-05** — còn quan sát trên board.
3. Nghiệm thu T0.4: chụp ảnh hiện trạng + ghi log bằng chứng **trước khi tháo bất cứ thứ gì**.
4. Sau khi có T1.2 → làm T3.1 (icon), T3.2 (EX8) trên simulator. **Bản đầu đã có** *(2026-10-03)*: xe tải vẽ từ hồ sơ + marker chấm tròn; còn thiếu số đo EX8 thật và quyết định về icon.
5. Sau khi có T1.4 → T3.5 (cân lọc), T5.9 (báo nhầm), rồi T5.5–T5.8 (đo + soak) → số liệu vào PROGRESS.