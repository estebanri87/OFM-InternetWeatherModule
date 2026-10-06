#ifndef OPENKNX_INTERNETWEATHER_IGNORE
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
        // Tagesdifferenz durch schrittweises Zählen, damit die Zeitumstellung
        // nicht durchschlägt. Der Bereich ist durch die Offset-Grenzen klein.
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

namespace
{
    uint32_t bufferBytesInUse = 0;

    // Lokale Uhrzeit hour:00 des Kalendertages, in dem t liegt.
    time_t todayAt(time_t t, int16_t hour)
    {
        struct tm tmLocal;
        localtime_r(&t, &tmLocal);
        tmLocal.tm_hour = hour;
        tmLocal.tm_min = 0;
        tmLocal.tm_sec = 0;
        tmLocal.tm_isdst = -1;
        return mktime(&tmLocal);
    }

    // Beginn des Kalendertages nach dem, in dem t liegt.
    time_t startOfNextDay(time_t t)
    {
        struct tm tmLocal;
        localtime_r(&t, &tmLocal);
        tmLocal.tm_mday += 1;
        tmLocal.tm_hour = 0;
        tmLocal.tm_min = 0;
        tmLocal.tm_sec = 0;
        tmLocal.tm_isdst = -1;
        return mktime(&tmLocal);
    }

    bool supportsTimeReference(WeatherLevel level)
    {
        return level == WeatherLevel::Hourly || level == WeatherLevel::Minutely15;
    }

    uint16_t stepsPerDay(WeatherLevel level)
    {
        const uint32_t step = weatherLevelSeconds(level);
        return (step > 0) ? (uint16_t)(86400 / step) : 1;
    }
} // namespace

// ---------------------------------------------------------------- Slot

uint32_t WeatherSlot::totalBufferBytes()
{
    return bufferBytesInUse;
}

void WeatherSlot::setup(uint8_t channelIndex, uint8_t slotIndex, WeatherProvider provider, uint32_t refreshSeconds)
{
    release();
    _unsupported = false;
    _overBudget = false;

    _measurandId = knx.paramByte(IW_SLOT_INDEX(channelIndex, slotIndex, Measurand));
    const WeatherMeasurandInfo* info = weatherMeasurand(_measurandId);
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

    _type = (PT_SlotValueType)IW_SLOT_FIELD(channelIndex, slotIndex, ValueType);
    _send = (PT_SendBehaviour)IW_SLOT_FIELD(channelIndex, slotIndex, Send);
    _agg = (PT_Aggregation)IW_SLOT_FIELD(channelIndex, slotIndex, Aggregation);
    _fromRef = (PT_FromRef)IW_SLOT_FIELD(channelIndex, slotIndex, FromRef);
    _toRef = (PT_ToRef)IW_SLOT_FIELD(channelIndex, slotIndex, ToRef);
    // Offset und Uhrzeit teilen sich dieselben Bytes; was gemeint ist, sagt der Zeitbezug.
    _from = (int16_t)knx.paramWord(IW_SLOT_INDEX(channelIndex, slotIndex, HourFrom));
    _to = (int16_t)knx.paramWord(IW_SLOT_INDEX(channelIndex, slotIndex, HourTo));

    // Die ETS blendet nicht passende Felder aus, ihre Bits bleiben aber stehen.
    // Deshalb hier auf das zurückführen, was zur Zeitebene passt.
    if (info->level == WeatherLevel::Current)
    {
        _from = 0;
        _to = 0;
    }
    if (!supportsTimeReference(info->level))
    {
        _fromRef = PT_FromRef::Relative;
        _toRef = PT_ToRef::Relative;
    }
    if (!info->aggregatable || info->level == WeatherLevel::Current) _type = PT_SlotValueType::Single;
    if (_type == PT_SlotValueType::Single) _toRef = PT_ToRef::Relative;
    if (_fromRef == PT_FromRef::TodayAt) _from = constrain(_from, (int16_t)0, (int16_t)23);
    if (_toRef == PT_ToRef::TodayAt) _to = constrain(_to, (int16_t)0, (int16_t)23);

    _info = info;
    _var = pv->var;
    _scale = pv->scale;

    // Pufferlänge: bei rein relativem Fenster dessen Spanne; mit Tagesbezug ein
    // ganzer Tag plus die relativen Anteile. Dazu die Marge für das
    // Weiterwandern bis zum nächsten Abruf.
    int32_t span = 1;
    if (usesDayReference())
    {
        int32_t relative = 0;
        if (_fromRef == PT_FromRef::Relative) relative = max(relative, (int32_t)abs(_from));
        if (_type == PT_SlotValueType::Interval && _toRef == PT_ToRef::Relative) relative = max(relative, (int32_t)abs(_to));
        span = stepsPerDay(info->level) + relative;
    }
    else if (_type == PT_SlotValueType::Interval)
        span = max((int32_t)1, (int32_t)(_to - _from + 1));

    int32_t margin = 1;
    const uint32_t step = weatherLevelSeconds(info->level);
    if (step > 0 && refreshSeconds > 0) margin = (int32_t)min((uint32_t)(refreshSeconds / step + 1), (uint32_t)16);

    const uint8_t capacity = (uint8_t)min(span + margin, (int32_t)IW_SLOT_MAX_SAMPLES);
    const uint32_t bytes = capacity * sizeof(float);
    if (bufferBytesInUse + bytes > IW_SLOT_BUDGET_BYTES)
    {
        _overBudget = true;
        _info = nullptr;
        return;
    }

    _capacity = capacity;
    _samples = new float[_capacity];
    for (uint8_t i = 0; i < _capacity; i++) _samples[i] = NAN;
    bufferBytesInUse += bytes;
}

