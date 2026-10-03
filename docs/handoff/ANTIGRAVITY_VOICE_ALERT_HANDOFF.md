# HANDOFF cho Antigravity — Cảnh báo giọng nói thay còi bíp (sensor-node)

> Tài liệu tự đủ. Làm **tuần tự từng step**, verify xong mới sang step kế, cập nhật
> `docs/roadmaps/voice-alert.state.md` sau mỗi step. Roadmap: `docs/roadmaps/voice-alert.roadmap.json`.
> Step 4 độc lập, có thể làm sớm; step 8 BLOCKED tới khi người dùng có loa + amp.

## 0. Quyết định đã chốt với người dùng
- Phát âm thanh trên **sensor-node** (ESP32-S3, Arduino) — chưa có phần cứng âm thanh → cần tài liệu chọn/đấu linh kiện.
- Nguồn tiếng: **clip ghi sẵn** nhúng trong flash (không TTS chạy trên chip, không TTS qua mạng).
- Nội dung: **tiếng Việt, theo vị trí**: 6 vị trí × 2 mức = **12 clip**.
- Chưa có file giọng thật → dùng **WAV placeholder** (âm hiệu sin, KHÔNG phải giọng nói) để kiểm chứng toàn pipeline; người dùng thay sau.

## 1. Bối cảnh code hiện tại (đã xác minh)
- `firmware/sensor-node/src/buzzer.cpp`: `buzzerTask` đọc `sharedStateGetNearest()` (chỉ khoảng cách nhỏ nhất, **không biết vị trí**),
  DANGER (`<= SENSOR_DANGER_CM=30`) kêu liên tục, CAUTION (`<= SENSOR_CAUTION_CM=100`) bíp mỗi `BUZZER_WARNING_PERIOD_MS=1000`,
  tắt nếu `sharedStateGetMute()` (nút Mute từ màn hình qua ESP-NOW) hoặc không có cảm biến hợp lệ. `BUZZER_PIN=11` (`firmware/shared/thresholds.h`).
- `shared_state.h/.cpp`: có `sharedStateGet/GetHealth/GetNearest/GetMute`. Chưa có hàm trả **chỉ số** cảm biến gần nhất.
- `src/main.cpp` giữ bảng `SENSOR_ESPNOW_SLOT[]` (chân vật lý → wire slot). Tạo buzzer task ở ~dòng 368. Có `warnIfReservedPin(...)`.
- PlatformIO: `platformio.ini` env `yolo_uno`, `yolo_uno_coreiot`, `native` (unity, chỉ biên dịch `distance_filter.cpp` + test). Board `yolo_uno`: 8 MB flash, partition `default_8MB.csv`. `platform = espressif32@7.0.1`, `framework = arduino`.
- Thứ tự **chân vật lý** `SENSOR_PINS[]`: 0 FRONT, 1 LEFT_FRONT, 2 RIGHT_FRONT, 3 LEFT_REAR, 4 RIGHT_REAR, 5 REAR.
  Thứ tự **wire slot** `espnow_slot_t` (`firmware/shared/espnow_protocol.h`): FRONT=0, REAR=1, LEFT_FRONT=2, LEFT_REAR=3, RIGHT_FRONT=4, RIGHT_REAR=5.
  **Hai thứ tự khác nhau — mọi nơi cần chọn clip phải đi qua `SENSOR_ESPNOW_SLOT[]`.** Dùng nhầm = báo sai phía (lỗi an toàn nghiêm trọng).

