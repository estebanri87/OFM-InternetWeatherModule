### Messwert

Die auszugebende Messgröße einschließlich ihrer **Zeitebene**. Die Zeitebene steht im Namen hinter dem Bindestrich:

* **Aktuell** — Momentanwert, ohne Offset und ohne Aggregation.
* **15-Minuten-Werte** — Viertelstundenraster, Offset in Viertelstunden.
* **Stundenwerte** — Stundenraster, Offset in Stunden.
* **Tageswerte** — Tagesraster, Offset in Tagen.

Nicht jede Messgröße gibt es auf jeder Zeitebene — angeboten wird nur, was der gewählte Wetter-Service tatsächlich liefert. So gibt es die Niederschlagswahrscheinlichkeit nur als Stunden- und Tageswert, Sonnenauf- und -untergang nur als Tageswert, und die Strahlungsgrößen nicht als Momentanwert.

Bei Tageswerten ist die Aggregation bereits Teil des Messwerts, weil beide Dienste Tageswerte fertig aggregiert liefern. "Temperatur Maximum", "Temperatur Minimum" und "Temperatur Mittel" sind daher drei getrennte Einträge.

Der Datenpunkttyp des Kommunikationsobjekts richtet sich automatisch nach dem gewählten Messwert.

***Hinweis:*** 15-Minuten-Werte liefert Open-Meteo nur dort, wo ein entsprechend hoch aufgelöstes Wettermodell verfügbar ist, also in Mitteleuropa und Nordamerika. Außerhalb dieser Gebiete bleibt der Wert leer. OpenWeatherMap kennt diese Zeitebene gar nicht.

