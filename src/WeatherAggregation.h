#pragma once
#include "OpenKNX.h"
#include <math.h>

// Fasst die Werte eines Intervalls zu einem Wert zusammen.
// NAN steht für "kein Wert vorhanden" und wird übersprungen.
// Rückgabe false, wenn kein einziger gültiger Wert dabei war.
inline bool weatherAggregate(const float* values, uint8_t count, PT_Aggregation mode, float& out)
{
    if (values == nullptr || count == 0) return false;

    float sum = 0.0f;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    uint8_t valid = 0;

    for (uint8_t i = 0; i < count; i++)
    {
        const float v = values[i];
        if (isnan(v)) continue;

        if (valid == 0)
        {
            minValue = v;
            maxValue = v;
        }
        else
        {
            if (v < minValue) minValue = v;
            if (v > maxValue) maxValue = v;
        }
        sum += v;
        valid++;
    }

    if (valid == 0) return false;

    switch (mode)
    {
        case PT_Aggregation::Mean: out = sum / (float)valid; break;
        case PT_Aggregation::Min: out = minValue; break;
        case PT_Aggregation::Max: out = maxValue; break;
        case PT_Aggregation::Sum: out = sum; break;
        case PT_Aggregation::Range: out = maxValue - minValue; break;
        default: out = sum / (float)valid; break;
    }
    return true;
}