## 2. Luật repo & an toàn
- **R2/R3:** hằng dùng chung và ngưỡng cảnh báo chỉ ở `firmware/shared/thresholds.h` (kể cả `BUZZER_PIN`, nên chân I2S và hằng `VOICE_*` cũng để ở đó). Không define trùng ở nơi khác.
- **R6:** module chưa dùng phải được build trong ≥1 env → `buzzer.cpp` và `voice_*` **luôn được biên dịch**, chọn bằng cờ runtime/macro `USE_VOICE_ALERT`.
- **R7:** mỗi file ≤ 400 dòng.
- Cảnh báo giọng nói **chỉ** dựa trên cảm biến hợp lệ (đã loại DISCONNECTED/STALE bởi `sharedStateGetNearest*`). Cảm biến mất kết nối **không được** xướng cảnh báo.
- Mute từ màn hình phải **dừng tiếng ngay** (không đợi hết clip).
- Không `delay()` chặn trong task mạng/đo; mọi chờ trong task âm thanh dùng `vTaskDelay`.
- Không commit credential; `git add <file cụ thể>`, cấm `git add -A`/`.`; chỉ sửa file trong "Target files" của step.
- Comment tiếng Việt cùng style code xung quanh. Sau cùng bắt buộc log `docs/logs/SENSOR_NODE_VOICE_ALERT_LOG.md`.

## 3. Contract (chữ ký chính xác)

### Thứ tự clip (cố định, generator + policy + test dùng chung)
`id = zone_block*6 + wire_slot`, `zone_block`: CAUTION=0, DANGER=1; `wire_slot` theo `espnow_slot_t`.
| id | clip | id | clip |
|---|---|---|---|
| 0 | caution_front | 6 | danger_front |
| 1 | caution_rear | 7 | danger_rear |
| 2 | caution_left_front | 8 | danger_left_front |
| 3 | caution_left_rear | 9 | danger_left_rear |
| 4 | caution_right_front | 10 | danger_right_front |
| 5 | caution_right_rear | 11 | danger_right_rear |

Câu mẫu (`tools/voice/phrases.json`): caution = "Cẩn thận, phía trước / phía sau / trái trước / trái sau / phải trước / phải sau";
danger = "Nguy hiểm! phía trước / phía sau / trái trước / trái sau / phải trước / phải sau". Mỗi clip ≤ 1,5 s.

### Định dạng audio
WAV PCM **16 kHz, mono, 16-bit**. Generator từ chối định dạng khác với thông báo rõ. Chuyển file thật bằng
`ffmpeg -i in.mp3 -ar 16000 -ac 1 -sample_fmt s16 out.wav`.

### `include/voice_clips.h` (sinh tự động, commit)
```c
#define VOICE_CLIP_COUNT 12
typedef struct { const int16_t *samples; uint32_t sample_count; } voice_clip_t;
extern const voice_clip_t VOICE_CLIPS[VOICE_CLIP_COUNT];   /* mảng mẫu nằm trong flash (const) */
#define VOICE_SAMPLE_RATE_HZ 16000
```
`gen_voice_clips.py --check` phải exit 1 nếu file sinh ra khác file đang commit.

### Hằng mới trong `firmware/shared/thresholds.h` (R3)
```c
#define VOICE_CAUTION_REPEAT_MS     4000  /* lặp CAUTION khi vẫn còn */
#define VOICE_DANGER_REPEAT_GAP_MS  1200  /* nghỉ giữa 2 lần xướng DANGER */
#define VOICE_MIN_GAP_MS             800  /* nghỉ tối thiểu giữa 2 clip khác nhau (chống spam) */
#define VOICE_REARM_SAFE_MS         2000  /* phải ở SAFE bấy nhiêu ms mới xướng lại từ đầu */
/* Chân I2S (đề xuất; PHẢI xác minh free — xem step 5) */
#define VOICE_I2S_BCLK_PIN 12
#define VOICE_I2S_LRC_PIN  13
#define VOICE_I2S_DOUT_PIN 14
```

### `include/voice_policy.h` (C++ thuần, không Arduino/FreeRTOS)
```cpp
typedef struct {
    bool     has_nearest;   /* sharedStateGetNearestIndex trả true */
    float    nearest_cm;
    uint8_t  nearest_slot;  /* WIRE slot (espnow_slot_t) — đã đổi qua SENSOR_ESPNOW_SLOT */
    bool     muted;
    bool     playing;       /* voicePlayerIsPlaying() */
} voice_input_t;

typedef enum { VOICE_ACT_NONE = 0, VOICE_ACT_PLAY, VOICE_ACT_STOP } voice_act_t;
typedef struct { voice_act_t type; int clip; } voice_action_t;   /* clip hợp lệ khi PLAY */

typedef struct { /* trạng thái nội bộ: last_clip, last_start_ms, last_end_ms, prev_playing, safe_since_ms, current_clip... */ } voice_policy_state_t;

void           voice_policy_init(voice_policy_state_t *s);
voice_action_t voice_policy_step(voice_policy_state_t *s, const voice_input_t *in, uint32_t now_ms);
```
Zone suy ra từ `nearest_cm` bằng ngưỡng `SENSOR_DANGER_CM`/`SENSOR_CAUTION_CM` (cùng quy tắc như `buzzer.cpp`: `<= DANGER` là danger; `<= CAUTION` là caution).

