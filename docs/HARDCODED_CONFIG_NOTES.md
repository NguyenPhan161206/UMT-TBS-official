# Ghi chú config cứng / khó bảo trì (Hardcoded Config Notes)

> Rà soát 2026-09-09 (quét literal toàn `firmware/`, trừ `.pio`/`managed_components`).
> Mục đích: **note lại** các chỗ dữ liệu số cấu hình bị "cứng" rải rác — danh sách
> nguồn để xử lý dần (đã đưa vào checklist roadmap, xem `docs/roadmaps/next-branch.roadmap.json`).
>
> ⚠️ KHÔNG phải danh sách "phải sửa ngay". Mức độ: A/B/C (A = nguy cơ drift thật).
> Chỉ sửa khi bước roadmap tương ứng chạm file đó, tránh làm phình scope G1.

---

## A. Threshold/ngưỡng bị copy thành CHUỖI text → drift (vi phạm tinh thần R3)

Khi đổi `SENSOR_CAUTION_CM`/`SENSOR_DANGER_CM` ở `firmware/shared/thresholds.h:44-45`,
những chỗ này **sai lệch mà mọi guard hiện tại không bắt** (check_rulechain chỉ đối
chiếu rule-chain; scan_secrets không liên quan):

| Vị trí | Giá trị cứng | Vấn đề |
|---|---|---|
| `firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_layout.c:299-301` | `"> 100cm : Safe"`, `"30-100cm : Caution"`, `"< 30cm : Danger"` | Legend UI copy ngưỡng thành string. Đổi ngưỡng = legend sai im lặng |
| `tools/test_mqtt_coreiot.py:41-42` | `CAUTION_CM = 100.0`, `DANGER_CM = 30.0` | Python mirror ngưỡng (drift risk đã biết) — guard mới `arch_guard.py` (B5) sẽ đối chiếu |

**Xử lý đề xuất:**
- Legend UI → `lv_label_set_text_fmt(... "%d-%d cm", SENSOR_DANGER_CM, SENSOR_CAUTION_CM)`
  → self-correct khi đổi ngưỡng (gắn step host_sim/T3.x).
- Python mirror → `arch_guard.py` check `CAUTION_CM/DANGER_CM == SENSOR_*_CM` (sinh kèm step 1).

## B. Hình học cảm biến nằm ở 2 nơi + 1 dead-data

Cùng "vị trí/hướng 6 cảm biến quanh xe" nhưng mã hóa 2 lần với 2 convention:

| Vị trí | Nội dung | Trạng thái |
|---|---|---|
| `sensor_model/sensor_model.c:13-20` `k_offsets_deg[]` (FRONT=0, REAR=180, LEFT_FRONT=-90…) → ghi `offset_deg` | Vị trí cảm biến theo độ | **DEAD**: grep toàn firmware = 2 (set ở `:38` + field `sensor_model.h:34`), **không nơi nào đọc** |
| `ui_dashboard/ui_dashboard_layout.c:229-236` `k_layout[]` `{x, y, angle}` | Vị trí + góc vẽ arc (LVGL angle convention) | Đang dùng (nơi điều khiển arc) |

→ Cùng thông tin, 2 chủ thể, không có cọc "1 nguồn". T3.2 (sơ đồ EX8) bắt buộc gộp lại.

**Xử lý đề xuất:** bỏ `offset_deg` dead-field + `k_offsets_deg[]`; giữ MỘT bảng geometry
(vd chuyển lên một `vehicle_layout.h`/profile struct khi dựng EX8 ở T3.2). DoD: grep
`offset_deg` trong firmware (trừ .pio) = 0.

## C. Timing / tuning ẩn dưới literal không tên (không grep được, khó tune)

