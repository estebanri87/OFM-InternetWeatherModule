#pragma once
#include "WeatherCatalog.h"

// Obergrenze der Zeitreihe je Slot. Ein ganzer Tag in Viertelstunden (96) plus
// Marge muss hineinpassen, weil "heute um" und "Tagesende" den ganzen Tag brauchen.
#define IW_SLOT_MAX_SAMPLES 104

// Modulweites Budget für alle Zeitreihen. Wird es überschritten, bleibt der
// betroffene Slot aus und meldet das, statt den Heap still zu erschöpfen.
#define IW_SLOT_BUDGET_BYTES 16384

// Ein frei belegbarer Wert-Slot eines Kanals.
//
// Die Offsets des Anwenders gelten relativ zu jetzt oder zur Uhrzeit des
// heutigen Tages; das Fenster wandert also zwischen zwei API-Abrufen weiter.
// Deshalb hält der Slot die Zeitreihe absolut verankert (_zeroEpoch) und wertet
// im Minutentakt neu aus, statt den Wert nur beim Abruf zu bestimmen.
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

    // true, wenn das Fenster am Kalendertag hängt ("heute um", "Tagesende").
    // Nach Mitternacht muss dann neu abgerufen werden.
    bool usesDayReference() const;

    // Fenster zum Zeitpunkt now, in Rasterschritten relativ zum aktuellen Schritt.
    // false, wenn das Ende vor dem Anfang liegt.
    bool window(time_t now, int32_t& first, int32_t& last) const;

    // Bereich, den der Abruf zum Zeitpunkt now abdecken muss, inklusive Marge für
    // das Weiterwandern bis zum nächsten Abruf.
    void need(time_t now, int16_t& from, int16_t& to) const;

    // Vom Provider: die geparste Reihe übergeben. times[] sind die UTC-Epochen
    // der Rasterschritte, values[] die Rohwerte in Anbieter-Einheit. Der Slot
    // kopiert sich heraus, was er braucht.
    void fill(const time_t* times, const float* values, uint16_t count, time_t now);
    void fillSingle(float rawValue, time_t slotStart);

    void invalidate() { _valid = false; }
    bool valid() const { return _valid; }

    // Ergebnis für den aktuellen Zeitpunkt. false, wenn keine Daten vorliegen.
    bool evaluate(time_t now, float& out) const;

    // Beschreibt die Belegung für die Diagnose. Geloggt wird im Kanal, weil die
    // OpenKNX-Log-Makros ein logPrefix() der Basisklasse brauchen.
    void describe(char* buffer, size_t length, uint8_t slotIndex) const;
    bool unsupported() const { return _unsupported; }
    bool overBudget() const { return _overBudget; }
    uint16_t bufferBytes() const { return (uint16_t)(_capacity * sizeof(float)); }

    // Belegte Bytes aller Slots des Moduls.
    static uint32_t totalBufferBytes();

  private:
    const WeatherMeasurandInfo* _info = nullptr;
    const char* _var = nullptr;
    float _scale = 1.0f;
    int16_t _from = 0; // Offset in Rasterschritten oder Stunde 0..23, je nach _fromRef
    int16_t _to = 0;   // Offset in Rasterschritten oder Stunde 0..23, je nach _toRef
    PT_FromRef _fromRef = PT_FromRef::Relative;
    PT_ToRef _toRef = PT_ToRef::Relative;
    PT_Aggregation _agg = PT_Aggregation::Mean;
    PT_SlotValueType _type = PT_SlotValueType::Single;
    PT_SendBehaviour _send = PT_SendBehaviour::OnChange;

    float* _samples = nullptr;
    uint8_t _capacity = 0;   // belegte Puffergröße inkl. Marge
    time_t _zeroEpoch = 0;   // Rasterschritt-Start von _samples[0]
    bool _valid = false;
    bool _unsupported = false; // Messwert gewählt, aber vom Anbieter nicht geliefert
    bool _overBudget = false;  // Puffer hätte das modulweite Budget überschritten
    uint8_t _measurandId = 0;
};

// Rasterschritt-Arithmetik. Tageswerte laufen kalendarisch, damit die
// Zeitumstellung nicht zu einem verschobenen Tag führt.
time_t weatherSlotStart(time_t t, WeatherLevel level);
time_t weatherStepAdvance(time_t t, WeatherLevel level, int16_t steps);
int32_t weatherStepsBetween(time_t from, time_t to, WeatherLevel level);
