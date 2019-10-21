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
 * 
 * Allgemeine Hinweise: https://www.mikrocontroller-elektronik.de/nodemcu-esp8266-tutorial-wlan-board-arduino-ide/
 * Hinweis: Die Libraries stehen hier: C:\Users\tengicki\Documents\Arduino\libraries und seit dem 23.09.2019 unter git
 *          Ursprung DS18B20 Ansteuerung vom beelogger, http://beelogger.de 
 * 
 * (C) Andreas Tengicki 2018,2019
*/

//#include <OneWire.h> 
#include <DallasTemperature.h> 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <DHTesp.h>

#define SERIALOUT 

//Alle 13 GPIO Pins von D0 bis D10, SD3, SD2
//int dataPins[13] = {16, 5, 4, 0, 2, 14, 12, 13, 15, 3, 1, 10, 9};
//Auf der 0 will das Thermometer nicht wir nehmen D1 fü den OneWire Bus
#define oneWirePin 5
//Ist D7 ist RXD2 und mit D8 TXD2 Teil der seriellen Schnittstelle, solange wir sicher so debuggen wollen
//  können wir D7 und D8 nicht mitbenutzen
//Der Zugriff auf GPIO10/SD3 und GPIO9/SD2 führte zum permanenten Reset/Neustarts des NodeMCU

#define sensorVersion "tgHZ04"

ESP8266WebServer server(80); 

OneWire *oneWire;
DallasTemperature *ds18b20;
#define oneWireMax 10
#define NOVAL -9999

int cntDev = 0;

struct Tdb18b20 {
  DeviceAddress address;
  String id;
  float temperature;
  boolean found;
  boolean changed = false;
  long tempTime = NOVAL;
  long reportTime = 0;
};

Tdb18b20 tempSensors[oneWireMax];

const int dhtPIN = 4;     // what digital pin the DHT22 is conected to
#define dhtTYPE DHTesp::DHT22   // there are multiple kinds of DHT sensors

struct Sdht22 {
  float temp;
  float hum;
  int messTime = NOVAL; 
  boolean changed = false;
  int reportTime = 0; 
};

DHTesp *dht22;
Sdht22 dhtdata;

#define configStart 32

struct TConfig {
  char cfgVersion[7];
  char DeviceID[16];
  char WIFISSID[30];
  char WIFIPWD[30];
  int ds18b20Aufloesung;
  int messTime;
  int delTime;
  int messDelay;    //[ms]
  double messDelta;
  double messDeltaHum;
  char host[128];
  int reportTime;
};

TConfig configs =  {sensorVersion,"HZT001","BUISNESSZUM","FE1996#ag!2008",12,120,60,300,0.5,5,"",300};

/* Neuer Part mit der Datenstruktur für die Konfiguration, 
 *  erstmal teste ich, ob der Compiler überhaupt versteht, was ich von ihm will.
 */

struct TConfConfig {
  String fieldname;
  String typ;
  int groesse;
  boolean readOnly;
  boolean secure;
  char *psval;
  int *pival;
  double *pdval;
  String description;
};

