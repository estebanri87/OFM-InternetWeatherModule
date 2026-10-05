### Wetter-Service

Bestimmt, von welchem Dienst die Daten dieses Kanals kommen — und damit zugleich, welche Kategorien und Messwerte die Slots anbieten.

* **Deaktiviert** — Kanal ist inaktiv und erscheint nicht in der Baumansicht.
* **OpenWeatherMap** — One Call 3.0, erfordert ein Abonnement und einen API Key.
* **Open-Meteo** — freie Nutzung unter CC BY 4.0, optional mit Abo und eigenem Server.

Die Dienste unterscheiden sich im Angebot erheblich:

* **Open-Meteo** liefert aktuelle Werte, 15-Minuten-Werte, Stundenwerte bis 168 Stunden und Tageswerte bis 16 Tage voraus. Zusätzlich nur hier: Strahlungsgrößen, Sonnenscheindauer, Tageslichtdauer, ET₀ und Werte aus der Vergangenheit über negative Offsets.
* **OpenWeatherMap** liefert aktuelle Werte, Stundenwerte bis 48 Stunden und Tageswerte bis 8 Tage voraus, aber keine 15-Minuten-Werte und keine Vergangenheit. Zusätzlich nur hier: Temperatur und gefühlte Temperatur für Morgen, Tag, Abend und Nacht als Tageswerte sowie die Sichtweite.

Ein Wechsel des Dienstes behält die Slot-Einstellungen dort, wo beide Dienste dieselbe Größe liefern. Messwerte, die der neue Dienst nicht kennt, müssen neu gewählt werden.

