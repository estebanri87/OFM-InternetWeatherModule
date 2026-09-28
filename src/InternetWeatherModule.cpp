#include "InternetWeatherModule.h"
#include "OpenMeteoChannel.h"
#include "OpenWeatherMapChannel.h"

#ifdef WLAN_WifiSSID
    #include "WiFi.h"
#else
    #include "NetworkModule.h"
#endif

InternetWeatherModule::InternetWeatherModule()
    : IWChannelOwnerModule(IW_ChannelCount)
{
}

const std::string InternetWeatherModule::name()
{
    return "InternetWeather";
}

void InternetWeatherModule::showInformations()
{
}

const std::string InternetWeatherModule::version()
{
#ifdef MODULE_InternetWeatherModule_Version
    return MODULE_InternetWeatherModule_Version;
#else
    // hides the module in the version output on the console, because the firmware version is sufficient.
    return "";
#endif
}

bool InternetWeatherModule::networkReady() const
{
#ifdef WLAN_WifiSSID
    return WiFi.isConnected();
#else
    return openknxNetwork.established();
#endif
}

void InternetWeatherModule::loop(bool configured)
{
    IWChannelOwnerModule::loop(configured);
    if (!configured || !networkReady()) return;

    const uint32_t nowMs = millis();
    if (_lastFetchMs != 0 && nowMs - _lastFetchMs < IW_FETCH_MIN_GAP_MS) return;

    const uint8_t channels = getNumberOfChannels();
    if (channels == 0) return;

    // Ringförmig den nächsten fälligen Kanal suchen, damit bei knappem Zeitbudget
    // nicht immer derselbe Kanal zum Zug kommt.
    for (uint8_t n = 0; n < channels; n++)
    {
        const uint8_t index = (uint8_t)((_fetchCursor + n) % channels);
        BaseWeatherChannel* channel = (BaseWeatherChannel*)getChannel(index);
        if (channel == nullptr || !channel->fetchDue(nowMs)) continue;

        channel->fetchNow();
        _lastFetchMs = millis();
        _fetchCursor = (uint8_t)((index + 1) % channels);
        return;
    }
}

OpenKNX::Channel* InternetWeatherModule::createChannel(uint8_t _channelIndex /* this parameter is used in macros, do not rename */)
{
    // Suspendiert verhält sich wie "Wetter-Service = Deaktiviert": der Kanal wird
    // nicht angelegt und ruft nichts ab. Seine Kommunikationsobjekte bleiben in
    // der ETS erhalten, weil deren Sichtbarkeit am Dienst hängt.
    if (ParamIW_CHSuspended)
        return nullptr;

    switch (ParamIW_CHWeatherChannelType)
    {
        case PT_ChannelType::OpenWeatherMap:
            return new OpenWeatherMapChannel(_channelIndex);

        case PT_ChannelType::OpenMeteo:
            // Ohne gewählte Nutzung/Lizenz darf Open-Meteo nicht abgefragt werden.
            if (ParamIW_OpenMeteo_UsageLicense == 0)
            {
                logInfoP("Kanal %u: Open-Meteo gewählt, aber keine Nutzung/Lizenz - Kanal bleibt aus", _channelIndex + 1);
                return nullptr;
            }
            return new OpenMeteoChannel(_channelIndex);

        default:
            return nullptr;
    }
}

void InternetWeatherModule::showHelp()
{
    openknx.console.printHelpLine("iw<CC> update", "Wetterdaten des Kanals CC sofort abrufen, z.B. iw01 update");
    openknx.console.printHelpLine("iw<CC> slots", "Belegung der drei Wert-Slots des Kanals CC anzeigen");
}

bool InternetWeatherModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.rfind("iw", 0) != 0) return false;

    auto channelString = cmd.substr(2);
    if (channelString.length() == 0) return false;

    auto pos = channelString.find_first_of(' ');
    std::string channelNumberString;
    std::string channelCmd;
    if (pos > 0 && pos != std::string::npos)
    {
        channelNumberString = channelString.substr(0, pos);
        channelCmd = channelString.substr(pos + 1);
    }
    else
    {
        channelNumberString = channelString;
        channelCmd = "";
    }

    auto channel = atoi(channelNumberString.c_str());
    if (channel < 1 || channel > getNumberOfChannels())
    {
        logInfoP("Kanal %d gibt es nicht", channel);
        return true;
    }

    auto weatherChannel = (BaseWeatherChannel*)getChannel(channel - 1);
    if (weatherChannel == nullptr)
    {
        logInfoP("Kanal %d ist nicht aktiv", channel);
        return true;
    }

    if (channelCmd.length() != 0)
        return weatherChannel->processCommand(channelCmd, diagnoseKo);

    return false;
}

InternetWeatherModule openknxInternetWeatherModule;
