### Offset

Abstand vom aktuellen Zeitraster-Schritt. Die Einheit richtet sich nach der Zeitebene des gewählten Messwerts:

| Zeitebene | Einheit | Bereich | Bedeutung von 0 |
|---|---|---|---|
| 15-Minuten-Werte | 1/4 Stunden | −192 … 671 | laufende Viertelstunde |
| Stundenwerte | Stunden | −48 … 167 | laufende Stunde |
| Tageswerte | Tage | −2 … 6 | heute |

Positive Werte zeigen in die Zukunft, negative in die Vergangenheit. Beispiele: Offset 1 bei Tageswerten ergibt "morgen", Offset −1 bei Stundenwerten die vergangene Stunde.

Bei "Intervall-Aggregationen" ist dies der **Beginn** des Intervalls.

