### Senden

Legt fest, wann das Kommunikationsobjekt geschrieben wird.

* **Nur bei Änderung** — Es wird nur gesendet, wenn sich der Wert gegenüber der letzten Ausgabe geändert hat. Das hält die Buslast niedrig.
* **Bei jedem Abruf** — Es wird nach jeder Neuberechnung gesendet, auch bei unverändertem Wert. Sinnvoll für Empfänger, die eine regelmäßige Lebendmeldung erwarten.

Das Leseflag ist bei allen Wert-Objekten gesetzt, eine Visualisierung kann den Wert also jederzeit aktiv anfordern.

