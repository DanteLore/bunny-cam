#include "suntime.h"
#include <time.h>

/* Approximate sunrise/sunset for Newbury, UK (51.4°N, 1.3°W), UTC, mid-month.
 * Times are in minutes from midnight UTC. */
static const struct {
    int rise_min;
    int set_min;
} sun_table[12] = {
    { 8*60+ 1, 16*60+14 },  /* Jan */
    { 7*60+27, 17*60+ 7 },  /* Feb */
    { 6*60+17, 18*60+ 3 },  /* Mar */
    { 5*60+ 2, 19*60+ 0 },  /* Apr */
    { 4*60+ 2, 19*60+55 },  /* May */
    { 3*60+44, 20*60+21 },  /* Jun */
    { 4*60+ 2, 20*60+ 9 },  /* Jul */
    { 4*60+55, 19*60+16 },  /* Aug */
    { 5*60+52, 18*60+ 6 },  /* Sep */
    { 6*60+48, 16*60+59 },  /* Oct */
    { 7*60+37, 16*60+ 9 },  /* Nov */
    { 8*60+ 4, 15*60+57 },  /* Dec */
};

bool suntime_is_daytime(time_t utc)
{
    if (utc == 0) return true;  /* unknown — assume daytime to avoid missing shots */

    struct tm t;
    gmtime_r(&utc, &t);
    int day_min = t.tm_hour * 60 + t.tm_min;
    return day_min >= sun_table[t.tm_mon].rise_min &&
           day_min <  sun_table[t.tm_mon].set_min;
}
