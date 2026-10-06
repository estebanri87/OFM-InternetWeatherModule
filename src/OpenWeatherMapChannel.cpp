#ifndef OPENKNX_INTERNETWEATHER_IGNORE
#include "OpenWeatherMapChannel.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"

// One Call 3.0 liefert 48 Stunden und 8 Tage in einer Antwort.
#define IW_OWM_MAX_SERIES 48

#ifdef ARDUINO_ARCH_RP2040
    #define IW_OWM_BASE "http://api.openweathermap.org/data/3.0/onecall"
#else
    #define IW_OWM_BASE "https://api.openweathermap.org/data/3.0/onecall"
#endif

namespace
{
    // Loest einen Katalog-Pfad wie "temp.morn" oder "weather.0.id" auf.
    JsonVariant resolvePath(JsonVariant node, const char* path)
    {
        const char* cursor = path;
        char part[24];

        while (*cursor != '\0' && !node.isNull())
        {
            uint8_t len = 0;
            while (*cursor != '\0' && *cursor != '.' && len < sizeof(part) - 1)
                part[len++] = *cursor++;
            part[len] = '\0';
            if (*cursor == '.') cursor++;

            bool numeric = (len > 0);
            for (uint8_t i = 0; i < len; i++)
                if (part[i] < '0' || part[i] > '9') { numeric = false; break; }

            // Getrennt zuweisen: ArduinoJson liefert für Index- und Namenszugriff
            // verschiedene Proxy-Typen, die sich nicht in einem ?: vereinen lassen.
            if (numeric)
                node = node[(size_t)atoi(part)];
            else
                node = node[part];
        }
        return node;
    }
} // namespace

int16_t OpenWeatherMapChannel::fetch(const WeatherRequest& request)
{
    const std::string apiKey = ParamIW_OpenWeatherMap_APIKeyStr;
    if (apiKey.empty())
    {
        logErrorP("Kein API Key hinterlegt, One Call 3.0 erfordert ein Abonnement");
        return -3;
    }

    std::string url = IW_OWM_BASE;
    url += "?units=metric&lang=de&exclude=minutely,alerts";
    url += "&lat=" + std::to_string(request.latitude);
    url += "&lon=" + std::to_string(request.longitude);
    url += "&appid=" + apiKey;

    logDebugP("GET %s (Key ausgeblendet)", IW_OWM_BASE);

    HTTPClient http;
#ifdef ARDUINO_ARCH_RP2040
    if (url.rfind("https://", 0) == 0) http.setInsecure();
#endif
    http.begin(url.c_str());
    http.setTimeout(8000);

    openknx.watchdog.loop();
    const int httpStatus = http.GET();
    openknx.watchdog.loop();

    if (httpStatus != 200)
    {
        http.end();
        return (int16_t)httpStatus;
    }

    const String body = http.getString();
    http.end();
    openknx.watchdog.loop();

    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok)
    {
        logErrorP("JSON konnte nicht gelesen werden");
        return -2;
    }

    const int32_t tzOffset = doc["timezone_offset"] | 0;

    // Aktuelle Werte
    const WeatherLevelRequest& currentReq = request.level(WeatherLevel::Current);
    if (currentReq.used && !doc["current"].isNull())
    {
        JsonVariant current = doc["current"];
        const time_t at = (time_t)(current["dt"] | 0);
        for (uint8_t i = 0; i < currentReq.varCount; i++)
        {
            JsonVariant v = resolvePath(current, currentReq.vars[i]);
            // Regen und Schnee fehlen bei trockenem Wetter ganz - das ist 0, kein Fehler.
            const float value = v.isNull() ? 0.0f : v.as<float>();
            applyCurrent(currentReq.vars[i], value, at);
        }
    }

    time_t* times = new time_t[IW_OWM_MAX_SERIES];
    float* values = new float[IW_OWM_MAX_SERIES];

    const WeatherLevel levels[] = {WeatherLevel::Hourly, WeatherLevel::Daily};
    const char* sections[] = {"hourly", "daily"};

    for (uint8_t l = 0; l < 2; l++)
    {
        const WeatherLevelRequest& lr = request.level(levels[l]);
        if (!lr.used) continue;

        JsonArray entries = doc[sections[l]];
        if (entries.isNull()) continue;

        const bool daily = (levels[l] == WeatherLevel::Daily);

        uint16_t count = 0;
        for (JsonVariant entry : entries)
        {
            if (count >= IW_OWM_MAX_SERIES) break;
            const int64_t dt = entry["dt"] | 0;
            if (daily)
            {
                // "dt" liegt mitten am Tag; der Slot braucht den Beginn des
                // lokalen Kalendertages als UTC-Epoche.
                const int64_t shifted = dt + tzOffset;
                times[count] = (time_t)((shifted / 86400) * 86400 - tzOffset);
            }
            else
                times[count] = (time_t)dt;
            count++;
        }
        if (count == 0) continue;

        for (uint8_t i = 0; i < lr.varCount; i++)
        {
            const bool isTimeOfDay = (strcmp(lr.vars[i], "sunrise") == 0 || strcmp(lr.vars[i], "sunset") == 0);
            const bool optional = (strncmp(lr.vars[i], "rain", 4) == 0 || strncmp(lr.vars[i], "snow", 4) == 0);

            uint16_t n = 0;
            for (JsonVariant entry : entries)
            {
                if (n >= count) break;
                JsonVariant v = resolvePath(entry, lr.vars[i]);
                if (v.isNull())
                    values[n] = optional ? 0.0f : NAN;
                else if (isTimeOfDay)
                {
                    const int64_t shifted = (int64_t)v.as<int64_t>() + tzOffset;
                    const int64_t midnight = (shifted / 86400) * 86400;
                    values[n] = (float)(shifted - midnight);
                }
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
