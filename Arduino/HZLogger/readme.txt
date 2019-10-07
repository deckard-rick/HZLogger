HZLogger
========

Unsere Lüftung bzw. Heizungsanlage ist 2008 gekauft, 2009 eingebaut, die Elektronik gelötet
ohne Schnittstellenzugang (zumindestens kein offizieller oder dokumentierter) und die Firma
die es gebaut hat, ist pleite bzw. gibt es nicht mehr.
Der Hersteller der Anlage Paul Wärmepumpen gibt auch keinen richtigen Support. Das 24V Netzteil
ist bereits ausgefallen und durch ein externes Ersatz, andere Fehler werden folgen.

Damit wird es unausweichlich eine vernünftige eigene Steuerung dafür einzusetzen. Dafür muss man aber
verstehen was passiert. Die Steuerung später kann man selber löten, oder einen "controllino" oder 
ähnliches einsetzen.

Erster Schritt ist zu überwachen was die Anlage tut.

Also plazieren wir 10 Temperatur Sensoren und zeichnen auf.

Gleichzeitig ist das mein erstes richtig eigenes ESP8622 Projekt, mit Löten, Arduino Programmierung
und dazugehöriger Verwaltung im Web.

gefundene Lösung:
-----------------
Bis zu 12 TempSensoren DS1820 an *einem* OneWire Bus. Dazu ein DHT22 der Temperatur und Luftfeuchtigkeit aus
dem Raum liefert. Ein Spannungwandler 24V->5V komplementiert das Board. Kommuniziert wird über das eingebaute
WLAN des ESP8622.

Probleme:
---------
* wenig Zeit
* erstmal ne Struktur finden
* Schaltung ging nicht, weil ich es mit 6 Bussen a 2 Sensoren probiert habe,
  da ich irgendwie OneWire und das es Adresses/IDs der Sensoren gibt
  nicht begriffen hatte.

7.10.2019
