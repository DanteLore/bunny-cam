#include "battery.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

/* GPIO13 = ADC2_CH4 -- free pin on AI Thinker ESP32-CAM, broken out on header.
 * ADC2 cannot be used while WiFi is active; read before wifi_connect(). */
#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_4
#define BATTERY_ADC_UNIT        ADC_UNIT_2

/* Voltage divider: 37k (high side) + 10k (low side).
 * Ratio measured empirically: 12V battery -> 2.4V at ADC pin = 5.0x */
#define VOLTAGE_DIVIDER_RATIO   5.0f

/* ESP32 ADC reads ~3% high due to nonlinearity in the mid-range.
 * Calibration factor derived from 3 measured vs reported readings (12-14V range). */
#define CALIBRATION_FACTOR      0.970f

#define NUM_SAMPLES             16

static const char *TAG = "bunny-cam-battery";

/* RTC memory -- survives deep sleep */
static RTC_DATA_ATTR float rtc_last_voltage = -1.0f;

void battery_read_and_cache(void)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = BATTERY_ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &chan_cfg));

    adc_cali_handle_t cali_handle;
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id  = BATTERY_ADC_UNIT,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle));

    int sum_mv = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        int raw, mv;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &raw));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &mv));
        sum_mv += mv;
    }

    float adc_v = (sum_mv / NUM_SAMPLES) / 1000.0f;
    rtc_last_voltage = adc_v * VOLTAGE_DIVIDER_RATIO * CALIBRATION_FACTOR;
    ESP_LOGI(TAG, "Battery: %.2fV (ADC pin: %.3fV)", rtc_last_voltage, adc_v);

    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(cali_handle));
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
}

float battery_last_voltage(void)
{
    return rtc_last_voltage;
}
