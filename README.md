# OFM-InternetWeatherModule

Dies ist ein Modul zur Integration von Internet Wetterdiensten.

## Abhängigkeiten

Das Modul setzt [OFM-Network](https://github.com/OpenKNX/OFM-Network) oder [OFM-WLAN](https://github.com/mgeramb/OFM-WLANModule) voraus.

> [!WARNING]
> ## Achtung blockierende Netzwerkabfragen!
>
> Die aktuelle Implementierung nutzt (derzeit noch) lange blockierende HTTP-Requests,
> diese können die **Funktion bzw. das Zeitverhalten anderer Module stören**.
> Von einem kombinierten Einsatz wird daher abgeraten, bzw. sollte dieser nur nach sorgfältiger Abwägung der Risiken erfolgen.
>
> Es ruft immer nur ein Kanal gleichzeitig ab, mit mindestens 3 Sekunden Abstand; ein einzelner Abruf blockiert aber bis zu 8 Sekunden.

## Konzept

Ein Kanal steht für **einen Ort** und hat **drei frei belegbare Wert-Slots** (Wetter A, B und C). Als Ort wählt der Kanal den Gerätestandort oder einen der beiden zentral festgelegten Orte von der Seite „Position und Richtung". Je Slot wird ausgewählt:

| Einstellung | Bedeutung |
|---|---|
| Kategorie | Fachliche Gruppe, filtert die Messwert-Liste |
| Messwert | Messgröße samt Zeitebene, z.B. „Temperatur (2 m) - Stundenwerte" |
| Typ | Einzelwert oder Aggregation über ein Intervall |
| Offset | Abstand zum aktuellen Rasterschritt, Einheit richtet sich nach der Zeitebene; bei Stunden- und 15-Minuten-Werten alternativ „heute um" eine feste Uhrzeit |
| bis Offset, Aggregation | Nur bei Intervall: Ende als Offset, „heute um" oder „Tagesende"; Mittelwert, Minimum, Maximum, Summe oder Spannbreite |
| Senden | Nur bei Änderung oder bei jedem Abruf |

Damit lassen sich ohne zusätzliche Logikkanäle unter anderem abbilden:

- **drei Größen zu einem Zeitpunkt** — A/B/C mit verschiedenem Messwert, alle Offset 0
- **eine Größe zu drei Zeitpunkten** — gleicher Messwert, Offset 0/1/2
- **drei Aggregate über ein Intervall** — gleicher Messwert und Intervall, Aggregation Mittel/Min/Max
- **Werte des Kalendertages** — z.B. „Temperatur heute um 14 Uhr" oder „Maximaltemperatur bis Tagesende"

Werden mehr als drei Werte für denselben Ort gebraucht, wird ein weiterer Kanal mit demselben Ort angelegt.

Windgeschwindigkeiten werden modulweit wahlweise in km/h (DPT 9.028) oder m/s (DPT 9.005) ausgegeben.

Der Datenpunkttyp des Kommunikationsobjekts richtet sich automatisch nach dem gewählten Messwert. Abgefragt werden nur die Variablen und nur der Zeitraum, die von den belegten Slots gebraucht werden; mehrere Slots auf derselben Größe kosten keinen zusätzlichen Abruf.

## Wetterdienste

| | Aktuell | 15-Minuten-Werte | Stundenwerte | Tageswerte |
|---|:--:|:--:|:--:|:--:|
| Open-Meteo | ✓ | ✓ | 168 Stunden | 16 Tage |
| OpenWeatherMap (One Call 3.0) | ✓ | — | 48 Stunden | 8 Tage |

**Nur bei Open-Meteo:** 15-Minuten-Werte, Strahlungsgrößen, Sonnenscheindauer, Tageslichtdauer, ET₀ und Werte aus der Vergangenheit (negative Offsets).
**Nur bei OpenWeatherMap:** Temperatur und gefühlte Temperatur für Morgen, Tag, Abend und Nacht als Tageswerte, sowie die Sichtweite.

Der vollständige Messwert-Katalog steht in [`src/weather-catalog.json`](src/weather-catalog.json) und ist in der [Applikationsbeschreibung](doc/Applikationsbeschreibung-InternetWeather.md) tabellarisch aufgeführt.

## Katalog pflegen

`src/weather-catalog.json` ist die einzige Pflegestelle. Daraus erzeugt `tools/Generate-Catalog.ps1` die ETS-Parametertypen, das Slot-Fragment und die C++-Metadaten; `tools/Verify-Catalog.ps1` prüft jede Kombination gegen die Live-API.

## Release Notes

Änderungen in Releases siehe [Changelog](CHANGELOG.md) 


## Hardware Unterstützung

|Prozessor | Status | Anmerkung                     |
|----------|--------|-------------------------------|
|RP2040    | Beta   | Keine Unterstützung für HTTPS |
|ESP32     | Beta   |                               |

Getestete Hardware:
- [OpenKNX Reg1-ETH](https://github.com/OpenKNX/OpenKNX/wiki/REG1-Eth)
- Adafruit ESP32 Feather V2

## Einbindung in die Anwendung

In das Anwendungs XML muss OFM-Network (oder OFM-WLAN) und das OFM-InternetWeatherModule aufgenommen werden:

```xml
  <op:define prefix="NET" ModuleType="11" 
    share="../lib/OFM-Network/src/Network.share.xml">
    <op:verify File="../lib/OFM-Network/library.json" ModuleVersion="2" /> 
  </op:define>

  <op:define prefix="IW" ModuleType="21"
    share=   "../lib/OFM-InternetWeatherModule/src/InternetWeatherModule.share.xml"
    template="../lib/OFM-InternetWeatherModule/src/InternetWeatherModule.templ.xml"
    NumChannels="30"
    KoSingleOffset="400"
    KoOffset="410">
    <op:verify File="../lib/OFM-InternetWeatherModule/library.json" ModuleVersion="0.7" /> 
  </op:define>
```

**Hinweis:** Pro Kanal werden 4 KOs benötigt (Diagnose sowie Wert A, B und C), bei 30 Kanälen also 120. Dazu kommt ein modulweites KO bei `KoSingleOffset`. Dies muss bei nachfolgenden Modulen bei KoOffset und KoSingleOffset entsprechend berücksichtigt werden.

In main.cpp muss das ebenfalls das Network- (oder WLAN-) und InternetWeatherModule hinzugefügt werden:

```
[...]
#include "NetworkModule.h"
#include "InternetWeatherModule.h"
[...]

void setup()
{
    [...]
    openknx.addModule(1, openknxNetwork);
    openknx.addModule(3, openknxInternetWeatherModule);
    [...]
}
```

## Zugang zu den Wetterdiensten

Die Architektur dieses Moduls erlaubt die Nutzung verschiedener Wetter-Dienste.

Derzeit sind folgende Wetteranbieter integriert:

* [Open-Meteo](#open-meteo)
* [OpenWeatherMap](#openweathermap)

Pull Requests für weitere Dienste sind willkommen!

### Open-Meteo

[Open-Meteo](https://open-meteo.com/) ist eine "Open-Source-Wetter-API" und bietet für nicht-kommerzielle Nutzung einen Zugang ohne API-Key ("Free-API").
Dieser ist auf maximal 10.000 gewichtete Aufrufe ("API calls") pro Tag beschränkt (Stand 2025-06-01, entspricht mit etwa 3.000 Aktualisierungen in Summe für alle Orte deutlich mehr als zu erwarten);
Nutzungsbedingungen siehe https://open-meteo.com/en/terms (nur englisch).

### OpenWeatherMap

Für die Anfragen wird ein API Key von [https://openweathermap.org](https://openweathermap.org) benötigt.
1000 Aufrufe pro Tag können gratis durchgeführt werden, jedoch muss auch dafür ein Account angelegt werden und die Subscription für das `One Call API 3.0` aktiviert werden. 
Bei der Subscription sollte das `Call per day limit` auf 1000 eingestellt werden, damit keine Kosten anfallen können.

![Subscription](doc/IW-Subscription.png)

Siehe https://openweathermap.org/price

## Lizenz

[GNU GPL v3](LICENSE)