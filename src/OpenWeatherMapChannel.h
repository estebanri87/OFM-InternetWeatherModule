#pragma once
#include "BaseWeatherChannel.h"

class OpenWeatherMapChannel : public BaseWeatherChannel
{
  public:
    OpenWeatherMapChannel(uint8_t index) : BaseWeatherChannel(index) {}

  protected:
    int16_t fetch(const WeatherRequest& request) override;
    WeatherProvider provider() const override { return WeatherProvider::OpenWeatherMap; }
};
