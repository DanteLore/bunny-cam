#pragma once

#include <stddef.h>
#include "esp_err.h"

esp_err_t upload_image(const uint8_t *data, size_t len);
