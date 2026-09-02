import type { Plugin } from "@opencode-ai/plugin"

/**
 * UMT-TBS Guard — enforcement tầng ④ (R1, R8, R9).
 *
 * Đây là lớp phòng thủ "defense-in-depth". Các cổng chính bắt buộc:
 *  1. permission rules trong opencode.json (deny git add -A, push myrepo, force push...)
 *  2. /verify + /commit commands (scan_secrets.py trước khi commit)
 *  3. CI (B7 — tương lai): Gitleaks + assertions
 * Plugin này chặn thêm các lệnh bash nguy hiểm nếu lọt qua tầng permission.
 * Lỗi plugin KHÔNG làm hỏng opencode (mọi hook bọc try/catch).
 */

const BLOCKED_COMMANDS: Array<{ rx: RegExp; why: string }> = [
  { rx: /^\s*git add -A\b/, why: "R9: cấm git add -A — stage file cụ thể" },
  { rx: /^\s*git add --all\b/, why: "R9: cấm git add --all" },
  { rx: /^\s*git add \.\s*$/, why: "R9: cấm git add ." },
  { rx: /^\s*git add \.\s+/, why: "R9: cấm git add . <path> (chấm = toàn repo)" },
  { rx: /^\s*git push\b[^\n]*\bmyrepo\b/, why: "R9: chỉ push origin, không push myrepo" },
  { rx: /^\s*git push\b[^\n]*\s(-f|--force)\b/, why: "R9: cấm force push" },
  { rx: /^\s*git push\b[^\n]*:\s*master\b/, why: "R9: không push thẳng master (dùng main)" },
  { rx: /^\s*rm -rf\b/, why: "cấm rm -rf (kể cả khi trong repo)" },
  { rx: /^\s*shred\b/, why: "cấm shred" },
]

function blockedReason(cmd: string): string | null {
  const line = cmd.replace(/\s+/g, " ").trim()
  for (const b of BLOCKED_COMMANDS) {
    if (b.rx.test(line)) return b.why
  }
  return null
}

export default (async () => {
  return {
    "tool.execute.before": async (input: any, output: any) => {
      try {
        if (!input || input.tool !== "bash") return
        // trường hợp hook nhận args qua input, hoặc qua output cho phép mutate
        const args = output?.args ?? input?.args
        const cmd: string = typeof args?.command === "string" ? args.command : ""
        if (!cmd.trim()) return
        const why = blockedReason(cmd)
        if (why) {
          throw new Error(`[GUARD] ${why}\n  command: ${cmd}`)
        }
      } catch (err) {
        // Nếu không phải lỗi plugin (đã là lỗi chặn) thì đẩy lên; lỗi khác nuốt để không làm hỏng tool
        if (err instanceof Error && err.message.startsWith("[GUARD]")) throw err
      }
    },
  }
}) satisfies Plugin