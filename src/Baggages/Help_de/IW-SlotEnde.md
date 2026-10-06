### Ende

Legt bei "Intervall-Aggregationen" fest, wie das Ende des Intervalls angegeben wird. Die Zeile erscheint nur bei Stunden- und 15-Minuten-Werten.

* **relativ zu jetzt** — Das Ende wird als Offset vom aktuellen Rasterschritt angegeben.
* **heute um** — Das Ende ist eine feste Uhrzeit des heutigen Tages.
* **Tagesende** — Das Intervall reicht bis zum letzten Rasterschritt des heutigen Tages, also bis 23 Uhr beziehungsweise 23:45 Uhr.

Beispiel: "Temperatur - Stundenwerte", relativ zu jetzt mit Offset 0 bis Tagesende, Maximum, ergibt die höchste Temperatur, die heute noch zu erwarten ist. Im Lauf des Tages fallen immer mehr Stunden aus dem Intervall heraus.

Liegt das Ende vor dem Beginn, wird kein Wert ausgegeben. Das kann bei "heute um" am Abend eintreten, wenn der Beginn relativ zu jetzt angegeben ist.

