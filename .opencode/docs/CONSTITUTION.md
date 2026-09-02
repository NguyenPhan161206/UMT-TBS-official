# CONSTITUTION — Quy tắc bắt buộc cho mọi agent (V2)

> File này được nạp vào context mỗi session qua `instructions`. **Mọi agent, mọi task, mọi commit
> phải tuân thủ.** Vi phạm P0 không được bỏ qua bằng "chỉ một lần".

Mỗi quy tắc gồm: **Tại sao** (lỗi thật từ repo cũ `supersonic-warning-system`) / **Kiểm tra** /
**Hành động khi vi phạm**.

---

## R1 — Secrets tuyệt đối không vào file tracked 🔴
- **Tại sao**: repo cũ đã rò rỉ 2 CoreIoT token + Wi-Fi password vào git history
  (`a4f9028`, `bd76189`, `5cf225f`), do hardcode trong header.
- **Luật**: credential (token CoreIoT, Wi-Fi pass, API key) **chỉ được nằm ở** `config/keys.json`
  (gitignored) hoặc file **sinh ra từ nó** (ví dụ `credentials.h` — gitignored, sinh bởi
  `tools/guard/gen_credentials.py`). Literal token/`PASS` trong bất kỳ file tracked nào = P0.
- **Kiểm tra**: `python3 tools/guard/scan_secrets.py` exit 0; `/verify`.
- **Hành động**: sửa ngay, xoá khỏi staged/commit (không chỉ "sửa sau"), báo ledger.

## R2 — Shared contract chỉ ở `firmware/shared/`
- **Tại sao**: struct `espnow_sensor_msg_t` định nghĩa trùng ở 2 board → lệch slot dữ liệu.
- **Luật**: struct/define/macro dùng chung giữa sensor-node và waveshare-screen **chỉ nằm ở
  `firmware/shared/`**. Cấm định nghĩa lại cùng symbol (`grep` toàn repo phải = 1 nơi).
- **Kiểm tra**: `grep -rn "<symbol>" firmware/ | wc -l` == 1.
- **Hành động**: xoá bản trùng, chuyển vào shared, sửa include.

## R3 — Thresholds 1 nguồn
- **Tại sao**: ngưỡng cảnh báo hiện rải rác 3 bộ (sensor-node, sensor_model, rule-chain).
- **Luật**: ngưỡng zone/chung **chỉ ở `firmware/shared/thresholds.h`**; firmware khác include;
  rule-chain JSON là snapshot có thể đối chiếu (xem R11).
- **Kiểm tra**: `python3 tools/guard/check_rulechain_thresholds.py` (best-effort).
- **Hành động**: sửa tại `thresholds.h`, không phải nơi khác.

## R4 — SENSOR_COUNT từ sizeof, không gõ tay
- **Tại sao**: `SENSOR_COUNT=6` gõ tay tách khỏi `SENSOR_PINS` → lệch khi đổi số cảm biến.
- **Luật**: `#define SENSOR_COUNT (sizeof(SENSOR_PINS)/sizeof(SENSOR_PINS[0]))` đi kèm
  `static_assert` tại `firmware/shared/thresholds.h`.
- **Kiểm tra**: `static_assert` compile-time; CI assert (B7).
- **Hành động**: sửa tại shared header.

## R5 — Không demo trong production
- **Tại sao**: `appTask` demo (mô phỏng khoảng cách) nằm ngay trong `main.cpp` production.
- **Luật**: demo/example/auto-play nằm ở `prototypes/` hoặc nhánh riêng, hoặc sau macro rõ ràng.
- **Kiểm tra**: review code (arch-guard); không có task mô phỏng trong luồng cảnh báo chính.
- **Hành động**: chuyển ra `prototypes/`, không xoá logic bằng cách "xem thôi".

## R6 — Không dead code trong build
- **Tại sao**: component `coreiot_client` không dùng nhưng vẫn compile → khó biết code sống hay chết.
- **Luật**: mọi module phải **được build trong ≥1 env CI** và được tham chiếu rõ. CoreIoT dùng
  cờ `USE_COREIOT` (0 mặc định) nhưng env `*_coreiot` phải build ở CI.
