#include "upload.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

#define UPLOAD_ENDPOINT \
    "https://cam.dantelore.com/upload?secret=" CONFIG_UPLOAD_SECRET "&camera_id=bunnycam"

/* S3 presigned URLs carry a large AWS Signature v4 query string */
#define PRESIGNED_URL_MAX 2048
#define RESPONSE_BUF_MAX  4096

static const char *TAG = "bunny-cam-upload";

typedef struct {
    char buf[RESPONSE_BUF_MAX];
    int  len;
} response_t;

static esp_err_t on_http_data(esp_http_client_event_t *evt)
{
    if (evt->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;
    response_t *r = evt->user_data;
    int copy = evt->data_len;
    if (r->len + copy >= RESPONSE_BUF_MAX) copy = RESPONSE_BUF_MAX - r->len - 1;
    memcpy(r->buf + r->len, evt->data, copy);
    r->len += copy;
    return ESP_OK;
}

static esp_err_t get_presigned_url(char *url_out, size_t url_out_len)
{
    response_t response = {0};

    esp_http_client_config_t config = {
        .url               = UPLOAD_ENDPOINT,
        .event_handler     = on_http_data,
        .user_data         = &response,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    int status    = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGE(TAG, "GET /upload failed: err=0x%x status=%d", err, status);
        return ESP_FAIL;
    }

    response.buf[response.len] = '\0';
    cJSON *json = cJSON_Parse(response.buf);
    const cJSON *url = cJSON_GetObjectItem(json, "url");
    if (!cJSON_IsString(url)) {
        ESP_LOGE(TAG, "No url field in response: %s", response.buf);
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    strlcpy(url_out, url->valuestring, url_out_len);
    cJSON_Delete(json);
    return ESP_OK;
}

static esp_err_t put_image(const char *presigned_url, const uint8_t *data, size_t len)
{
    esp_http_client_config_t config = {
        .url               = presigned_url,
        .method            = HTTP_METHOD_PUT,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size_tx    = 4096,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");

    esp_err_t err = esp_http_client_open(client, len);
    if (err == ESP_OK) {
        esp_http_client_write(client, (const char *)data, len);
        esp_http_client_fetch_headers(client);
    }

    int status = esp_http_client_get_status_code(client);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGE(TAG, "PUT to S3 failed: err=0x%x status=%d", err, status);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t upload_image(const uint8_t *data, size_t len)
{
    char presigned_url[PRESIGNED_URL_MAX];

    esp_err_t err = get_presigned_url(presigned_url, sizeof(presigned_url));
    if (err != ESP_OK) return err;

    err = put_image(presigned_url, data, len);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Image uploaded (%d bytes)", len);
    }
    return err;
}
