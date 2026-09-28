### Automatische Aktualisierung

Häufigkeit, mit der die Wetterdaten abgerufen werden. Pro Kanal wird je Intervall genau eine Abfrage durchgeführt; die Abfragen aller Kanäle laufen nacheinander, nie gleichzeitig.

Bei der Wahl des Intervalls sind die Aufruf-Limits der Free API zu beachten (10.000 Aufrufe pro Tag, 5.000 pro Stunde, 600 pro Minute). Beispiel: 10 Minuten bei 30 aktiven Kanälen ergibt 6 × 30 × 24 = 4.320 Aufrufe pro Tag.

Unabhängig vom Abrufintervall werden die ausgegebenen Werte **jede Minute** neu bestimmt. Ein Slot mit 15-Minuten-Werten und Offset 0 zeigt daher immer die aktuelle Viertelstunde, auch wenn der letzte Abruf länger zurückliegt.