### sensor-node (`src/main.cpp`, `buzzer.cpp`, `shared_state.cpp`)
| Vị trí | Literal | Ý nghĩa |
|---|---|---|
| `src/main.cpp:136` | `pdMS_TO_TICKS(500)` | Chờ cảm biến ổn định sau power-on |
| `src/main.cpp:235,287` | `pdMS_TO_TICKS(20)` | Poll tick network/coreiot task |
| `src/buzzer.cpp:83` | `pdMS_TO_TICKS(20)` | Poll tick buzzer task |
| `src/shared_state.cpp:27,42,54` | `pdMS_TO_TICKS(10)` | Mutex wait timeout |
| `src/main.cpp:299` | `delay(200)` | Chờ Serial/USB CDC sẵn sàng |
| `src/main.cpp:298` | `Serial.begin(115200)` | Baudrate debug |
| `src/main.cpp:308,319,329,340` | stack `4096/4096/2048/4096` | Kích thước stack 4 task |
| `src/main.cpp:310,321,331,339` | priority `2/1/1/1` | Priority các task |
| `src/main.cpp:270` | `char payload[256]` | Buffer telemetry JSON |
| `src/plugins/coreiot/coreiot_client.h:15,18` | `COREIOT_PUBLISH_INTERVAL_MS 500`, `COREIOT_MQTT_RETRY_INTERVAL_MS 3000` | ✅ đã đặt tên (define trong header) — OK |

### waveshare-screen
| Vị trí | Literal | Ý nghĩa |
|---|---|---|
| `components/ui_dashboard/ui_dashboard.c:129` | `lv_timer_create(sys_info_timer_cb, 2000, ...)` | Chu kỳ cập nhật SYSTEM page |
| `components/ui_dashboard/ui_dashboard_layout.c:35-36` | `lv_anim_set_time(..., 400)` | Chu kỳ blink cảnh báo |
| `components/ui_dashboard/ui_dashboard.c:101` | `lv_obj_set_size(content, LV_PCT(100), 440)` | Chiều cao content (440 vs screen 480) |
| `components/coreiot_client/coreiot_client.c:39-40` | `MQTT_DOWN_DEBOUNCE_MS 6000`, `WIFI_RECONNECT_RETRY_MS 3000` | ✅ đã đặt tên (define đầu file) — OK |

**Xử lý đề xuất:** gom thành 1 config header per-firmware (vd `components/cfg/task_cfg.h`
hoặc section riêng); bước này THẤP ưu tiên — các mục đã có tên (`COREIOT_*`, `MQTT_*`)
không cần động.

## D. Được loại (KHÔNG phải vấn đề — ghi để khỏi tái hỏi)

- **Số px layout LVGL** (`ui_dashboard_layout.c`: 66 literal 180/28/160/260/440…): pixel-
  precise là bản chất UI — không trừu tượng hóa.
- **Hằng vật lý / đổi đơn vị** (`ui_dashboard_system.c`: 3600/1024/100…; 360°, µs↔ms):
  mang ý nghĩa cố hữu.
- **Ngưỡng trong `test_thresholds.cpp:19-24`** (`TEST_ASSERT_EQUAL_INT(100,...)`):
  CỐ Ý "chốt" giá trị như test, không phải cấu hình.
- **`ESPNOW_SENSOR_SLOT_COUNT 6` + `static_assert`** (`espnow_protocol.h:45,68`): chặn
  mở rộng sensor vô trách nhiệm — có chủ đích, không phải khó mở rộng.
- **Tham số đo JSN-SR04T** (`thresholds.h`), **GPIO** (`SENSOR_PINS`): đã tập trung
  đúng nơi (R3/R4).

---

## Bảng "blast radius" (xử lý xong theo đề xuất = sửa 1 chỗ, đụng 1 chỗ)

| Muốn đổi... | Chạm đúng... | Được bảo vệ bởi |
|---|---|---|
| Ngưỡng zone 30/100 | `thresholds.h` (1 dòng) | `arch_guard.py` (B5) + `check_rulechain_thresholds.py` |
| Legend hiển thị zone | tự đổi theo threshold (nếu đã làm macro) | không — đang là string cứng (mục A) |
| Heuristic crossing | `hazard_core.c` | test boundary CI |
| Hình học 6 cảm biến / sơ đồ EX8 | 1 bảng geometry duy nhất (sau mục B) | — (đang 2 chỗ) |
| Vị trí/giao diện widget | `ui_dashboard_layout.c` | host_sim (step 3) |
| Số cảm biến | thêm/bớt `SENSOR_PINS` | `static_assert` R4 tự chặn lệch |

## Checklist nguồn → roadmap
Đã chuyển các mục A, B, C thành các mục xử lý trong
`docs/roadmaps/next-branch.roadmap.json` (step mới để dọn config + nới các step liên quan).