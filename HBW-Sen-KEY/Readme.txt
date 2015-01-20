Homematic Wired RFID-Interface
==============================================

Das Modul HBW-Sen-KEY liest RFID-Transponder aus und schickt die ausgelesenen IDs über den RS485-Bus. Als RFID-Leser wird das folgende Modul verwendet (RC522): http://www.amazon.de/SainSmart-Mifare-Module-KeyCard-Arduino/dp/B00G6FK4DQ
Anstelle eines Arduino mit RS485-Interface kann auch der Universalsensor verwendet werden (SW ist pinkompatibel):
http://www.fhemwiki.de/wiki/Universalsensor

Zusätzlich bietet das Modul die Möglichkeit vier Taster einzulesen. Für die Taster können die internen Pullups eingeschaltet werden, so dass die jeweiligen Eingänge gegen Masse geschaltet werden können. Alternativ können die Pullups ausgeschaltet werden, um Logik-Level-Signale einzulesen (z.B. von Bewegungssensoren, Touchsensoren, etc.). Konfiguriert werden können diese Einstellungen über das Webfrontend. Hier können die Tastereingänge auch invertiert werden, falls bei den Tastern z.B. Öffner anstelle von Schließern verwendet werden, oder - wie bereits beschrieben - die internen Pullups aktiviert sind und die Eingänge gegen Masse geschaltet werden.


Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-Sen-KEY.pm Datei in den Ordner \FHEM\lib\HM485\Devices kopiert werden (Das Device gibt sich als HW-Typ 0x85 aus).

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
4 - Status LED
5 - Rx Debug
6 - Tx Debug
8 - Bedientaster (Reset)
9  - RFID-Modul Reset
10 - RFID-Modul SDA
11 - RFID-Modul MOSI
12 - RFID-Modul MISO
13 - RFID-Modul SCK
A0 - Tastereingang 1
A1 - Tastereingang 2
A2 - Tastereingang 3
A3 - Tastereingang 4


Das Device wertet keine IDs der RFID-Transponder aus, sondern verschickt diese lediglich auf dem RS485 Bus. Die Auswertung muss in der FHEM-Zentrale erfolgen.

Beispielkonfiguration (Auszug):
define HBW_Sen_KEY_HBW4447487_01 HM485 AABBFFFF_01
attr HBW_Sen_KEY_HBW4447487_01 eventMap 3548356836:KnownID_Markus 0:removed

define keyCard notify HBW_Sen_KEY_HBW4447487_01:.*rfid_key:.*KnownID.* { fhem "set RelayDoor on";; fhem "define atOff at +00:00:05 set RelayDoor off";;}