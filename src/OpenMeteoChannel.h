#pragma once
#include "BaseWeatherChannel.h"

class OpenMeteoChannel : public BaseWeatherChannel
{
  private:
    float avg(JsonArray& arr, int begin, int n);
    void fillForecast(JsonObject& json, JsonObject& jsonHourly, int vi, ForecastDayWheatherData& wheater);
    void fillForecast(JsonObject& json, CurrentWheatherData& wheater);
    void fillForecast(JsonObject& json, int vi, ForecastHourWheatherData& wheater);

  protected:
    int16_t fillWeather(CurrentWheatherData& currentWeather, ForecastDayWheatherDataWithDescription* dayForecasts, int numDays, ForecastHourWheatherData& hour1Weather, ForecastHourWheatherData& hour2Weather) override;

  public:
    OpenMeteoChannel(uint8_t index);
    const std::string name() override;
};
