### Zeitbezug

Legt fest, worauf sich der Beginn des Wertes bezieht. Die Zeile erscheint nur bei Stunden- und 15-Minuten-Werten.

* **relativ zu jetzt** — Der Beginn wird als Offset vom aktuellen Rasterschritt angegeben und wandert mit der Zeit mit.
* **heute um** — Der Beginn ist eine feste Uhrzeit des heutigen Tages. Der Wert bleibt bis Mitternacht an dieser Uhrzeit und springt dann auf den Folgetag.

Beispiel: "Temperatur - Stundenwerte", Einzelwert, heute um 14 Uhr, gibt den ganzen Tag die Temperatur um 14 Uhr aus.

Tageswerte beziehen sich immer auf heute und haben deshalb keinen Zeitbezug.

***Hinweis:*** Liegt die Uhrzeit bereits in der Vergangenheit, werden vergangene Werte benötigt. Diese liefert nur Open-Meteo. Bei OpenWeatherMap bleibt der Wert dann bis Mitternacht leer.

