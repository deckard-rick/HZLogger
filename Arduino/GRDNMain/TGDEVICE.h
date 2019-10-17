#ifndef TGDEVICE_H
#define TGDEVICE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#define configStart 32
#define NOVAL -9999

class TtgConfConfig
{
  public:
    String fieldname;
    String typ;
    int groesse;
    boolean secure;
    String description;
    String *psval;
    int *pival;
    float *pdval;
    TtgConfConfig *next;
    TtgConfConfig(const String& aFieldname,const String& aTyp, const int aGroesse, const boolean aSecure, const String& aDescription, String *aps, int *api, float *apf);
};

class TtgDeviceConfig
{
  public:
    TtgDeviceConfig(const String& aDeviceVersion);
    void addConfig(const String& aFieldname, const String& aTyp, const int aGroesse, const boolean aSecure, const String& aDescription, String* aps, int* api, float* apf);
    void setValue(const String& fieldname, const String& value);
    String getValue(const String& fieldname);
    int getValueI(const String& fieldname);
    float getValueD(const String& fieldname);
    String getJson(boolean withAll);
    void putJson(const String& json);
    int getEEPROMSize(); 
    void readEEPROM();
    void writeEEPROM();
    String getHtmlForm();
  protected:
  private:
    String deviceVersion;  //MAX 7 Zeichen
    TtgConfConfig *firstelement = NULL, *lastelement = NULL;
    TtgConfConfig* getFieldElement(const String& fieldname);
};

class TtgSensor
{
  public:
    TtgSensor(const String& t_id, float *t_pdelta) {id = t_id; pdelta = t_pdelta;};
    String id;
    float value;
    float newValue;
    float *pdelta;
    int messTime; 
    boolean changed;
    int reportTime;
    TtgSensor* next = NULL; 
    void messWert();
  protected:
    virtual float doGetMessValue();
};

class TtgSensorsList
{
  public:
    boolean hasSensors();
    void add(TtgSensor *t_value);
    boolean messWerte();
    boolean checkReporting(int t_reportTime);
    String getJson(const boolean t_angefordert);
    virtual String getHTML();
  private:
    TtgSensor *firstelement=NULL, *lastelement=NULL; 
};

class TtgActor
{
  public:
    TtgActor(const String& t_id, int *t_maintime) {id = t_id; maintime = t_maintime;};
    int setAutoTimes(int t_start, int t_time);
    String id;
    char status='N';  //Y on, O autoON, F autoOFF, N off
    boolean changed = false;
    int autoStart = 0;
    int autoEnd = 0;
    int endTime = 0;
    TtgActor* next = NULL;
    void action();
    virtual void doCalcStatus();
    void setStatus(char t_status);
  protected:
    virtual void doAction();
    virtual void doActivate();
    virtual void doDeactivate();
  private: 
    int *maintime;
};

class TtgDevice;

class TtgActorsList
{
  public:
    boolean hasMembers();
    TtgActor* add(TtgActor *t_value);
    boolean action(TtgDevice *t_device);
    String getJson(const boolean t_angefordert);
    void setStatus(String t_id, char t_status);
    void setEndtime(String t_id, int t_endtime);
    virtual String getHTML();
  protected:
  private:
    TtgActor *firstelement=NULL, *lastelement=NULL; 
};

class TtgDevice
{
  public:
    TtgDevice(const String& aDeviceVersion);
    void writelog(const String& s, boolean crLF=true);
    void deviceSetup();
    void registerSensors(TtgSensorsList* t_sensors);
    void registerActors(TtgActorsList* t_actor);
    virtual void doCalcStatus();
    boolean httpRequest(const String& url, const String& values, const boolean t_withresponse, String& response);
    void deviceLoop();
  protected:
    TtgDeviceConfig *deviceConfig;
    virtual void doHello();
    void setTimerActive(boolean value=true);
    virtual void doRegister();
    virtual void doAfterConfigChange();
    virtual void doSetup();
    virtual void doLoop();
    int maintime = 0;   //[s]  Wir schalten im Garten unabhängig vom Wochentag, d.h. Verwaltung von 00:00:00 bis 23:59:59 reicht, also 0 bis 86399 Sekunden reicht.
    TtgSensorsList *sensors = NULL;
    TtgActorsList *actors = NULL;
  private:
    String deviceVersion;
    String logModus = "S"; //"":nix "S":serial "<ip:port>"für Debugging via Netzwerk (später)
    String deviceID, wifiSSID, wifiPWD, host;
    ESP8266WebServer *server = new ESP8266WebServer(80);
    boolean timerActive = false;
    int lastTimeMS = -1;
    int mainTimeMS = 0; //[ms] damit die Weiterschaltung in ms funnktioniert, aber
    int messTime = 60; //[s] 1m
    int reportTime = 300; //[s] 5m
    int delTime = 600; //[s] 10m 
    int actorTime = 5; //[s]
    int lastMessTime = -1, lastActorTime = -1;
    int loopDelayMS = 50; //[ms] Wie lange der Loop pausiert am Ende
    String urlgettimesec;
    String urlsensordata;
    String urlactordata;
    String htmlHeader();
    String htmlFooter();
    String jsonHeader();
    String getValuesJson(const boolean t_angefordert);
    String getActorsJson(const boolean t_angefordert);
    void serverOnDashboard();
    String getHtmlConfig();
    void serverOnConfig();
    void serverOnSaveConfig();
    void serverOnWriteConfig();
    void serverOnGetConfig();
    void serverOnPutConfig();
    void serverOnGetValues();
    void serverOnGetActors();
    void serverOnSetActor();
    void timer();
};

#endif
