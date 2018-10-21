# Umbau Shelly1 auf AskSinPP
- [Sketch](https://raw.githubusercontent.com/jp112sdl/Beispiel_AskSinPP/master/examples/HM-LC-SW1-FM_Shelly1/HM-LC-SW1-FM_Shelly1.ino)
- [Deckel](https://github.com/jp112sdl/Beispiel_AskSinPP/blob/master/examples/HM-LC-SW1-FM_Shelly1/Deckel.stl)

#### 1. Gehäuse öffnen
![1](https://raw.githubusercontent.com/jp112sdl/Beispiel_AskSinPP/master/examples/HM-LC-SW1-FM_Shelly1/1_Shelly_Open.jpeg)

#### 2. ESP entfernen
_Geht wunderbar mit einem kleinen Lötbrenner. 3-4 Sekunden auf die Mitte des ESP halten und mit einer Pinzette abheben_
![2](https://raw.githubusercontent.com/jp112sdl/Beispiel_AskSinPP/master/examples/HM-LC-SW1-FM_Shelly1/2_Shelly_RemoveESP.jpg)

#### 3. Einen Pro Mini mit CC1101 "verheiraten" und den [Sketch](https://raw.githubusercontent.com/jp112sdl/Beispiel_AskSinPP/master/examples/HM-LC-SW1-FM_Shelly1/HM-LC-SW1-FM_Shelly1.ino) flashen 
![3](https://raw.githubusercontent.com/jp112sdl/Beispiel_AskSinPP/master/examples/HM-LC-SW1-FM_Shelly1/4_ProMiniWithCC1101.jpg)

#### 4. Die Pro Mini Pins Vcc (3.3V), GND, 5 und 8 mit den markierten Lötstellen verdrahten
![4](https://raw.githubusercontent.com/jp112sdl/Beispiel_AskSinPP/master/examples/HM-LC-SW1-FM_Shelly1/5_ShellyWithoutESP.jpg)

#### 5. Eine Isolation zwischen Pro Mini und Shelly anbringen und das Ganze mit etwas Kleber fixieren
![5](https://raw.githubusercontent.com/jp112sdl/Beispiel_AskSinPP/master/examples/HM-LC-SW1-FM_Shelly1/6_ShellyAssembled.jpeg)

#### 6. [Deckel](https://github.com/jp112sdl/Beispiel_AskSinPP/blob/master/examples/HM-LC-SW1-FM_Shelly1/Deckel.stl) ausdrucken und am Shelly verkleben
![6](https://raw.githubusercontent.com/jp112sdl/Beispiel_AskSinPP/master/examples/HM-LC-SW1-FM_Shelly1/7_ShellyCase.jpg)

**Zuletzt noch anlernen -> Das Gerät erscheint als HM-LC-Sw1-FM in der HomeMatic Zentrale.
Zum Anlernen den Taster an "SW" lange (ca. 3 Sekunden) drücken.**
