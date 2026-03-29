#include "sntp_sync.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"

#define NVS_NAMESPACE    "bunnycam"
#define NVS_KEY_TIME     "last_time"
#define SNTP_TIMEOUT_MS  10000

static const char *TAG = "bunny-cam-sntp";

static void save_time(time_t t)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_i64(h, NVS_KEY_TIME, (int64_t)t);
    nvs_commit(h);
    nvs_close(h);
}

static time_t load_time(void)
{
    nvs_handle_t h;
    int64_t t = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return 0;
    nvs_get_i64(h, NVS_KEY_TIME, &t);
    nvs_close(h);
    return (time_t)t;
}

time_t sntp_sync(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    esp_err_t err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(SNTP_TIMEOUT_MS));
    esp_netif_sntp_deinit();

    if (err == ESP_OK) {
        time_t now = time(NULL);
        save_time(now);
        ESP_LOGI(TAG, "SNTP synced: %lld", (long long)now);
        return now;
    }

    ESP_LOGW(TAG, "SNTP timed out, using NVS fallback");
    return load_time();
}
