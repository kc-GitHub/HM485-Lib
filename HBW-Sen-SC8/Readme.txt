Homematic Wired Tastermodul 8fach
==============================================

Das Modul HBW-Sen-SC8 liest 8 Tastereingänge ein. Neben den normalen HM485-Events "kurzer Tastendruck" und "langer Tastendruck" werden auch Events bei "Doppelklick" und beim Loslassen einer lang gedrückten Taste gesendet.

Für die Taster können die internen Pullups eingeschaltet werden, so dass die jeweiligen Eingänge gegen Masse geschaltet werden können. Alternativ können die Pullups ausgeschaltet werden, um Logik-Level-Signale einzulesen (z.B. von Bewegungssensoren, Touchsensoren, etc.). Konfiguriert werden können diese Einstellungen über das Webfrontend. Hier können die Tastereingänge auch invertiert werden, falls bei den Tastern z.B. Öffner anstelle von Schließern verwendet werden, oder - wie bereits beschrieben - die internen Pullups aktiviert sind und die Eingänge gegen Masse geschaltet werden.

Anstelle eines Arduino mit RS485-Interface kann auch der Universalsensor verwendet werden (SW ist pinkompatibel):
http://www.fhemwiki.de/wiki/Universalsensor



Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-Sen-SC8.pm Datei in den Ordner \FHEM\lib\HM485\Devices kopiert werden (Das Device gibt sich als HW-Typ 0x86 aus).

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
4 - Status LED
5 - Rx Debug
6 - Tx Debug
8 - Bedientaster (Reset)
A0 - Tastereingang 1
A1 - Tastereingang 2
A2 - Tastereingang 3
A3 - Tastereingang 4
A4 - Tastereingang 5
A5 - Tastereingang 6
3  - Tastereingang 7
7  - Tastereingang 8
