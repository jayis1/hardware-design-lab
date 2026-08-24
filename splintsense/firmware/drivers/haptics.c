/*
 * SplintSense haptic policy
 * Author: jayis1
 */
#include "haptics.h"

const char *haptics_pattern_for_alert(alert_level_t level, float persistence_minutes)
{
    switch (level) {
    case ALERT_CRITICAL:
        return persistence_minutes > 18.0f ? "triple-long" : "double-long";
    case ALERT_WARNING:
        return persistence_minutes > 10.0f ? "triple-short" : "double-short";
    case ALERT_CAUTION:
        return "single-short";
    case ALERT_INFO:
        return "tick";
    case ALERT_NONE:
    default:
        return "silent";
    }
}
