#include "OpenMeteoChannel.h"
#ifdef ARDUINO_ARCH_RP2040
#define OpenMeteoUrl "http://api.open-meteo.com/v1/forecast"
#else
#define OpenMeteoUrl "https://api.open-meteo.com/v1/forecast"
#endif

OpenMeteoChannel::OpenMeteoChannel(uint8_t index)
    : BaseWeatherChannel(index)
{
}

const std::string OpenMeteoChannel::name()
{
    return "OpenMeteo";
}

int16_t OpenMeteoChannel::fillWeather(CurrentWheatherData& currentWeather, ForecastDayWheatherDataWithDescription* dayForecasts, int numDays, ForecastHourWheatherData& hour1Weather, ForecastHourWheatherData& hour2Weather)
{
    // TODO check using csv-result

    // <Enumeration Text="Bitte wählen..."                  Value="0" Id="%ENID%" />
    // => will prevent creation of an open-meteo-channel

    // <Enumeration Text="Nicht kommerziell ('Free API')"   Value="1" Id="%ENID%" />
    // <Enumeration Text="API Subscription"                 Value="2" Id="%ENID%" />
    // <Enumeration Text="Selbst gehostet"                  Value="3" Id="%ENID%" />

    String url = OpenMeteoUrl;
    if (ParamIW_OpenMeteo_UsageLicense == 2 || ParamIW_OpenMeteo_UsageLicense == 3)
    {
        // usage with api-key or self hosted
        url = String(ParamIW_OpenMeteo_ServerURL, 80);
        url += "/v1/forecast";
    }

    // TODO set timezone, when implemented in common
    url += "?timezone=Europe%2FBerlin";

    if (ParamIW_OpenMeteo_UsageLicense == 2)
    {
        // with api-key, required for commercial usage
        url += "&appid=";
        // TODO CHECK: no URL-encoding, expected to not contain any special characters
        // TODO use new producer string-parameter macro
        url += String(ParamIW_OpenMeteo_APIKey, 40);
    }    

    url += "&latitude=";
    url += ParamIW_CHWeatherLocationType == 0 ? ParamBASE_Latitude : ParamIW_CHLatitude;
    url += "&longitude=";
    url += ParamIW_CHWeatherLocationType == 0 ? ParamBASE_Longitude : ParamIW_CHLongitude;

    url += "&current=";
    // TODO select based on configuration?
    // SAME for hourly ...
    url += "temperature_2m,apparent_temperature,relative_humidity_2m,surface_pressure,wind_speed_10m,wind_gusts_10m,wind_direction_10m,rain,snowfall,cloud_cover";
    // "Every weather variable available in hourly data, is available as current condition as well." [Open-Meteo API-Doc]
    url += ",uv_index";
    // ... SAME for hourly

    url += "&hourly=";
    // SAME as current ...
    url += "temperature_2m,apparent_temperature,relative_humidity_2m,surface_pressure,wind_speed_10m,wind_gusts_10m,wind_direction_10m,rain,snowfall,cloud_cover";
    // "Every weather variable available in hourly data, is available as current condition as well." [Open-Meteo API-Doc]
    url += ",uv_index";
    // ... SAME as current
    // additional value for forecast:
    url += ",precipitation_probability";

    url += "&daily=";
    url += "temperature_2m_min,temperature_2m_max,wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant,rain_sum,snowfall_sum,precipitation_probability_max,uv_index_max,et0_fao_evapotranspiration";

    // allow easy finding of hour for forcast
    url += "&timeformat=unixtime";

    // depends on forecast length, current day is included in day count
    url += "&forecast_days=";
    url += String(numDays);

#ifdef OPENKNX_DEBUG
    const size_t urlLen = url.length();
    const size_t lineLen = 100;
    for (size_t i = 0; i < urlLen; i += lineLen)
    {
        logDebugP("Call URL: %s", url.substring(i, std::min(i + lineLen, urlLen)).c_str());
    }
#endif

    HTTPClient http;
#ifdef ARDUINO_ARCH_RP2040
    if (url.startsWith("https://"))
        http.setInsecure();
#endif
    http.begin(url);

    // Send HTTP GET request
    auto httpStatus = http.GET();
    if (httpStatus != 200)
    {
        http.end();
        return httpStatus;
    }

    String jsonStr = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError jsonErr = deserializeJson(doc, jsonStr);
    if (jsonErr) {
        logErrorP("JSON parse error: %s", jsonErr.c_str());
        return -2;
    }

    JsonObject current = doc["current"];
    fillForecast(current, currentWeather);

    /*
    "daily_units": {
        "time": "unixtime",
        "temperature_2m_max": "°C",
        "temperature_2m_min": "°C"
    },
    "daily": {
        "time": [
            1722722400,
            1722808800
        ],
        "temperature_2m_max": [
            28.1,
            21.7
        ],
        "temperature_2m_min": [
            17,
            18.3
        ]
    }
    */
    JsonObject daily = doc["daily"];
    JsonObject hourly = doc["hourly"];
    for (int i = 0; i < numDays; i++)
    {
        fillForecast(daily, hourly, i, dayForecasts[i]);
    }

    /*
    "hourly_units": {
        "time": "unixtime",
        "temperature_2m": "°C"
    },
    "hourly": {
        "time": [
            1722722400,
            ..
            1722891600
        ],
        "temperature_2m": [
            20.7,
            ..
            18.3
        ]
    },
    */

    // find the index of the following hour
    const uint32_t curTimestamp = current["time"];
    JsonArray hourlyTimes = hourly["time"];
    int hour = 0;
    for (JsonVariant t : hourlyTimes)
    {
        if (t > curTimestamp)
        {
            // found first hour after current hour
            break;
        }
        hour++;
    }

    fillForecast(hourly, hour + 0, hour1Weather);
    fillForecast(hourly, hour + 1, hour2Weather);

    return httpStatus;
}

