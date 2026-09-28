### Wetter-Kanal

Ein Kanal liefert Wetterdaten für **einen Ort**. Er stellt drei frei belegbare Wert-Slots bereit (Wetter A, B und C) sowie ein Diagnose-Objekt mit dem HTTP-Statuscode des letzten Abrufs.

Der Abruf erfolgt bei Open-Meteo. Die Zeitzone wird automatisch aus den Koordinaten des Ortes bestimmt — Tagesgrenzen und Uhrzeiten beziehen sich damit immer auf die Ortszeit des Wetterstandorts, nicht auf die Gerätezeitzone. Das ist bei Orten in einer anderen Zeitzone als der des Geräts das fachlich richtige Verhalten.

Das Modul fragt nur die Variablen und nur den Zeitraum ab, die von den drei Slots tatsächlich gebraucht werden. Ein Kanal mit wenigen Slots erzeugt daher sehr kleine Anfragen.

