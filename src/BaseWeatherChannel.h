#pragma once
#include "OpenKNX.h"

#include "ArduinoJson.h"
#include "HTTPClient.h"


// simple compile-time-checks for ko calculation
#if ((IW_KoCHTomorrowDescription - IW_KoCHTodayDescription) != (IW_KoCHTomorrowClouds - IW_KoCHTodayClouds))
    #error "KO offset for tomorrow is NOT constant!"
#endif
#if ((IW_KoCHForecastDescription - IW_KoCHTodayDescription) != (IW_KoCHForecastClouds - IW_KoCHTodayClouds))
    #error "KO offset for switchable forecast is NOT constant!"
#endif
#if ((IW_KoCHTomorrowDescription - IW_KoCHTomorrowClouds) != (IW_KoCHForecastDescription - IW_KoCHForecastClouds))
    #error "different structure of KO-groups"
#endif

// ko numbers relative to today forecast
#define IW_KoOffset_Tomorrow (IW_KoCHTomorrowDescription - IW_KoCHTodayDescription)
#define IW_KoOffset_Forecast (IW_KoCHForecastDescription - IW_KoCHTodayDescription)

// ET0 KO offsets relative to the day's description KO
#define IW_KoOffset_TodayET0    (IW_KoCHTodayET0 - IW_KoCHTodayDescription)
#define IW_KoOffset_TomorrowET0 (IW_KoCHTomorrowET0 - IW_KoCHTomorrowDescription)
#define IW_KoOffset_ForecastET0 (IW_KoCHForecastET0 - IW_KoCHForecastDescription)

// Day 3-7 KO offsets relative to today forecast
#define IW_KoOffset_Day3 (IW_KoCHDay3Description - IW_KoCHTodayDescription)
#define IW_KoOffset_Day4 (IW_KoCHDay4Description - IW_KoCHTodayDescription)
#define IW_KoOffset_Day5 (IW_KoCHDay5Description - IW_KoCHTodayDescription)
#define IW_KoOffset_Day6 (IW_KoCHDay6Description - IW_KoCHTodayDescription)
#define IW_KoOffset_Day7 (IW_KoCHDay7Description - IW_KoCHTodayDescription)

// ET0 KO offsets for Day3-7 (relative to their own description KO)
#define IW_KoOffset_Day3ET0 (IW_KoCHDay3ET0 - IW_KoCHDay3Description)
#define IW_KoOffset_Day4ET0 (IW_KoCHDay4ET0 - IW_KoCHDay4Description)
#define IW_KoOffset_Day5ET0 (IW_KoCHDay5ET0 - IW_KoCHDay5Description)
#define IW_KoOffset_Day6ET0 (IW_KoCHDay6ET0 - IW_KoCHDay6Description)
#define IW_KoOffset_Day7ET0 (IW_KoCHDay7ET0 - IW_KoCHDay7Description)

// Number of forecast days
#define IW_NUM_FORECAST_DAYS 7

// Day KO offsets array helper (relative to IW_KoCHTodayDescription)
static const int IW_KoOffset_Days[] = {
    0,                    // Today
    IW_KoOffset_Tomorrow, // Tomorrow
    IW_KoOffset_Day3,     // Day 3
    IW_KoOffset_Day4,     // Day 4
    IW_KoOffset_Day5,     // Day 5
    IW_KoOffset_Day6,     // Day 6
    IW_KoOffset_Day7,     // Day 7
};

// ET0 KO offsets array helper (relative to each day's description KO)
static const int IW_KoOffset_DayET0[] = {
    IW_KoOffset_TodayET0,    // Today
    IW_KoOffset_TomorrowET0, // Tomorrow
    IW_KoOffset_Day3ET0,     // Day 3
    IW_KoOffset_Day4ET0,     // Day 4
    IW_KoOffset_Day5ET0,     // Day 5
    IW_KoOffset_Day6ET0,     // Day 6
    IW_KoOffset_Day7ET0,     // Day 7
};


struct CurrentWheatherData
{
    float temperature_C = 0;
    float temperatureFeelsLike_C = 0;
    float humidity_percent = 0;
    uint16_t pressure_hPa = 0;
    float windSpeed_Km_h = 0;
    float windGust_Km_h = 0;
    uint16_t windDirection_deg = 0;
    float rain_mm = 0;
    float snow_mm = 0;
    float uvi_unitOne = 0;
    uint8_t cloudsCover_percent = 0;
};

struct ForecastHourWheatherData
{
    float temperature_C = 0;
    float temperatureFeelsLike_C = 0;
    float humidity_percent = 0;
    uint16_t pressure_hPa = 0;
    float windSpeed_Km_h = 0;
    float windGust_Km_h = 0;
    uint16_t windDirection_deg = 0;
    uint8_t probabilityOfPrecipitation_percent = 0;
    float rain_mm = 0;
    float snow_mm = 0;
    float uvi_unitOne = 0;
    uint8_t cloudsCover_percent = 0;
};


struct ForecastDayWheatherData
{
    float temperatureMin_C = 0;
    float temperatureMax_C = 0;
    float temperatureMorning_C = 0;
    float temperatureDay_C = 0;
    float temperatureEvening_C = 0;
    float temperatureNight_C = 0;

    float temperatureFeelsLikeMorning_C = 0;
    float temperatureFeelsLikeDay_C = 0;
    float temperatureFeelsLikeEvening_C = 0;
    float temperatureFeelsLikeNight_C = 0;

    float humidity_percent = 0;
    uint16_t pressure_hPa = 0;
    float windSpeed_Km_h = 0;
    float windGust_Km_h = 0;
    uint16_t windDirection_deg = 0;
    uint8_t probabilityOfPrecipitation_percent = 0;
    float rain_mm = 0;
    float snow_mm = 0;
    float uvi_unitOne = 0;
    uint8_t cloudsCover_percent = 0;
    float et0_mm = NAN; // ET₀ Reference Evapotranspiration (only Open-Meteo)
};

struct ForecastDayWheatherDataWithDescription : ForecastDayWheatherData
{
    char description[15] = {0};
};


class BaseWeatherChannel : public OpenKNX::Channel
{
  private:
    unsigned long _lastApiCall = 0;
    unsigned long _updateIntervalInMs = 0;
    bool _available = false;
    ForecastDayWheatherDataWithDescription _dayForecasts[IW_NUM_FORECAST_DAYS];
    void buildDescription(char* description, float rain, float snow, uint8_t clouds, const char* prefix);
    void updateDayForecastKo(ForecastDayWheatherDataWithDescription& day, int koOffset, int et0KoOffset);
    void fetchData();

  protected:
    BaseWeatherChannel(uint8_t index);
    virtual int16_t fillWeather(CurrentWheatherData& currentWeather, ForecastDayWheatherDataWithDescription* dayForecasts, int numDays, ForecastHourWheatherData& hour1Weather, ForecastHourWheatherData& hour2Weather) = 0;
    void setValueCompare(GroupObject& groupObject, const KNXValue& value, const Dpt& type);
    void setValueCompare(uint goNumber, const KNXValue& value, const Dpt& type);

  public:
    void loop() override;
    void setup() override;
    void processInputKo(GroupObject& ko) override;
    virtual bool processCommand(const std::string cmd, bool diagnoseKo);
};