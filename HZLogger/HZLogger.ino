/*
 * 12.06.2018 Anpassung zur Prüfung an welchen Ports wir eine Signal haben.
 *            dazu braucht es das .begin, sonst wird der getDeviceCount()
 *            nicht aktualisiert
 *
 * 14.06.2018 Messen von Mittelwerten, und Zeitsteuerung für nicht so häufige Aktionen
 * 15.06.2018 Einbau der Gültigkeit der Werte
 * 17.06.2018 Einbei der WiFi Verbindung und des WiFi-Servers mit Main und Values
 * 18.06.2018 Abfrage per JSON, Struktur für Konfig-Werte
 * 28.06.2018 Form erweitert um <br/>
 * 11.10.2018 Form erweitern um Kommentare
 * 13.10.2018 Entgegennahme der Werte aus der Form
 * 13.10.2018 Speichern mi EEPROM (also Festschreibung auch nach Neustart)
 * 11.11.2018 Senden per Values JSON an Server (ungetestet)
 * 11.11.2018 Kalibrierung eines Messondenpärchens (bringt aber nicht viel)
 * 21.04.2019 Test des Werte sendens an den Datenserver, tut d.h. Produktionsumgebung kann aufgebaut werden.
 * 23.09.2019 Jetzt unter C:\DatenAndreas\tgsIOT\Arduino\HZLogger und unter git, noch ohne Remote Origin.
 * 30.09.2019 Step1: Umstellung auf OneWire als Bus über die Adressen der TempSensoren, warum bin ich nicht früher drauf gekommen. (aus Japan, im Shinkansen)
 *            Versuche in der ersten Version, hatten ergeben, dass das mit den Korrekturwerten nichts bringt, daher kommt das alles raus
 * 02.10.2019 Step2: DevAddress ist ein uint[8] Feld, da müssen wir eine HexID-wie eine MacAdresse draus machen,
 *            zudem müssen wir mit records/struct arbeiten und nicht mit Index, die Teile können sich ja verschieben oder ersetzen, dann finden wir die
 *            anderen Informationen nicht mehr
 * 03.10.2019 Verwaltung der Anschlüsse, wir ändern so ein wenig die Philosphie, der Sensor kann sich komplett selbst verwalten
 *            um das EPROM nicht über zubelasten, wird aber nur bei Änderungen geschrieben
 *            Zudem Umstelung der Messung auf messtime und reporttime je Sensor, damit er nicht immer alles meldet.
 * 04.10.2019 Mittlererweile ist die Software eigentlich einsatzfähig, ich habe im Urlaub aber Zeit und damit sie als Grundlage für weitere Schaltungen dienen kann,
 *            kann es natürlich noch verbessert werden.
 *            Idee ist, alle Konfigwerte über eine Datenstruktur und je eine Lese/Schreib Routine abzuwicklen, (oder ohne), dann kann der Konfig-Teil immer gleich bleiben.
 * 05.10.2019 Datenbasiertes Konfig Handling abgeschlossen, ein paar Verbesserungen am HTML, jetzt ist es gut, es muss nur nochmal getestet werden (wieder daheim dann)
 * 07.10.2019 putConfig mit Ausnahme des Auslesens des POST Bodies mit dem json aus der http Anfrage
 * 09.10.2019 für den DHT22 brauchen wir auch noch Anschluss Definitionen, sonst beist es sich mit anderen Geräten
 * 20.10.2019 parallel mit dem tgDevice begonnen, will aber im ersten Schritt den HZLogger normal zum laufen bringen. Bei der Beschäftigung mit GARDENMain hat sich aber
 *            ergeben, das es genügt mit ID und DeviceID zu arbeiten, die ganze Anschuss Geschichte kann eigentlich wieder raus.
 * 21.10.2019 Mit PullUp Wiederstand 4,7kOhm kann er nur zwei Temp Sensoren betreiben. Mit 700Ohm geht es bis vier Sensoren, der 5te will nicht, ich vermute da kommen
 *            zuwenig mA aus der Stromversorgung über den NodeMCU, d.h. wir sollten es mit der externen Stromversorgung versuchen.
 * 21.10.2019 DHT22 getestet, funktioniert, wenn mann ihn richtig anschließt
 * 21.10.2019 Config Modul geändert, man darf kein sizeof verwenden, für Array Größen und auf == aufpassen, vom Prinzip her tut es aber!
 * 23.10.2019 Umbau des HZLoggers auf das tgDeviceModul
 * 30.10.2019 ging eigentlich gut, Device stürzte aber immer schnell wieder ab, vermutlich Speicherprobleme mit Strings, tgDevice wird umgebaut auf char* (tgOutBuffer)
 * 24.11.2019 Umbau auf TGCharbuffer war mühsamm, weil es viele Stellen betrifft, aber erfolgreich
 *            Sensor schmierte aber nach x Sekunden messen immer wieder ab => YIELD() also genauer yield() notwendig, damit nicht die Watchdog des ESP automatisch einen Restart durchführt
 *            Dashboard mit Values und json.Values tut, bin nun am testen des Config-Formulars
 * 29.11.2019 weiteres Bug-Fixing. Aber nun funktionieren html und json senden
 * 01.12.2019 configStart aus dem Beispiel zum EEPROM ist Blödsinn, weil dann der Buffer zu klein ist, nun tuts (endlich).
 * 24.12.2019 WiFi mit TimeOut und startet ggf AcceesPoint
 * 24.12.2019 Webseite kann Konfiguration via json holen, Device sendet deviceid nicht mehr doppelt.
 * 25.12.2019 auch das putconfig von der Webseite tut, Version der Config und DeviceID müssen passen, wifi-Parameter werden nicht verändert.
 * => Damit ist die erste Fassung tatsächlich vollständig implementiert.
 * 31.12.2019 Jetzt tut auch die Platine, musste noch einige Lötfehler beseitigen, hier noch ein paar kleine Verbesserungen, bzw in der Library
 * 04.01.2020 im Rahmen der Dokumentation noch ein wenig geglättet
 *
 *
 * Allgemeine Hinweise: https://www.mikrocontroller-elektronik.de/nodemcu-esp8266-tutorial-wlan-board-arduino-ide/
 * Hinweis: Die Libraries stehen hier: C:\Users\tengicki\Documents\Arduino\libraries und seit dem 23.09.2019 unter git
 *          Ursprung DS18B20 Ansteuerung vom beelogger, http://beelogger.de
 *
 * (C) Andreas Tengicki 2018,2019
*/

