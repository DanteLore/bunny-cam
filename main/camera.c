#include "camera.h"
#include "esp_camera.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WARMUP_FRAMES 3

static const char *TAG = "bunny-cam-camera";

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

static void apply_auto_adjustments(void)
{
    sensor_t *s = esp_camera_sensor_get();

    /* Auto exposure -- AEC2 is the more advanced algorithm */
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 1);
    s->set_ae_level(s, 0);

    /* Auto gain -- high ceiling lets the sensor brighten low-light scenes */
    s->set_gain_ctrl(s, 1);
    s->set_gainceiling(s, GAINCEILING_8X);

    /* Auto white balance -- handles daylight vs shade vs artificial light */
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);  /* 0 = auto */

    /* Flip 180° for upside-down mounting */
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);

    /* Image quality corrections */
    s->set_bpc(s, 1);      /* black pixel correction */
    s->set_wpc(s, 1);      /* white pixel correction */
    s->set_raw_gma(s, 1);  /* gamma correction */
    s->set_lenc(s, 1);     /* lens shading correction */
}

static void discard_warmup_frames(void)
{
    for (int i = 0; i < WARMUP_FRAMES; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "AE settled after %d warmup frames", WARMUP_FRAMES);
}

void camera_init(void)
{
    ESP_ERROR_CHECK(esp_camera_init(&camera_config));
    apply_auto_adjustments();
    discard_warmup_frames();
}