- **Kiểm tra**: CI build matrix; không module nào `#include` mất gốc.
- **Hành động**: xoá module chết hoặc chuyển nhánh; nếu giữ → ghi rõ trong README/ledger.

## R7 — File code không quá 400 dòng
- **Tại sao**: `ui_dashboard.c` 837 dòng, `Config.h` 116 dòng nhồi nhét nhiều chủ đề.
- **Luật**: `*.c|*.cpp|*.h` (firmware) ≤ 400 dòng/file; nếu vượt → tách theo module/trách nhiệm.
- **Kiểm tra**: scan tự động (CI size-gate, B7) + arch-guard review.
- **Hành động**: tách file, không xoá trách nhiệm.

## R8 — Git hygiene
- **Tại sao**: `sdkconfig` (104K) bị track, `dependencies.lock` bị ignore → tái lập build không chắc.
- **Luật**: không track `sdkconfig*`, `build/`, binary, `config/keys.json`; **track**
  `dependencies.lock`; release binary qua GitHub Release/LFS.
- **Kiểm tra**: `git ls-files | grep -E 'sdkconfig|keys.json'` = rỗng.
- **Hành động**: `git rm --cached` đúng file, thêm vào `.gitignore`, commit riêng.

## R9 — Git: 1 remote, conventional, không add -A
- **Tại sao**: repo cũ 2 remote (origin=YdtTran, myrepo=NguyenPhan) không đồng bộ; commit lộn xộn.
- **Luật**: chỉ `origin` (repo UMT-TBS-official); commit message conventional
  (`feat/fix/docs/refactor/chore/test/ci`); trước commit chạy `/verify`; **cấm `git add -A`/`.`**;
  cấm force push.
- **Kiểm tra**: `git remote -v` = 1 remote; plugin guard + permission; `git log --oneline` đọc được.
- **Hành động**: thêm file cụ thể; reset về chưa commit nếu lỡ `add -A`.

## R10 — Test & DoD đo được
- **Tại sao**: hầu hết thay đổi cũ không có cách kiểm chứng ngoài "build pass".
- **Luật**: mỗi task phải có DoD kèm **lệnh xác minh chạy được** (unit test, script, grep count,
  flash-and-observe). "Looks right" = không DONE.
- **Kiểm tra**: `/verify`; DoD trong roadmap JSON có `verification_command`.
- **Hành động**: viết test/script trước hoặc cùng code.

## R11 — CoreIoT: snapshot + token tự sinh (cách b)
- **Tại sao**: token cũ đã lộ; rule-chain không ai đối chiếu với firmware.
- **Luật**: rule-chain JSON trong `cloud/coreiot/rule_chain/` là **snapshot được track**; ngưỡng
  khớp `thresholds.h` (check best-effort); credential **luôn là token mới** do chủ tài khoản tự sinh
  (không tái dùng token đã commit bất kỳ đâu).
- **Kiểm tra**: `python3 tools/guard/check_rulechain_thresholds.py`; `scan_secrets.py` không hit.
- **Hành động**: tạo token mới trên console, cập nhật `config/keys.json`, vô hiệu token cũ.

## R12 — Tối thiểu agent, làm theo roadmap
- **Tại sao**: quá nhiều agent chuyên biệt sinh từ lộ trình repo cũ, tốn context.
- **Luật**: làm việc qua roadmap (`/plan` → `/step N`); chỉ thêm agent mới khi task thực sự cần.
- **Kiểm tra**: number of agents trong `.opencode/agent/` không phình vô cớ.
- **Hành động**: dùng lại agent hiện có; ghi đề xuất mới vào ledger trước khi tạo.

---

## Mức vi phạm
- **P0** (R1, R2, R3, R4, R8, R9): chặn commit, sửa ngay trong cùng PR; ghi ledger.
- **P1** (R5, R6, R7, R10, R11, R12): cảnh báo, phải có kế hoạch sửa trong sprint hiện tại.

## Kiểm tra mọi task xong (SELFCHECK)
Xem `.opencode/docs/SELFCHECK.md` — 10 câu hỏi trả lời trước khi báo "DONE".