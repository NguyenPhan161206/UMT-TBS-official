# Prompt audit — file chỉ dẫn cho agent (2026-10-05)

Diff đề xuất: [`prompt-audit-2026-10-05.diff`](prompt-audit-2026-10-05.diff) — **chưa áp dụng** (yêu cầu chỉ là `prompt-audit`).
Kiểm tra: `git apply --check docs/audits/prompt-audit-2026-10-05.diff` → OK (28 hunk, 14 file).

## Giả định (Step 0)
- **Phạm vi**: toàn bộ bề mặt chỉ dẫn trong thư mục làm việc (yêu cầu không nêu file cụ thể).
- **Model đích**: Claude Opus 5.5 — model đang chạy audit; không file nào ghim model riêng.
- **Dấu hiệu nhà cung cấp khác Anthropic**: `.agents/skills/` dành cho Antigravity (Google), `.opencode/` cho opencode (nhà cung cấp cấu hình trong `opencode.json`, không đọc). Gần như mọi phát hiện là **sai lệch so với chính repo** (Nhóm 2), không phụ thuộc model.
- **Bỏ qua**: `.opencode/opencode.json` và `.opencode/plugin/guard.ts` (settings/hook của agent — có thể chứa secret, không đọc); `.opencode/node_modules/`; `config/keys*.json`; memory người dùng `~/.claude/projects/.../memory/` (ngoài project); `docs/handoff/*`, `docs/roadmaps/*` (artifact của từng task đã xong, không phải chỉ dẫn thường trực).
- Không có code nào gọi API LLM (grep `anthropic|openai|gemini|claude-|gpt-` = 0).

## Kiểm kê (Step 1)
| Loại | File |
|---|---|
| Chỉ dẫn gốc | `AGENTS.md` (local, gitignored — khác template đúng 2 cổng COM), `AGENTS.template.md`, `CONTRIBUTING.md` (AGENTS.md bắt đọc trước khi commit) |
| Luật | `.opencode/docs/CONSTITUTION.md` |
| Skill | `.claude/skills/dev-orchestrator/` (+ `references/`), `.agents/skills/{dev-orchestrator, selfcheck, roadmap_checklist, cross_device_reconfig, esp32_screen_debug, lvgl_v9_warning_display (+4 references)}` |
| Agent opencode | `.opencode/agent/{dev-orchestrator, arch-guard, secrets-responder}.md` |
| Command opencode | `.opencode/command/{build, commit, flash, plan, scaffold, step, test, verify}.md` |

Nguồn gốc (Step 2): hầu hết từ commit scaffold `3c765a2` (2026-09-02); `selfcheck`/`roadmap_checklist` chuyển từ `.opencode/docs/` sang skill ở `3b59b0c` (2026-09-03); template sửa lần cuối `21e62b6` (2026-09-09).

## Tóm tắt
Các phát hiện quan trọng nhất đều là **chỉ dẫn nói khác code hiện tại**:
1. Skill `lvgl_v9_warning_display` dạy máy trạng thái 4 mức **100/50/20 cm**, hysteresis 5 cm và thanh trượt chỉnh ngưỡng lúc chạy. Repo dùng 3 zone **100/30** chỉ từ `thresholds.h` (R3). Agent làm UI theo skill sẽ vi phạm R3.
2. `/plan`, agent `dev-orchestrator` và skill `roadmap_checklist` coi lộ trình B0–B9 của tháng 9 là lộ trình hiện hành ("đang thực thi"), trong khi trạng thái thật nằm ở `docs/CHECKLIST.md`.
3. Hai chỉ dẫn "build/kiểm tra" lại **nạp firmware lên board**: `build_and_flash.bat` không nhận tham số và luôn chạy `pio run -e yolo_uno -t upload` (P4, P5, P13). Ngoài ra, R7 ghi "scan tự động" cho giới hạn 400 dòng nhưng thực tế không có (P8).

