/*
 * 08.1o.2098 Kopiert aus dem HZLogger als GRDNMain
 *            Steuerung von Bewässerung, Pumpe und Hasenbeleuchtung
 *            
 *  HINWEIS: Ursprünglich hatte ein Actor eine ID und eine Bezeichnung. Aber eigentlich braucht des Device die globale Bezeichnung nicht wissem.
 *           Entweder ich bin mit der Steuerungszentrale (Webseite) verbunden, da geht mehr und da wird es dann auch vernünftige Bezeichnungen
 *           geben. Oder aber ich bin auf dem Device und dann kann ich auch miz den id's arbeiten. (17.10.2019-tg)
 *  TODO
 *    
 * (C) Andreas Tengicki 2019
*/
#include <tgDevice.hpp>

//https://desire.giesecke.tk/index.php/2018/01/30/esp32-dht11/
#include <DHTesp.h>
const int dhtPIN = 4;     // what digital pin the DHT22 is conected to
#define dhtTYPE DHTesp::DHT22   // there are multiple kinds of DHT sensors

#define RELAISW1PIN 2
#define RELAISW2PIN 14
#define RELAISW3PIN 12
#define RELAISW4PIN 13
#define RELAIS24PIN 15
#define RELAISW1PIN 3
#define RELAISWPPIN 1

#define DE215ANALOG A0

//Alle 13 GPIO Pins von D0 bis D10, SD3, SD2
//int dataPins[13] = {16, 5, 4, 0, 2, 14, 12, 13, 15, 3, 1, 10, 9};
//Auf der 0 will das Thermometer nicht wir nehmen D1 fü den OneWire Bus
#define oneWirePin 5
//Ist D7 ist RXD2 und mit D8 TXD2 Teil der seriellen Schnittstelle, solange wir sicher so debuggen wollen
//  können wir D7 und D8 nicht mitbenutzen
//Der Zugriff auf GPIO10/SD3 und GPIO9/SD2 führte zum permanenten Reset/Neustarts des NodeMCU

#define deviceVersion "GRDM01"

#ifndef NOVAL
#define NOVAL -9999
#endif

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

class TDE215SensorFeuchte : public TtgSensor
{
  public:
    TDE215SensorFeuchte(const String& aId, float *apDelta):TtgSensor(aId,apDelta) {;};
  protected:
    float doGetMessValue(); //DE215ANALOG
  private:
};

class TRelaisActor : public TtgActor
{
  public:
    TRelaisActor(int t_pin, const String& t_id, int *t_maintime):
                 TtgActor(t_id,t_maintime) 
                 {pin = t_pin;};
  protected:
    void doActivate();
    void doDeactivate();
  private:
    int pin;
};

class TExternActor : public TtgActor
{
  public:
    TExternActor(TGDevice* t_device, const String& t_id, int *t_maintime):
                 TtgActor(t_id,t_maintime) 
                 {device = t_device;};
  protected:
    void doActivate();
    void doDeactivate();
  private:
    TGDevice *device;
};

class TGardenMainActorsList : public TtgActorsList
{
  public:
    TGardenMainActorsList(TGDevice* t_device):TtgActorsList() {device = t_device;};
  protected:
    void doCalcStatus();
  private:
    TGDevice *device;  
};

class TGardenMainDevice : public TGDevice
{
  public:
    TGardenMainDevice(const String& aDeviceVersion):TGDevice(aDeviceVersion) {;}; 
    String urlextactor;
  protected:
    void doHello();
    void doRegister();
    void doAfterConfigChange();
    void doSetup();
    void doCalcStatus();
  private:
    DHTesp *dht = new DHTesp;
    float messDeltaTemp,messDeltaHum,messDeltaBoden;
    TRelaisActor *actwaterint[4];
    TExternActor *actwaterext[3];
    TRelaisActor *actpumpe, *actlicht;
    int waterStart,waterTime1,waterTime2,waterTime3,waterTime4,waterTime5,waterTime6,waterTime7,waterPercent;
};

float TDHT22SensorTemp::doGetMessValue()
{
  return dht->getTemperature();
}


float TDHT22SensorHum::doGetMessValue()
{
  return dht->getHumidity();
}

float TDE215SensorFeuchte::doGetMessValue()
{
  return analogRead(DE215ANALOG);
}

void TRelaisActor::doActivate()
{
  digitalWrite(pin,true);
}

void TRelaisActor::doDeactivate()
{
  digitalWrite(pin,false);
}

void TExternActor::doActivate()
{
  String response;
  device->httpRequest(((TGardenMainDevice *)device)->urlextactor + "?id=" + id + "&endtime="+String(endTime), "", false, response);
}

void TExternActor::doDeactivate()
{
  
}

void TGardenMainActorsList::doCalcStatus()
{
  device->doCalcStatus();  
}

void TGardenMainDevice::doHello()
{
  writelog("");
  writelog("GRDNMain (Garten Bewässerung Main");
  writelog("");
  
  TGDevice::doHello();
}

