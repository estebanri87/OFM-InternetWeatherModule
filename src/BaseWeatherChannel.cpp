#include "BaseWeatherChannel.h"
#include <string.h>

// Vor diesem Zeitpunkt ist die Systemzeit nicht per NTP gesetzt.
#define IW_TIME_VALID_FROM 1577836800LL

bool WeatherRequest::any() const
{
    for (uint8_t i = 0; i < IW_LEVEL_COUNT; i++)
        if (levels[i].used) return true;
    return false;
}

BaseWeatherChannel::BaseWeatherChannel(uint8_t index)
{
    _channelIndex = index;
}

void BaseWeatherChannel::setup()
{
    switch (ParamIW_WeatherRefreshInterval)
    {
        case 1: _updateIntervalMs = 10 * 60 * 1000; break;
        case 2: _updateIntervalMs = 30 * 60 * 1000; break;
        case 3: _updateIntervalMs = 60 * 60 * 1000; break;
        default: _updateIntervalMs = 0; break;
    }

    const uint32_t refreshSeconds = _updateIntervalMs / 1000;
    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
    {
        _slots[i].setup(_channelIndex, i, provider(), refreshSeconds);
        if (_slots[i].configured() && _slots[i].info()->dpt == WeatherDpt::Dpt16_1) _textSlotIndex = (int8_t)i;
    }

    // Startversatz, damit nach dem Booten nicht alle Kanäle gleichzeitig abrufen.
    _nextFetchMs = 60000 + (uint32_t)_channelIndex * 3000;

    logSlots();
}

bool BaseWeatherChannel::fetchDue(uint32_t nowMs) const
{
    if (_fetchPending) return true;
    if (_updateIntervalMs == 0) return false;
    if (nowMs < _nextFetchMs) return false;
    return (_lastFetchMs == 0) || (nowMs - _lastFetchMs >= _updateIntervalMs);
}

void BaseWeatherChannel::loop()
{
    const uint32_t nowMs = millis();

    // Auswertung im Minutentakt: der Offset gilt relativ zu jetzt, der aktuelle
    // Rasterschritt wandert also auch ohne neuen Abruf weiter.
    if (nowMs - _lastEvaluateMs < 60000 && _lastEvaluateMs != 0) return;
    _lastEvaluateMs = nowMs;

    const time_t now = time(nullptr);
    if (now < IW_TIME_VALID_FROM) return;

    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
        publishSlot(i, now);
}

void BaseWeatherChannel::fetchNow()
{
    _fetchPending = false;
    _lastFetchMs = millis();

    const time_t now = time(nullptr);
    if (now < IW_TIME_VALID_FROM)
    {
        logWarningP("Systemzeit nicht gesetzt, Abruf übersprungen");
        return;
    }

    WeatherRequest request;
    buildRequest(request);
    if (!request.any())
    {
        logDebugP("Kein Slot belegt, kein Abruf nötig");
        return;
    }

    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
        _slots[i].invalidate();
    _textRain = NAN;
    _textSnow = NAN;
    _textClouds = NAN;

    const int16_t status = fetch(request);
    _lastHttpStatus = status;
    KoIW_CHHTTPStatus.value(status, DPT_Value_2_Count);

    if (status != 200)
    {
        logErrorP("Abruf fehlgeschlagen, Status %d", status);
        return;
    }

    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
        publishSlot(i, time(nullptr));
}

const char* BaseWeatherChannel::textHelperVar(uint8_t which) const
{
    // Der OpenKNX Wetter-Text existiert nur als Tageswert; die Hilfsgrößen sind
    // Regen, Schnee und Bewölkung desselben Tages.
    if (provider() == WeatherProvider::OpenMeteo)
    {
        switch (which)
        {
            case 0: return "rain_sum";
            case 1: return "snowfall_sum";
            case 2: return "cloud_cover_mean";
        }
    }
    else
    {
        switch (which)
        {
            case 0: return "rain";
            case 1: return "snow";
            case 2: return "clouds";
        }
    }
    return nullptr;
}

void BaseWeatherChannel::addVar(WeatherLevelRequest& lr, const char* var) const
{
    if (var == nullptr) return;
    for (uint8_t i = 0; i < lr.varCount; i++)
        if (strcmp(lr.vars[i], var) == 0) return;
    if (lr.varCount >= IW_MAX_VARS_PER_LEVEL) return;
    lr.vars[lr.varCount++] = var;
}

