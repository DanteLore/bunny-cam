#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "battery.h"
#include "camera.h"
#include "http.h"
#include "led.h"
#include "upload.h"
#include "wifi.h"

#define SLEEP_DURATION_US       (20LL * 1000000)
#define UPLOAD_INTERVAL_US      (30LL * 1000000)
#define BRIEF_AWAKE_WINDOW_US   (10LL * 1000000)
#define API_ACTIVE_WINDOW_US    (120LL * 1000000)

static const char *TAG = "bunny-cam";

static bool should_stay_awake(int64_t awake_since_us)
{
    int64_t now        = esp_timer_get_time();
    int64_t last_hit   = http_last_activity_us();

    if (last_hit > 0) {
        /* API has been hit -- stay awake until 120s of inactivity */
        return (now - last_hit) < API_ACTIVE_WINDOW_US;
    }

    /* No API hit yet -- stay awake for brief window only */
    return (now - awake_since_us) < BRIEF_AWAKE_WINDOW_US;
}

static void do_capture_and_upload(void)
{
    upload_status(esp_timer_get_time() / 1000000, battery_last_voltage());

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return;
    }
    upload_image(fb->buf, fb->len);
    esp_camera_fb_return(fb);
}

void app_main(void)
{
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);

    led_init();
    battery_read_and_cache();
    camera_init();
    wifi_init();
    wifi_connect();

    ESP_ERROR_CHECK(mdns_init());
    mdns_hostname_set("bunny-cam");
    mdns_instance_name_set("Bunny Cam");

    do_capture_and_upload();

    http_server_start();
    led_on();
    ESP_LOGI(TAG, "bunny-cam ready");

    int64_t awake_since_us   = esp_timer_get_time();
    int64_t last_upload_us   = awake_since_us;

    while (should_stay_awake(awake_since_us)) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        if ((esp_timer_get_time() - last_upload_us) >= UPLOAD_INTERVAL_US) {
            do_capture_and_upload();
            last_upload_us = esp_timer_get_time();
        }
    }

    led_off();
    wifi_disconnect();
    ESP_LOGI(TAG, "Going to sleep for %llds", SLEEP_DURATION_US / 1000000);
    esp_deep_sleep(SLEEP_DURATION_US);
}
