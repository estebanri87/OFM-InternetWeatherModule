# (upcoming) v0.7.0 "Freie Wert-Slots"

***Update-Hinweis:*** Diese Version bricht mit dem bisherigen Aufbau. Das
KO-Layout ist vollständig neu, bestehende Projekte müssen die Wetter-Kanäle
neu parametrieren und die Gruppenadressen neu verknüpfen.

* Breaking: Ein Kanal steht für einen Ort und hat drei frei belegbare Wert-Slots
  statt 215 fest vorgegebener Kommunikationsobjekte. Je Slot werden Kategorie,
  Messwert samt Zeitebene, Einzelwert oder Intervall-Aggregation, Offset und
  Sendeverhalten gewählt.
* Breaking: 4 statt 215 Kommunikationsobjekte je Kanal, Kanalzahl von 5 auf 30.
* Breaking: Die Umschaltung "Prognose Heute/Morgen" entfällt; sie wird durch den
  frei wählbaren Offset je Slot abgelöst. Damit entfallen auch die
  Konsolenkommandos "s0" und "s1".
* Feature: Vier Zeitebenen je nach Verfügbarkeit - Aktuell, 15-Minuten-,
  Stunden- und Tageswerte. Offsets reichen bei Open-Meteo auch in die
  Vergangenheit.
* Feature: Die Messwert-Auswahl richtet sich nach dem gewählten Wetter-Service.
  Angeboten wird nur, was der Dienst tatsächlich liefert; die Offset-Grenzen
  entsprechen seinem Vorhersagezeitraum.
* Feature: Abgefragt werden nur die benötigten Variablen und nur der benötigte
  Zeitraum. Mehrere Slots auf derselben Größe teilen sich einen Abruf.
* Feature: Die ausgegebenen Werte werden im Minutentakt neu bestimmt, unabhängig
  vom Abrufintervall. Ein Slot mit 15-Minuten-Werten und Offset 0 zeigt damit
  immer die laufende Viertelstunde.
* Feature: Es ruft immer nur ein Kanal gleichzeitig ab, mit Mindestabstand und
  Startversatz nach dem Booten.
* Feature: Suspendierte Kanäle werden in der Baumansicht gekennzeichnet.
* Feature: Neuer Messwert-Katalog als einzige Pflegestelle
  (src/weather-catalog.json) samt Generator und Prüfskript gegen die Live-API.
* Fix: Der Open-Meteo-API-Key wurde als "appid" statt "apikey" gesendet und war
  dadurch wirkungslos.
* Fix: Die Zeitzone war auf Europe/Berlin festgelegt. Sie wird jetzt aus den
  Koordinaten des Ortes bestimmt, und der Zeitversatz der Zeitstempel wird
  korrekt herausgerechnet.
* Fix: Der Luftdruck wurde in hPa auf DPT 9.006 (Pascal) ausgegeben, also um den
  Faktor 100 daneben.
* Fix: Schneemengen liefen über DPT 7.011 und verloren die Nachkommastellen;
  sie nutzen jetzt DPT 9.026 wie die übrigen Niederschlagswerte.
* Doc: Applikationsbeschreibung neu geschrieben, die Hilfetexte werden daraus
  erzeugt.


# (upcoming) v0.6.0

* Change: Kanalauswahl nach OpenKNX-Standard – eigener Tab "Kanalauswahl" mit einer Zeile je Kanal (Kanal / Wetterdienst / Beschreibung)
* Change: Der Schieberegler "Verfügbare Kanäle" und der Tab "(mehr)" entfallen; ein Kanal wird über "Deaktiviert" beim Wetterdienst abgeschaltet
* Change: Deaktivierte Kanäle erscheinen nicht mehr in der Baumansicht; die Beschreibung bleibt trotzdem eingebbar
* Change: "Bezeichnung" heißt jetzt durchgängig "Beschreibung"
* Breaking: Der Kanalzähler entfällt – bestehende Projekte müssen die Kanäle neu aktivieren
* Fix: Ein neu angelegter Kanal ist standardmäßig "Deaktiviert" statt "Open-Meteo"


# (upcoming) v0.5.1

* Fix: Open-Meteo API hatte die komplette Basis-URL überschrieben


# (2026-02-17) v0.5.0 "Fix und KO-Benennung"

* Fix: Fehler im Speicherlayout behoben
* Feature: Gruppenobjekte umbenannt


# (2025-10-18) v0.4.2 "OpenKNX Update"

* Update: Aktualisierung zur Verwendung mit Common 1.5 und Producer 3.11.0


# (2025-07-25) v0.4.1 "Fix UVI Tageswerte"

* Fix #8: UV-Index heute und morgen war fehlerhaft (Im KO für heute wurden der Wert für morgen ausgegeben, das KO für morgen wurde nicht beschrieben)


# (2025-07-14) v0.4

***Update-Hinweis:*** Falls Open-Meteo als Wetterdienst verwendet wird, 
muss beim Update von früheren Versionen die Nutzung/Lizenz gewählt werden,
damit ein Abruf von Wetterdaten erfolgt.
 

* Erweiterung Open-Meteo
  * Unterstützung von API-Key und Server-URL
  * Dokumentation
* Änderung von Standard-Wetter-Dienst auf Open-Meteo


# Bis v0.3 (2024-08)

Versionen für interne Tests.