Số lượng theo nhóm:
- **Nhóm 1** (lời lẽ/scaffold kiểu model cũ): **0**. Các câu nhấn mạnh như CẤM/KHÔNG đều gắn ràng buộc thật (secret, git), không có think-step-by-step hay prefill.
- **Nhóm 2** (file cấu hình lỗi thời/mâu thuẫn): **15 phát hiện có đề xuất sửa** (11 cao, 4 trung bình) và **6 mục flag**.
- **Nhóm 3** (mô tả tool): không áp dụng. Description của skill/agent là text định tuyến.
- **Nhóm 4** (cấu hình request): không áp dụng. Danh sách sub-agent gồm 3 agent opencode khác vai trò, 0 trùng.

---

## Phát hiện — độ tin cậy CAO

### P1 — `test_mqtt_coreiot.py` bị ghi là "chưa port sang V2"
- **Vị trí**: `AGENTS.template.md:69`, `AGENTS.md:69`
- **Bằng chứng**: "`tools/test_mqtt_coreiot.py` (chưa port sang V2 — xem roadmap)"
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: Tool được port V2 ngay ở commit `0032169` (2026-09-02, cùng ngày viết dòng này), có `--scenario`. `docs/CHECKLIST.md` T1.1 ✅ và skill `roadmap_checklist` B9a ✅.
- **Hành động**: `rewrite` → "(schema V2; `--scenario <tên>` phát kịch bản từ `tools/scenarios.py`)".

### P2 — Mục "Kiểm thử cục bộ" trỏ thư mục không tồn tại, lệnh chỉ chạy 1 test
- **Vị trí**: `AGENTS.template.md:61-64`, `AGENTS.md:61-64`
- **Bằng chứng**: "`firmware/waveshare-screen/test/` (làm ở giai đoạn firmware)"; lệnh `.../hazard_core_tests # host test hazard_core (T1.3)`
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: `firmware/waveshare-screen/test/` không tồn tại. Test của màn hình nằm ở `host_sim/tests/`, và `host_sim/CMakeLists.txt` đăng ký 13 test ctest. Test sensor-node dùng Unity (`[env:native]`).
- **Hành động**: `rewrite` → lệnh `ctest --test-dir build/host_sim` và `pio test -e native` (`build/` đã gitignore).

### P3 — `/test` dạy build test sensor-node bằng g++ "không phụ thuộc" và coi sim là "nếu có"
- **Vị trí**: `.opencode/command/test.md:10-16`
- **Bằng chứng**: "build với g++ thuần (không phụ thuộc Arduino) … `g++ -std=c++17 …`"; "**Mô phỏng LVGL/SDL** (nếu có)"
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: Các test `#include <unity.h>` và `platformio.ini` có `test_framework = unity`, nên lệnh g++ không biên dịch được. `host_sim` đã có.
- **Hành động**: `rewrite` → `pio test -e native` và lệnh ctest của host_sim.

### P4 — `/flash` dùng `build_and_flash.bat flash <COM>`
- **Vị trí**: `.opencode/command/flash.md:17`
- **Bằng chứng**: "Trên Windows dùng `build_and_flash.bat flash <COM>`."
- **Pattern**: Nhóm 2 — Volatile specifics (lệnh/cờ không còn được định nghĩa).
- **Lý do**: Script không đọc tham số (`%1`); luôn chạy `pio run -e yolo_uno -t upload` và tự dò cổng.
- **Hành động**: `rewrite` → dùng lệnh `pio … --upload-port COMx`, ghi rõ script không nhận tham số.

### P5 — Checklist của `cross_device_reconfig` vô tình nạp firmware
- **Vị trí**: `.agents/skills/cross_device_reconfig/SKILL.md:62`
- **Bằng chứng**: "Running project batch scripts (e.g. `firmware/sensor-node/build_and_flash.bat build`) successfully activates ESP-IDF."
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: Script bỏ qua `build`, không kích hoạt ESP-IDF (dùng PlatformIO), và **upload lên board đang cắm**.
- **Hành động**: `rewrite` → mục kiểm `pio run -e yolo_uno` (chỉ build), ghi chú script sẽ nạp firmware.