#include <tgDevice.hpp>
#include <tgLogging.hpp>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHTesp.h>

//Alle 13 GPIO Pins von D0 bis D10, SD3, SD2
//int dataPins[13] = {16, 5, 4, 0, 2, 14, 12, 13, 15, 3, 1, 10, 9};
//Auf der 0 will das Thermometer nicht wir nehmen D1 fü den OneWire Bus

#define ONE_WRIE_BUS 4

//Ist D7 ist RXD2 und mit D8 TXD2 Teil der seriellen Schnittstelle, solange wir sicher so debuggen wollen
//  können wir D7 und D8 nicht mitbenutzen
//Der Zugriff auf GPIO10/SD3 und GPIO9/SD2 führte zum permanenten Reset/Neustarts des NodeMCU

#define deviceVersion "tgHZ05"

#ifndef NOVAL
#define NOVAL -9999
#endif

const int dhtPIN = 5;     // what digital pin the DHT22 is conected to
#define dhtTYPE DHTesp::DHT22   // there are multiple kinds of DHT sensors

class TDS18B20Sensor : public TGSensor
{
  public:
    TDS18B20Sensor(DallasTemperature *t_ds18b20, DeviceAddress t_address, const char* aId, float *apDelta):TGSensor(aId,apDelta)
                  {ds18b20=t_ds18b20;
                   for (int i=0; i<8; ++i) address[i] = t_address[i];};
    //void initSensor(DallasTemperature *t_ds18b20, DeviceAddress t_address, const char* aId, float *apDelta);
  protected:
    void dogetvalue();
  private:
    DallasTemperature *ds18b20;
    DeviceAddress address  ;
};

class TDHT22SensorTemp : public TGSensor
{
  public:
    TDHT22SensorTemp(DHTesp *aDht, const char* aId, float *apDelta):TGSensor(aId,apDelta) {dht=aDht;};
  protected:
    void dogetvalue();
  private:
    DHTesp *dht;
};

class TDHT22SensorHum : public TGSensor
{
  public:
    TDHT22SensorHum(DHTesp *aDht, char* aId, float *apDelta):TGSensor(aId,apDelta) {dht=aDht;};
  protected:
    void dogetvalue();
  private:
    DHTesp *dht;
};

OneWire oneWire(ONE_WRIE_BUS);
DallasTemperature ds18b20(&oneWire);
DHTesp dht;

//TDS18B20Sensor db18b20Sensors[10];

class THZLoggerDevice : public TGDevice
{
  public:
    THZLoggerDevice(const char* aDeviceVersion):TGDevice(aDeviceVersion)
      { initChar(deviceid,16,"HZLOGzz");
        initChar(wifiSSID,16,"YOURSSID");
        initChar(wifiPWD ,32,"YOURWIFIPASSWORD");
        initChar(host    ,32, "");
      }

