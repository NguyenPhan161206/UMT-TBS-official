#include "shared_state.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace
{
SemaphoreHandle_t s_mutex = nullptr;
float s_distanceCm[SENSOR_COUNT] = {0};
bool s_valid[SENSOR_COUNT] = {false};
} // namespace

void sharedStateInit()
{
    if (s_mutex == nullptr)
    {
        s_mutex = xSemaphoreCreateMutex();
    }
}

void sharedStateSet(size_t sensorIndex, float distanceCm, bool valid)
{
    if (sensorIndex >= SENSOR_COUNT)
    {
        return;
    }
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        s_distanceCm[sensorIndex] = distanceCm;
        s_valid[sensorIndex] = valid;
        xSemaphoreGive(s_mutex);
    }
}

bool sharedStateGet(size_t sensorIndex, float &distanceCm)
{
    if (sensorIndex >= SENSOR_COUNT)
    {
        return false;
    }
    bool valid = false;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
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
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
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