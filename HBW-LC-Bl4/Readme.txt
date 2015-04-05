Homematic Wired Client zur Raffstore-Steuerung
==============================================

Die SW verwandelt einen Arduino und acht Relais in einen 4-Kanal-Rolladenaktor.
Anstelle eines Arduino mit RS485-Interface kann auch der Universalsensor verwendet werden (SW ist pinkompatibel):
http://www.fhemwiki.de/wiki/Universalsensor
Als Relais können bspw. diese verwendet werden:
http://www.amazon.de/Kanal-Relay-Relais-Module-Arduino/dp/B00AEIDWXK/ref=sr_1_3?ie=UTF8&qid=1412971707&sr=8-3
Das Modul verhält sich wie das HMW-LC-Bl1 mit 4 Kanälen.

Aktuell implementierte Befehle:
0x00-0xC8 = 0-100%
0xC9 = Stop
0xFF = Toggle

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-LC-Bl4.pm Datei in den Ordner \FHEM\lib\HM485\Devices kopiert werden (Das Device gibt sich als HW-Typ 0x82 aus).
Config der Fahrzeiten (hoch/runter/Richtungswechsel der Lamellen) kann über das FHEM-Webfrontend vorgenommen werden.

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
4 - Status LED
5 - Rx Debug
6 - Tx Debug
8 - Bedientaster (Reset)
A0 - Ansteuerung des 1. Relais (Kanal 1 ein/aus)
A1 - Ansteuerung des 2. Relais (Kanal 1 Richtung)
A2 - Ansteuerung des 3. Relais (Kanal 2 ein/aus)
A3 - Ansteuerung des 4. Relais (Kanal 2 Richtung)
A4 - Ansteuerung des 5. Relais (Kanal 3 ein/aus)
A5 - Ansteuerung des 6. Relais (Kanal 3 Richtung)
3  - Ansteuerung des 7. Relais (Kanal 4 ein/aus)
7  - Ansteuerung des 8. Relais (Kanal 4 Richtung)


Beispielconfig:
Um ein Raffstore mit nur einer Taste zu steuern, kann z.B. das folgende define verwendet werden:

define blindToggle notify HMW_LC_Bl1_DR_HBW0140913_01 { fhem "set HM485_LAN RAW 01234567 98 00000001 7802FF" }

Hierbei steht "HMW_LC_Bl1_DR_HBW0140913" für den Client, an den der Taster angeschlossen ist und "_01" für den Kanal des Tasters.
01234567 ist die Geräteadresse des Moduls für die Raffstore-Steuerung
Die "02" in "7802FF" gibt an, dass der 2. Raffstore-Kanal gesteuert werden soll.