const int devConfigCnt = 12;
TConfConfig devConfig[devConfigCnt] = { {"version","S",7,true,false,configs.cfgVersion,NULL,NULL,""}, 
                             {"deviceID","S",16,false,false,configs.DeviceID,NULL,NULL,"Device ID, ist auch in der Server-Konfiguration hinterlegt (identifiziert)"},
                             {"wifissid","S",30,false,true,configs.WIFISSID,NULL,NULL,"WLAN zum reinh&auml;ngen"},
                             {"wifipwd","S",30,false,true,configs.WIFIPWD,NULL,NULL,"zugeh&ouml;riges WLAN Passwort"},
                             {"dsaufloesung","I",0,false,false,NULL,&(configs.ds18b20Aufloesung),NULL,"Aufl&ouml;sung des Messsensors (Parameter des ds18b20s)"},
                             {"messTime","I",0,false,false,NULL,&(configs.messTime),NULL,"Sekunden nach denen ein Messwert ung&uuml;ltig wird"},
                             {"delTime","I",0,false,false,NULL,&(configs.delTime),NULL,"&Auml;nderung nach der ein Wert sofort reported wird"},
                             {"messDelay","I",0,false,false,NULL,&(configs.messDelay),NULL,"Delay zwischen den Einzel-Messungen [ms]"},
                             {"messDelta","D",0,false,false,NULL,NULL,&(configs.messDelta),"&Auml;nderung nachdem ein Temperatur-Wert sofort reported wird"},
                             {"messDeltaHum","D",0,false,false,NULL,NULL,&(configs.messDeltaHum),"&Auml;nderung nachdem der Hum-Wert sofort reported wird"},
                             {"host","S",128,false,true,configs.host,NULL,NULL,"URL an die mit http gesendet wird"},
                             {"reportTime","I",0,false,false,NULL,&(configs.reportTime),NULL,"Sekunden nach denen die Messwerte sp&auml;testens reported wird"}
                           };
                           
int checkMessTime = 0;

void writelog(String s, boolean crLF=true)
{
#ifdef SERIALOUT
  if (crLF)
    Serial.println(s);
  else  
    Serial.print(s);
#endif
}

void setup(void) 
{
#ifdef SERIALOUT
  Serial.begin(9600);
#endif
  writelog("");
  writelog("HZLogger");
  writelog("");

  writelog("wait 10s");
  delay(10000);
  writelog("START Initialisierung");

  WiFi.begin(configs.WIFISSID,configs.WIFIPWD);
  //TODO hier brauchen wir noch ein TimeOut
  while (WiFi.status() != WL_CONNECTED) 
   {
    delay(500);
    writelog(".",false);
   }
  writelog(".");

  writelog("WiFi connected IP:",false);
  writelog(WiFi.localIP().toString());
  
  writelog("initializiere D1");
  oneWire = new OneWire(oneWirePin);
  ds18b20 = new DallasTemperature (oneWire);

  writelog("initializiere DHT22");
  dht22 = new DHTesp();
  dht22->setup(dhtPIN, dhtTYPE);  

  writelog("wait 500");
  delay(500);

  ds18b20->begin();
  delay(500);

  checkPins();

  writelog("init EEPROM");
  EEPROM.begin(sizeof(configs));
  
  writelog("load Config");
  readConfig();
  
  writelog("init Server");
  server.on("/",serverOnMain);
  server.on("/config",serverOnConfig);
  server.on("/saveconfig",serverOnSaveConfig);
  server.on("/check",serverOnCheck);
  server.on("/values",serverOnValues);
  server.on("/valuesjson",serverOnValuesJson);
  server.on("/writeconfig",serverOnWriteConfig);
  server.on("/getconfig",serverOnGetConfig);
  server.on("/putconfig",serverOnPutConfig);

  writelog("Start Server");
  server.begin();

  writelog("ENDE Initialisierung");
}

