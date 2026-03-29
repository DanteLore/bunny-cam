#pragma once

#include <stdbool.h>
#include <time.h>

/* Returns true if the UTC time falls between sunrise and sunset for Newbury, UK.
 * Returns true (assume daytime) if utc is 0 / unknown. */
bool suntime_is_daytime(time_t utc);
