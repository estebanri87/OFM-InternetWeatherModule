#pragma once
#include "WeatherSlot.h"

#define IW_LEVEL_COUNT 4
#define IW_MAX_VARS_PER_LEVEL (IW_SLOT_COUNT + 3) // drei Slots plus Hilfsgrößen des Wetter-Textes

// Was ein Kanal von seinem Anbieter braucht: je Zeitebene die dedupliziert
// benötigten Variablen und der abzudeckende Bereich in Rasterschritten,
// relativ zum aktuellen Schritt.
struct WeatherLevelRequest
{
    bool used = false;
    int16_t from = 0;
    int16_t to = 0;
    const char* vars[IW_MAX_VARS_PER_LEVEL] = {};
    uint8_t varCount = 0;
};

struct WeatherRequest
{
    float latitude = 0.0f;
    float longitude = 0.0f;
    WeatherLevelRequest levels[IW_LEVEL_COUNT];

    const WeatherLevelRequest& level(WeatherLevel l) const { return levels[(uint8_t)l]; }
    bool any() const;
};

class BaseWeatherChannel : public OpenKNX::Channel
{
  public:
    void setup() override;
    void loop() override;
    void processInputKo(GroupObject& ko) override;
    virtual bool processCommand(const std::string cmd, bool diagnoseKo);
    const std::string name() override { return "WeatherChannel"; }

    // Vom Modul gesteuert: nur ein Kanal ruft gleichzeitig ab.
    bool fetchDue(uint32_t nowMs) const;
    void requestFetch() { _fetchPending = true; }
    void fetchNow();

    uint16_t bufferBytes() const;
    void logSlots();

  protected:
    BaseWeatherChannel(uint8_t index);

    // Vom Anbieter zu implementieren. Rückgabe: HTTP-Status, oder negativ bei
    // Verarbeitungsfehlern. Der Anbieter reicht jede geparste Reihe über
    // applySeries() bzw. applyCurrent() zurück.
    virtual int16_t fetch(const WeatherRequest& request) = 0;
    virtual WeatherProvider provider() const = 0;

    // Vom Anbieter aufzurufen, sobald eine Reihe geparst ist.
    void applySeries(WeatherLevel level, const char* var, const time_t* times, const float* values, uint16_t count);
    void applyCurrent(const char* var, float value, time_t at);

    // Hilfsgrößen des OpenKNX Wetter-Textes für diesen Anbieter (Regen, Schnee, Bewölkung).
    const char* textHelperVar(uint8_t which) const;

  private:
    void buildRequest(WeatherRequest& request, time_t now) const;
    void publishSlot(uint8_t slotIndex, time_t now);
    void buildWeatherText(char* target, uint8_t slotIndex, time_t now) const;
    void addVar(WeatherLevelRequest& lr, const char* var) const;

    WeatherSlot _slots[IW_SLOT_COUNT];
    float _lastSent[IW_SLOT_COUNT] = {NAN, NAN, NAN};
    char _lastText[IW_SLOT_COUNT][15] = {};

    // Der OpenKNX Wetter-Text wird aus Regen, Schnee und Bewölkung gebaut. Er ist
    // der einzige abgeleitete Messwert, deshalb genügen drei Einzelwerte für den
    // konfigurierten Offset statt einer eigenen Zeitreihe. Der Text wird beim
    // Abruf gebildet; bei Tageswerten reicht das, weil sich die Tagesgrenze nur
    // einmal täglich verschiebt und das Abrufintervall höchstens eine Stunde ist.
    int8_t _textSlotIndex = -1;
    float _textRain = NAN;
    float _textSnow = NAN;
    float _textClouds = NAN;

    uint32_t _updateIntervalMs = 0;
    uint32_t _lastFetchMs = 0;
    uint32_t _nextFetchMs = 0;
    uint32_t _lastEvaluateMs = 0;
    int16_t _lastDay = -1; // Tag im Jahr der letzten Auswertung, für den Abruf nach Mitternacht
    bool _fetchPending = false;
    int16_t _lastHttpStatus = 0;
};