void TGardenMainDevice::doRegister()
{
  setTimerActive();

  registerSensors(new TtgSensorsList());
  sensors->add(new TDHT22SensorTemp(dht,"DHT22-TEMP",&messDeltaTemp));
  sensors->add(new TDHT22SensorHum(dht,"DHT22-HUM",&messDeltaHum));
  sensors->add(new TDE215SensorFeuchte("DE215",&messDeltaBoden));
  

  registerActors(new TGardenMainActorsList(this));
  actwaterint[0] = (TRelaisActor*) actors->add(new TRelaisActor(RELAISW1PIN,"RL-WATER1", &maintime)); 
  actwaterint[1] = (TRelaisActor*) actors->add(new TRelaisActor(RELAISW2PIN,"RL-WATER2", &maintime)); 
  actwaterint[2] = (TRelaisActor*) actors->add(new TRelaisActor(RELAISW3PIN,"RL-WATER3", &maintime)); 
  actwaterint[3] = (TRelaisActor*) actors->add(new TRelaisActor(RELAISW4PIN,"RL-WATER4", &maintime)); 
  actwaterext[0] = (TExternActor*) actors->add(new TExternActor(this,       "RL-WATER5", &maintime)); 
  actwaterext[1] = (TExternActor*) actors->add(new TExternActor(this,       "RL-WATER6", &maintime)); 
  actwaterext[2] = (TExternActor*) actors->add(new TExternActor(this,       "RL-WATER7", &maintime)); 
  actlicht       = (TRelaisActor*) actors->add(new TRelaisActor(RELAIS24PIN,"RL-LICHT24",&maintime)); 
  actpumpe       = (TRelaisActor*) actors->add(new TRelaisActor(RELAISWPPIN,"RL-PUMPE",  &maintime)); 
  
  TGDevice::doRegister();
  
  deviceconfig->addConfig("messDeltaTemp","F",0,false,"Delta ab der eine Temperatur reported wird",NULL,NULL,&messDeltaTemp);
  deviceconfig->addConfig("messDeltaHum","F",0,false,"Delta ab der eine Luftfeuchtigkeit reported wird",NULL,NULL,&messDeltaHum);
  deviceconfig->addConfig("messDeltaBoden","F",0,false,"Delta ab der eine Bodenfeuchtigkeit reported wird",NULL,NULL,&messDeltaBoden);

  deviceconfig->addConfig("waterStart","I",0,false,"Startzeit Beregnung",NULL,&waterStart,NULL);
  deviceconfig->addConfig("waterTimer1","I",0,false,"Standard Dauer Wasser 1",NULL,&waterTime1,NULL);
  deviceconfig->addConfig("waterTimer2","I",0,false,"Standard Dauer Wasser 2",NULL,&waterTime2,NULL);
  deviceconfig->addConfig("waterTimer3","I",0,false,"Standard Dauer Wasser 3",NULL,&waterTime3,NULL);
  deviceconfig->addConfig("waterTimer4","I",0,false,"Standard Dauer Wasser 4",NULL,&waterTime4,NULL);
  deviceconfig->addConfig("waterTimer5","I",0,false,"Standard Dauer Wasser 5",NULL,&waterTime5,NULL);
  deviceconfig->addConfig("waterTimer6","I",0,false,"Standard Dauer Wasser 6",NULL,&waterTime6,NULL);
  deviceconfig->addConfig("waterTimer7","I",0,false,"Standard Dauer Wasser 7",NULL,&waterTime7,NULL);
  deviceconfig->addConfig("waterPercent","I",0,false,"Stärke Beregnung in Prozent",NULL,&waterPercent,NULL);
  deviceconfig->addConfig("urlextactor","S",32,false,"Zugriff auf externe Actoren",&urlextactor,NULL,NULL);     
}

void TGardenMainDevice::doSetup()
{
  TGDevice::doSetup();

  writelog("initializiere DHT22");
  dht->setup(dhtPIN, dhtTYPE);  
}

void TGardenMainDevice::doAfterConfigChange()
{
  TGDevice::doAfterConfigChange();
  int starttime = waterStart;  
  starttime = actwaterint[0]->setAutoTimes(starttime,(int) (waterTime1 * (waterPercent/100.0)));
  starttime = actwaterint[1]->setAutoTimes(starttime,(int) (waterTime2 * (waterPercent/100.0)));
  starttime = actwaterext[0]->setAutoTimes(starttime,(int) (waterTime5 * (waterPercent/100.0)));
  starttime = actwaterext[1]->setAutoTimes(starttime,(int) (waterTime6 * (waterPercent/100.0)));
  starttime = actwaterint[2]->setAutoTimes(starttime,(int) (waterTime3 * (waterPercent/100.0)));
  starttime = actwaterint[3]->setAutoTimes(starttime,(int) (waterTime4 * (waterPercent/100.0)));
  starttime = actwaterext[2]->setAutoTimes(starttime,(int) (waterTime7 * (waterPercent/100.0)));
}

void TGardenMainDevice::doCalcStatus()
{
  boolean d_needpump = false;
  for (int i=0; i < sizeof(actwaterint); i++)
    d_needpump = d_needpump or (actwaterint[i]->status == 'Y') or (actwaterint[i]->status == 'A');
    
  for (int i=0; i < sizeof(actwaterext); i++)
    d_needpump = d_needpump or (actwaterext[i]->status == 'Y') or (actwaterext[i]->status == 'A');
    
  if (d_needpump and (actpumpe->status = 'F'))
    actpumpe->setStatus('A');  
  if (!d_needpump and (actpumpe->status = 'A'))
    actpumpe->setStatus('F');
}      

TGDevice *device = new TGardenMainDevice(deviceVersion);

void setup(void) 
{
  device->deviceSetup();  
}

void loop(void) 
{
  device->deviceLoop();
}
