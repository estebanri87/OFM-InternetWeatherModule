#pragma once
#include "OpenKNX.h"
#include "ChannelOwnerModule.h"

// Mindestabstand zwischen zwei Abrufen. Bei 30 Kanälen ist ein vollständiger
// Durchlauf damit in gut anderthalb Minuten erledigt.
#define IW_FETCH_MIN_GAP_MS 3000

class InternetWeatherModule : public IWChannelOwnerModule
{
  public:
    InternetWeatherModule();
    const std::string name() override;
    const std::string version() override;
    void showInformations() override;
    void loop(bool configured) override;
    OpenKNX::Channel* createChannel(uint8_t _channelIndex /* this parameter is used in macros, do not rename */) override;
    void showHelp() override;
    bool processCommand(const std::string cmd, bool diagnoseKo) override;

  private:
    bool networkReady() const;

    // Es ruft immer nur ein Kanal gleichzeitig ab: fetchNow() blockiert, und der
    // ChannelOwner bedient über freeLoopIterate mehrere Kanäle je Durchlauf.
    uint32_t _lastFetchMs = 0;
    uint8_t _fetchCursor = 0;
};

extern InternetWeatherModule openknxInternetWeatherModule;
