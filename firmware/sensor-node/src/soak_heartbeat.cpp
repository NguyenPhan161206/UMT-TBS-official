#include "soak_heartbeat.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_timer.h>

#include "soak_diag.h"

// tbs_reset_reason_name() nhận int theo thứ tự esp_reset_reason_t — chặn lệch nếu IDF đổi enum.
static_assert(ESP_RST_POWERON == 1 && ESP_RST_PANIC == 4 && ESP_RST_TASK_WDT == 6 &&
                  ESP_RST_BROWNOUT == 9,
              "esp_reset_reason_t khác bảng trong firmware/shared/soak_diag.h");

namespace
{
uint32_t s_bootCount = 0;
const char *s_resetReason = "UNKNOWN";
uint32_t s_lastBeatMs = 0;
TaskHandle_t s_sensorTask = nullptr;
TaskHandle_t s_netTask = nullptr;
TaskHandle_t s_buzzTask = nullptr;

long stackHwm(TaskHandle_t t)
{
    return t != nullptr ? (long)uxTaskGetStackHighWaterMark(t) : -1L;
}
} // namespace

void soakHeartbeatBoot()
{
    s_resetReason = tbs_reset_reason_name((int)esp_reset_reason());

    Preferences prefs;
    if (prefs.begin("diag", false))
    {
        s_bootCount = prefs.getUInt("boot", 0) + 1;
        prefs.putUInt("boot", s_bootCount);
        prefs.end();
    }
    Serial.printf("BOOT node boot=%lu rr=%s\n", (unsigned long)s_bootCount, s_resetReason);
}

void soakHeartbeatSetTasks(TaskHandle_t sensor, TaskHandle_t net, TaskHandle_t buzz)
{
    s_sensorTask = sensor;
    s_netTask = net;
    s_buzzTask = buzz;
}

void soakHeartbeatPoll(uint32_t nowMs, const SoakNodeCounters &c)
{
    if (nowMs - s_lastBeatMs < TBS_SOAK_HEARTBEAT_INTERVAL_MS)
    {
        return;
    }
    s_lastBeatMs = nowMs;

    Serial.printf("SOAK node up=%lu boot=%lu rr=%s heap=%lu minheap=%lu blk=%lu "
                  "tx_ok=%lu tx_fail=%lu mqtt_rc=%ld hwm_sensor=%ld hwm_net=%ld hwm_buzz=%ld\n",
                  (unsigned long)(esp_timer_get_time() / 1000000LL),
                  (unsigned long)s_bootCount, s_resetReason,
                  (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
                  (unsigned long)ESP.getMaxAllocHeap(),
                  (unsigned long)c.txOk, (unsigned long)c.txFail, (long)c.mqttReconnects,
                  stackHwm(s_sensorTask), stackHwm(s_netTask), stackHwm(s_buzzTask));
}