### P6 — `cross_device_reconfig` liệt kê placeholder template chưa từng có
- **Vị trí**: `.agents/skills/cross_device_reconfig/SKILL.md:52`
- **Bằng chứng**: "Replace placeholders `<IDF_PATH>`, `<SHELL_PROFILE_PATH>`, `<PYTHON_VENV_PATH>`, …"
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: `git log -p AGENTS.template.md` không có lần nào chứa 3 placeholder này. Template chỉ có `<SENSOR_NODE_PORT, …>` và `<WAVESHARE_SCREEN_PORT, …>`.
- **Hành động**: `rewrite` (skill cũ hơn template → sửa skill theo template).

### P7 — `/scaffold` bảo đọc file `SELFCHECK`, `ROADMAP_CHECKLIST` đã xoá
- **Vị trí**: `.opencode/command/scaffold.md:14-15`
- **Bằng chứng**: "đọc qua từng file (`opencode.json`, `CONSTITUTION`, `SELFCHECK`, `ROADMAP_CHECKLIST`, …)"
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: `.opencode/docs/SELFCHECK.md` và `ROADMAP_CHECKLIST.md` bị xoá ở `3b59b0c` (2026-09-03) và chuyển thành skill trong `.agents/skills/`.
- **Hành động**: `rewrite` → trỏ tới 2 skill.

### P8 — R7 (≤ 400 dòng) ghi có "scan tự động" nhưng không có
- **Vị trí**: `.opencode/docs/CONSTITUTION.md:51`
- **Bằng chứng**: "**Kiểm tra**: scan tự động (CI size-gate, B7) + arch-guard review."
- **Pattern**: Nhóm 2 — Volatile specifics; Nhóm 1d — Unenforced instructions.
- **Lý do**: CI size-gate `check_size.py` đo **kích thước `firmware.bin`**. `arch_guard.py` B7 chỉ đếm dòng `hazard_core.{c,h}`. Ví dụ: `host_sim/main.c` 424 dòng vẫn qua mọi guard.
- **Hành động**: `rewrite` mô tả cho đúng. Nếu R7 quan trọng, nên thêm một guard đếm dòng thật (xem F3).

### P9 — Lộ trình B0–B9 được coi là lộ trình hiện hành
- **Vị trí**:
  - `.agents/skills/roadmap_checklist/SKILL.md:3, 8-9, 58-62`
  - `.opencode/command/plan.md:7`
  - `.opencode/agent/dev-orchestrator.md:17`
- **Bằng chứng**: "Nguồn chuẩn hoá lộ trình. Mọi agent đối chiếu checklist này khi nhận việc"; "1. **Config + enforcement** (B0–B1, B6e) — đang thực thi."; "gọi skill `roadmap_checklist` (ưu tiên B0–B9)"
- **Pattern**: Nhóm 2 — History narratives / Volatile specifics.
- **Lý do**: Bảng của chính skill ghi B0, B1, B6e ✅ XONG, mâu thuẫn với "đang thực thi". Trạng thái và thứ tự ưu tiên đang được duy trì ở `docs/CHECKLIST.md` (cập nhật 2026-10-05, T0–T5). `/plan` lập kế hoạch theo ưu tiên cũ.
- **Hành động**: `rewrite` skill thành "lịch sử dựng repo + trỏ `docs/CHECKLIST.md`"; `rewrite` 2 chỗ gọi nó.

