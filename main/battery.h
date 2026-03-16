#pragma once

/* Read battery voltage via ADC1_CH5 (GPIO33) and cache in RTC memory.
 * Must be called before wifi_connect() -- ADC1 is unavailable while WiFi is active. */
void battery_read_and_cache(void);

/* Returns the voltage (V) cached by the most recent battery_read_and_cache() call.
 * Value is retained across deep sleep cycles via RTC memory.
 * Returns -1.0 if no reading has been taken yet. */
float battery_last_voltage(void);
