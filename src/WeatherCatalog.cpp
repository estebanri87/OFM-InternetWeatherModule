#ifndef OPENKNX_INTERNETWEATHER_IGNORE
#include "WeatherCatalog.h"

const WeatherMeasurandInfo* weatherMeasurand(uint8_t id)
{
    if (id == 0) return nullptr;

    // IW_MEASURANDS ist nach id aufsteigend sortiert.
    uint8_t lo = 0;
    uint8_t hi = IW_MEASURAND_COUNT - 1;
    while (lo <= hi)
    {
        uint8_t mid = (uint8_t)((lo + hi) / 2);
        uint8_t midId = IW_MEASURANDS[mid].id;
        if (midId == id) return &IW_MEASURANDS[mid];
        if (midId < id)
            lo = (uint8_t)(mid + 1);
        else
        {
            if (mid == 0) break;
            hi = (uint8_t)(mid - 1);
        }
    }
    return nullptr;
}

const WeatherProviderVar* weatherProviderVar(const WeatherMeasurandInfo& info, WeatherProvider provider)
{
    const WeatherProviderVar* pv = (provider == WeatherProvider::OpenWeatherMap) ? &info.openweathermap : &info.openmeteo;
    return (pv->var == nullptr) ? nullptr : pv;
}

uint32_t weatherLevelSeconds(WeatherLevel level)
{
    switch (level)
    {
        case WeatherLevel::Current: return 0;
        case WeatherLevel::Minutely15: return 900;
        case WeatherLevel::Hourly: return 3600;
        case WeatherLevel::Daily: return 0;
    }
    return 0;
}

const char* weatherLevelName(WeatherLevel level)
{
    switch (level)
    {
        case WeatherLevel::Current: return "Aktuell";
        case WeatherLevel::Minutely15: return "15-Min";
        case WeatherLevel::Hourly: return "Stunde";
        case WeatherLevel::Daily: return "Tag";
    }
    return "?";
}

const char* weatherMeasurandName(const WeatherMeasurandInfo& info)
{
    // Für die Diagnose reicht der Variablenname des Anbieters, der die Größe
    // liefert; abgeleitete Werte tragen keinen.
    if (info.openmeteo.var != nullptr) return info.openmeteo.var;
    if (info.openweathermap.var != nullptr) return info.openweathermap.var;
    return "(abgeleitet)";
}
#endif
