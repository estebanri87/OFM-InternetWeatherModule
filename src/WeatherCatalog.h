#pragma once
#include "OpenKNX.h"
#include "WeatherCatalog.gen.h"

// Wetterdienst, Werte wie im ETS-Parameter "Wetter-Service" (PT_ChannelType).
enum class WeatherProvider : uint8_t
{
    OpenWeatherMap = 1,
    OpenMeteo = 2
};

// Ein Slot belegt 6 Byte im Kanalblock, Slot A beginnt bei Byte 9.
#define IW_SLOT_COUNT 3
#define IW_SLOT_BASE IW_CHSlotACategoryOpenmeteo
#define IW_SLOT_STRIDE (IW_CHSlotBCategoryOpenmeteo - IW_CHSlotACategoryOpenmeteo)

// Byte-Versatz der Slot-Parameter relativ zum Slot-Anfang
#define IW_SLOT_OFF_FLAGS 0
#define IW_SLOT_OFF_MEASURAND 1
#define IW_SLOT_OFF_FROM 2
#define IW_SLOT_OFF_TO 4

// Liefert den Katalogeintrag zur Messwert-Id, oder nullptr bei unbekannter Id.
// Faengt damit zugleich einen Restwert ab, der nach einem Kategorie- oder
// Anbieterwechsel im gemeinsamen Speicherbyte stehengeblieben ist.
const WeatherMeasurandInfo* weatherMeasurand(uint8_t id);

// Anbieter-Variable eines Messwerts, oder nullptr wenn dieser Anbieter die
// Groesse nicht liefert.
const WeatherProviderVar* weatherProviderVar(const WeatherMeasurandInfo& info, WeatherProvider provider);

// Rasterweite einer Zeitebene in Sekunden. Tageswerte liefern 0, weil ein
// Kalendertag wegen der Zeitumstellung keine feste Laenge hat.
uint32_t weatherLevelSeconds(WeatherLevel level);

const char* weatherLevelName(WeatherLevel level);
const char* weatherMeasurandName(const WeatherMeasurandInfo& info);
