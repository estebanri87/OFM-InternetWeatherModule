#include "WeatherSlot.h"
#include "WeatherAggregation.h"

// ---------------------------------------------------------------- Rasterarithmetik

time_t weatherSlotStart(time_t t, WeatherLevel level)
{
    const uint32_t step = weatherLevelSeconds(level);
    if (step > 0) return (time_t)((t / step) * step);

    if (level == WeatherLevel::Daily)
    {
        struct tm tmLocal;
        localtime_r(&t, &tmLocal);
        tmLocal.tm_hour = 0;
        tmLocal.tm_min = 0;
        tmLocal.tm_sec = 0;
        tmLocal.tm_isdst = -1;
        return mktime(&tmLocal);
    }
    return t;
}

time_t weatherStepAdvance(time_t t, WeatherLevel level, int16_t steps)
{
    const uint32_t step = weatherLevelSeconds(level);
    if (step > 0) return (time_t)(t + (int32_t)steps * (int32_t)step);

    if (level == WeatherLevel::Daily)
    {
        // Kalendarisch rechnen statt 86400 zu addieren, sonst kippt die Rechnung
        // an den Umstellungstagen um eine Stunde und damit um einen Tag.
        struct tm tmLocal;
        localtime_r(&t, &tmLocal);
        tmLocal.tm_mday += steps;
        tmLocal.tm_hour = 0;
        tmLocal.tm_min = 0;
        tmLocal.tm_sec = 0;
        tmLocal.tm_isdst = -1;
        return mktime(&tmLocal);
    }
    return t;
}

int32_t weatherStepsBetween(time_t from, time_t to, WeatherLevel level)
{
    const uint32_t step = weatherLevelSeconds(level);
    if (step > 0) return (int32_t)((to - from) / (int32_t)step);

    if (level == WeatherLevel::Daily)
    {
        // Tagesdifferenz durch schrittweises Zaehlen, damit die Zeitumstellung
        // nicht durchschlaegt. Der Bereich ist durch die Offset-Grenzen klein.
        if (to == from) return 0;
        const int8_t dir = (to > from) ? 1 : -1;
        time_t cursor = weatherSlotStart(from, level);
        const time_t target = weatherSlotStart(to, level);
        for (int32_t n = 0; n < 400; n++)
        {
            if (cursor == target) return n * dir;
            cursor = weatherStepAdvance(cursor, level, dir);
        }
        return 0;
    }
    return 0;
}

// ---------------------------------------------------------------- Slot

void WeatherSlot::setup(uint8_t channelIndex, uint8_t slotIndex, WeatherProvider provider, uint32_t refreshSeconds)
{
    release();

    const uint16_t base = (uint16_t)(IW_SLOT_BASE + slotIndex * IW_SLOT_STRIDE);
    const uint16_t paramBase = (uint16_t)(IW_ParamBlockOffset + channelIndex * IW_ParamBlockSize + base);

    const uint8_t flags = knx.paramByte(paramBase + IW_SLOT_OFF_FLAGS);
    const uint8_t measurandId = knx.paramByte(paramBase + IW_SLOT_OFF_MEASURAND);

    _measurandId = measurandId;

    const WeatherMeasurandInfo* info = weatherMeasurand(measurandId);
    if (info == nullptr) return;

    // Nach einem Anbieter- oder Kategoriewechsel kann eine Id stehenbleiben, die
    // der neue Dienst nicht liefert. Dann bleibt der Slot still, statt ins Leere
    // zu greifen; der Kanal weist beim Start darauf hin.
    const WeatherProviderVar* pv = weatherProviderVar(*info, provider);
    if (pv == nullptr)
    {
        _unsupported = true;
        return;
    }

    _info = info;
    _var = pv->var;
    _scale = pv->scale;
    _type = (PT_SlotValueType)((flags >> 3) & 0x01);
    _agg = (PT_Aggregation)((flags >> 1) & 0x03);
    _send = (PT_SendBehaviour)(flags & 0x01);

    _offsetFrom = (int16_t)knx.paramWord(paramBase + IW_SLOT_OFF_FROM);
    _offsetTo = (int16_t)knx.paramWord(paramBase + IW_SLOT_OFF_TO);

    if (_info->level == WeatherLevel::Current)
    {
        _offsetFrom = 0;
        _offsetTo = 0;
    }
    if (_type == PT_SlotValueType::Single || !_info->aggregatable) _offsetTo = _offsetFrom;
    if (_offsetTo < _offsetFrom) _offsetTo = _offsetFrom;

    _span = (uint8_t)min((int32_t)(_offsetTo - _offsetFrom + 1), (int32_t)IW_SLOT_MAX_SAMPLES);

    // Marge: so viele Schritte, wie zwischen zwei Abrufen vergehen koennen.
    uint8_t margin = 1;
    const uint32_t step = weatherLevelSeconds(_info->level);
    if (step > 0 && refreshSeconds > 0) margin = (uint8_t)min((uint32_t)(refreshSeconds / step + 1), (uint32_t)16);

    uint16_t capacity = (uint16_t)(_span + margin);
    if (capacity > IW_SLOT_MAX_SAMPLES) capacity = IW_SLOT_MAX_SAMPLES;
    _capacity = (uint8_t)capacity;

    _samples = new float[_capacity];
    for (uint8_t i = 0; i < _capacity; i++) _samples[i] = NAN;
}

