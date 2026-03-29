#pragma once

#include <time.h>

/* Sync system clock via SNTP. On success, persists the time to NVS.
 * On failure, falls back to the last time stored in NVS.
 * Returns the best known UTC time, or 0 if completely unknown. */
time_t sntp_sync(void);
