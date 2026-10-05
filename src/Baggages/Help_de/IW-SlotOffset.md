### Offset

Abstand vom aktuellen Zeitraster-Schritt. Die Einheit richtet sich nach der Zeitebene des gewählten Messwerts:

* **15-Minuten-Werte** zählen in Viertelstunden. 0 ist die laufende Viertelstunde.
* **Stundenwerte** zählen in Stunden. 0 ist die laufende Stunde.
* **Tageswerte** zählen in Tagen. 0 ist heute.

Positive Werte zeigen in die Zukunft, negative in die Vergangenheit. Beispiele: Offset 1 bei Tageswerten ergibt "morgen", Offset −1 bei Stundenwerten die vergangene Stunde.

Wie weit der Offset reichen darf, hängt vom Wetter-Service ab. Open-Meteo liefert 168 Stunden und 16 Tage voraus sowie 48 Stunden und 2 Tage zurück. OpenWeatherMap liefert 48 Stunden und 8 Tage voraus und kennt keine Vergangenheit; dort beginnt der Bereich bei 0.

Bei "Intervall-Aggregationen" ist dies der **Beginn** des Intervalls.

