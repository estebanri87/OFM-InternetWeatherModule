// ERZEUGT von tools/Generate-Catalog.ps1 aus src/weather-catalog.json - nicht von Hand aendern.
//
// Der Messwert-Enum selbst (enum class PT_Measurand) kommt vom OpenKNXproducer
// aus der share.xml (op:headerExport="enum") und steht in knxprod.h.
// Diese Datei ergaenzt ihn um die Metadaten, die die ETS nicht kennt.
#pragma once
#include <stdint.h>

#define IW_MEASURAND_COUNT 67

enum class WeatherLevel : uint8_t
{
    Current,
    Minutely15,
    Hourly,
    Daily,
};

enum class WeatherDpt : uint8_t
{
    Dpt16_1,
    Dpt5_10,
    Dpt9_1,
    Dpt9_7,
    Dpt9_6,
    Dpt9_28,
    Dpt5_3,
    Dpt9_26,
    Dpt5_1,
    Dpt7_5,
    Dpt10_1,
    Dpt9_22,
    Dpt9_31,
};

struct WeatherMeasurandInfo
{
    uint8_t      id;            // = PT_Measurand
    const char*  omVariable;    // Open-Meteo-Variable, nullptr bei abgeleiteten Werten
    WeatherLevel level;
    WeatherDpt   dpt;
    float        scale;         // Rohwert * scale = KNX-Wert
    bool         aggregatable;
};