**Bảng luật (bắt buộc, test phải phủ):**
1. `muted` → nếu đang phát thì `STOP`, ngược lại `NONE`.
2. `!has_nearest` hoặc SAFE → `NONE` (clip đang phát được đọc nốt, không cắt). Ghi nhận thời điểm vào SAFE; chỉ khi SAFE liên tục ≥ `VOICE_REARM_SAFE_MS` thì xoá `last_clip` (lần sau xướng ngay). Chống chatter quanh biên 100 cm.
3. DANGER:
   - đang phát clip CAUTION → `PLAY(danger clip)` ngay (giành quyền);
   - đang phát clip DANGER → `NONE` (không ngắt, kể cả đổi slot);
   - không phát: `PLAY` nếu clip khác `last_clip` (và đã qua `VOICE_MIN_GAP_MS` từ lúc clip trước kết thúc) hoặc `now − last_end ≥ VOICE_DANGER_REPEAT_GAP_MS`.
4. CAUTION:
   - đang phát bất kỳ clip → `NONE`;
   - không phát: `PLAY` nếu clip khác `last_clip` (qua `VOICE_MIN_GAP_MS`) hoặc `now − last_end ≥ VOICE_CAUTION_REPEAT_MS`.
5. `last_end` được ghi nhận khi `prev_playing` chuyển true→false. `PLAY` cập nhật `last_clip`, `last_start_ms`.
6. Tương quan `clip id`: `(zone==DANGER ? 6 : 0) + nearest_slot`; `nearest_slot >= 6` → `NONE`.

### `include/voice_player.h`
```cpp
bool voicePlayerBegin();                 /* khởi tạo I2S + task phát; false nếu lỗi (không crash, log lý do) */
void voicePlayerPlay(int clipId);        /* không chặn; ngắt clip đang phát; clipId ngoài [0,COUNT) bỏ qua */
void voicePlayerStop();                  /* dừng ngay, xả im lặng */
bool voicePlayerIsPlaying();
```
Dùng `ESP_I2S.h` nếu Arduino core đi kèm `espressif32@7.0.1` có; nếu không dùng `driver/i2s_std.h` (ESP-IDF). Ghi lựa chọn vào log. Ghi theo khối ≤ 512 mẫu, kiểm cờ ngắt giữa các khối. Khi dừng/kết thúc, gửi một khối zero để amp không phát tiếng lạch cạch.

### `include/voice_alert.h`
```cpp
void voiceAlertTask(void *pvParameters);   /* FreeRTOS task: poll TASK_POLL_INTERVAL_MS */
```
Trong vòng lặp: `sharedStateGetNearestIndex(idx, cm)` → `slot = SENSOR_ESPNOW_SLOT[idx]` → `voice_policy_step` → `voicePlayerPlay/Stop`.

### `shared_state` thêm
```cpp
bool sharedStateGetNearestIndex(size_t &sensorIndex, float &distanceCm);  /* cùng quy tắc hợp lệ/health như sharedStateGetNearest */
```
`include/sensor_slot_map.h`: `static const uint8_t SENSOR_ESPNOW_SLOT[SENSOR_COUNT] = {...};` — **giữ nguyên giá trị/thứ tự hiện có trong main.cpp** (sao chép nguyên, đừng viết lại từ trí nhớ).

