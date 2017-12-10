# SBAquaControl
Schullebernd Aqua Control ist eine WLAN Aquarium Lichtsteuerung (Tageslichtsimulation) für LED Beleuchtungen.

Eine komplette Beschreibung zum Aufbau der Steuerung sowie zum Selbstbau einer Aquarium LED Beleuchtung ist auf http://schullebernd.de/ zu finden.

Um das Projekt zu verwenden, lade dir das git Archiv herunter und entpacke es in den library Ordner deiner Arduino IDE (in der Regel _\[User]\Dokumente\Arduino\libraries\_)

Der Verdrahtungsplan liegt als Fritzing Projekt im Ordner ./extras/Circuit.

# ACHTUNG
Bei Verwendung der **Arduino IDE** bitte mit dem Boardmanager die **ESP8266 Version 2.2.0** installieren. Bei Verwendung der Version 2.3.0 stürtzt der Controller ab. Ich versuche bereits das Problem zu identifizieren und zu beheben.
Bei Verwendung von **Visual Micro** (Arduino IDE für Microsoft Visual Studio) bitte die **ESP8266 Version 2.3.0** installieren. Bei Verwendung der Version 2.2.0 stürtzt der Controller ab.

ESP8266 Version | Arduino IDE | Visual Micro
----------------|-------------|-------------
2.2.0 | Ok | funktioniert nicht
2.3.0 | funktioniert nicht | Ok

Das Projekt befindet sich zur Zeit im Aufbau. Eine detailierte Beschreibung hier auf Github erfolgt in Kürze.

# Änderungen

## 2017-12-10 Build 0.5.001
- Testmodus hinzugefügt: In der Weboberläche gibt es einen Testmodus. Man kann für jeden Kanal einen Wert eingeben und auf "Testmodus aktivieren" klicken. Die Steuerung stellt die Lichtkanäle anschließend auf diese Werte ein. Der Testmodus kann über die Schaltfläche "Testmodus deaktivieren" verlassen werden. Der Testmodus bleibt maximal 60 Sekunden lang aktiviert.
