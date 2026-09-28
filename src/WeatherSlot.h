#pragma once
#include "WeatherCatalog.h"

// Obergrenze der Zeitreihe je Slot. Deckt 24 h Stundenwerte oder 16 h
// Viertelstundenwerte ab; darueber hinaus wird das Fenster begrenzt.
#define IW_SLOT_MAX_SAMPLES 64

// Ein frei belegbarer Wert-Slot eines Kanals.
//
// Die Offsets des Anwenders gelten relativ zu jetzt, der aktuelle Rasterschritt
// wandert also zwischen zwei API-Abrufen weiter. Deshalb haelt der Slot die
// Zeitreihe absolut verankert (_zeroEpoch) und wertet im Minutentakt neu aus,
// statt den Wert nur beim Abruf zu bestimmen.
class WeatherSlot
{
  public:
    void setup(uint8_t channelIndex, uint8_t slotIndex, WeatherProvider provider, uint32_t refreshSeconds);
    void release();

    bool configured() const { return _info != nullptr; }
    const WeatherMeasurandInfo* info() const { return _info; }
    WeatherLevel level() const { return _info->level; }
    const char* variable() const { return _var; }
    PT_SendBehaviour sendBehaviour() const { return _send; }

    // Spanne, die der Abruf abdecken muss, in Rasterschritten relativ zum
    // aktuellen Schritt. Die Marge faengt das Weiterwandern zwischen zwei
    // Abrufen ab.
    int16_t needFrom() const { return _offsetFrom; }
    int16_t needTo() const { return (int16_t)(_offsetFrom + _capacity - 1); }

    // Vom Provider: die geparste Reihe uebergeben. times[] sind die UTC-Epochen
    // der Rasterschritte, values[] die Rohwerte in Anbieter-Einheit. Der Slot
    // kopiert sich heraus, was er braucht.
    void fill(const time_t* times, const float* values, uint16_t count, time_t now);
    void fillSingle(float rawValue, time_t slotStart);

    void invalidate() { _valid = false; }
    bool valid() const { return _valid; }

    // Ergebnis fuer den aktuellen Zeitpunkt. false, wenn keine Daten vorliegen.
    bool evaluate(time_t now, float& out) const;

    // Beschreibt die Belegung für die Diagnose. Geloggt wird im Kanal, weil die
    // OpenKNX-Log-Makros ein logPrefix() der Basisklasse brauchen.
    void describe(char* buffer, size_t length, uint8_t slotIndex) const;
    bool unsupported() const { return _unsupported; }
    uint8_t measurandId() const { return _measurandId; }
    uint16_t bufferBytes() const { return (uint16_t)(_capacity * sizeof(float)); }

  private:
    const WeatherMeasurandInfo* _info = nullptr;
    const char* _var = nullptr;
    float _scale = 1.0f;
    int16_t _offsetFrom = 0;
    int16_t _offsetTo = 0;
    PT_Aggregation _agg = PT_Aggregation::Mean;
    PT_SlotValueType _type = PT_SlotValueType::Single;
    PT_SendBehaviour _send = PT_SendBehaviour::OnChange;

    float* _samples = nullptr;
    uint8_t _capacity = 0;   // belegte Puffergroesse inkl. Marge
    uint8_t _span = 1;       // Anzahl Werte des Aggregationsfensters
    time_t _zeroEpoch = 0;   // Rasterschritt-Start von _samples[0]
    bool _valid = false;
    bool _unsupported = false; // Messwert gewählt, aber vom Anbieter nicht geliefert
    uint8_t _measurandId = 0;
};

// Rasterschritt-Arithmetik. Tageswerte laufen kalendarisch, damit die
// Zeitumstellung nicht zu einem verschobenen Tag fuehrt.
time_t weatherSlotStart(time_t t, WeatherLevel level);
time_t weatherStepAdvance(time_t t, WeatherLevel level, int16_t steps);
int32_t weatherStepsBetween(time_t from, time_t to, WeatherLevel level);
