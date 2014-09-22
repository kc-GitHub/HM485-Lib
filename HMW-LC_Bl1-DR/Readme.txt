Homematic Wired Client zur Raffstore-Steuerung
==============================================

Verwandelt einen Arduino und zwei Relais in ein HMW_LC_BL1-Modul. Als Relais können bspw. diese verwendet werden:
http://www.exp-tech.de/Shields/5V-2-Channel-Relay-Shield.html

Aktuell implementierte Befehle:
0x00-0xC8 = 0-100%
0xC9 = Stop
0xFF = Toggle

Config der Fahrzeiten (hoch/runter/Richtungswechsel der Lamellen) kann über das FHEM-Webfrontend vorgenommen werden. Werte werden im EEPROM gespeichert.

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
4 - Status LED
5 - Rx Debug
6 - Tx Debug
8 - Bedientaster (Reset)
9 - Taster 1 (gegen Masse geschaltet)
10 - Taster 2 (gegen Masse geschaltet)
11 - Ansteuerung des 1. Relais (ein/aus)
12 - Ansteuerung des 2. Relais (Richtung)

Achtung: Die Taster und die Rollo-Ansteuerung sind intern noch nicht gepeert, d.h. die Reaktion auf einen Tastendruck muss in der FHEM-Zentrale noch konfiguriert werden. Z.B. mit: define blindToggle notify HMW_LC_Bl1_DR_HBW0140913_01 { fhem "set HM485_LAN RAW 01234567 98 00000001 7802FF" }