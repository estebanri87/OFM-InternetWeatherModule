#pragma once
#include "BaseWeatherChannel.h"


class OpenWeatherMapChannel : public BaseWeatherChannel
{
  private:
    void fillForecast(JsonObject& json, ForecastDayWheatherData& wheater);
    void fillForecast(JsonObject& json, CurrentWheatherData& wheater);
    void fillForecast(JsonObject& json, ForecastHourWheatherData& wheater);
  protected:
    int16_t fillWeather(CurrentWheatherData& currentWeather, ForecastDayWheatherDataWithDescription* dayForecasts, int numDays, ForecastHourWheatherData& hour1Weather, ForecastHourWheatherData& hour2Weather) override;
  
  public:
    OpenWeatherMapChannel(uint8_t index);
    const std::string name() override;
 };