  protected:
    void doHello();
    void doRegister();
  private:
    float messDeltaTemp = 2;
    float messDeltaHum = 5;
    void adrToId(DeviceAddress devAdr, char* id);
    void initChar(char* t_field, const int t_maxlen, const String& value);
    void registerTempSensors();
};

/*
void TDS18B20Sensor::initSensor(DallasTemperature *t_ds18b20, DeviceAddress t_address, const char* aId, float *apDelta)
{
  ds18b20=t_ds18b20;
  for (int i=0; i<8; ++i) address[i] = t_address[i];

  TtgSensor::initSensor(aId,apDelta);
}
*/

void TDS18B20Sensor::dogetvalue()
{
  //TGLogging::get()->write("TDS18B20Sensor::doGetMessValue A")->crlf();
  ds18b20->requestTemperaturesByAddress(address);
  //TGLogging::get()->write("TDS18B20Sensor::doGetMessValueB")->crlf();
  newValue = ds18b20->getTempC(address);
  //TGLogging::get()->write("TDS18B20Sensor::doGetMessValue C")->crlf();
}

void TDHT22SensorTemp::dogetvalue()
{
  //TGLogging::get()->write("TDHT22SensorTemp::doGetMessValue")->crlf();
  newValue = dht->getTemperature();
}

void TDHT22SensorHum::dogetvalue()
{
  //TGLogging::get()->write("TDHT22SensorHum::doGetMessValue")->crlf();
  newValue = dht->getHumidity();
}

void THZLoggerDevice::initChar(char* t_field, const int t_maxlen, const String& value)
{
  int len = value.length();
  if (len >= t_maxlen) len = t_maxlen - 1;
  for(int i=0; i<len; ++i) t_field[i] = value[i];
  for(int i=len; i<t_maxlen; ++i) t_field[i] = '\0';
}

void THZLoggerDevice::doHello()
{
  TGLogging::get()->write("***HZLogger***")->crlf();
  TGDevice::doHello();
}

void THZLoggerDevice::adrToId(DeviceAddress devAdr, char* id)
{
  strcpy(id,"Ox");
  char hex[2];

  for (int i=0; i<8; i++)
    sprintf(id+(i*2),"%2X",devAdr[i]);
  for (int i=0; i<strlen(id); ++i)
    if (id[i]==' ')
      {
        id[i] = '0';
      }
}

void THZLoggerDevice::registerTempSensors()
{
  TGLogging::get()->write("init DS18B20")->crlf();
  ds18b20.begin();
  delay(500);

  /* Lesen der aktuellen Anzahl */
  int cntDev = ds18b20.getDeviceCount();
  TGLogging::get()->write("DS18B20 Count:")->write(cntDev)->crlf();

  /* Die Devices als Sensoren einfach einsortieren, sie werden immer in
   *  der glechen Reihenfolge geliefert und ändern sich ja selten.
   */
  for(int i=0; i < cntDev; i++)
    {
      DeviceAddress devAdr; //Die DeviceAddress ist ein uint[8]
      char id[20]; id[0]='\0';
      if( ds18b20.getAddress(devAdr, i))
        {
          adrToId(devAdr,id);
          sensors->add(new TDS18B20Sensor(&ds18b20,devAdr,id,&messDeltaTemp));
          //db18b20Sensors[i].initSensor(&ds18b20,devAdr,id,&messDeltaTemp);
          //sensors->add(&(db18b20Sensors[i]));

          TGLogging::get()->write("id: ")->write(id)->crlf();
        }
    }
}

void THZLoggerDevice::doRegister()
{
  //setTimerActive();  //noTimer necessary

  //In first version we have a dynamic process fo alocatig the temperature sensors. But this is not necessary
  //the HZLogger is working fix a fixed numer of wired temperature sensores, Ich we change thies sensors we
  //can reboot the HZLooger without any Problem.

  registerSensorsList(new TGSensorsList());

  registerTempSensors();

  TGLogging::get()->write("init DHT22")->crlf();
  dht.setup(dhtPIN, dhtTYPE);
  sensors->add(new TDHT22SensorTemp(&dht,"DHT22-TEMP",&messDeltaTemp));
  sensors->add(new TDHT22SensorHum(&dht,"DHT22-HUM",&messDeltaHum));

  TGDevice::doRegister();

  deviceconfig->addConfig("messDeltaTemp",'F',0,false,"Delta ab der eine Temperatur reported wird",NULL,NULL,&messDeltaTemp);
  deviceconfig->addConfig("messDeltaHum",'F',0,false,"Delta ab der eine Luftfeuchtigkeit reported wird",NULL,NULL,&messDeltaHum);
}

THZLoggerDevice device(deviceVersion);

void setup(void)
{
  device.deviceSetup();
}

void loop(void)
{
  device.deviceLoop();
}
