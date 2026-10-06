#ifndef OPENKNX_INTERNETWEATHER_IGNORE
#include "OpenMeteoChannel.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"

// Obergrenze der je Zeitebene geparsten Reihe. Durch den bedarfsgerechten
// Zeitraum bleiben reale Anfragen weit darunter.
#define IW_OM_MAX_SERIES 200

#ifdef ARDUINO_ARCH_RP2040
    #define IW_OM_DEFAULT_BASE "http://api.open-meteo.com"
#else
    #define IW_OM_DEFAULT_BASE "https://api.open-meteo.com"
#endif

void OpenMeteoChannel::appendLevel(std::string& url, const WeatherRequest& request, WeatherLevel level,
                                   const char* section, const char* pastParam, const char* forecastParam) const
{
    const WeatherLevelRequest& lr = request.level(level);
    if (!lr.used || lr.varCount == 0) return;

    url += "&";
    url += section;
    url += "=";
    for (uint8_t i = 0; i < lr.varCount; i++)
    {
        if (i > 0) url += ",";
        url += lr.vars[i];
    }

    if (pastParam == nullptr) return;

    const int32_t past = (lr.from < 0) ? -lr.from : 0;
    const int32_t forecast = (lr.to >= 0) ? lr.to + 1 : 1;

    if (past > 0)
    {
        url += "&";
        url += pastParam;
        url += "=";
        url += std::to_string(past);
    }
    url += "&";
    url += forecastParam;
    url += "=";
    url += std::to_string(forecast);
}

std::string OpenMeteoChannel::buildUrl(const WeatherRequest& request) const
{
    std::string base = IW_OM_DEFAULT_BASE;
    if (ParamIW_OpenMeteo_UsageLicense >= 2)
    {
        const std::string configured = ParamIW_OpenMeteo_ServerURLStr;
        if (!configured.empty()) base = configured;
    }

    std::string url = base + "/v1/forecast?timezone=auto&timeformat=unixtime";

    url += "&latitude=" + std::to_string(request.latitude);
    url += "&longitude=" + std::to_string(request.longitude);

    // Open-Meteo erwartet den Schlüssel als "apikey"; frühere Versionen sendeten
    // faelschlich "appid", der Schluessel blieb dadurch wirkungslos.
    if (ParamIW_OpenMeteo_UsageLicense == 2)
    {
        const std::string key = ParamIW_OpenMeteo_APIKeyStr;
        if (!key.empty()) url += "&apikey=" + key;
    }

    appendLevel(url, request, WeatherLevel::Current, "current", nullptr, nullptr);
    appendLevel(url, request, WeatherLevel::Minutely15, "minutely_15", "past_minutely_15", "forecast_minutely_15");
    appendLevel(url, request, WeatherLevel::Hourly, "hourly", "past_hours", "forecast_hours");
    appendLevel(url, request, WeatherLevel::Daily, "daily", "past_days", "forecast_days");

    return url;
}

int16_t OpenMeteoChannel::fetch(const WeatherRequest& request)
{
    const std::string url = buildUrl(request);
    logDebugP("GET %s", url.c_str());

    HTTPClient http;
#ifdef ARDUINO_ARCH_RP2040
    if (url.rfind("https://", 0) == 0) http.setInsecure();
#endif
    http.begin(url.c_str());
    http.setTimeout(8000); // deutlich unter dem 16-s-Watchdog-Fenster

    openknx.watchdog.loop();
    const int httpStatus = http.GET();
    openknx.watchdog.loop();

    if (httpStatus != 200)
    {
        http.end();
        return (int16_t)httpStatus;
    }

    // Erst puffern, dann parsen: getStream() ist bei chunked HTTPS unzuverlässig.
    const String body = http.getString();
    http.end();
    openknx.watchdog.loop();

    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok)
    {
        logErrorP("JSON konnte nicht gelesen werden");
        return -2;
    }

    const int32_t utcOffset = doc["utc_offset_seconds"] | 0;

    // Aktuelle Werte
    const WeatherLevelRequest& currentReq = request.level(WeatherLevel::Current);
    if (currentReq.used && !doc["current"].isNull())
    {
        JsonObject current = doc["current"];
        const time_t at = (time_t)((int64_t)(current["time"] | 0) - utcOffset);
        for (uint8_t i = 0; i < currentReq.varCount; i++)
        {
            JsonVariant v = current[currentReq.vars[i]];
            if (!v.isNull()) applyCurrent(currentReq.vars[i], v.as<float>(), at);
        }
    }

    // Zeitreihen
    time_t* times = new time_t[IW_OM_MAX_SERIES];
    float* values = new float[IW_OM_MAX_SERIES];

    const WeatherLevel levels[] = {WeatherLevel::Minutely15, WeatherLevel::Hourly, WeatherLevel::Daily};
    const char* sections[] = {"minutely_15", "hourly", "daily"};

    for (uint8_t l = 0; l < 3; l++)
    {
        const WeatherLevelRequest& lr = request.level(levels[l]);
        if (!lr.used) continue;

        JsonObject section = doc[sections[l]];
        if (section.isNull()) continue;

        JsonArray timeArray = section["time"];
        if (timeArray.isNull()) continue;

        uint16_t count = 0;
        for (JsonVariant t : timeArray)
        {
            if (count >= IW_OM_MAX_SERIES) break;
            times[count++] = (time_t)((int64_t)t.as<int64_t>() - utcOffset);
        }
        if (count == 0) continue;

        for (uint8_t i = 0; i < lr.varCount; i++)
        {
            JsonArray valueArray = section[lr.vars[i]];
            if (valueArray.isNull()) continue;

            // Sonnenauf- und -untergang kommen als Zeitstempel; daraus werden die
            // Sekunden seit lokaler Mitternacht, denn das Ziel ist DPT 10.001.
            const bool isTimeOfDay = (strcmp(lr.vars[i], "sunrise") == 0 || strcmp(lr.vars[i], "sunset") == 0);

            uint16_t n = 0;
            for (JsonVariant v : valueArray)
            {
                if (n >= count) break;
                if (v.isNull())
                    values[n] = NAN;
                else if (isTimeOfDay)
                    values[n] = (float)((int64_t)v.as<int64_t>() - utcOffset - (int64_t)times[n]);
                else
                    values[n] = v.as<float>();
                n++;
            }
            if (n > 0) applySeries(levels[l], lr.vars[i], times, values, n);
        }
        openknx.watchdog.loop();
    }

    delete[] times;
    delete[] values;
    return 200;
}
#endif