void WeatherSlot::release()
{
    if (_samples != nullptr)
    {
        delete[] _samples;
        _samples = nullptr;
    }
    _info = nullptr;
    _var = nullptr;
    _capacity = 0;
    _span = 1;
    _valid = false;
}

void WeatherSlot::fillSingle(float rawValue, time_t slotStart)
{
    if (_samples == nullptr) return;
    for (uint8_t i = 0; i < _capacity; i++) _samples[i] = NAN;
    _samples[0] = rawValue;
    _zeroEpoch = slotStart;
    _valid = true;
}

void WeatherSlot::fill(const time_t* times, const float* values, uint16_t count, time_t now)
{
    if (_samples == nullptr || times == nullptr || values == nullptr || count == 0) return;

    // Aktueller Rasterschritt: letzter Eintrag, dessen Fenster 'now' enthaelt.
    int32_t current = -1;
    for (uint16_t i = 0; i < count; i++)
    {
        if (times[i] <= now) current = (int32_t)i;
        else break;
    }
    if (current < 0)
    {
        _valid = false;
        return;
    }

    const int32_t first = current + _offsetFrom;
    for (uint8_t i = 0; i < _capacity; i++)
    {
        const int32_t idx = first + i;
        _samples[i] = (idx >= 0 && idx < (int32_t)count) ? values[idx] : NAN;
    }

    // Verankerung: auch wenn der gewuenschte erste Schritt ausserhalb der
    // gelieferten Reihe liegt, muss der Zeitbezug stimmen.
    _zeroEpoch = weatherStepAdvance(times[current], _info->level, _offsetFrom);
    _valid = true;
}

bool WeatherSlot::evaluate(time_t now, float& out) const
{
    if (!_valid || _samples == nullptr || _info == nullptr) return false;

    const time_t wantFirst = weatherStepAdvance(weatherSlotStart(now, _info->level), _info->level, _offsetFrom);
    const int32_t shift = weatherStepsBetween(_zeroEpoch, wantFirst, _info->level);

    if (shift < 0 || shift + _span > _capacity) return false;

    float aggregated = 0.0f;
    if (!weatherAggregate(_samples + shift, _span, _agg, aggregated)) return false;

    out = aggregated * _scale;
    return true;
}

void WeatherSlot::describe(char* buffer, size_t length, uint8_t slotIndex) const
{
    const char letter = (char)('A' + slotIndex);

    if (_unsupported)
    {
        snprintf(buffer, length, "Wert %c: Messwert %u liefert der gewählte Dienst nicht", letter, _measurandId);
        return;
    }
    if (!configured())
    {
        snprintf(buffer, length, "Wert %c: nicht belegt", letter);
        return;
    }

    if (_span > 1)
        snprintf(buffer, length, "Wert %c: %s (%s) Offset %d..%d, Aggregation %u, Puffer %u Byte",
                 letter, _var, weatherLevelName(_info->level), _offsetFrom, _offsetTo, (uint8_t)_agg, bufferBytes());
    else
        snprintf(buffer, length, "Wert %c: %s (%s) Offset %d, Puffer %u Byte",
                 letter, _var, weatherLevelName(_info->level), _offsetFrom, bufferBytes());
}
