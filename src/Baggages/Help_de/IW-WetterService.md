### Wetter-Service

Bestimmt, von welchem Dienst die Daten dieses Kanals kommen — und damit zugleich, welche Kategorien und Messwerte die Slots anbieten.

| Wert | Bedeutung |
|------|-----------|
| Deaktiviert | Kanal ist inaktiv und erscheint nicht in der Baumansicht |
| OpenWeatherMap | One Call 3.0, erfordert ein Abonnement und einen API Key |
| Open-Meteo | Freie Nutzung unter CC BY 4.0, optional mit Abo und eigenem Server |

Die Dienste unterscheiden sich im Angebot erheblich:

| | Aktuell | 15-Minuten-Werte | Stundenwerte | Tageswerte |
|---|:--:|:--:|:--:|:--:|
| OpenWeatherMap | ✓ | — | 48 Stunden | 8 Tage |
| Open-Meteo | ✓ | ✓ | 168 Stunden | 16 Tage |

**Nur bei Open-Meteo:** 15-Minuten-Werte, Strahlungsgrößen, Sonnenscheindauer, Tageslichtdauer, ET₀ sowie Werte aus der Vergangenheit (negative Offsets).

**Nur bei OpenWeatherMap:** Temperatur und gefühlte Temperatur für Morgen, Tag, Abend und Nacht als Tageswerte, sowie die Sichtweite.

Ein Wechsel des Dienstes behält die Slot-Einstellungen dort, wo beide Dienste dieselbe Größe liefern. Messwerte, die der neue Dienst nicht kennt, müssen neu gewählt werden.