### P10 — Skill LVGL dạy zone/ngưỡng/tab trái với code (và trái R3)
- **Vị trí**: `.agents/skills/lvgl_v9_warning_display/SKILL.md:14-64`
- **Bằng chứng**: bảng màu `COLOR_APPROACHING/WARNING/CRITICAL` với "20 cm < Distance ≤ 50 cm"; máy trạng thái 4 mức + "5cm hysteresis"; "`lv_tabview` … Interactive sliders (`lv_slider`) for threshold tuning".
- **Pattern**: Nhóm 2 — Volatile specifics; mâu thuẫn với luật dự án.
- **Lý do**:
  - `thresholds.h`: `SENSOR_CAUTION_CM` 100, `SENSOR_DANGER_CM` 30 (R3: chỉ một nguồn); `hazard_classify()` có 3 zone, không hysteresis.
  - Màu trong `ui_dashboard_theme.h` khác bảng của skill.
  - UI dùng 3 tab tự dựng COLLISION/SYSTEM/SETUP, không `lv_tabview`, không chỉnh ngưỡng lúc chạy.
- **Hành động**: `rewrite` mục 1–3 thành "đọc từ code" (thresholds.h, hazard_core, theme.h, bố cục thật).

### P11 — Skill LVGL khuyên 2 framebuffer PSRAM; thiếu bài học heap LVGL
- **Vị trí**: `.agents/skills/lvgl_v9_warning_display/SKILL.md:87`
- **Bằng chứng**: "Allocate 2 full framebuffers in PSRAM … (~1.5 MB total)."
- **Pattern**: Nhóm 2 — Volatile specifics; keep-list 11 (re-baseline có thể phải thêm chữ).
- **Lý do**: `src/main.c` cố ý chọn `TEAR_AVOID_MODE_NONE` — một buffer, vẽ từng phần — có ghi lý do (tránh tranh bus PSRAM với Wi-Fi). Pool LVGL cố định (`CONFIG_LV_MEM_SIZE_KILOBYTES`) từng cạn ở 64 KB làm treo; đã nâng 128 KB. Dòng bounce buffer 40 dòng vẫn đúng (BSP có dùng), giữ nguyên.
- **Hành động**: `rewrite` dòng framebuffer + `add` dòng heap LVGL.

## Phát hiện — độ tin cậy TRUNG BÌNH