void BaseWeatherChannel::buildRequest(WeatherRequest& request) const
{
    request.latitude = ParamIW_CHWeatherLocationType ? ParamIW_CHLatitude : ParamBASE_Latitude;
    request.longitude = ParamIW_CHWeatherLocationType ? ParamIW_CHLongitude : ParamBASE_Longitude;

    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
    {
        const WeatherSlot& slot = _slots[i];
        if (!slot.configured()) continue;

        WeatherLevelRequest& lr = request.levels[(uint8_t)slot.level()];
        if (!lr.used)
        {
            lr.used = true;
            lr.from = slot.needFrom();
            lr.to = slot.needTo();
        }
        else
        {
            // Ein Zeitfenster über die Vereinigung aller Slots dieser Ebene -
            // drei Slots auf derselben Größe kosten so keinen zweiten Abruf.
            if (slot.needFrom() < lr.from) lr.from = slot.needFrom();
            if (slot.needTo() > lr.to) lr.to = slot.needTo();
        }

        if (slot.info()->dpt == WeatherDpt::Dpt16_1)
        {
            for (uint8_t h = 0; h < 3; h++)
                addVar(lr, textHelperVar(h));
        }
        else
            addVar(lr, slot.variable());
    }
}

void BaseWeatherChannel::applySeries(WeatherLevel level, const char* var, const time_t* times, const float* values, uint16_t count)
{
    const time_t now = time(nullptr);

    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
    {
        WeatherSlot& slot = _slots[i];
        if (!slot.configured() || slot.level() != level) continue;
        if (slot.variable() == nullptr || strcmp(slot.variable(), var) != 0) continue;
        slot.fill(times, values, count, now);
    }

    // Hilfsgrößen des Wetter-Textes am konfigurierten Offset herausgreifen
    if (_textSlotIndex < 0) return;
    const WeatherSlot& textSlot = _slots[_textSlotIndex];
    if (textSlot.level() != level) return;

    int32_t current = -1;
    for (uint16_t i = 0; i < count; i++)
    {
        if (times[i] <= now) current = (int32_t)i;
        else break;
    }
    if (current < 0) return;
    const int32_t idx = current + textSlot.needFrom();
    if (idx < 0 || idx >= (int32_t)count) return;

    if (textHelperVar(0) != nullptr && strcmp(var, textHelperVar(0)) == 0) _textRain = values[idx];
    else if (textHelperVar(1) != nullptr && strcmp(var, textHelperVar(1)) == 0) _textSnow = values[idx];
    else if (textHelperVar(2) != nullptr && strcmp(var, textHelperVar(2)) == 0) _textClouds = values[idx];
}

void BaseWeatherChannel::applyCurrent(const char* var, float value, time_t at)
{
    const time_t slotStart = weatherSlotStart(at, WeatherLevel::Current);
    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
    {
        WeatherSlot& slot = _slots[i];
        if (!slot.configured() || slot.level() != WeatherLevel::Current) continue;
        if (slot.variable() == nullptr || strcmp(slot.variable(), var) != 0) continue;
        slot.fillSingle(value, slotStart);
    }
}

void BaseWeatherChannel::buildWeatherText(char* target, uint8_t slotIndex, time_t now) const
{
    // Gleiche Heuristik wie bisher: Schnee schlägt Regen, Regen schlägt Bewölkung.
    const float rain = isnan(_textRain) ? 0.0f : _textRain;
    const float snow = isnan(_textSnow) ? 0.0f : _textSnow;
    const uint8_t clouds = isnan(_textClouds) ? 0 : (uint8_t)_textClouds;

    char body[14] = {};
    if (snow >= 0.1f)
        snprintf(body, sizeof(body), ParamIW_TextSnowStr.c_str(), snow);
    else if (rain >= 0.1f)
        snprintf(body, sizeof(body), ParamIW_TextRainStr.c_str(), rain);
    else if (clouds >= 20)
        snprintf(body, sizeof(body), ParamIW_TextCloudsStr.c_str(), clouds);
    else
        snprintf(body, sizeof(body), "%s", ParamIW_TextSunStr.c_str());

    const std::string prefix = (_slots[slotIndex].needFrom() == 0) ? ParamIW_TextPrefixDayCurrentStr : ParamIW_TextPrefixDayNextStr;
    snprintf(target, 15, "%s%s", prefix.c_str(), body);
}

