# CD — Release firmware.bin lên GitHub Release (UMT-TBS V2)

> Decision record (2026-09-09). Nguồn thực thi: `docs/roadmaps/cd-release.roadmap.json`.

## 1. Mục tiêu
Sau khi code build xanh và gắn tag phiên bản `v*`, tự động đóng gói **firmware.bin**
(3 env) + checksum lên **GitHub Release** để bất kỳ ai cũng tải được firmware mà
không cần môi trường build. Đúng tinh thần R8: binary không commit vào git, phát
hành qua GitHub Release/LFS.

## 2. Trigger
- `push` tag khớp `v*` (vd `v0.1.0`, `v0.1.1`) → chạy workflow `release.yml`.
- Push nhánh thường KHÔNG chạy workflow này (chỉ `ci.yml` chạy).

## 3. Artifact naming (chuẩn)
| Artifact (trên Release) | Nguồn (sau build CI) |
|---|---|
| `firmware-sensor-node-yolo_uno-<tag>.bin` | `firmware/sensor-node/.pio/build/yolo_uno/firmware.bin` |
| `firmware-sensor-node-yolo_uno_coreiot-<tag>.bin` | `firmware/sensor-node/.pio/build/yolo_uno_coreiot/firmware.bin` |
| `firmware-waveshare-screen-yolo_uno-<tag>.bin` | `firmware/waveshare-screen/.pio/build/yolo_uno/firmware.bin` |
| `SHA256SUMS.txt` | `sha256sum *.bin` trong thư mục staging `dist/` |

`<tag>` = `github.ref_name` (vd `v0.1.0-preview`).

## 4. Permissions & secret
- Workflow-level `permissions: contents: write` — dùng `GITHUB_TOKEN` (tự động),
  **không cần PAT**.
- Credential firmware: tự sinh dummy `credentials.h` từ `config/keys.json` tạm
  (pattern `ci.yml`) — **không có literal secret nào trong workflow** (R1).
- `softprops/action-gh-release@v2` tự tạo Release từ tag nếu chưa tồn tại và đính file.

## 5. Không làm (phạm vi)
- ❌ **Không flash board** — flash là thao tác vật lý, AGENTS.md yêu cầu
  flash-and-observe bằng tay (bước 7/10 roadmap). CD dừng ở "có binary tải được".
- ❌ Không gộp vào `ci.yml` — workflow đứng riêng để release tag tự cô lập,
  build thất bại không kẹt chung với push thường.
- ❌ Không tự deploy rule-chain CoreIoT (vẫn import tay, R11 snapshot track).

## 6. Quy trình release tay (demo)
```bash
git tag v0.1.0 && git push origin v0.1.0
# chờ GitHub Actions xanh → vào GitHub → Releases → tải .bin tương ứng
# kiểm chứng checksum: sha256sum -c SHA256SUMS.txt
```
- Muốn chạy lại sau lỗi workflow: sửa code/workflow → `git push`, rồi
  `git tag -f v0.1.0 && git push origin v0.1.0 --force-with-lease` (hoặc dùng tag mới).
- Xoá release/test thử: `gh release delete v0.1.0-preview --yes && git push origin :refs/tags/v0.1.0-preview`.

## 7. DoD đo được
`gh release view <tag> --json assets -q '.assets[].name'` liệt kê ≥ 4 asset
(3 `.bin` + `SHA256SUMS.txt`).