#pragma once
#include "BaseWeatherChannel.h"

class OpenMeteoChannel : public BaseWeatherChannel
{
  public:
    OpenMeteoChannel(uint8_t index) : BaseWeatherChannel(index) {}

  protected:
    int16_t fetch(const WeatherRequest& request) override;
    WeatherProvider provider() const override { return WeatherProvider::OpenMeteo; }

  private:
    std::string buildUrl(const WeatherRequest& request) const;
    void appendLevel(std::string& url, const WeatherRequest& request, WeatherLevel level,
                     const char* section, const char* pastParam, const char* forecastParam) const;
};
