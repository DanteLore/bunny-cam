#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t upload_status(int64_t uptime_s, float battery_v);
esp_err_t upload_image(const uint8_t *data, size_t len);