static const WeatherMeasurandInfo IW_MEASURANDS[IW_MEASURAND_COUNT] =
{
    {  16, nullptr,                           WeatherLevel::Daily,       WeatherDpt::Dpt16_1,       1.0f, false },   // OpenKNXWetterTextDay
    {  17, "weather_code",                    WeatherLevel::Current,     WeatherDpt::Dpt5_10,       1.0f, false },   // WettercodeWMOCur
    {  18, "weather_code",                    WeatherLevel::Hourly,      WeatherDpt::Dpt5_10,       1.0f, false },   // WettercodeWMOHour
    {  19, "weather_code",                    WeatherLevel::Daily,       WeatherDpt::Dpt5_10,       1.0f, false },   // WettercodeWMODay
    {  32, "temperature_2m",                  WeatherLevel::Current,     WeatherDpt::Dpt9_1,        1.0f, true  },   // Temperatur2MCur
    {  33, "temperature_2m",                  WeatherLevel::Minutely15,  WeatherDpt::Dpt9_1,        1.0f, true  },   // Temperatur2MQ15
    {  34, "temperature_2m",                  WeatherLevel::Hourly,      WeatherDpt::Dpt9_1,        1.0f, true  },   // Temperatur2MHour
    {  35, "temperature_2m_max",              WeatherLevel::Daily,       WeatherDpt::Dpt9_1,        1.0f, true  },   // Temperatur2MMaximumDay
    {  36, "temperature_2m_min",              WeatherLevel::Daily,       WeatherDpt::Dpt9_1,        1.0f, true  },   // Temperatur2MMinimumDay
    {  37, "temperature_2m_mean",             WeatherLevel::Daily,       WeatherDpt::Dpt9_1,        1.0f, true  },   // Temperatur2MMittelDay
    {  38, "apparent_temperature",            WeatherLevel::Current,     WeatherDpt::Dpt9_1,        1.0f, true  },   // GefuehlteTemperaturCur
    {  39, "apparent_temperature",            WeatherLevel::Minutely15,  WeatherDpt::Dpt9_1,        1.0f, true  },   // GefuehlteTemperaturQ15
    {  40, "apparent_temperature",            WeatherLevel::Hourly,      WeatherDpt::Dpt9_1,        1.0f, true  },   // GefuehlteTemperaturHour
    {  41, "apparent_temperature_max",        WeatherLevel::Daily,       WeatherDpt::Dpt9_1,        1.0f, true  },   // GefuehlteTemperaturMaximumDay
    {  42, "apparent_temperature_min",        WeatherLevel::Daily,       WeatherDpt::Dpt9_1,        1.0f, true  },   // GefuehlteTemperaturMinimumDay
    {  43, "apparent_temperature_mean",       WeatherLevel::Daily,       WeatherDpt::Dpt9_1,        1.0f, true  },   // GefuehlteTemperaturMittelDay
    {  48, "relative_humidity_2m",            WeatherLevel::Current,     WeatherDpt::Dpt9_7,        1.0f, true  },   // RelativeLuftfeuchte2MCur
    {  49, "relative_humidity_2m",            WeatherLevel::Minutely15,  WeatherDpt::Dpt9_7,        1.0f, true  },   // RelativeLuftfeuchte2MQ15
    {  50, "relative_humidity_2m",            WeatherLevel::Hourly,      WeatherDpt::Dpt9_7,        1.0f, true  },   // RelativeLuftfeuchte2MHour
    {  51, "surface_pressure",                WeatherLevel::Current,     WeatherDpt::Dpt9_6,      100.0f, true  },   // LuftdruckCur
    {  52, "surface_pressure",                WeatherLevel::Hourly,      WeatherDpt::Dpt9_6,      100.0f, true  },   // LuftdruckHour
    {  64, "wind_speed_10m",                  WeatherLevel::Current,     WeatherDpt::Dpt9_28,       1.0f, true  },   // Windgeschwindigkeit10MCur
    {  65, "wind_speed_10m",                  WeatherLevel::Minutely15,  WeatherDpt::Dpt9_28,       1.0f, true  },   // Windgeschwindigkeit10MQ15
    {  66, "wind_speed_10m",                  WeatherLevel::Hourly,      WeatherDpt::Dpt9_28,       1.0f, true  },   // Windgeschwindigkeit10MHour
    {  67, "wind_speed_10m_max",              WeatherLevel::Daily,       WeatherDpt::Dpt9_28,       1.0f, true  },   // Windgeschwindigkeit10MMaximumDay
    {  68, "wind_gusts_10m",                  WeatherLevel::Current,     WeatherDpt::Dpt9_28,       1.0f, true  },   // Windboeen10MCur
    {  69, "wind_gusts_10m",                  WeatherLevel::Minutely15,  WeatherDpt::Dpt9_28,       1.0f, true  },   // Windboeen10MQ15
    {  70, "wind_gusts_10m",                  WeatherLevel::Hourly,      WeatherDpt::Dpt9_28,       1.0f, true  },   // Windboeen10MHour
    {  71, "wind_gusts_10m_max",              WeatherLevel::Daily,       WeatherDpt::Dpt9_28,       1.0f, true  },   // Windboeen10MMaximumDay
    {  72, "wind_direction_10m",              WeatherLevel::Current,     WeatherDpt::Dpt5_3,        1.0f, false },   // Windrichtung10MCur
    {  73, "wind_direction_10m",              WeatherLevel::Minutely15,  WeatherDpt::Dpt5_3,        1.0f, false },   // Windrichtung10MQ15
    {  74, "wind_direction_10m",              WeatherLevel::Hourly,      WeatherDpt::Dpt5_3,        1.0f, false },   // Windrichtung10MHour
    {  75, "wind_direction_10m_dominant",     WeatherLevel::Daily,       WeatherDpt::Dpt5_3,        1.0f, false },   // Windrichtung10MVorherrschendDay
    {  80, "precipitation",                   WeatherLevel::Current,     WeatherDpt::Dpt9_26,       1.0f, true  },   // NiederschlagCur
    {  81, "precipitation",                   WeatherLevel::Minutely15,  WeatherDpt::Dpt9_26,       1.0f, true  },   // NiederschlagQ15
    {  82, "precipitation",                   WeatherLevel::Hourly,      WeatherDpt::Dpt9_26,       1.0f, true  },   // NiederschlagHour
    {  83, "precipitation_sum",               WeatherLevel::Daily,       WeatherDpt::Dpt9_26,       1.0f, true  },   // NiederschlagSummeDay
    {  84, "rain",                            WeatherLevel::Current,     WeatherDpt::Dpt9_26,       1.0f, true  },   // RegenCur
    {  85, "rain",                            WeatherLevel::Minutely15,  WeatherDpt::Dpt9_26,       1.0f, true  },   // RegenQ15
    {  86, "rain",                            WeatherLevel::Hourly,      WeatherDpt::Dpt9_26,       1.0f, true  },   // RegenHour
    {  87, "rain_sum",                        WeatherLevel::Daily,       WeatherDpt::Dpt9_26,       1.0f, true  },   // RegenSummeDay
    {  88, "snowfall",                        WeatherLevel::Current,     WeatherDpt::Dpt9_26,      10.0f, true  },   // SchneefallCur
    {  89, "snowfall",                        WeatherLevel::Minutely15,  WeatherDpt::Dpt9_26,      10.0f, true  },   // SchneefallQ15
    {  90, "snowfall",                        WeatherLevel::Hourly,      WeatherDpt::Dpt9_26,      10.0f, true  },   // SchneefallHour
    {  91, "snowfall_sum",                    WeatherLevel::Daily,       WeatherDpt::Dpt9_26,      10.0f, true  },   // SchneefallSummeDay
    {  92, "precipitation_probability",       WeatherLevel::Hourly,      WeatherDpt::Dpt5_1,        1.0f, true  },   // NiederschlagswahrscheinlichkeitHour
    {  93, "precipitation_probability_max",   WeatherLevel::Daily,       WeatherDpt::Dpt5_1,        1.0f, true  },   // NiederschlagswahrscheinlichkeitMaximumDay
    {  96, "cloud_cover",                     WeatherLevel::Current,     WeatherDpt::Dpt5_1,        1.0f, true  },   // BewoelkungCur
    {  97, "cloud_cover",                     WeatherLevel::Hourly,      WeatherDpt::Dpt5_1,        1.0f, true  },   // BewoelkungHour
    {  98, "sunshine_duration",               WeatherLevel::Hourly,      WeatherDpt::Dpt7_5,        1.0f, true  },   // SonnenscheindauerHour
    {  99, "sunshine_duration",               WeatherLevel::Daily,       WeatherDpt::Dpt7_5,        1.0f, true  },   // SonnenscheindauerDay
    { 100, "daylight_duration",               WeatherLevel::Daily,       WeatherDpt::Dpt7_5,        1.0f, true  },   // TageslichtdauerDay
    { 101, "sunrise",                         WeatherLevel::Daily,       WeatherDpt::Dpt10_1,       1.0f, false },   // SonnenaufgangDay
    { 102, "sunset",                          WeatherLevel::Daily,       WeatherDpt::Dpt10_1,       1.0f, false },   // SonnenuntergangDay
    { 112, "shortwave_radiation",             WeatherLevel::Minutely15,  WeatherDpt::Dpt9_22,       1.0f, true  },   // GlobalstrahlungQ15
    { 113, "shortwave_radiation",             WeatherLevel::Hourly,      WeatherDpt::Dpt9_22,       1.0f, true  },   // GlobalstrahlungHour
    { 114, "direct_radiation",                WeatherLevel::Minutely15,  WeatherDpt::Dpt9_22,       1.0f, true  },   // DirektstrahlungQ15
    { 115, "direct_radiation",                WeatherLevel::Hourly,      WeatherDpt::Dpt9_22,       1.0f, true  },   // DirektstrahlungHour
    { 116, "diffuse_radiation",               WeatherLevel::Minutely15,  WeatherDpt::Dpt9_22,       1.0f, true  },   // DiffusstrahlungQ15
    { 117, "diffuse_radiation",               WeatherLevel::Hourly,      WeatherDpt::Dpt9_22,       1.0f, true  },   // DiffusstrahlungHour
    { 118, "direct_normal_irradiance",        WeatherLevel::Minutely15,  WeatherDpt::Dpt9_22,       1.0f, true  },   // DirektnormalstrahlungQ15
    { 119, "direct_normal_irradiance",        WeatherLevel::Hourly,      WeatherDpt::Dpt9_22,       1.0f, true  },   // DirektnormalstrahlungHour
    { 120, "uv_index",                        WeatherLevel::Current,     WeatherDpt::Dpt9_31,       1.0f, true  },   // UVIndexCur
    { 121, "uv_index",                        WeatherLevel::Hourly,      WeatherDpt::Dpt9_31,       1.0f, true  },   // UVIndexHour
    { 122, "uv_index_max",                    WeatherLevel::Daily,       WeatherDpt::Dpt9_31,       1.0f, true  },   // UVIndexMaximumDay
    { 128, "et0_fao_evapotranspiration",      WeatherLevel::Hourly,      WeatherDpt::Dpt9_26,       1.0f, true  },   // ET0ReferenzVerdunstungHour
    { 129, "et0_fao_evapotranspiration",      WeatherLevel::Daily,       WeatherDpt::Dpt9_26,       1.0f, true  },   // ET0ReferenzVerdunstungSummeDay
};
