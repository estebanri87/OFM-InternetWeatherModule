#include "OpenWeatherMapChannel.h"
#ifdef ARDUINO_ARCH_RP2040 
#define OpenWeatherMapUrl "http://api.openweathermap.org/data/3.0/onecall?units=metric&lang=de&exclude=minutely,alerts"
#else
#define OpenWeatherMapUrl "https://api.openweathermap.org/data/3.0/onecall?units=metric&lang=de&exclude=minutely,alerts"
#endif

OpenWeatherMapChannel::OpenWeatherMapChannel(uint8_t index)
    : BaseWeatherChannel(index)
{
}

const std::string OpenWeatherMapChannel::name()
{
    return "OpenWeatherMap";
}

int16_t OpenWeatherMapChannel::fillWeather(CurrentWheatherData& currentWeather, ForecastDayWheatherDataWithDescription* dayForecasts, int numDays, ForecastHourWheatherData& hour1Weather, ForecastHourWheatherData& hour2Weather)
{
    String url = OpenWeatherMapUrl;
    url += "&appid=";
    url += (const char*)ParamIW_OpenWeatherMap_APIKey;
    url += "&lat=";
    url += ParamIW_CHWeatherLocationType == 0 ? ParamBASE_Latitude : ParamIW_CHLatitude;
    url += "&lon=";
    url += ParamIW_CHWeatherLocationType == 0 ? ParamBASE_Longitude : ParamIW_CHLongitude;
    logDebugP("Call: %s", url.c_str());
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
    JsonDocument doc;
    DeserializationError jsonErr = deserializeJson(doc, http.getString());
    http.end();
    if (jsonErr) {
        logErrorP("JSON parse error: %s", jsonErr.c_str());
        return -2;
    }

    JsonObject current = doc["current"];
    fillForecast(current, currentWeather);
  
    JsonArray daily = doc["daily"];
    int availableDays = daily.size();
    int daysToFill = (numDays < availableDays) ? numDays : availableDays;
    for (int i = 0; i < daysToFill; i++)
    {
        JsonObject dayObj = daily[i];
        fillForecast(dayObj, dayForecasts[i]);
    }

    JsonArray hourly = doc["hourly"];
    JsonObject hour1 = hourly[1];
    fillForecast(hour1, hour1Weather);
    JsonObject hour2 = hourly[2];
    fillForecast(hour2, hour2Weather);

    return httpStatus;
}

void OpenWeatherMapChannel::fillForecast(JsonObject& json, CurrentWheatherData& wheater)
{
    wheater.temperature_C = json["temp"];                  // 22.34
    wheater.temperatureFeelsLike_C = json["feels_like"];   // 21.95
    wheater.humidity_percent = json["humidity"];                 // 69
    wheater.pressure_hPa = json["pressure"];                 // 1006
    wheater.windSpeed_Km_h = 3.6 * (float)json["wind_speed"]; // 69
    wheater.windGust_Km_h = 3.6 * (float)json["wind_gust"];   // 69
    wheater.windDirection_deg = json["wind_deg"];            // 70
    JsonObject rainObject = json["rain"];
    wheater.rain_mm = rainObject ? (float) rainObject["1h"] : (float)0; // 2.5
    JsonObject snowObject = json["snow"];
    wheater.snow_mm = snowObject ? (float) snowObject["1h"] : (float)0; // 2.5
    wheater.uvi_unitOne = json["uvi"];                                 // 6.29
    wheater.cloudsCover_percent = json["clouds"];                           // 40
}

void OpenWeatherMapChannel::fillForecast(JsonObject& json, ForecastHourWheatherData& wheater)
{
    fillForecast(json, (CurrentWheatherData&) wheater);
    wheater.probabilityOfPrecipitation_percent = round(100. * (float) json["pop"]);    // 0.70
}

void OpenWeatherMapChannel::fillForecast(JsonObject& json, ForecastDayWheatherData& wheater)
{
    JsonObject tempObject = json["temp"];
    wheater.temperatureDay_C = tempObject["day"];     // 21.95
    wheater.temperatureNight_C = tempObject["night"]; // 21.95
    wheater.temperatureEvening_C = tempObject["eve"]; // 21.95
    wheater.temperatureMorning_C = tempObject["morn"]; // 21.95
    wheater.temperatureMin_C = tempObject["min"];      // 21.95
    wheater.temperatureMax_C = tempObject["max"];      // 21.95
    tempObject = json["feels_like"];
    wheater.temperatureFeelsLikeDay_C = tempObject["day"];     // 21.95
    wheater.temperatureFeelsLikeNight_C = tempObject["night"]; // 21.95
    wheater.temperatureFeelsLikeEvening_C = tempObject["eve"]; // 21.95
    wheater.temperatureFeelsLikeMorning_C = tempObject["morn"]; // 21.95
    wheater.humidity_percent = json["humidity"];                 // 69
    wheater.pressure_hPa = json["pressure"];                 // 1006
    wheater.windSpeed_Km_h = 3.6 * (float)json["wind_speed"]; // 69
    wheater.windGust_Km_h = 3.6 * (float)json["wind_gust"];   // 69
    wheater.windDirection_deg = json["wind_deg"];            // 70
    wheater.rain_mm = json["rain"];                         // 2.5
    wheater.snow_mm = json["snow"];                         // 2.5
    wheater.probabilityOfPrecipitation_percent = round(100. * (float) json["pop"]);    // 0.70
    wheater.uvi_unitOne = json["uvi"];                           // 6.29
    wheater.cloudsCover_percent = json["clouds"];                     // 40
}