### Phần cứng (đề xuất, người dùng chưa có — step 7 viết vào tài liệu)
- Module **MAX98357A** (I2S class-D, mono) + loa 4–8 Ω 3–5 W (cabin xe tải ồn → ưu tiên loa ≥ 5 W, đường kính ≥ 50 mm).
- Đấu: `BCLK→GPIO12`, `LRC→GPIO13`, `DIN→GPIO14`, `VIN→5 V`, `GND→GND chung`, `SD` để hở (mono mix) hoặc kéo lên 3V3 qua trở; `GAIN` mặc định.
- Nguồn xe tải thường 24 V → cần bộ hạ áp 5 V (buck) đủ ≥ 1 A, chống nhiễu; chung GND với sensor-node.
- **GPIO 12/13/14 là đề xuất**: phải xác minh với danh sách GPIO cấm trong `docs/HARDWARE_INSTALLATION.md` (47/48 PSRAM, 26–32 flash SPI0, 19/20 USB) và không trùng `SENSOR_PINS` (3,4,5,6,7,8,9,10,17,18,21,38) và `BUZZER_PIN=11`. Nếu trùng/cấm thì chọn chân khác và cập nhật mọi nơi (thresholds.h là nguồn duy nhất).

---

## 4. WORKER PROMPT — làm lần lượt

### 🛠 STEP 1 — tools/voice (WAV → mảng C)
**Target files:** `tools/voice/phrases.json`, `tools/voice/gen_voice_clips.py`, `tools/voice/test_gen_voice_clips.py` (create).
**Objective:** theo §3 (thứ tự clip, định dạng WAV, đầu ra `voice_clips.h` + `voice_clips_gen.cpp`). Chỉ dùng thư viện chuẩn Python (`wave`, `struct`, `argparse`, `json`, `math`). Cờ: `--wav-dir` (mặc định `firmware/sensor-node/assets/voice`), `--out-dir`, `--placeholder` (sinh 12 WAV sin 16 kHz mono 16-bit, mỗi clip tần số/độ dài khác nhau để phân biệt, độ dài ≤ 1 s), `--check`. Đầu ra **deterministic** (không timestamp). Validate: từ chối stereo / ≠16 kHz / ≠16-bit / >1,5 s / thiếu file với thông báo nêu tên file.
**Ràng buộc:** không thêm dependency pip; không đụng firmware.
**DoD:** `python3 -m pytest tools/voice/test_gen_voice_clips.py -q` pass với test cho: 4 loại WAV sai, thứ tự clip (id↔tên ↔ espnow_slot_t), deterministic (2 lần chạy trùng byte), `--check` exit 1 khi lệch, `--check` exit 0 khi khớp. Test dùng thư mục tạm (`tmp_path`), không ghi vào firmware.

### 🛠 STEP 2 — Sinh tài sản placeholder
**Context:** Step 1 xong. **Target files:** `firmware/sensor-node/include/voice_clips.h`, `firmware/sensor-node/src/voice_clips_gen.cpp`, `firmware/sensor-node/assets/voice/README.md` (+ 12 WAV placeholder tạo ra trong `assets/voice/`: `caution_front.wav`, `caution_rear.wav`, `caution_left_front.wav`, `caution_left_rear.wav`, `caution_right_front.wav`, `caution_right_rear.wav`, và 6 file `danger_*.wav` tương ứng).
**Việc:** chạy `python3 tools/voice/gen_voice_clips.py --placeholder`, rồi sinh header/cpp. README ghi rõ: file hiện là **placeholder (âm hiệu sin, không phải giọng nói)**, cách thay bằng giọng thật (TTS tiếng Việt hoặc tự thu → `ffmpeg` ra 16 kHz mono 16-bit → chép đè → chạy lại generator → `--check`), danh sách câu ở `tools/voice/phrases.json`.
**DoD:** `python3 tools/voice/gen_voice_clips.py --check` exit 0; `VOICE_CLIP_COUNT 12`; tổng mảng ≤ 700 KB (in ra số byte); mảng là `const` (nằm flash).

