#include "http.h"
#include "esp_camera.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_app_desc.h"
#include "cJSON.h"

static const char *TAG = "bunny-cam-http";
static int64_t last_activity_us = 0;

int64_t http_last_activity_us(void)
{
    return last_activity_us;
}

static void note_activity(void)
{
    last_activity_us = esp_timer_get_time();
}

static esp_err_t status_handler(httpd_req_t *req)
{
    note_activity();
    wifi_ap_record_t ap;
    esp_wifi_sta_get_ap_info(&ap);

    const esp_app_desc_t *app = esp_app_get_description();
    char build_timestamp[32];
    snprintf(build_timestamp, sizeof(build_timestamp), "%s %s", app->date, app->time);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "build_timestamp", build_timestamp);
    cJSON_AddNumberToObject(root, "uptime_s",        esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(root, "free_heap",       esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "min_free_heap",   esp_get_minimum_free_heap_size());
    cJSON_AddNumberToObject(root, "rssi",            ap.rssi);
    cJSON_AddNumberToObject(root, "wifi_channel",    ap.primary);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, json);
    free(json);
    return err;
}

static esp_err_t image_handler(httpd_req_t *req)
{
    note_activity();
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

void http_server_start(void)
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

    httpd_uri_t status_uri = {
        .uri     = "/status",
        .method  = HTTP_GET,
        .handler = status_handler,
    };
    httpd_register_uri_handler(server, &status_uri);
}