void WeatherSlot::release()
{
    if (_samples != nullptr)
    {
        delete[] _samples;
        _samples = nullptr;
        bufferBytesInUse -= _capacity * sizeof(float);
    }
    _info = nullptr;
    _var = nullptr;
    _capacity = 0;
    _valid = false;
}

bool WeatherSlot::usesDayReference() const
{
    if (_info == nullptr || !supportsTimeReference(_info->level)) return false;
    if (_fromRef == PT_FromRef::TodayAt) return true;
    return _type == PT_SlotValueType::Interval && _toRef != PT_ToRef::Relative;
}

bool WeatherSlot::window(time_t now, int32_t& first, int32_t& last) const
{
    if (_info == nullptr) return false;

    if (_info->level == WeatherLevel::Current)
    {
        first = 0;
        last = 0;
        return true;
    }

    const WeatherLevel level = _info->level;
    const time_t current = weatherSlotStart(now, level);

    first = (_fromRef == PT_FromRef::TodayAt) ? weatherStepsBetween(current, todayAt(now, _from), level) : _from;

    if (_type == PT_SlotValueType::Single)
        last = first;
    else
    {
        switch (_toRef)
        {
            case PT_ToRef::TodayAt: last = weatherStepsBetween(current, todayAt(now, _to), level); break;
            // Letzter Rasterschritt, der heute noch beginnt.
            case PT_ToRef::EndOfDay: last = weatherStepsBetween(current, startOfNextDay(now), level) - 1; break;
            default: last = _to; break;
        }
    }
    return last >= first;
}

void WeatherSlot::need(time_t now, int16_t& from, int16_t& to) const
{
    int32_t first = 0;
    int32_t last = 0;
    if (!window(now, first, last)) first = 0;
    from = (int16_t)first;
    to = (int16_t)(first + max((int32_t)_capacity, (int32_t)1) - 1);
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

    // Aktueller Rasterschritt: letzter Eintrag, dessen Fenster 'now' enthält.
    int32_t current = -1;
    for (uint16_t i = 0; i < count; i++)
    {
        if (times[i] <= now) current = (int32_t)i;
        else break;
    }

    int32_t first = 0;
    int32_t last = 0;
    if (current < 0 || !window(now, first, last))
    {
        _valid = false;
        return;
    }

    const int32_t start = current + first;
    for (uint8_t i = 0; i < _capacity; i++)
    {
        const int32_t idx = start + i;
        _samples[i] = (idx >= 0 && idx < (int32_t)count) ? values[idx] : NAN;
    }

    // Verankerung: auch wenn der gewünschte erste Schritt außerhalb der
    // gelieferten Reihe liegt, muss der Zeitbezug stimmen.
    _zeroEpoch = weatherStepAdvance(times[current], _info->level, (int16_t)first);
    _valid = true;
}

bool WeatherSlot::evaluate(time_t now, float& out) const
{
    if (!_valid || _samples == nullptr || _info == nullptr) return false;

    int32_t first = 0;
    int32_t last = 0;
    if (!window(now, first, last)) return false;

    const time_t wantFirst = weatherStepAdvance(weatherSlotStart(now, _info->level), _info->level, (int16_t)first);
    const int32_t shift = weatherStepsBetween(_zeroEpoch, wantFirst, _info->level);
    const int32_t span = last - first + 1;

    // Liegt das Fenster außerhalb des Puffers, etwa kurz nach Mitternacht vor dem
    // nächsten Abruf, wird nichts gesendet statt eines falschen Wertes.
    if (shift < 0 || shift + span > _capacity) return false;

    float aggregated = 0.0f;
    if (!weatherAggregate(_samples + shift, (uint8_t)span, _agg, aggregated)) return false;

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
    if (_overBudget)
    {
        snprintf(buffer, length, "Wert %c: Messwert %u bleibt aus, Puffer-Budget von %u Byte erschöpft", letter, _measurandId, IW_SLOT_BUDGET_BYTES);
        return;
    }
    if (!configured())
    {
        snprintf(buffer, length, "Wert %c: nicht belegt", letter);
        return;
    }

    char from[16];
    char to[16];
    if (_fromRef == PT_FromRef::TodayAt)
        snprintf(from, sizeof(from), "heute %d Uhr", _from);
    else
        snprintf(from, sizeof(from), "%d", _from);

    if (_type == PT_SlotValueType::Single)
    {
        snprintf(buffer, length, "Wert %c: %s (%s) %s, Puffer %u Byte",
                 letter, _var, weatherLevelName(_info->level), from, bufferBytes());
        return;
    }

    switch (_toRef)
    {
        case PT_ToRef::TodayAt: snprintf(to, sizeof(to), "heute %d Uhr", _to); break;
        case PT_ToRef::EndOfDay: snprintf(to, sizeof(to), "Tagesende"); break;
        default: snprintf(to, sizeof(to), "%d", _to); break;
    }
    snprintf(buffer, length, "Wert %c: %s (%s) %s bis %s, Aggregation %u, Puffer %u Byte",
             letter, _var, weatherLevelName(_info->level), from, to, (uint8_t)_agg, bufferBytes());
}
#endif
