Homematic Wired S0-Interface
==============================================

Das Modul HBW-Sen-EP liest bis zu 8 angeschlossene S0-Signale ein und sendet die aktuellen Zählerstände an die Zentrale.
Über das S0-Interface können bspw. Stromzähler, Gaszähler, Wärmemengenzähler, Wasserzähler, etc. eingelesen werden.
Anstelle eines Arduino mit RS485-Interface kann auch der Universalsensor verwendet werden (SW ist pinkompatibel):
http://www.fhemwiki.de/wiki/Universalsensor


Damit FHEM das Homebrew-Device richtig erkennt, muss die Datei hbw_sen_ep.xml in den Ordner \FHEM\lib\HM485\Devices\xml kopiert werden (Das Device gibt sich als HW-Typ 0x84 aus).
Config der Sendehäufigkeit kann über das FHEM-Webfrontend vorgenommen werden. 

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
4 - Status LED
5 - Rx Debug
6 - Tx Debug
8 - Bedientaster (Reset)
A0 - Input des 1. S0-Signals
A1 - Input des 2. S0-Signals
A2 - Input des 3. S0-Signals
A3 - Input des 4. S0-Signals
A4 - Input des 5. S0-Signals
A5 - Input des 6. S0-Signals
3  - Input des 7. S0-Signals
7  - Input des 8. S0-Signals


Das Device gibt nur die Anzahl der Impulse weiter - eine Umrechnung in die entsprechende Einheit muss in FHEM konfiguriert werden.
Beispielkonfiguration:

define EnergieMessung HM485 AAAABBBB
attr EnergieMessung firmwareVersion 3.06
attr EnergieMessung model HBW_Sen_EP
attr EnergieMessung room HM485
attr EnergieMessung serialNr HBW3315899
define FileLog_EnergieMessung FileLog ./log/EnergieMessung-%Y.log EnergieMessung*|EnergieMessung_07:energy:.*
attr FileLog_EnergieMessung logtype text
attr FileLog_EnergieMessung room HM485

define EnergieMessung_01 HM485 AAAABBBB_01
attr EnergieMessung_01 firmwareVersion 3.06
attr EnergieMessung_01 model HBW_Sen_EP
attr EnergieMessung_01 room HM485
attr EnergieMessung_01 serialNr HBW3315899
attr EnergieMessung_01 stateFormat energy
attr EnergieMessung_01 subType counter
attr EnergieMessung_01 userReadings energy monotonic {ReadingsVal("EnergieMessung_07","counter",0)/100.0;;;;}, power differential {ReadingsVal("EnergieMessung_07","counter",0);;;;}