### 🛠 STEP 3 — voice_policy + test native
**Context:** Step 2 xong (`VOICE_CLIP_COUNT`). **Target files:** `include/voice_policy.h`, `src/voice_policy.cpp`, `test/test_voice_policy.cpp`, `platformio.ini`, `firmware/shared/thresholds.h` (hằng `VOICE_*` §3, **không** thêm chân I2S ở step này).
**Objective:** hiện thực đúng bảng luật §3. `platformio.ini` env `native`: thêm `+<voice_policy.cpp>` vào `build_src_filter` và `+<test_voice_policy.cpp>` vào `test_filter`. Test (unity, theo mẫu `test/test_distance_filter.cpp`) phải phủ tối thiểu: (a) SAFE/`!has_nearest` im lặng; (b) CAUTION xướng 1 lần rồi lặp đúng `VOICE_CAUTION_REPEAT_MS`; (c) CAUTION→DANGER ngắt ngay (PLAY clip danger đúng slot); (d) DANGER đang phát không bị đổi slot/CAUTION ngắt; (e) đổi slot trong CAUTION xướng lại nhưng không sớm hơn `VOICE_MIN_GAP_MS`; (f) dao động 98↔103 cm liên tục **không** tạo chuỗi PLAY dồn dập (đếm số PLAY trong 10 s ≤ 3); (g) muted → STOP ngay khi đang phát, im khi không phát; (h) vào lại sau ≥ `VOICE_REARM_SAFE_MS` ở SAFE thì xướng ngay; (i) slot ≥ 6 → NONE; (j) clip id đúng `(danger?6:0)+slot` cho cả 12 tổ hợp.
**DoD:** `cd firmware/sensor-node && pio test -e native` PASS (gồm test cũ); thử đảo điều kiện (ví dụ cho DANGER bị ngắt) → ít nhất 1 test FAIL, rồi hoàn nguyên và ghi vào báo cáo.

### 🛠 STEP 4 — Nearest index + tách slot map
**Target files:** `include/shared_state.h`, `src/shared_state.cpp`, `include/sensor_slot_map.h` (create), `src/main.cpp` (modify).
**Objective:** theo §3. Đọc kỹ `sharedStateGetNearest` hiện có để **cùng quy tắc** hợp lệ/health. Chuyển `SENSOR_ESPNOW_SLOT[]` sang header mới (sao chép nguyên văn từ `main.cpp`), `main.cpp` include header và xoá bản cũ.
**DoD:** `pio run -e yolo_uno` + `pio run -e yolo_uno_coreiot` + `pio test -e native` thành công; `grep -rn "SENSOR_ESPNOW_SLOT\[" firmware/sensor-node/src firmware/sensor-node/include` chỉ định nghĩa ở `sensor_slot_map.h` (ngoài chỗ sử dụng); telemetry/ESP-NOW không đổi. (Lưu ý `test/test_thresholds.cpp` có bản sao riêng để kiểm thử — không đụng.)

### 🛠 STEP 5 — voice_player (I2S)
**Context:** Step 2 xong. **Target files:** `include/voice_player.h`, `src/voice_player.cpp`, `firmware/shared/thresholds.h` (thêm 3 chân I2S).
**Objective:** theo §3. Trước khi chốt chân, đối chiếu danh sách GPIO cấm trong `docs/HARDWARE_INSTALLATION.md` và `SENSOR_PINS`; thêm `static_assert`/kiểm tra trong `test/test_thresholds.cpp`? — **không**, file đó ngoài danh sách; thay vào đó dùng `TBS_STATIC_ASSERT` trong `thresholds.h` để chặn chân I2S trùng `BUZZER_PIN`/chân cố định, và nêu trong báo cáo cách bạn đã kiểm tra trùng với SENSOR_PINS. Task phát nên ghim core 0, ưu tiên thấp hơn SensorTask. `voicePlayerBegin()` thất bại phải trả false và không treo.
**DoD:** build 2 env SUCCESS; không malloc/new theo mỗi lần phát; `pio test -e native` vẫn PASS (không kéo Arduino vào native env).

