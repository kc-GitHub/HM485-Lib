Homematic Wired Relaismodul 8fach
==============================================

Das Modul HBW-LC-Sw8 schaltet bis zu 8 angeschlossene Relais. Als Relais können z.B. solche Module verwendet werden:
http://www.amazon.de/Kanal-Relay-Relais-Module-Arduino/dp/B00AEIDWXK

Anstelle eines Arduino mit RS485-Interface kann auch der Universalsensor verwendet werden (SW ist pinkompatibel):
http://www.fhemwiki.de/wiki/Universalsensor


Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-LC-Sw8.pm Datei in den Ordner \FHEM\lib\HM485\Devices kopiert werden (Das Device gibt sich als HW-Typ 0x83 aus).

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
4 - Status LED
5 - Rx Debug
6 - Tx Debug
8 - Bedientaster (Reset)
A0 - Relais 1
A1 - Relais 2
A2 - Relais 3
A3 - Relais 4
A4 - Relais 5
A5 - Relais 6
3  - Relais 7
7  - Relais 8