String adrToId(DeviceAddress devAdr)
{
  String erg = "";
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

/* CheckPins, war mal ganz einfach, jetzt wird es kompliziert
 *  * wir lesen alle vorhandenen Device Adresses und prüfen ob wir sie kennen
 *  * wenn neu, kommt die an den Anfang des Arrays, dadurch haben wir immer die letzten aktiven im Array tempSensors
 *  * wenn nicht gefunden verschieben wir diese hinter die aktiven, zu alte fliegen raus.
 *  * damit der Anschluss nicht verloren geht, speichern wir (id,anschluss) fest im Eprom hinter der Konfig.
 */
void checkPins() 
{
  writelog("Check Devices - Start");

  /* Lesen der aktuellen Anzahl */
  cntDev = ds18b20->getDeviceCount();
  writelog("cntDev:",false);
  writelog(String(cntDev));

  /* Die Devices als Sensoren einfach einsortieren, sie werden immer in 
   *  der glechen Reihenfolge geliefert und ändern sich ja selten.
   */
  for(int i=0; i < cntDev; i++)
    {
      DeviceAddress devAdr; //Die DeviceAddress ist ein uint[8]
      if( ds18b20->getAddress(devAdr, i))
        {
          String id = adrToId(devAdr);
          writelog("id: "+id,true);
          for (int j=0; j<8; j++)  
            tempSensors[i].address[j] = devAdr[j];
          tempSensors[i].id = id;
          tempSensors[i].temperature = NOVAL;
          tempSensors[i].changed = false;
          tempSensors[i].tempTime = NOVAL;
        }
    }
  writelog("Check Devices - End");
}

boolean timeTest(int checkTime, int sec)
{
  int ms = millis();
  return (((ms - checkTime) / 1000) > sec) || ((ms - checkTime) < 0);
}

void messWerte()
{
  float newTemperatures[oneWireMax];

  //Messwert älter als eine Stunde, kann weg
  for (int i=0; i < cntDev; i++)
    {
      tempSensors[i].temperature = 0;
      if (timeTest(tempSensors[i].tempTime,configs.delTime))
        tempSensors[i].tempTime = NOVAL;
    }
  if (timeTest(dhtdata.messTime,configs.delTime))
    dhtdata.messTime = NOVAL;

  ds18b20->requestTemperatures();
  checkMessTime = millis();          
  for(byte j=0 ;j < cntDev; j++) 
      {
        float temp =  ds18b20->getTempC(tempSensors[j].address);
        if (temp != DEVICE_DISCONNECTED_C) 
          {
            if (abs( tempSensors[j].temperature - temp) > configs.messDelta)
              tempSensors[j].changed = true;
            tempSensors[j].temperature = temp;
            tempSensors[j].tempTime = checkMessTime;

            writelog("D",false);
            writelog(String(oneWirePin),false);
            writelog(" [",false);
            writelog(String(tempSensors[j].id),false);
            writelog("]: ",false);
            writelog(String(tempSensors[j].temperature),false);
            writelog(" 'C");
          }
      }
  /* TGMARK hier jetzt dht22.messen  */
  TempAndHumidity values = dht22->getTempAndHumidity();

  checkMessTime = millis();          

  if (abs(dhtdata.temp - values.temperature) > configs.messDelta)
    dhtdata.changed = true; 
  dhtdata.temp = values.temperature;;
  writelog("D",false);
  writelog(String(dhtPIN),false);
  writelog(" [DHT22-TEMP] :",false);
  writelog(String(dhtdata.temp),false);
  writelog("'C");
  
  if (abs(dhtdata.hum - values.humidity) > configs.messDeltaHum)
    dhtdata.changed = true; 
  dhtdata.hum = values.humidity;
  writelog("D",false);
  writelog(String(dhtPIN),false);
  writelog(" [DHT22-HUM] :",false);
  writelog(String(dhtdata.hum),false);
  writelog("%");

  dhtdata.messTime = checkMessTime; 
}

String htmlHeader()
{
  String html = "<html><body>"; 
  html += "<h1>HZLogger (Heinzungs-Logger)</h1>";
  html += "<p>Device ID:"+String(configs.DeviceID)+"</br>";
  html += "<p>Version"+String(sensorVersion)+"</p>";
  return html;
}

String htmlFooter()
{
  String html = "";
  html += "<p><a href=\"/\">Main</a></br>";
  html += "<small>Copyright Andreas Tengicki 2018-2020</small>";
  html += "</body></html>"; 
  return html;
}

void serverOnMain()
{
  String html = htmlHeader();
  html += "<h2>Main</h2>";
  html += "<p><a href=\"/config\">Konfiguration</a></br>";
  html += "<p><a href=\"/check\">Check tempSensor</a></br>";
  html += "<p><a href=\"/values\">aktuelle Werte</a></br>";
  html += "<p><a href=\"/valuesjson\">aktuelle Werte in JSON</a></br>";
  html += "<p><a href=\"/getconfig\">aktuelle Konfiguration in JSON</a></br>";
  html += htmlFooter(); 

  server.send(200, "text/html", html);
}

int getConfigIndex(String fieldname)
{
  int erg = -1;
  for (int i=0; i < devConfigCnt; i++)
    if (devConfig[i].fieldname == fieldname)
      {
        erg = i;
        break;
      }
  return erg;
}

String getConfigValue(String fieldname)
{
  String erg = "";
  int i = getConfigIndex(fieldname);
  if (i>=0)
    if (devConfig[i].typ == "S")
      erg = String(devConfig[i].psval);
    else if (devConfig[i].typ == "I")
      erg = String(*(devConfig[i].pival));
    else if (devConfig[i].typ == "D")
      erg = String(*(devConfig[i].pdval));
  return erg;
}

void setConfigValue(String fieldname, String value)
{
  int i = getConfigIndex(fieldname);
  if (i>=0)
    if (devConfig[i].typ == "S")
      value.toCharArray(devConfig[i].psval,devConfig[i].groesse);
    else if (devConfig[i].typ == "I")
      *(devConfig[i].pival) = value.toInt();
    else if (devConfig[i].typ == "D")
      *(devConfig[i].pdval) = value.toFloat();
}

String getHtmlConfig()
{
  String html = htmlHeader(); 
  html += "<h2>Konfiguration</h2>";
  html += "<form action=\"saveconfig\" method=\"POST\">";
  writelog("getHTMLConfig - 1");
  /*
  html += "<label>DeviceID</label><input type=\"text\" name=\"deviceID\" size=30 value=\""+String(configs.DeviceID)+"\"/><br/>";
  html += "Device ID, ist auch in der Server-Konfiguration hinterlegt (identifiziert).<br/><br/>";
  html += "<label>SSID</label><input type=\"text\" name=\"ssid\" size=30 value=\""+String(configs.WIFISSID)+"\"/><br/>";
  html += "WLAN zum reinh&auml;ngen<br/><br/>";
  html += "<label>Pwd</label><input type=\"text\" name=\"pwd\"  size=30 value=\""+String(configs.WIFIPWD)+"\"/><br/>";
  html += "zugeh&ouml;riges WLAN Passwort<br/><br/>";
  html += "<br/>";
  html += "<label>Aufl&ouml;sung</label><input type=\"text\" name=\"aufloesung\"  size=10 value=\""+String(configs.ds18b20Aufloesung)+"\"/><br/>";
  html += "Aufl&ouml;sung des Messsensors (Parameter des ds18b20s).<br/><br/>";
  html += "<label>Mess-Anzahl</label><input type=\"text\" name=\"messloop\"  size=10 value=\""+String(configs.messLoop)+"\"/><br/>";
  html += "Ermittlung des Messwerts aus Mess-Anzahl Werten.<br/><br/>";
  html += "<label>Mess-Time</label><input type=\"text\" name=\"messtime\"  size=10 value=\""+String(configs.messTime)+"\"/><br/>";
  html += "Sekunden nach denen erneut eine Messung durchgef&uuml;hrt wird.<br/><br/>";
  html += "<label>Delete-Time</label><input type=\"text\" name=\"deltime\"  size=10 value=\""+String(configs.delTime)+"\"/><br/>";
  html += "Sekunden nach denen ein Messwert ung&uuml;ltig wird.<br/><br/>";
  html += "<label>Mess-Delta</label><input type=\"text\" name=\"delta\"  size=10 value=\""+String(configs.messDelta)+"\"/><br/>";
  html += "&Auml;nderung nach der ein Wert sofort reported wird.<br/><br/>";
  html += "<label>Host</label><input type=\"text\" name=\"host\"  size=128 value=\""+String(configs.host)+"\"/><br/>";
  html += "URL an die mit http gesendet wird.<br/><br/>";
  html += "<label>Report-Time</label><input type=\"text\" name=\"reporttime\"  size=10 value=\""+String(configs.reportTime)+"\"/><br/>";
  html += "Sekunden nach denen die Messwerte sp&auml;testens reported wird.<br/><br/>";
  */
  for (int i=0; i < devConfigCnt; i++)
    {
      html += "<label>"+devConfig[i].fieldname+"</label><input type=\"text\" name=\""+devConfig[i].fieldname+"\"  size="+String(devConfig[i].groesse);
      html += " value=\""+getConfigValue(devConfig[i].fieldname)+"\"/><br/>";
      html += devConfig[i].description+"<br/><br/>";
    }
  /**/
  //html += "<input type=\"submit\" value=\"Werte &auml;ndern\">"; 
  html += "<button type=\"submit\" name=\"action\">Werte &auml;ndern</button>";
  html += "</form>";
  html += "<form action=\"writeconfig\">";
  //html += "<input type=\"submit\" value=\"Config festschreiben\">"; 
  html += "<button type=\"submit\" name=\"action\">Config festschreiben</button>";
  html += "</form>";
  html += htmlFooter(); 
  return html;
}
  
void serverOnConfig()
{
  server.send(200, "text/html", getHtmlConfig());
}

void serverOnSaveConfig()
{
  for(int i=0; i<server.args(); i++)
    setConfigValue(server.argName(i), server.arg(i));
  /**  
    {
      String fn = server.argName(i);
      if (fn == "deviceID")
        server.arg(i).toCharArray(configs.DeviceID,16);
      else if (fn == "ssid")
        server.arg(i).toCharArray(configs.WIFISSID,30);
      else if (fn == "pwd")
        server.arg(i).toCharArray(configs.WIFIPWD,30);
      else if (fn == "aufloesung")
        configs.ds18b20Aufloesung = server.arg(i).toInt();
      else if (fn == "messloop")
        configs.messLoop = server.arg(i).toInt();
      else if (fn == "messtime")
        configs.messTime = server.arg(i).toInt();
      else if (fn == "deltime")
        configs.delTime = server.arg(i).toInt();
      else if (fn == "delta")
        configs.messDelta = server.arg(i).toFloat();
      else if (fn == "host")
        server.arg(i).toCharArray(configs.host,128);
      else if (fn == "reporttime")
        configs.reportTime = server.arg(i).toInt();
    }
  */  
  server.send(200, "text/html", getHtmlConfig());
}

void serverOnWriteConfig()
{
  writeConfig();
  server.send(200, "text/html", getHtmlConfig());
}

String getHtmlCheck()
{
  String html = htmlHeader();
  html += "<h2>Anschluss-Check</h2>";
  for (int i=0; i<oneWireMax; i++)
    if (tempSensors[i].id != "")
      html += "<label>"+String(i)+": "+tempSensors[i].id+"</label><br/>";
  html += htmlFooter(); 
  return html; 
}

void serverOnCheck()
{
  checkPins();
  server.send(200, "text/html", getHtmlCheck());
}

void serverOnValues()
{
  String html = htmlHeader(); 
  html += "<h2>aktuelle Werte</h2>";
  html += "<table><tr><th>Address (ID)</th><th>Bez</th><th>vor Sekunden</th><th>Temp</th></tr>";
  for (int i=0; i < cntDev; i++)
    if (tempSensors[i].tempTime != NOVAL)
      {
        html += "<tr><td>"+tempSensors[i].id+"</td>"; 
        long sec = (millis() - tempSensors[i].tempTime) / 1000;
        html += "<td>"+String(sec)+"</td>"; 
        html += "<td>"+String(tempSensors[i].temperature)+"</td>"; 
        html += "</tr>"; 
      }
  if (dhtdata.messTime != NOVAL)
    {
      html += "<tr><td>DHT22-TEMP</td>"; 
      long sec = (millis() - dhtdata.messTime) / 1000;
      html += "<td>"+String(sec)+"</td>"; 
      html += "<td>"+String(dhtdata.temp)+"</td>"; 
      html += "</tr>"; 
      html += "<tr><td>DHT22-HUM</td>"; 
      html += "<td>"+String(sec)+"</td>"; 
      html += "<td>"+String(dhtdata.hum)+"</td>"; 
      html += "</tr>"; 
    }  
  html += "</table>"; 
  html += htmlFooter();

  server.send(200, "text/html", html);
}

String getValuesJson(boolean angefordert)
{
  String json = "{ \"Version\": \""+String(sensorVersion)+"\", \"DeviceID\": \""+String(configs.DeviceID)+"\", \"values\" : { ";

  boolean first = true;
  long now = millis();
  
  for (int i=0; i < cntDev; i++)
    if (angefordert || tempSensors[i].changed || timeTest(tempSensors[i].reportTime,configs.reportTime))
      if (tempSensors[i].tempTime != NOVAL)
        {
          if (!first) json += ", ";
          json += "\"V"+String(i+1)+"\" : {"; 
          json += "\"id\" : \""+String(tempSensors[i].id)+"\","; 
          long sec = (millis() - tempSensors[i].tempTime) / 1000;
          json += "\"sec\" : \""+String(sec)+"\",";
          json += ", \"value\" : \""+String(tempSensors[i].temperature)+"\""; 
          json += "} "; 
          first = false;
          if (!angefordert)
            {
              tempSensors[i].changed = false;
              tempSensors[i].reportTime = now;
            }
        }
  if (angefordert || dhtdata.changed || timeTest(dhtdata.reportTime,configs.reportTime))
    if (dhtdata.messTime != NOVAL)
      {
        if (!first) json += ", ";
        json += "\"V"+String(cntDev+1)+"\" : {"; 
        json += "\"id\" : \"DHT22-TEMP\","; 
        long sec = (millis() - dhtdata.messTime) / 1000;
        json += "\"sec\" : \""+String(sec)+"\",";
        json += ", \"value\" : \""+String(dhtdata.temp)+"\""; 
        json += "}, "; 
        json += "\"V"+String(cntDev+2)+"\" : {"; 
        json += "\"id\" : \"DHT22-HUM\","; 
        json += "\"sec\" : \""+String(sec)+"\",";
        json += ", \"value\" : \""+String(dhtdata.hum)+"\""; 
        json += "} "; 
      } 

  json += "} }";

  if (!first)
    return json;
  else 
    return "";
}

void serverOnValuesJson()
{
  server.send(200, "application/json", getValuesJson(true));
}

String getConfigJson(boolean withAll)
{
  String json = "{ \"Version\": \""+String(sensorVersion)+"\", \"DeviceID\": \""+String(configs.DeviceID)+"\", \"configs\" : { ";

  boolean first = true;
  for (int i=0; i < devConfigCnt; i++)
    {
      if (withAll || !devConfig[i].secure)
        {
          if (!first) json += ",";
          json += "\""+devConfig[i].fieldname+"\": \""+getConfigValue(devConfig[i].fieldname)+"\"";
          first = false;
        }  
    }
 /*   
  if (withAll)
    {
      json += "\"WIFISSID\": \""+String(configs.WIFISSID)+"\",";
      json += "\"WIFIPWD\": \""+String(configs.WIFIPWD)+"\",";
    }
  json += "\"ds18b20Aufloesung\": \""+String(configs.ds18b20Aufloesung)+"\",";
  json += "\"messLoop\": \""+String(configs.messLoop)+"\",";
  json += "\"delTime\": \""+String(configs.delTime)+"\",";
  json += "\"messDelta\": \""+String(configs.messDelta)+"\",";
  json += "\"reportServer\": \""+String(configs.host)+"\",";
  json += "\"reportTime\": \""+String(configs.reportTime);
  */
  json += "}  } }";
  return json;
}

void serverOnGetConfig()
{
  boolean withAll = false;
  if (server.args() > 0)
    if (server.argName(0) == "all");
      withAll = server.arg(0) == "Y";

  server.send(200, "application/json", getConfigJson(withAll));
}

//https://playground.arduino.cc/Code/EEPROMLoadAndSaveSettings
void readConfig()
{
  boolean valid = true;
  for (int i=0; i<7; i++)
    if (EEPROM.read(configStart + i) != sensorVersion[i])
      {
        valid = false;
        break;
      }  
  if (valid)  
    {
      for (unsigned int i=0; i<sizeof(configs); i++)
        *((char*)&configs + i) = EEPROM.read(configStart + i);  
    }
}

void writeConfig()
{
  for (unsigned int i=0; i<sizeof(configs); i++)
    EEPROM.write(configStart + i, *((char*)&configs + i));
  EEPROM.commit();    
}


// http JSON Post:
// https://techtutorialsx.com/2016/07/21/esp8266-post-requests/
boolean sendToHost(String host, String values)
{
  boolean erg = false;
  
  if (host[0] != '\0')
    {
      writelog("Value Request to: \""+host+"\"\n");
      HTTPClient http;    //Declare object of class HTTPClient

      writelog("Values: \""+values+"\"\n");

      http.begin(host);   //Specify request destination
      http.addHeader("Content-Type", "application/json");  //Specify content-type header

      int httpCode = http.POST(values);           //Send the request
      writelog("httpCode:"+String(httpCode)+":\n");
      if (httpCode < 0)
        writelog("[HTTP] ... failed, error: "+http.errorToString(httpCode)+"\n");

      String payload = http.getString();          //Get the response payload

      writelog("Response:"+payload+":\n");

      http.end();  //Close connection

      erg = true;
    }
  return erg;
}

//Quatsch, wenn wir vom aussen die Konfig wollen, dann holen wir sie über getConfig
//void serverOnSendConfig()

void serverOnPutConfig()
{
  /**10.01.2019-tg
  *  Hier braucht es einen schmalen JSON Parser, der unsere Werte liefert.
  *  dabei ist aber zu beachten, dass die verschachtelten Strukturen richtig
  *  geliefert werden.
  *  In der Config ist zum Beispiel der komplexeste Wert: configs.korrWerte.D1.k1 = 4711 
  *  
  * 05.10.2019 
  * Es gibt keine verschachtelten Werte mit in der Config, maximal in der Anschlussverwaltung
  *
  * 07.10.2019
  * Damit (also ohne Wiederholgruppen, können wir das lesen vereinfachen
  * { setzt auf Modus 1, dann kommt ein Feldname, dann :, dann ein Wert und ggf ein ,
  * } können wir überlesen, wenn ein feld1 : { feld2 kommt, führt das reset zum lesen von feld2
  */
  String json = ""; //hier muss der POST-Body der Abfrage rein
  int modus = 0;
  String fieldname = "";
  String fieldvalue = "";
  for (int i=0; i<json.length(); i++)
    {
      char c = json[i];
      if ((modus == 0) and (c == '{')) modus = 1;
      else if ((modus == 1) and (c == '"')) modus = 2;
      else if ((modus == 2) and (c == '"')) modus = 3;
      else if (modus == 2) fieldname += c;
      else if ((modus == 3) and (c == ':')) modus = 4;
      else if (((modus == 2) or (modus == 4)) and (c == '{')) modus = 1;
      else if ((modus == 4) and (c == '"')) modus = 5;
      else if ((modus == 5) and (c == '"'))
        {
          if ((fieldname == "DeviceID") and (fieldvalue != String(configs.DeviceID)))
            return;
          setConfigValue(fieldname,fieldvalue);
          fieldname = "";
          fieldvalue = "";
          modus = 1;
        }
      else if (modus == 5) fieldvalue += c;
    }
}

void loop(void) 
{
  server.handleClient();
  
  if (timeTest(checkMessTime,configs.messTime))
    {
      messWerte();
      sendToHost(String(configs.host), getValuesJson(false));
    }

  delay(50);
}