### 🛠 STEP 6 — voice_alert + cờ USE_VOICE_ALERT
**Context:** Step 3, 4, 5 xong. **Target files:** `include/voice_alert.h`, `src/voice_alert.cpp`, `src/main.cpp`, `platformio.ini`.
**Objective:** theo §3. `platformio.ini`: thêm `-D USE_VOICE_ALERT=1` vào `[env] build_flags`. `main.cpp`: `#if USE_VOICE_ALERT` tạo `voiceAlertTask` (gọi `voicePlayerBegin()` trước; nếu false → log và **fallback sang buzzerTask**), ngược lại tạo `buzzerTask` như cũ; thêm `warnIfReservedPin` cho 3 chân I2S khi `USE_VOICE_ALERT`; sửa comment đầu file mô tả task. Không xoá `buzzer.cpp`.
**DoD:** `pio run -e yolo_uno`, `pio run -e yolo_uno_coreiot` SUCCESS; `PLATFORMIO_BUILD_FLAGS="-DUSE_VOICE_ALERT=0" pio run -e yolo_uno` SUCCESS; báo cáo dung lượng flash từng build (≤ 80% partition app); `pio test -e native` PASS. Nếu có board + amp: mô tả quan sát (nghe được clip nào, đúng phía không); nếu chưa có thì ghi rõ "chưa kiểm chứng trên phần cứng".

### 🛠 STEP 7 — Tài liệu
**Target files:** `docs/HARDWARE_INSTALLATION.md`, `docs/INSTALLATION_SENSOR_NODE.md`, `docs/logs/SENSOR_NODE_VOICE_ALERT_LOG.md`, `docs/CHECKLIST.md`.
**Việc:** thêm mục "Âm thanh cảnh báo (I2S)": BOM, bảng đấu dây (khớp chân trong `thresholds.h`), lưu ý nguồn xe tải 24 V → 5 V, chung GND, vị trí đặt loa, ồn cabin, cách thay giọng thật. Lưu ý cả hai tài liệu có bảng "S0..S5" theo **chỉ số chân vật lý** — không trộn với wire slot khi viết. Log theo AGENTS.md (mục tiêu, file sửa, kết quả test, cách vận hành, TODO). CHECKLIST: cập nhật dòng T3.3 (giọng nói thay còi; **phần cứng chưa nghiệm thu; giọng đang placeholder**).
**DoD:** `python3 tools/guard/scan_secrets.py` và `python3 -m pytest tools/guard/test_guard.py tools/voice/test_gen_voice_clips.py -q` pass; log nêu rõ hai TODO trên.

### 🛠 STEP 8 — Nghiệm thu phần cứng (BLOCKED nếu chưa có loa + amp)
**Target file:** `docs/TEST_PROTOCOL.md`. Thêm mục "Cảnh báo giọng nói": bảng ca (6 vị trí × CAUTION/DANGER xướng đúng phía; đổi vị trí; Mute từ màn hình dừng ngay; rút dây cảm biến → không xướng cảnh báo cho nó; DANGER ngắt CAUTION; vào/ra SAFE không spam; mức ồn nghe được ở vị trí tai người lái) + cột Kết quả/Ghi chú. Nếu chưa có phần cứng: ghi "chưa thực hiện", không điền kết quả giả.
**DoD:** `grep -c "Cảnh báo giọng nói" docs/TEST_PROTOCOL.md` ≥ 1.

---

## 5. Ngoài phạm vi
- Không TTS trên chip/qua mạng, không nhận dạng giọng nói, không đa ngôn ngữ chạy runtime.
- Không đổi giao thức ESP-NOW, bộ lọc khoảng cách, UI màn hình (nhãn "BUZZER:" trên màn có thể đổi tên sau — việc riêng).
- Không tự bịa file giọng thật; không commit/push nếu người dùng chưa yêu cầu.

## 6. Báo cáo cuối cho người dùng
Danh sách step DONE/BLOCKED, output verify, dung lượng flash, và nhắc rõ 3 việc người dùng còn phải làm:
**(1)** mua/đấu loa + MAX98357A + nguồn 5 V; **(2)** tạo giọng tiếng Việt thật cho 12 câu rồi chạy generator; **(3)** nghiệm thu step 8 trên xe/bàn thử.
