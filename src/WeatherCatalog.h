#pragma once
#include "OpenKNX.h"
#include "WeatherCatalog.gen.h"

// Wetterdienst, Werte wie im ETS-Parameter "Wetter-Service" (PT_ChannelType).
enum class WeatherProvider : uint8_t
{
    OpenWeatherMap = 1,
    OpenMeteo = 2
};

// Drei Slots je Kanal. Die Slot-Parameter werden über die vom Producer für Slot A
// erzeugten Makros adressiert (Byte, Maske, Shift); Slot B und C liegen um
// IW_SLOT_STRIDE dahinter. So muss keine Bitposition von Hand gepflegt werden.
#define IW_SLOT_COUNT 3
#define IW_SLOT_STRIDE (IW_CHSlotBMeasurand - IW_CHSlotAMeasurand)

// Liest ein Bitfeld eines Slot-Parameters, z.B. IW_SLOT_FIELD(ch, slot, ValueType).
#define IW_SLOT_INDEX(channel, slot, field) \
    (IW_ParamBlockOffset + (channel) * IW_ParamBlockSize + IW_CHSlotA##field + (slot) * IW_SLOT_STRIDE)
#define IW_SLOT_FIELD(channel, slot, field) \
    ((knx.paramByte(IW_SLOT_INDEX(channel, slot, field)) & IW_CHSlotA##field##Mask) >> IW_CHSlotA##field##Shift)

// Liefert den Katalogeintrag zur Messwert-Id, oder nullptr bei unbekannter Id.
// Fängt damit zugleich einen Restwert ab, der nach einem Kategorie- oder
// Anbieterwechsel im gemeinsamen Speicherbyte stehengeblieben ist.
const WeatherMeasurandInfo* weatherMeasurand(uint8_t id);

// Anbieter-Variable eines Messwerts, oder nullptr wenn dieser Anbieter die
// Größe nicht liefert.
const WeatherProviderVar* weatherProviderVar(const WeatherMeasurandInfo& info, WeatherProvider provider);

// Rasterweite einer Zeitebene in Sekunden. Tageswerte liefern 0, weil ein
// Kalendertag wegen der Zeitumstellung keine feste Länge hat.
uint32_t weatherLevelSeconds(WeatherLevel level);

const char* weatherLevelName(WeatherLevel level);
const char* weatherMeasurandName(const WeatherMeasurandInfo& info);