void BaseWeatherChannel::publishSlot(uint8_t slotIndex, time_t now)
{
    WeatherSlot& slot = _slots[slotIndex];
    if (!slot.configured() || !slot.valid()) return;

    GroupObject& ko = knx.getGroupObject(IW_KoCalcNumber(IW_KoCHSlotA + slotIndex));
    const bool always = (slot.sendBehaviour() == PT_SendBehaviour::Always);

    if (slot.info()->dpt == WeatherDpt::Dpt16_1)
    {
        char text[15] = {};
        buildWeatherText(text, slotIndex, now);
        if (!always && strcmp(text, _lastText[slotIndex]) == 0) return;
        strncpy(_lastText[slotIndex], text, sizeof(_lastText[slotIndex]) - 1);
        ko.value(text, DPT_String_8859_1);
        return;
    }

    float value = 0.0f;
    if (!slot.evaluate(now, value)) return;

    if (!always && !isnan(_lastSent[slotIndex]) && _lastSent[slotIndex] == value) return;
    _lastSent[slotIndex] = value;

    switch (slot.info()->dpt)
    {
        case WeatherDpt::Dpt9_1: ko.value(value, DPT_Value_Temp); break;
        case WeatherDpt::Dpt9_6: ko.value(value, DPT_Value_Pres); break;
        case WeatherDpt::Dpt9_7: ko.value(value, DPT_Value_Humidity); break;
        case WeatherDpt::Dpt9_22: ko.value(value, Dpt(9, 22)); break;
        case WeatherDpt::Dpt9_26: ko.value(value, DPT_Rain_Amount); break;
        case WeatherDpt::Dpt9_28: ko.value(value, DPT_Value_Wsp_kmh); break;
        case WeatherDpt::Dpt9_31: ko.value(value, DPT_Value_Tempd); break;
        case WeatherDpt::Dpt5_1: ko.value((uint8_t)lroundf(value), DPT_Scaling); break;
        case WeatherDpt::Dpt5_3: ko.value((uint8_t)lroundf(value), DPT_Angle); break;
        case WeatherDpt::Dpt5_10: ko.value((uint8_t)lroundf(value), DPT_Value_1_Ucount); break;
        case WeatherDpt::Dpt7_1: ko.value((uint16_t)lroundf(value), DPT_Value_2_Ucount); break;
        case WeatherDpt::Dpt7_5: ko.value((uint16_t)lroundf(value), DPT_TimePeriodSec); break;
        case WeatherDpt::Dpt10_1:
        {
            // Der Anbieter liefert Sekunden seit lokaler Mitternacht.
            const uint32_t secs = (uint32_t)lroundf(value);
            struct tm tmValue = {};
            tmValue.tm_hour = (int)((secs / 3600) % 24);
            tmValue.tm_min = (int)((secs / 60) % 60);
            tmValue.tm_sec = (int)(secs % 60);
            ko.value(&tmValue, DPT_TimeOfDay);
            break;
        }
        default: break;
    }
}

void BaseWeatherChannel::processInputKo(GroupObject& ko)
{
    if (ko.asap() == IW_KoRefreshWeatherData) _fetchPending = true;
}

uint16_t BaseWeatherChannel::bufferBytes() const
{
    uint16_t total = 0;
    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
        total = (uint16_t)(total + _slots[i].bufferBytes());
    return total;
}

void BaseWeatherChannel::logSlots()
{
    logInfoP("Wetter-Kanal %u, Puffer %u Byte", _channelIndex + 1, bufferBytes());
    logIndentUp();
    char line[120];
    for (uint8_t i = 0; i < IW_SLOT_COUNT; i++)
    {
        _slots[i].describe(line, sizeof(line), i);
        if (_slots[i].unsupported())
            logWarningP("%s", line);
        else
            logInfoP("%s", line);
    }
    logIndentDown();
}

bool BaseWeatherChannel::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd == "update")
    {
        _fetchPending = true;
        logInfoP("Abruf angefordert");
        return true;
    }
    if (cmd == "slots")
    {
        logSlots();
        logInfoP("Letzter HTTP-Status: %d", _lastHttpStatus);
        return true;
    }
    return false;
}
