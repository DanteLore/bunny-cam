#include <string.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_camera.h"

static const char *TAG = "bunny-cam";

/* WiFi event group -- bit 0 set when connected */
static EventGroupHandle_t wifi_events;
#define WIFI_CONNECTED_BIT BIT0

/* AI Thinker ESP32-CAM pin assignments */
static const camera_config_t camera_config = {
    .pin_pwdn     = 32,
    .pin_reset    = -1,
    .pin_xclk     = 0,
    .pin_sccb_sda = 26,
    .pin_sccb_scl = 27,
    .pin_d7       = 35,
    .pin_d6       = 34,
    .pin_d5       = 39,
    .pin_d4       = 36,
    .pin_d3       = 21,
    .pin_d2       = 19,
    .pin_d1       = 18,
    .pin_d0       = 5,
    .pin_vsync    = 25,
    .pin_href     = 23,
    .pin_pclk     = 22,
    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size   = FRAMESIZE_VGA,
    .jpeg_quality = 12,
    .fb_count     = 1,
};

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_events, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    wifi_events = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(wifi_events, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
}

static esp_err_t image_handler(httpd_req_t *req)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    esp_err_t err = httpd_resp_send(req, (const char *)fb->buf, fb->len);

    esp_camera_fb_return(fb);
    return err;
}

static httpd_handle_t start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t image_uri = {
        .uri     = "/image",
        .method  = HTTP_GET,
        .handler = image_handler,
    };
    httpd_register_uri_handler(server, &image_uri);

    return server;
}

void app_main(void)
{
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);
    ESP_ERROR_CHECK(esp_camera_init(&camera_config));

    wifi_init();
    start_http_server();

    ESP_LOGI(TAG, "bunny-cam ready");
}
