<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2026 Steffen Rittmeier -->

# Applikationsbeschreibung Internet Wetter (InternetWeather)

Das Modul stellt Wetterdaten von [Open-Meteo](https://open-meteo.com/) als KNX-Gruppenobjekte bereit.

Jeder Kanal steht für **einen Ort**. Innerhalb eines Kanals gibt es **drei frei belegbare Wert-Slots** (Wetter A, B und C). Für jeden Slot wird ausgewählt, welche Messgröße auf welcher Zeitebene ausgegeben werden soll — als Einzelwert zu einem Zeitpunkt oder als Aggregation über ein Intervall.

Daraus ergibt sich ein wichtiger Unterschied zu früheren Versionen: Es gibt keine festen Blöcke wie "Prognose Heute" oder "Prognose nächste Stunde" mehr. Stattdessen wird jeder gewünschte Wert einzeln zusammengestellt. Wer mehr als drei Werte für denselben Ort braucht, legt einen weiteren Kanal mit demselben Ort an.

Automatisierungslogik (Beschattung, Bewässerung, Heizungsführung) bildet das Modul nicht ab — dafür ist das Logikmodul zuständig.

---

## Inhaltsverzeichnis

- [Datenquelle](#datenquelle)
  - [Nutzung und Lizenz](#nutzunglizenz-open-meteo)
  - [Server-Basis-URL](#server-basis-url-open-meteo)
  - [API Key](#api-key-open-meteo)
  - [Automatische Aktualisierung](#automatische-aktualisierung)
- [Kanal](#wetter-kanal)
  - [Ort](#ort)
  - [Breitengrad](#breitengrad) / [Längengrad](#längengrad)
- [Wert-Slots](#bezeichnung)
  - [Kategorie](#kategorie) / [Messwert](#messwert)
  - [Typ](#typ) / [Offset](#offset) / [bis Offset](#bis-offset) / [Aggregation](#aggregation)
  - [Senden](#senden)
- [Wetterbeschreibung als Text](#wolkenlos)
- [Zeitebenen und Verfügbarkeit](#zeitebenen-und-verfügbarkeit)

---

# Datenquelle

<!-- DOC HelpContext="Service" -->
## Wetter-Kanal

Ein Kanal liefert Wetterdaten für **einen Ort**. Er stellt drei frei belegbare Wert-Slots bereit (Wetter A, B und C) sowie ein Diagnose-Objekt mit dem HTTP-Statuscode des letzten Abrufs.

Der Abruf erfolgt bei Open-Meteo. Die Zeitzone wird automatisch aus den Koordinaten des Ortes bestimmt — Tagesgrenzen und Uhrzeiten beziehen sich damit immer auf die Ortszeit des Wetterstandorts, nicht auf die Gerätezeitzone. Das ist bei Orten in einer anderen Zeitzone als der des Geräts das fachlich richtige Verhalten.

Das Modul fragt nur die Variablen und nur den Zeitraum ab, die von den drei Slots tatsächlich gebraucht werden. Ein Kanal mit wenigen Slots erzeugt daher sehr kleine Anfragen.

<!-- DOCEND -->

---

<!-- DOC HelpContext="WetterService" -->
## Wetter-Service

Bestimmt, von welchem Dienst die Daten dieses Kanals kommen — und damit zugleich, welche Kategorien und Messwerte die Slots anbieten.

* **Deaktiviert** — Kanal ist inaktiv und erscheint nicht in der Baumansicht.
* **OpenWeatherMap** — One Call 3.0, erfordert ein Abonnement und einen API Key.
* **Open-Meteo** — freie Nutzung unter CC BY 4.0, optional mit Abo und eigenem Server.

Die Dienste unterscheiden sich im Angebot erheblich:

* **Open-Meteo** liefert aktuelle Werte, 15-Minuten-Werte, Stundenwerte bis 168 Stunden und Tageswerte bis 16 Tage voraus. Zusätzlich nur hier: Strahlungsgrößen, Sonnenscheindauer, Tageslichtdauer, ET₀ und Werte aus der Vergangenheit über negative Offsets.
* **OpenWeatherMap** liefert aktuelle Werte, Stundenwerte bis 48 Stunden und Tageswerte bis 8 Tage voraus, aber keine 15-Minuten-Werte und keine Vergangenheit. Zusätzlich nur hier: Temperatur und gefühlte Temperatur für Morgen, Tag, Abend und Nacht als Tageswerte sowie die Sichtweite.

Ein Wechsel des Dienstes behält die Slot-Einstellungen dort, wo beide Dienste dieselbe Größe liefern. Messwerte, die der neue Dienst nicht kennt, müssen neu gewählt werden.

<!-- DOCEND -->

---

<!-- DOC HelpContext="OpenWeatherMapAPIKey" -->
## API Key (OpenWeatherMap)

Der API Key für die One Call 3.0 API. Er wird bei jedem Abruf über den URL-Parameter `appid` übergeben.

One Call 3.0 ist nur im Abonnement „One Call by Call" verfügbar; bis 1.000 Aufrufe pro Tag fallen keine Kosten an. Für das Abonnement wird eine Zahlungsmethode hinterlegt, auch wenn das Freikontingent nicht überschritten wird.

Siehe https://openweathermap.org/api/one-call-3

<!-- DOCEND -->

---

<!-- DOC HelpContext="OpenMeteoUsage" -->
## Nutzung/Lizenz (Open-Meteo)

Über diesen Parameter *muss* ausgewählt werden, in welcher Form Open-Meteo genutzt werden soll.

***Wichtig:*** Ohne eine Angabe kann Open-Meteo nicht verwendet werden!

* **Bitte wählen...** — Vorgabewert, solange noch keine explizite Auswahl getroffen wurde.
* **Nicht kommerziell ('Free API')** — Zur ausschließlich nicht-kommerziellen Nutzung unter Einhaltung der im Rahmen des anonymen Zugriffs erlaubten Abfrageanzahl.
* **API Subscription** — Zur kommerziellen Nutzung, entsprechend bestehendem Abo.
* **Selbst gehostet** — Falls ein eigener Server betrieben wird, über den die Wetterdaten abgerufen werden können.

Die Wetterdaten stehen unter der Creative-Commons-Lizenz Attribution 4.0 International (CC BY 4.0). Datenquellen und genaue Nutzungsbedingungen siehe https://open-meteo.com/en/license

<!-- DOCEND -->

---

<!-- DOC HelpContext="OpenMeteoServerBaseUrl" -->
## Server-Basis-URL (Open-Meteo)

Basis-URL des zu verwendenden Servers mit Pfadangabe vor `/v1/forecast` einschließlich Protokoll.

***Achtung:*** Auf RP2040-basierten Geräten wird derzeit kein HTTPS unterstützt!

Beispiel: `https://customer-api.open-meteo.com` falls die API unter der URL `https://customer-api.open-meteo.com/v1/forecast` angeboten wird.

Siehe https://open-meteo.com/en/pricing

<!-- DOCEND -->

---

<!-- DOC HelpContext="OpenMeteoAPIKey" -->
## API Key (Open-Meteo)

Der API-Key wird benötigt zur Nutzung mit "API Subscription".

Der Wert wird bei Abfragen unverändert über den URL-Parameter `apikey` übergeben.

Beispiel: `&apikey=abc123`

Siehe https://open-meteo.com/en/pricing

<!-- DOCEND -->

---

<!-- DOC HelpContext="WeatherRefreshInterval" -->
## Automatische Aktualisierung

Häufigkeit, mit der die Wetterdaten abgerufen werden. Pro Kanal wird je Intervall genau eine Abfrage durchgeführt; die Abfragen aller Kanäle laufen nacheinander, nie gleichzeitig.

Bei der Wahl des Intervalls sind die Aufruf-Limits der Free API zu beachten (10.000 Aufrufe pro Tag, 5.000 pro Stunde, 600 pro Minute). Beispiel: 10 Minuten bei 30 aktiven Kanälen ergibt 6 × 30 × 24 = 4.320 Aufrufe pro Tag.

Unabhängig vom Abrufintervall werden die ausgegebenen Werte **jede Minute** neu bestimmt. Ein Slot mit 15-Minuten-Werten und Offset 0 zeigt daher immer die aktuelle Viertelstunde, auch wenn der letzte Abruf länger zurückliegt.

<!-- DOCEND -->

---

# Kanal

<!-- DOC HelpContext="WeatherLocationType" -->
## Ort

Ort, für den das Wetter bestimmt werden soll.

* **Gerätestandort aus Allgemein** — Die Geo-Koordinaten aus den allgemeinen Geräteeinstellungen werden verwendet.
* **Anderer Ort** — Die Geo-Koordinaten werden hier im Kanal eingestellt.

<!-- DOCEND -->

---

<!-- DOC HelpContext="Latitude" -->
## Breitengrad

Breitengrad im Dezimalformat des Ortes, für den das Wetter bestimmt werden soll.

Beispiel: `48.2083` für Wien

<!-- DOCEND -->

---

<!-- DOC HelpContext="Longitude" -->
## Längengrad

Längengrad im Dezimalformat des Ortes, für den das Wetter bestimmt werden soll.

Beispiel: `16.3731` für Wien

<!-- DOCEND -->

---

# Wert-Slots

<!-- DOC HelpContext="SlotBezeichnung" -->
## Bezeichnung

Freitext für diesen Wert-Slot. Der Text erscheint als Name des Kommunikationsobjekts und in der Baumansicht.

Eine aussagekräftige Benennung erleichtert die Zuordnung erheblich, da alle Slots technisch gleich aussehen — erst Kategorie und Messwert legen fest, was ausgegeben wird. Beispiele: "Aussentemperatur", "Regen morgen", "Wind Mittel 6h".

<!-- DOCEND -->

---

<!-- DOC HelpContext="SlotKategorie" -->
## Kategorie

Fachliche Gruppe, aus der der Messwert stammt. Die Kategorie dient nur der Vorauswahl: Sie bestimmt, welche Einträge in der Liste "Messwert" angeboten werden.

Verfügbare Kategorien:

* **Zusammenfassung und Überblick** — OpenKNX Wetter-Text, Wettercode
* **Temperatur** — Temperatur, gefühlte Temperatur, Taupunkt
* **Feuchte und Druck** — relative Luftfeuchte, Luftdruck, Sichtweite
* **Wind** — Windgeschwindigkeit, Windböen, Windrichtung
* **Niederschlag** — Niederschlag, Regen, Schneefall, Niederschlagswahrscheinlichkeit
* **Bewölkung und Sonne** — Bewölkung, Sonnenscheindauer, Sonnenauf- und -untergang, Tageslichtdauer
* **Strahlung** — Global-, Direkt-, Diffus- und Direktnormalstrahlung, UV-Index
* **Landwirtschaft** — ET₀ Referenz-Verdunstung

Angeboten werden nur die Kategorien, zu denen der gewählte Wetter-Service auch Messwerte liefert. Bei OpenWeatherMap fehlt daher die Landwirtschaft.

***Hinweis:*** Nach einem Wechsel der Kategorie muss der Messwert neu ausgewählt werden.

<!-- DOCEND -->

---

<!-- DOC HelpContext="SlotMesswert" -->
## Messwert

Die auszugebende Messgröße einschließlich ihrer **Zeitebene**. Die Zeitebene steht im Namen hinter dem Bindestrich:

* **Aktuell** — Momentanwert, ohne Offset und ohne Aggregation.
* **15-Minuten-Werte** — Viertelstundenraster, Offset in Viertelstunden.
* **Stundenwerte** — Stundenraster, Offset in Stunden.
* **Tageswerte** — Tagesraster, Offset in Tagen.

Nicht jede Messgröße gibt es auf jeder Zeitebene — angeboten wird nur, was der gewählte Wetter-Service tatsächlich liefert. So gibt es die Niederschlagswahrscheinlichkeit nur als Stunden- und Tageswert, Sonnenauf- und -untergang nur als Tageswert, und die Strahlungsgrößen nicht als Momentanwert.

Bei Tageswerten ist die Aggregation bereits Teil des Messwerts, weil beide Dienste Tageswerte fertig aggregiert liefern. "Temperatur Maximum", "Temperatur Minimum" und "Temperatur Mittel" sind daher drei getrennte Einträge.

Der Datenpunkttyp des Kommunikationsobjekts richtet sich automatisch nach dem gewählten Messwert.

***Hinweis:*** 15-Minuten-Werte liefert Open-Meteo nur dort, wo ein entsprechend hoch aufgelöstes Wettermodell verfügbar ist, also in Mitteleuropa und Nordamerika. Außerhalb dieser Gebiete bleibt der Wert leer. OpenWeatherMap kennt diese Zeitebene gar nicht.

<!-- DOCEND -->

---

<!-- DOC HelpContext="SlotTyp" -->
## Typ

Legt fest, ob ein einzelner Zeitpunkt oder ein Zeitraum ausgegeben wird.

* **Einzelwert** — Der Wert an genau einer Stelle des Rasters, bestimmt durch "Offset".
* **Intervall-Aggregationen** — Mehrere aufeinanderfolgende Werte von "Offset" bis "bis Offset" werden nach der gewählten Aggregation zu einem Wert zusammengefasst.

Die Zeile erscheint nur bei Messwerten, die sich sinnvoll zusammenfassen lassen. Bei Wettercode, Wetter-Text, Windrichtung sowie Sonnenauf- und -untergang bleibt es beim Einzelwert, weil ein Mittelwert dieser Größen keine sinnvolle Aussage ergäbe.

<!-- DOCEND -->

---

<!-- DOC HelpContext="SlotOffset" -->
## Offset

Abstand vom aktuellen Zeitraster-Schritt. Die Einheit richtet sich nach der Zeitebene des gewählten Messwerts:

* **15-Minuten-Werte** zählen in Viertelstunden. 0 ist die laufende Viertelstunde.
* **Stundenwerte** zählen in Stunden. 0 ist die laufende Stunde.
* **Tageswerte** zählen in Tagen. 0 ist heute.

Positive Werte zeigen in die Zukunft, negative in die Vergangenheit. Beispiele: Offset 1 bei Tageswerten ergibt "morgen", Offset −1 bei Stundenwerten die vergangene Stunde.

Wie weit der Offset reichen darf, hängt vom Wetter-Service ab. Open-Meteo liefert 168 Stunden und 16 Tage voraus sowie 48 Stunden und 2 Tage zurück. OpenWeatherMap liefert 48 Stunden und 8 Tage voraus und kennt keine Vergangenheit; dort beginnt der Bereich bei 0.

Bei "Intervall-Aggregationen" ist dies der **Beginn** des Intervalls.

<!-- DOCEND -->

---

<!-- DOC HelpContext="SlotOffsetBis" -->
## bis Offset

Ende des Intervalls bei "Intervall-Aggregationen", in derselben Einheit wie "Offset". Das Intervall schließt beide Grenzen ein.

Beispiel: Stundenwerte mit Offset 0 und bis Offset 5 fassen die laufende und die folgenden fünf Stunden zusammen, also sechs Werte.

Ist "bis Offset" kleiner als "Offset", wird kein Wert ausgegeben.

Die Intervall-Länge ist begrenzt auf 96 Viertelstunden, 48 Stunden beziehungsweise 7 Tage.

<!-- DOCEND -->

---

<!-- DOC HelpContext="SlotAggregation" -->
## Aggregation

Rechenvorschrift, mit der die Werte des Intervalls zu einem Wert zusammengefasst werden.

* **Mittelwert (Durchschnitt)** — Summe geteilt durch Anzahl.
* **Minimum** — kleinster Wert im Intervall.
* **Maximum** — größter Wert im Intervall.
* **Summe** — Summe aller Werte im Intervall.

***Hinweis zur Einheit:*** "Summe" ist nur bei Mengengrößen wie Regen oder Schneefall physikalisch sinnvoll. Bei Raten- und Zustandsgrößen wie Temperatur, Wind oder Strahlung bleibt der Datenpunkttyp unverändert, obwohl die Summe eine andere physikalische Größe darstellt. Dort sind Mittelwert, Minimum oder Maximum die passende Wahl.

<!-- DOCEND -->

---

<!-- DOC HelpContext="SlotSenden" -->
## Senden

Legt fest, wann das Kommunikationsobjekt geschrieben wird.

* **Nur bei Änderung** — Es wird nur gesendet, wenn sich der Wert gegenüber der letzten Ausgabe geändert hat. Das hält die Buslast niedrig.
* **Bei jedem Abruf** — Es wird nach jeder Neuberechnung gesendet, auch bei unverändertem Wert. Sinnvoll für Empfänger, die eine regelmäßige Lebendmeldung erwarten.

Das Leseflag ist bei allen Wert-Objekten gesetzt, eine Visualisierung kann den Wert also jederzeit aktiv anfordern.

<!-- DOCEND -->

---

# Wetterbeschreibung als Text

Der Messwert "OpenKNX Wetter-Text" fasst Regen, Schneefall und Bewölkung zu einem 14 Zeichen langen Text zusammen (DPT 16.001). Die folgenden Vorlagen bestimmen den Wortlaut. `XXX` wird jeweils durch den zugehörigen Zahlenwert ersetzt.

<!-- DOC HelpContext="WeatherConditionSun" -->
## Wolkenlos

Wird verwendet, wenn weder nennenswerter Niederschlag noch nennenswerte Bewölkung vorliegt.

<!-- DOCEND -->

<!-- DOC HelpContext="WeatherConditionClouds" -->
## Wolken

Wird bei Bewölkung verwendet. `XXX` wird durch den Prozentwert der Bedeckung ersetzt.

<!-- DOCEND -->

<!-- DOC HelpContext="WeatherConditionRain" -->
## Regen

Wird bei Regen verwendet. `XXX` wird durch die Regenmenge in Millimetern ersetzt.

<!-- DOCEND -->

<!-- DOC HelpContext="WeatherConditionSnow" -->
## Schnee

Wird bei Schneefall verwendet. `XXX` wird durch die Schneemenge in Millimetern ersetzt.

<!-- DOCEND -->

<!-- DOC HelpContext="WeatherConditionCurrentDayPrefix" -->
## Prefix aktueller Tag

Wird dem Text vorangestellt, wenn der Slot den aktuellen Tag ausgibt, also bei Offset 0.

<!-- DOCEND -->

<!-- DOC HelpContext="WeatherConditionNextDayPrefix" -->
## Prefix nächster Tag

Wird dem Text vorangestellt, wenn der Slot einen künftigen Tag ausgibt, also bei einem Offset größer als 0.

<!-- DOCEND -->

---

# Zeitebenen und Verfügbarkeit

Die folgende Übersicht zeigt, welche Messgröße auf welcher Zeitebene angeboten wird. Die Liste wird aus `src/weather-catalog.json` gepflegt und ist gegen die Open-Meteo-API verifiziert (`tools/Verify-Catalog.ps1`).

| Messgröße | Aktuell | 15-Min | Stunde | Tag | Datenpunkttyp |
|---|:--:|:--:|:--:|:--:|---|
| OpenKNX Wetter-Text | | | | ✓ | 16.001 |
| Wettercode (WMO) | ✓ | | ✓ | ✓ | 5.010 |
| Temperatur (2 m) | ✓ | ✓ | ✓ | Max/Min/Mittel | 9.001 |
| Gefühlte Temperatur | ✓ | ✓ | ✓ | Max/Min/Mittel | 9.001 |
| Relative Luftfeuchte (2 m) | ✓ | ✓ | ✓ | | 9.007 |
| Luftdruck | ✓ | | ✓ | | 9.006 |
| Windgeschwindigkeit (10 m) | ✓ | ✓ | ✓ | Maximum | 9.028 |
| Windböen (10 m) | ✓ | ✓ | ✓ | Maximum | 9.028 |
| Windrichtung (10 m) | ✓ | ✓ | ✓ | vorherrschend | 5.003 |
| Niederschlag | ✓ | ✓ | ✓ | Summe | 9.026 |
| Regen | ✓ | ✓ | ✓ | Summe | 9.026 |
| Schneefall | ✓ | ✓ | ✓ | Summe | 9.026 |
| Niederschlagswahrscheinlichkeit | | | ✓ | Maximum | 5.001 |
| Bewölkung | ✓ | | ✓ | | 5.001 |
| Sonnenscheindauer | | | ✓ | ✓ | 7.005 |
| Tageslichtdauer | | | | ✓ | 7.005 |
| Sonnenaufgang / Sonnenuntergang | | | | ✓ | 10.001 |
| Globalstrahlung | | ✓ | ✓ | | 9.022 |
| Direktstrahlung | | ✓ | ✓ | | 9.022 |
| Diffusstrahlung | | ✓ | ✓ | | 9.022 |
| Direktnormalstrahlung | | ✓ | ✓ | | 9.022 |
| UV-Index | ✓ | | ✓ | Maximum | 9.031 |
| ET₀ Referenz-Verdunstung | | | ✓ | Summe | 9.026 |

---

# Gruppenobjekte

Je Kanal gibt es vier Kommunikationsobjekte:

| Nr. | Name | Richtung | Datenpunkttyp |
|---|---|---|---|
| 0 | Diagnose HTTP | Ausgang | 8.001 — HTTP-Statuscode des letzten Abrufs, 200 = in Ordnung |
| 1 | Wert A | Ausgang | richtet sich nach dem gewählten Messwert |
| 2 | Wert B | Ausgang | richtet sich nach dem gewählten Messwert |
| 3 | Wert C | Ausgang | richtet sich nach dem gewählten Messwert |

Zusätzlich gibt es ein modulweites Objekt, das einen sofortigen Abruf aller Kanäle auslöst.
