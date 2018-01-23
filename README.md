# Beispiel einer Umsetzung eines HM-[1-Kanal Senders](https://github.com/pa-pa/AskSinPP/tree/master/examples/HM-RC-P1) mittels Arduino Pro mini, CC1101 und [AskSinPP Library](https://github.com/pa-pa/AskSinPP).

## Hardwaresektion
### Bauteile
- 1x Arduino Pro Mini (3.3V 8MHz) (ca. 2,20 EUR bei [eBay](https://www.ebay.de/itm/272812248667))
- 1x CC1101 Funkmodul 868MHz (ca. 2,60 EUR bei [eBay](https://www.ebay.de/itm/272455136087))
- 1x Batteriehalter 2x AA (ca. 2,30 EUR bei [eBay](http://www.ebay.de/itm/200988520956)) 
- 1x FTDI Adapter (falls nicht schon vorhanden, gibts bei [Amazon](https://www.amazon.de/FT232RL-FTDI-USB-auf-TTL-Serienadapter-Arduino/dp/B00HSXDGOE))
- 2x Taster (beliebig, z.B. [Kurzhubtaster](https://www.ebay.de/itm/112592788992))
- 2x AA Batterien

### Verdrahtung
_allgemeingültige Verdrahtung (Anschluss CC1101 an Arduino Pro Mini) auch für andere AskSinPP-Projekte_
![wiring](Images/wiring_arduino_cc1101.png)

**Der Anschluss des Sende-Tasters**
- **erfolgt an A0 / GND!**

**Der Anschluss der Batterien**
- **+ an VCC**
- **- an GND**

**_Mithilfe von Kupferlackdraht kann das Funkmodul platzspared "huckepack" am Arduino Pro Mini angeschlossen werden:_**
![wiring2](Images/arduino_bottom.jpg)

Unterseite:
![wiring3](Images/arduino_top.jpg)

### Anschluss des FTDI Adapters zum Flashen
- Um die Schaltung sehr kompakt zu halten, verzichte ich auf das Anlöten eines Steckverbinders. Es reicht aus, wenn man die Pins des FTDI Adapters durch die Lötbohrungen steckt. Während des Flash-Vorgangs verkantet man beide Platinen leicht, damit die Kontaktierung gegeben ist.

![flash](Images/ftdi_flash.jpg)

## Softwareabteilung
### Arduino IDE
- Arduino IDE [herunterladen](https://www.arduino.cc/en/Main/Software) und installieren
- AskSinPP Bibliothek als [ZIP herunterladen](https://github.com/pa-pa/AskSinPP/archive/master.zip) 
- notwendige Bibliotheken in der Arduino IDE hinzufügen:
  - Sketch -> Bibliothek einbinden -> .ZIP-Bibliothek hinzufügen
    - heruntergeladene AskSinPP Bibliothek ZIP-Datei auswählen
  - Sketch -> Bibliothek einbinden -> Bibliotheken verwalten
    - im Suchfeld folgende Bibliotheken suchen und installieren:
      - TimerOne
      - EnableInterrupt
      - Low-Power
 - Board einstellen:
   - Board: `Arduino Pro or Pro Mini`
   - Prozessor: `ATmega328P (3.3V, 8 MHz)`
   - Port: `COM-Port` des FTDI Adapters <br>
   
 - Sketch öffnen:
   - Datei 
     - Beispiele 
       - AskSinPP 
         - HM-RC-P1
         
   - Anpassungen:
     - gerade wenn man mehrere Geräte des selben Typs verwenden will, muss man für jedes Gerät eine eigene Seriennummer vergeben! Es sind dabei sowohl die `Device ID` (Hexadezimal) als auch die `Device Serial` (Freitext) zu ändern!
     
     
     <img src="https://github.com/jp112sdl/Beispiel_AskSinPP/blob/master/Images/sketch_serial.png" width=400>
         
- Sketch hochladen:
  - Sketch
    - Hochladen

### Anlernen an HomeMatic
- Geräte anlernen -> HM-Gerät anlernen klicken
- **Config-Taster** am Arduino Pro Mini **kurz** drücken
- das neue Gerät erscheint anschließend im Posteingang
<img src="https://github.com/jp112sdl/Beispiel_AskSinPP/blob/master/Images/hm_anlernen1.png" width=400>

- Einstellungen -> Geräte -> "+" anklicken, anschließend den Kanal anklicken
  - "gesicherte" Übertragung deaktivieren
<img src="https://github.com/jp112sdl/Beispiel_AskSinPP/blob/master/Images/hm_config1.png" width=400>

**AES Verschlüsselung für die gesicherte Übertragung ist prinzipiell auch möglich, ist aber etwas kompliziert zu beschreiben (auch wenn die Sache selbst relativ unkompliziert ist**
