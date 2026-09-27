# (upcoming) v0.6.0

* Change: Kanalauswahl nach OpenKNX-Standard – eigener Tab "Kanalauswahl" mit einer Zeile je Kanal (Kanal / Wetterdienst / Beschreibung)
* Change: Der Schieberegler "Verfügbare Kanäle" und der Tab "(mehr)" entfallen; ein Kanal wird über "Deaktiviert" beim Wetterdienst abgeschaltet
* Change: Deaktivierte Kanäle erscheinen nicht mehr in der Baumansicht; die Beschreibung bleibt trotzdem eingebbar
* Change: "Bezeichnung" heißt jetzt durchgängig "Beschreibung"
* Breaking: Der Kanalzähler entfällt – bestehende Projekte müssen die Kanäle neu aktivieren


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

