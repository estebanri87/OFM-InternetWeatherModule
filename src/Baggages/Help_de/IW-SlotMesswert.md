### Messwert

Die auszugebende Messgröße einschließlich ihrer **Zeitebene**. Die Zeitebene steht im Namen hinter dem Bindestrich:

| Zeitebene | Bedeutung |
|---|---|
| **Aktuell** | Momentanwert. Kein Offset, keine Aggregation. |
| **15-Minuten-Werte** | Viertelstundenraster. Offset in 1/4 Stunden. |
| **Stundenwerte** | Stundenraster. Offset in Stunden. |
| **Tageswerte** | Tagesraster. Offset in Tagen. |

Nicht jede Messgröße gibt es auf jeder Zeitebene — angeboten wird nur, was Open-Meteo tatsächlich liefert. So gibt es die Niederschlagswahrscheinlichkeit nur als Stunden- und Tageswert, Sonnenauf- und -untergang nur als Tageswert, und die Strahlungsgrößen nicht als Momentanwert.

Bei Tageswerten ist die Aggregation bereits Teil des Messwerts, weil Open-Meteo Tageswerte fertig aggregiert liefert: "Temperatur (2 m) Maximum", "Temperatur (2 m) Minimum" und "Temperatur (2 m) Mittel" sind daher drei getrennte Einträge.

Der Datenpunkttyp des Kommunikationsobjekts richtet sich automatisch nach dem gewählten Messwert.

***Hinweis:*** 15-Minuten-Werte liefert Open-Meteo nur dort, wo ein entsprechend hoch aufgelöstes Wettermodell verfügbar ist (Mitteleuropa, Nordamerika). Außerhalb dieser Gebiete bleibt der Wert leer.

