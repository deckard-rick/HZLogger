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
 * 
 * Allgemeine Hinweise: https://www.mikrocontroller-elektronik.de/nodemcu-esp8266-tutorial-wlan-board-arduino-ide/
 * Hinweis: Die Libraries stehen hier: C:\Users\tengicki\Documents\Arduino\libraries und seit dem 23.09.2019 unter git
 *          Ursprung DS18B20 Ansteuerung vom beelogger, http://beelogger.de 
 * 
 * (C) Andreas Tengicki 2018,2019
*/

#include <tgDevice.hpp>
#include <OneWire.h>
#include <DallasTemperature.h> 
#include <DHTesp.h>

//Alle 13 GPIO Pins von D0 bis D10, SD3, SD2
//int dataPins[13] = {16, 5, 4, 0, 2, 14, 12, 13, 15, 3, 1, 10, 9};
//Auf der 0 will das Thermometer nicht wir nehmen D1 fü den OneWire Bus

#define ONE_WRIE_BUS 5

//Ist D7 ist RXD2 und mit D8 TXD2 Teil der seriellen Schnittstelle, solange wir sicher so debuggen wollen
//  können wir D7 und D8 nicht mitbenutzen
//Der Zugriff auf GPIO10/SD3 und GPIO9/SD2 führte zum permanenten Reset/Neustarts des NodeMCU

#define deviceVersion "tgHZ05"

#ifndef NOVAL
#define NOVAL -9999
#endif

const int dhtPIN = 4;     // what digital pin the DHT22 is conected to
#define dhtTYPE DHTesp::DHT22   // there are multiple kinds of DHT sensors

class TDS18B20Sensor : public TtgSensor
{
  public:
    TDS18B20Sensor(DallasTemperature *t_ds18b20, DeviceAddress t_address, const String& aId, float *apDelta):TtgSensor(aId,apDelta) 
                  {ds18b20=t_ds18b20; 
                   for (int i=0; i<8; ++i) address[i] = t_address[i];};
  protected:
    float doGetMessValue();
  private:
    DallasTemperature *ds18b20;
    DeviceAddress address  ;
};

class TDHT22SensorTemp : public TtgSensor
{
  public:
    TDHT22SensorTemp(DHTesp *aDht, const String& aId, float *apDelta):TtgSensor(aId,apDelta) {dht=aDht;};
  protected:
    float doGetMessValue();
  private:
    DHTesp *dht;  
};

class TDHT22SensorHum : public TtgSensor
{
  public:
    TDHT22SensorHum(DHTesp *aDht, const String& aId, float *apDelta):TtgSensor(aId,apDelta) {dht=aDht;};
  protected:
    float doGetMessValue();
  private:
    DHTesp *dht;  
};

OneWire oneWire(ONE_WRIE_BUS);
DallasTemperature ds18b20(&oneWire);
DHTesp dht;

class THZLoggerDevice : public TGDevice
{
  public:
    THZLoggerDevice(const String& aDeviceVersion):TGDevice(aDeviceVersion)
      { initChar(deviceID,16,"HZLOGzz");
        initChar(wifiSSID,16,"BUISNESSZUM");
        initChar(wifiPWD ,32,"FE1996#ag!2008");
        initChar(host    ,32, "");
      }  
        
  protected:
    void doHello();
    void doRegister();
    void doSetup();
  private:
    float messDeltaTemp = 2;
    float messDeltaHum = 5;
    String adrToId(DeviceAddress devAdr);
    void initChar(char* t_field, const int t_maxlen, const String& value);
    void registerTempSensors();
};

float TDS18B20Sensor::doGetMessValue()
{
  ds18b20->requestTemperaturesByAddress(address);
  return ds18b20->getTempC(address);
}

float TDHT22SensorTemp::doGetMessValue()
{
  return dht->getTemperature();
}

float TDHT22SensorHum::doGetMessValue()
{
  return dht->getHumidity();
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
  writelog("");
  writelog("HZLogger");
  writelog("");
  
  TGDevice::doHello();
}

String THZLoggerDevice::adrToId(DeviceAddress devAdr)
{
  String erg = "Ox";
  char hex[2];
  
  for (int i=0; i<8; i++)
    {
      sprintf(hex,"%2X",devAdr[i]);
      erg += String(hex);
    }
  for (int i=0; i<erg.length(); ++i)
    {
      if (erg[i]==' ') 
        {
          erg[i] = '0';  
        }
    }  
  return erg;
}

void THZLoggerDevice::registerTempSensors() 
{
  writelog("register Temperature Sensors - Start");

  writelog("init DS18B20");
  //oneWire = new OneWire(oneWirePin);
  //ds18b20 = new DallasTemperature (oneWire);
  ds18b20.begin();
  delay(500);

  /* Lesen der aktuellen Anzahl */
  writelog("read DS18B20 Count");
  int cntDev = ds18b20.getDeviceCount();
  writelog("cntDev:"+String(cntDev));

  /* Die Devices als Sensoren einfach einsortieren, sie werden immer in 
   *  der glechen Reihenfolge geliefert und ändern sich ja selten.
   */
  for(int i=0; i < cntDev; i++)
    {
      DeviceAddress devAdr; //Die DeviceAddress ist ein uint[8]
      if( ds18b20.getAddress(devAdr, i))
        {
          String id = adrToId(devAdr);
          //sensors->add(new TDS18B20Sensor(&ds18b20,devAdr,id,&messDeltaTemp));

          writelog("id: "+id,true);
        }
    }
  writelog("register Temperature Sensors - End");
}


void THZLoggerDevice::doRegister()
{
  //setTimerActive();  //noTimer necessary

  //In first version we have a dynamic process fo alocatig the temperature sensors. But this is not necessary
  //the HZLogger is working fix a fixed numer of wired temperature sensores, Ich we change thies sensors we
  //can reboot the HZLooger without any Problem.
  
  registerSensors(new TtgSensorsList());
  
  registerTempSensors();

  writelog("init DHT22");
  dht.setup(dhtPIN, dhtTYPE);  

  writelog("add DHT22 sensors");
  //sensors->add(new TDHT22SensorTemp(&dht,"DHT22-TEMP",&messDeltaTemp));
  //sensors->add(new TDHT22SensorHum(&dht,"DHT22-HUM",&messDeltaHum));

  TGDevice::doRegister();
    
  deviceconfig->addConfig("messDeltaTemp","F",0,false,"Delta ab der eine Temperatur reported wird",NULL,NULL,&messDeltaTemp);   
  deviceconfig->addConfig("messDeltaHum","F",0,false,"Delta ab der eine Luftfeuchtigkeit reported wird",NULL,NULL,&messDeltaHum);
}

void THZLoggerDevice::doSetup()
{
  //writelog("THZLoggerDevice::doSetup()");
  TGDevice::doSetup();
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