void OpenMeteoChannel::fillForecast(JsonObject& json, CurrentWheatherData& wheater)
{
    wheater.temperature_C          = json["temperature_2m"];
    wheater.temperatureFeelsLike_C = json["apparent_temperature"];
    wheater.humidity_percent       = json["relative_humidity_2m"];
    wheater.pressure_hPa           = json["surface_pressure"];       // at ground level
    wheater.windSpeed_Km_h         = json["wind_speed_10m"];         // open-meteo returns value in Km/h
    wheater.windGust_Km_h          = json["wind_gusts_10m"];         // open-meteo returns value in Km/h
    wheater.windDirection_deg      = json["wind_direction_10m"];
    wheater.rain_mm                = json["rain"];
    wheater.snow_mm                = 0.1 * (float)json["snowfall"];  // open-meteo returns value in cm
    wheater.uvi_unitOne            = json["uv_index"];
    wheater.cloudsCover_percent    = json["cloud_cover"];

    // TODO check integraten of additional fields from Open-Meteo:
    // * Is Day or Night [0/1] - not planned, as day-calc is available in LOG
    // * Precipitation   [mm] Gesamtniederschlagsmenge, also womöglich "nur" die Summe aller Arten ?
    // * Showers         [mm] ?
    // * Weather Code    [0..100] - see end of https://open-meteo.com/en/docs
}

void OpenMeteoChannel::fillForecast(JsonObject& json, int vi, ForecastHourWheatherData& wheater)
{
    // same as for current, but value-arrays instead of values
    wheater.temperature_C                      = json["temperature_2m"][vi];
    wheater.temperatureFeelsLike_C             = json["apparent_temperature"][vi];
    wheater.humidity_percent                   = json["relative_humidity_2m"][vi];
    wheater.pressure_hPa                       = json["surface_pressure"][vi];      // at ground level
    wheater.windSpeed_Km_h                     = json["wind_speed_10m"][vi];        // open-meteo returns value in Km/h
    wheater.windGust_Km_h                      = json["wind_gusts_10m"][vi];        // open-meteo returns value in Km/h
    wheater.windDirection_deg                  = json["wind_direction_10m"][vi];
    wheater.rain_mm                            = json["rain"][vi];
    wheater.snow_mm                            = 0.1 * (float)json["snowfall"][vi]; // open-meteo returns value in cm
    wheater.uvi_unitOne                        = json["uv_index"][vi];
    wheater.cloudsCover_percent                = json["cloud_cover"][vi];

    wheater.probabilityOfPrecipitation_percent = json["precipitation_probability"][vi];
}

float OpenMeteoChannel::avg(JsonArray& arr, int begin, int n)
{
    float sum = 0.0;
    for (size_t i = 0; i < n; i++)
    {
        sum += (float)arr[begin + i];
    }
    return sum / n;
}

void OpenMeteoChannel::fillForecast(JsonObject& json, JsonObject& jsonHourly, int vi, ForecastDayWheatherData& wheater)
{
    const int vih = 24 * vi;
    const int vihNight = vih +  0;
    const int vihMorn  = vih +  6;
    const int vihDay   = vih + 12;
    const int vihEve   = vih + 18;

    JsonArray temperature = jsonHourly["temperature_2m"];
    wheater.temperatureNight_C   = temperature[vihNight];
    wheater.temperatureMorning_C = temperature[vihMorn];
    wheater.temperatureDay_C     = temperature[vihDay];
    wheater.temperatureEvening_C = temperature[vihEve];

    wheater.temperatureMin_C = json["temperature_2m_min"][vi];
    wheater.temperatureMax_C = json["temperature_2m_max"][vi];

    JsonArray apparentTemperature = jsonHourly["apparent_temperature"];
    wheater.temperatureFeelsLikeNight_C   = apparentTemperature[vihNight];
    wheater.temperatureFeelsLikeMorning_C = apparentTemperature[vihMorn];
    wheater.temperatureFeelsLikeDay_C     = apparentTemperature[vihDay];
    wheater.temperatureFeelsLikeEvening_C = apparentTemperature[vihEve];

    JsonArray hourlyHumidity = jsonHourly["relative_humidity_2m"];
    wheater.humidity_percent = avg(hourlyHumidity, vih, 24);

    JsonArray hourlyPressure = jsonHourly["surface_pressure"];
    wheater.pressure_hPa = avg(hourlyPressure, vih, 24);

    wheater.windSpeed_Km_h    = json["wind_speed_10m_max"][vi];          // open-meteo returns value in Km/h
    wheater.windGust_Km_h     = json["wind_gusts_10m_max"][vi];          // open-meteo returns value in Km/h
    wheater.windDirection_deg = json["wind_direction_10m_dominant"][vi];

    wheater.rain_mm = json["rain_sum"][vi];
    wheater.snow_mm = 0.1 * (float)json["snowfall_sum"][vi]; // open-meteo returns value in cm
    wheater.probabilityOfPrecipitation_percent = json["precipitation_probability_max"][vi];
    wheater.uvi_unitOne = json["uv_index_max"][vi];

    JsonArray hourlyCloud = jsonHourly["cloud_cover"];
    wheater.cloudsCover_percent = avg(hourlyCloud, vih, 24);

    // ET₀ Reference Evapotranspiration (only available in Open-Meteo)
    wheater.et0_mm = json["et0_fao_evapotranspiration"][vi];
}