### P12 — Lệnh build/flash cứng `/dev/ttyACM*` trong khi template dùng placeholder
- **Vị trí**: `AGENTS.template.md:40-41, 48-49`, `AGENTS.md:40-41, 48-49`
- **Bằng chứng**: Overview ghi `<SENSOR_NODE_PORT, …>` (bản local: **COM4**/**COM9**); lệnh ngay dưới vẫn là `--upload-port /dev/ttyACM0` / `/dev/ttyACM1`.
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: `scan_env.py` chỉ điền `<SENSOR_NODE_PORT…>` / `<WAVESHARE_SCREEN_PORT…>`, nên lệnh không bao giờ được cập nhật. Bản local tự mâu thuẫn (COM4 ở trên, `/dev/ttyACM0` ở dưới).
- **Hành động**: `rewrite` → dùng đúng placeholder (template) / COM4, COM9 (bản local).

### P13 — `/build` gợi ý `build_and_flash.bat` để kiểm build
- **Vị trí**: `.opencode/command/build.md:14-15`
- **Bằng chứng**: "build xác minh qua CI (B7) hoặc máy Windows `build_and_flash.bat`."
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: Script build rồi upload lên board.
- **Hành động**: `rewrite`.

### P14 — Liên kết tuyệt đối tới checkout cũ `supersonic-sensor-ACLAB`
- **Vị trí**: `.agents/skills/lvgl_v9_warning_display/SKILL.md:96-99`, `references/lvgl_demos_architecture_review.md:12`
- **Bằng chứng**: `file:///e:/supersonic-sensor-ACLAB/.agents/skills/lvgl_v9_warning_display/references/…`
- **Pattern**: Nhóm 2 — Volatile specifics (hardcoded paths).
- **Lý do**: Các file đích có trong repo này theo đường dẫn tương đối (thư mục `demos` nằm trong `managed_components/`, do component manager sinh ra và gitignored).
- **Hành động**: `rewrite` sang đường dẫn tương đối.

### P15 — Tài liệu reference của skill LVGL trình bày thiết kế chưa từng làm như SOP
- **Vị trí**: `references/ui_development_pipeline.md` (vd dòng 26-56, 161), `references/lvgl_demos_architecture_review.md` (vd dòng 142, 183-202)
- **Bằng chứng**: "a **5cm hysteresis buffer** must be implemented"; "All CH422G writes and GT911 touch reads MUST acquire `xI2C_Mutex`"
- **Pattern**: Nhóm 2 — Volatile specifics.
- **Lý do**: Không có máy 4 mức, `lv_tabview` hay `xI2C_Mutex` trong firmware (grep = 0). Đây là tài liệu thiết kế ban đầu, chỉ đọc khi cần.
- **Hành động**: `add` ghi chú đầu file (giữ nội dung cũ làm tài liệu tham khảo).

## Flag — không đề xuất sửa (cần bạn quyết)

- **F1 — Cổng serial bị quy định khác nhau ở 5 file** (cùng commit `3c765a2`, git không phân định được cái nào mới hơn):
  - `AGENTS.md` (COM4/COM9)
  - `esp32_screen_debug/SKILL.md:18-20` (COM8/COM9, kèm đường dẫn máy cụ thể `E:\esp\v6.0.2\…`, `C:\Espressif\…`)
  - `roadmap_checklist/SKILL.md:66`
  - `.opencode/command/flash.md:6-7`
  - `.opencode/agent/dev-orchestrator.md:17` (`/dev/ttyACM0`/`1`)

  Cần quyết: có lấy `AGENTS.md` (sinh riêng cho từng máy bởi `cross_device_reconfig`) làm nơi duy nhất, các file khác chỉ trỏ tới nó? (P9 đã bỏ cổng cứng ở `dev-orchestrator.md:17` như hệ quả.)
- **F2 — Quy trình git mâu thuẫn**:
  - `AGENTS.md:78-87`, `CONTRIBUTING.md:8-13`, `.opencode/command/commit.md:21-25` bảo `git push origin main`.
  - `docs/GIT_GUIDE.md:69-79` (chưa track) và lịch sử repo (nhánh `khoa`, `nguyen` rồi merge) làm việc trên nhánh riêng.

  Agent làm theo `/commit` khi đang ở `khoa` sẽ push `main` local. Đây là lệnh push nên chỉ flag. Cần quyết: push nhánh riêng hay `main`.
- **F3 — Phạm vi R7 lệch nhau**: `selfcheck/SKILL.md:17` ("mọi file ≤ 400 dòng") so với `CONSTITUTION.md:50` (`*.c|*.cpp|*.h` firmware). Thu hẹp selfcheck là nới một luật cấm nên chỉ flag.
- **F4 — Luật luồng UI trái với code**: `lvgl_v9_warning_display/SKILL.md:80-81` cấm gọi LVGL trong callback MQTT và đòi `xI2C_Mutex`. Trong khi đó `src/main.c` `on_coreiot_data` gọi UI trực tiếp dưới `esp_lv_adapter_lock`, và không có `xI2C_Mutex`. Cần quyết bên nào đúng.
- **F5 — (thấp)** `AGENTS.template.md:30-31` cứng `/usr/bin/python3 (3.12+)` và "board không có sẵn trên máy dev". Đây là sự thật của từng máy nhưng không phải placeholder.
- **F6 — (thấp)** `AGENTS.md:94-104` bắt buộc chạy `dev-orchestrator` (roadmap JSON + ledger) cho **mọi** thay đổi hơn 1 file, kèm cú pháp `Skill(...)` riêng của Claude Code trong file dùng chung cho nhiều agent. Đây là quyết định quy trình của nhóm.

## Áp dụng
```bash
git apply docs/audits/prompt-audit-2026-10-05.diff                                # tất cả
git apply --include=.opencode/command/flash.md docs/audits/prompt-audit-2026-10-05.diff   # từng file
```
Diff chỉ sửa file chỉ dẫn/tài liệu, không đụng code. `AGENTS.md` là file local (gitignored), nên sửa ở đó chỉ có tác dụng trên máy này.
