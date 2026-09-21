#include "shared_state.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "task_cfg.h"

namespace
{
SemaphoreHandle_t s_mutex = nullptr;
float s_distanceCm[SENSOR_COUNT] = {0};
bool s_isMuted = false;
bool s_valid[SENSOR_COUNT] = {false};
/* Trạng thái sức khỏe từng cảm biến; khởi tạo DISCONNECTED để tránh
 * cảnh báo ma trước khi cảm biến báo cáo lần đầu tiên. */
sensor_health_t s_health[SENSOR_COUNT];
} // namespace

void sharedStateInit()
{
    if (s_mutex == nullptr)
    {
        s_mutex = xSemaphoreCreateMutex();
    }
    // Khởi tạo health = DISCONNECTED: tránh cảnh báo ma khi cảm biến chưa báo cáo lần nào.
    for (size_t i = 0; i < SENSOR_COUNT; ++i)
    {
        s_health[i] = SENSOR_HEALTH_DISCONNECTED;
    }
}

void sharedStateSet(size_t sensorIndex, float distanceCm, bool valid)
{
    if (sensorIndex >= SENSOR_COUNT)
    {
        return;
    }
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE)
    {
        s_distanceCm[sensorIndex] = distanceCm;
        s_valid[sensorIndex] = valid;
        xSemaphoreGive(s_mutex);
    }
}

void sharedStateSetHealth(size_t sensorIndex, sensor_health_t health)
{
    if (sensorIndex >= SENSOR_COUNT)
    {
        return;
    }
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE)
    {
        s_health[sensorIndex] = health;
        xSemaphoreGive(s_mutex);
    }
}

sensor_health_t sharedStateGetHealth(size_t sensorIndex)
{
    if (sensorIndex >= SENSOR_COUNT)
    {
        return SENSOR_HEALTH_DISCONNECTED;
    }
    sensor_health_t h = SENSOR_HEALTH_DISCONNECTED;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE)
    {
        h = s_health[sensorIndex];
        xSemaphoreGive(s_mutex);
    }
    return h;
}

bool sharedStateGet(size_t sensorIndex, float &distanceCm)
{
    if (sensorIndex >= SENSOR_COUNT)
    {
        return false;
    }
    bool valid = false;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE)
    {
        distanceCm = s_distanceCm[sensorIndex];
        valid = s_valid[sensorIndex];
        xSemaphoreGive(s_mutex);
    }
    return valid;
}

bool sharedStateGetNearest(float &nearestCm)
{
    bool found = false;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE)
    {
        for (size_t i = 0; i < SENSOR_COUNT; ++i)
        {
            if (s_valid[i] && s_distanceCm[i] > 0.0f)
            {
                if (!found || s_distanceCm[i] < nearestCm)
                {
                    nearestCm = s_distanceCm[i];
                    found = true;
                }
            }
        }
        xSemaphoreGive(s_mutex);
    }
    return found;
}
void sharedStateSetMute(bool mute)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_isMuted = mute;
    xSemaphoreGive(s_mutex);
}

bool sharedStateGetMute()
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool m = s_isMuted;
    xSemaphoreGive(s_mutex);
    return m;
}
