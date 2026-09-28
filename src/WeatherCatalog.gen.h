// ERZEUGT von tools/Generate-Catalog.ps1 aus src/weather-catalog.json - nicht von Hand ändern.
//
// Der Messwert-Enum (enum class PT_Measurand) kommt vom OpenKNXproducer aus der
// share.xml (op:headerExport="enum") und steht in knxprod.h. Diese Datei ergänzt
// ihn um die Metadaten, die die ETS nicht kennt.
#pragma once
#include <stdint.h>

#define IW_MEASURAND_COUNT 86

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
    Dpt7_1,
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

// Rohwert * scale = KNX-Wert. var == nullptr: Anbieter liefert diese Größe nicht.
struct WeatherProviderVar
{
    const char* var;
    float       scale;
};

struct WeatherMeasurandInfo
{
    uint8_t            id;            // = PT_Measurand
    WeatherProviderVar openweathermap;  // OpenWeatherMap
    WeatherProviderVar openmeteo;       // Open-Meteo
    WeatherLevel       level;
    WeatherDpt         dpt;
    bool               aggregatable;
};

// Nach id aufsteigend sortiert - binäre Suche zulässig.
static const WeatherMeasurandInfo IW_MEASURANDS[IW_MEASURAND_COUNT] =
{
    {   1, { "@openknx_text", 1.0f },         { "@openknx_text", 1.0f },         WeatherLevel::Daily,       WeatherDpt::Dpt16_1,    false },   // OpenKNXWetterTextDay
    {   2, { nullptr, 1.0f },                 { "weather_code", 1.0f },          WeatherLevel::Current,     WeatherDpt::Dpt5_10,    false },   // WettercodeWMOCur
    {   3, { nullptr, 1.0f },                 { "weather_code", 1.0f },          WeatherLevel::Hourly,      WeatherDpt::Dpt5_10,    false },   // WettercodeWMOHour
    {   4, { nullptr, 1.0f },                 { "weather_code", 1.0f },          WeatherLevel::Daily,       WeatherDpt::Dpt5_10,    false },   // WettercodeWMODay
    {   5, { "weather.0.id", 1.0f },          { nullptr, 1.0f },                 WeatherLevel::Current,     WeatherDpt::Dpt7_1,     false },   // WetterzustandCodeCur
    {   6, { "weather.0.id", 1.0f },          { nullptr, 1.0f },                 WeatherLevel::Hourly,      WeatherDpt::Dpt7_1,     false },   // WetterzustandCodeHour
    {   7, { "weather.0.id", 1.0f },          { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt7_1,     false },   // WetterzustandCodeDay
    {  10, { "temp", 1.0f },                  { "temperature_2m", 1.0f },        WeatherLevel::Current,     WeatherDpt::Dpt9_1,     true  },   // Temperatur2MCur
    {  11, { nullptr, 1.0f },                 { "temperature_2m", 1.0f },        WeatherLevel::Minutely15,  WeatherDpt::Dpt9_1,     true  },   // Temperatur2MQ15
    {  12, { "temp", 1.0f },                  { "temperature_2m", 1.0f },        WeatherLevel::Hourly,      WeatherDpt::Dpt9_1,     true  },   // Temperatur2MHour
    {  13, { "temp.max", 1.0f },              { "temperature_2m_max", 1.0f },    WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // TemperaturMaximumDay
    {  14, { "temp.min", 1.0f },              { "temperature_2m_min", 1.0f },    WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // TemperaturMinimumDay
    {  15, { nullptr, 1.0f },                 { "temperature_2m_mean", 1.0f },   WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // TemperaturMittelDay
    {  16, { "temp.morn", 1.0f },             { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // TemperaturMorgenDay
    {  17, { "temp.day", 1.0f },              { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // TemperaturTagDay
    {  18, { "temp.eve", 1.0f },              { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // TemperaturAbendDay
    {  19, { "temp.night", 1.0f },            { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // TemperaturNachtDay
    {  20, { "feels_like", 1.0f },            { "apparent_temperature", 1.0f },  WeatherLevel::Current,     WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturCur
    {  21, { nullptr, 1.0f },                 { "apparent_temperature", 1.0f },  WeatherLevel::Minutely15,  WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturQ15
    {  22, { "feels_like", 1.0f },            { "apparent_temperature", 1.0f },  WeatherLevel::Hourly,      WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturHour
    {  23, { nullptr, 1.0f },                 { "apparent_temperature_max", 1.0f }, WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturMaximumDay
    {  24, { nullptr, 1.0f },                 { "apparent_temperature_min", 1.0f }, WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturMinimumDay
    {  25, { nullptr, 1.0f },                 { "apparent_temperature_mean", 1.0f }, WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturMittelDay
    {  26, { "feels_like.morn", 1.0f },       { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturMorgenDay
    {  27, { "feels_like.day", 1.0f },        { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturTagDay
    {  28, { "feels_like.eve", 1.0f },        { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturAbendDay
    {  29, { "feels_like.night", 1.0f },      { nullptr, 1.0f },                 WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // GefuehlteTemperaturNachtDay
    {  30, { "dew_point", 1.0f },             { "dew_point_2m", 1.0f },          WeatherLevel::Current,     WeatherDpt::Dpt9_1,     true  },   // TaupunktCur
    {  31, { "dew_point", 1.0f },             { "dew_point_2m", 1.0f },          WeatherLevel::Hourly,      WeatherDpt::Dpt9_1,     true  },   // TaupunktHour
    {  32, { "dew_point", 1.0f },             { "dew_point_2m_mean", 1.0f },     WeatherLevel::Daily,       WeatherDpt::Dpt9_1,     true  },   // TaupunktDay
    {  40, { "humidity", 1.0f },              { "relative_humidity_2m", 1.0f },  WeatherLevel::Current,     WeatherDpt::Dpt9_7,     true  },   // RelativeLuftfeuchte2MCur
    {  41, { nullptr, 1.0f },                 { "relative_humidity_2m", 1.0f },  WeatherLevel::Minutely15,  WeatherDpt::Dpt9_7,     true  },   // RelativeLuftfeuchte2MQ15
    {  42, { "humidity", 1.0f },              { "relative_humidity_2m", 1.0f },  WeatherLevel::Hourly,      WeatherDpt::Dpt9_7,     true  },   // RelativeLuftfeuchte2MHour
    {  43, { "humidity", 1.0f },              { "relative_humidity_2m_mean", 1.0f }, WeatherLevel::Daily,       WeatherDpt::Dpt9_7,     true  },   // RelativeLuftfeuchte2MDay
    {  44, { "pressure", 100.0f },            { "surface_pressure", 100.0f },    WeatherLevel::Current,     WeatherDpt::Dpt9_6,     true  },   // LuftdruckCur
    {  45, { "pressure", 100.0f },            { "surface_pressure", 100.0f },    WeatherLevel::Hourly,      WeatherDpt::Dpt9_6,     true  },   // LuftdruckHour
    {  46, { "pressure", 100.0f },            { "surface_pressure_mean", 100.0f }, WeatherLevel::Daily,       WeatherDpt::Dpt9_6,     true  },   // LuftdruckDay
    {  47, { "visibility", 1.0f },            { nullptr, 1.0f },                 WeatherLevel::Current,     WeatherDpt::Dpt7_1,     true  },   // SichtweiteCur
    {  48, { "visibility", 1.0f },            { nullptr, 1.0f },                 WeatherLevel::Hourly,      WeatherDpt::Dpt7_1,     true  },   // SichtweiteHour
    {  50, { "wind_speed", 3.6f },            { "wind_speed_10m", 1.0f },        WeatherLevel::Current,     WeatherDpt::Dpt9_28,    true  },   // Windgeschwindigkeit10MCur
    {  51, { nullptr, 1.0f },                 { "wind_speed_10m", 1.0f },        WeatherLevel::Minutely15,  WeatherDpt::Dpt9_28,    true  },   // Windgeschwindigkeit10MQ15
    {  52, { "wind_speed", 3.6f },            { "wind_speed_10m", 1.0f },        WeatherLevel::Hourly,      WeatherDpt::Dpt9_28,    true  },   // Windgeschwindigkeit10MHour
    {  53, { "wind_speed", 3.6f },            { "wind_speed_10m_max", 1.0f },    WeatherLevel::Daily,       WeatherDpt::Dpt9_28,    true  },   // WindgeschwindigkeitMaximumDay
    {  54, { "wind_gust", 3.6f },             { "wind_gusts_10m", 1.0f },        WeatherLevel::Current,     WeatherDpt::Dpt9_28,    true  },   // Windboeen10MCur
    {  55, { nullptr, 1.0f },                 { "wind_gusts_10m", 1.0f },        WeatherLevel::Minutely15,  WeatherDpt::Dpt9_28,    true  },   // Windboeen10MQ15
    {  56, { "wind_gust", 3.6f },             { "wind_gusts_10m", 1.0f },        WeatherLevel::Hourly,      WeatherDpt::Dpt9_28,    true  },   // Windboeen10MHour
    {  57, { "wind_gust", 3.6f },             { "wind_gusts_10m_max", 1.0f },    WeatherLevel::Daily,       WeatherDpt::Dpt9_28,    true  },   // WindboeenMaximumDay
    {  58, { "wind_deg", 1.0f },              { "wind_direction_10m", 1.0f },    WeatherLevel::Current,     WeatherDpt::Dpt5_3,     false },   // Windrichtung10MCur
    {  59, { nullptr, 1.0f },                 { "wind_direction_10m", 1.0f },    WeatherLevel::Minutely15,  WeatherDpt::Dpt5_3,     false },   // Windrichtung10MQ15
    {  60, { "wind_deg", 1.0f },              { "wind_direction_10m", 1.0f },    WeatherLevel::Hourly,      WeatherDpt::Dpt5_3,     false },   // Windrichtung10MHour
    {  61, { "wind_deg", 1.0f },              { "wind_direction_10m_dominant", 1.0f }, WeatherLevel::Daily,       WeatherDpt::Dpt5_3,     false },   // WindrichtungVorherrschendDay
    {  70, { nullptr, 1.0f },                 { "precipitation", 1.0f },         WeatherLevel::Current,     WeatherDpt::Dpt9_26,    true  },   // NiederschlagCur
    {  71, { nullptr, 1.0f },                 { "precipitation", 1.0f },         WeatherLevel::Minutely15,  WeatherDpt::Dpt9_26,    true  },   // NiederschlagQ15
    {  72, { nullptr, 1.0f },                 { "precipitation", 1.0f },         WeatherLevel::Hourly,      WeatherDpt::Dpt9_26,    true  },   // NiederschlagHour
    {  73, { nullptr, 1.0f },                 { "precipitation_sum", 1.0f },     WeatherLevel::Daily,       WeatherDpt::Dpt9_26,    true  },   // NiederschlagSummeDay
    {  74, { "rain.1h", 1.0f },               { "rain", 1.0f },                  WeatherLevel::Current,     WeatherDpt::Dpt9_26,    true  },   // RegenCur
    {  75, { nullptr, 1.0f },                 { "rain", 1.0f },                  WeatherLevel::Minutely15,  WeatherDpt::Dpt9_26,    true  },   // RegenQ15
    {  76, { "rain.1h", 1.0f },               { "rain", 1.0f },                  WeatherLevel::Hourly,      WeatherDpt::Dpt9_26,    true  },   // RegenHour
    {  77, { "rain", 1.0f },                  { "rain_sum", 1.0f },              WeatherLevel::Daily,       WeatherDpt::Dpt9_26,    true  },   // RegenSummeDay
    {  78, { "snow.1h", 1.0f },               { "snowfall", 10.0f },             WeatherLevel::Current,     WeatherDpt::Dpt9_26,    true  },   // SchneefallCur
    {  79, { nullptr, 1.0f },                 { "snowfall", 10.0f },             WeatherLevel::Minutely15,  WeatherDpt::Dpt9_26,    true  },   // SchneefallQ15
    {  80, { "snow.1h", 1.0f },               { "snowfall", 10.0f },             WeatherLevel::Hourly,      WeatherDpt::Dpt9_26,    true  },   // SchneefallHour
    {  81, { "snow", 1.0f },                  { "snowfall_sum", 10.0f },         WeatherLevel::Daily,       WeatherDpt::Dpt9_26,    true  },   // SchneefallSummeDay
    {  82, { "pop", 100.0f },                 { "precipitation_probability", 1.0f }, WeatherLevel::Hourly,      WeatherDpt::Dpt5_1,     true  },   // NiederschlagswahrscheinlichkeitHour
    {  83, { "pop", 100.0f },                 { "precipitation_probability_max", 1.0f }, WeatherLevel::Daily,       WeatherDpt::Dpt5_1,     true  },   // NiederschlagswahrscheinlichkeitMaximumDay
    {  90, { "clouds", 1.0f },                { "cloud_cover", 1.0f },           WeatherLevel::Current,     WeatherDpt::Dpt5_1,     true  },   // BewoelkungCur
    {  91, { "clouds", 1.0f },                { "cloud_cover", 1.0f },           WeatherLevel::Hourly,      WeatherDpt::Dpt5_1,     true  },   // BewoelkungHour
    {  92, { "clouds", 1.0f },                { "cloud_cover_mean", 1.0f },      WeatherLevel::Daily,       WeatherDpt::Dpt5_1,     true  },   // BewoelkungDay
    {  93, { nullptr, 1.0f },                 { "sunshine_duration", 1.0f },     WeatherLevel::Hourly,      WeatherDpt::Dpt7_5,     true  },   // SonnenscheindauerHour
    {  94, { nullptr, 1.0f },                 { "sunshine_duration", 1.0f },     WeatherLevel::Daily,       WeatherDpt::Dpt7_5,     true  },   // SonnenscheindauerDay
    {  95, { nullptr, 1.0f },                 { "daylight_duration", 1.0f },     WeatherLevel::Daily,       WeatherDpt::Dpt7_5,     true  },   // TageslichtdauerDay
    {  96, { "sunrise", 1.0f },               { "sunrise", 1.0f },               WeatherLevel::Daily,       WeatherDpt::Dpt10_1,    false },   // SonnenaufgangDay
    {  97, { "sunset", 1.0f },                { "sunset", 1.0f },                WeatherLevel::Daily,       WeatherDpt::Dpt10_1,    false },   // SonnenuntergangDay
    { 110, { nullptr, 1.0f },                 { "shortwave_radiation", 1.0f },   WeatherLevel::Minutely15,  WeatherDpt::Dpt9_22,    true  },   // GlobalstrahlungQ15
    { 111, { nullptr, 1.0f },                 { "shortwave_radiation", 1.0f },   WeatherLevel::Hourly,      WeatherDpt::Dpt9_22,    true  },   // GlobalstrahlungHour
    { 112, { nullptr, 1.0f },                 { "direct_radiation", 1.0f },      WeatherLevel::Minutely15,  WeatherDpt::Dpt9_22,    true  },   // DirektstrahlungQ15
    { 113, { nullptr, 1.0f },                 { "direct_radiation", 1.0f },      WeatherLevel::Hourly,      WeatherDpt::Dpt9_22,    true  },   // DirektstrahlungHour
    { 114, { nullptr, 1.0f },                 { "diffuse_radiation", 1.0f },     WeatherLevel::Minutely15,  WeatherDpt::Dpt9_22,    true  },   // DiffusstrahlungQ15
    { 115, { nullptr, 1.0f },                 { "diffuse_radiation", 1.0f },     WeatherLevel::Hourly,      WeatherDpt::Dpt9_22,    true  },   // DiffusstrahlungHour
    { 116, { nullptr, 1.0f },                 { "direct_normal_irradiance", 1.0f }, WeatherLevel::Minutely15,  WeatherDpt::Dpt9_22,    true  },   // DirektnormalstrahlungQ15
    { 117, { nullptr, 1.0f },                 { "direct_normal_irradiance", 1.0f }, WeatherLevel::Hourly,      WeatherDpt::Dpt9_22,    true  },   // DirektnormalstrahlungHour
    { 118, { "uvi", 1.0f },                   { "uv_index", 1.0f },              WeatherLevel::Current,     WeatherDpt::Dpt9_31,    true  },   // UVIndexCur
    { 119, { "uvi", 1.0f },                   { "uv_index", 1.0f },              WeatherLevel::Hourly,      WeatherDpt::Dpt9_31,    true  },   // UVIndexHour
    { 120, { "uvi", 1.0f },                   { "uv_index_max", 1.0f },          WeatherLevel::Daily,       WeatherDpt::Dpt9_31,    true  },   // UVIndexMaximumDay
    { 130, { nullptr, 1.0f },                 { "et0_fao_evapotranspiration", 1.0f }, WeatherLevel::Hourly,      WeatherDpt::Dpt9_26,    true  },   // ET0ReferenzVerdunstungHour
    { 131, { nullptr, 1.0f },                 { "et0_fao_evapotranspiration", 1.0f }, WeatherLevel::Daily,       WeatherDpt::Dpt9_26,    true  },   // ET0ReferenzVerdunstungSummeDay
};
