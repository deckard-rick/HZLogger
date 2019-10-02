/*
 * Ursprung DS18B20 Ansteuerung vom beelogger,
 * Erläuterungen dieses Programmcodes unter http://beelogger.de 
 *
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
 * 02.10.19   Step2: DevAddress ist ein uint[8] Feld, da müssen wir eine HexID-wie eine MacAdresse draus machen, 
 *            zudem müssen wir mit records/struct arbeiten und nicht mit Index, die Teile können sich ja verschieben oder ersetzen, dann finden wir die 
 *            anderen Informationen nicht mehr
 * 
 * Hinweis: Die Libraries stehen hier: C:\Users\tengicki\Documents\Arduino\libraries und seit dem 23.09.2019 unter git
 * 
 * ToDo: Lesen Konfig von Server aus Gerät
 *       Schreibren von Konfig von Server an Gerät
 *        
 * (C) Andreas Tengicki 2018,2019
*/

//#include <OneWire.h> 
#include <DallasTemperature.h> 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>

#define SERIALOUT 

//Alle 13 GPIO Pins von D0 bis D10, SD3, SD2
//int dataPins[13] = {16, 5, 4, 0, 2, 14, 12, 13, 15, 3, 1, 10, 9};
//Auf der 0 will das Thermometer nicht wir nehmen D1 fü den OneWire Bus
#define oneWirePin 5
//Ist D7 ist RXD2 und mit D8 TXD2 Teil der seriellen Schnittstelle, solange wir sicher so debuggen wollen
//  können wir D7 und D8 nicht mitbenutzen
//Der Zugriff auf GPIO10/SD3 und GPIO9/SD2 führte zum permanenten Reset/Neustarts des NodeMCU

#define oneWireMax 12
int cntDev = 0;

OneWire *oneWire;
DallasTemperature *ds18b20;
ESP8266WebServer server(80); 

#define configStart 32
#define sensorVersion "tgHZ04"

struct Tdb18b20 {
  DeviceAddress address;
  String id;
  String anschluss;
  float temperature;
  boolean found;
};

Tdb18b20 tempSensors[oneWireMax];
long temperatureTime;
#define NOVAL -9999

struct TConfig {
  char cfgVersion[7];
  char DeviceID[16];
  char WIFISSID[30];
  char WIFIPWD[30];
  int ds18b20Aufloesung;
  int messLoop;
  int pinTime;
  int messTime;
  int delTime;
  double messDelta;
  char host[128];
  int reportTime;
};

TConfig configs =  {sensorVersion,"HZT001","BUISNESSZUM","FE1996#ag!2008",12,25,120,60,300,0.5,"",300};

struct TAnschluss{
  char id[41]; //8*(4+1)+\0
  char anschluss[16];
};

TAnschluss anschluesse[oneWireMax];

bool messChanged = false;
int checkPinTime = 0;
int checkMessTime = 0;
int checkReportTime = 0;


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
  writelog("DS18B20 Test");
  writelog("");

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
  temperatureTime = NOVAL;

  writelog("wait 500");
  delay(500);

  checkPins();

  writelog("init EEPROM");
  EEPROM.begin(sizeof(configs)+sizeof(anschluesse));
  
  writelog("load Config");
  readConfig();
  
  writelog("init Server");
  server.on("/",serverOnMain);
  server.on("/config",serverOnConfig);
  server.on("/values",serverOnValues);
  server.on("/valuesjson",serverOnValuesJson);
  server.on("/saveform",serverOnSaveForm);
  server.on("/writeconfig",serverOnWriteConfig);
  server.on("/configjson",serverOnConfigJson);
  server.on("/sendvalues",serverOnSendValues);
  server.on("/sendconfig",serverOnSendConfig);
  server.on("/getconfig",serverOnGetConfig);

  writelog("Start Server");
  server.begin();

  writelog("ENDE Initialisierung");
}

String adrToId(DeviceAddress devAdr)
{
  String erg = "";
  char hex[5];
  for (int i=0; i<8; i++)
    {
      sprintf(hex,"%4X",devAdr[i]);
      erg += "x"+String(hex);
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
  ds18b20->begin();
  int cntNew = ds18b20->getDeviceCount();
  writelog("D1: cntNew:",false);
  writelog(String(cntNew));

  /* Rücksetzten des found-Flags */
  for(int i; i < cntDev; i++)
    tempSensors[i].found = false;

  /* Wenn sich was ändert, müssen wir die Zuordnung neu speichern */
  boolean changed = false;

  /* Prüfen ob Device noch da und neue an den Anfang setzen */
  for(int i; i < cntNew; i++)
    {
      DeviceAddress devAdr; //Die DeviceAddress ist ein uint[8]
      if( ds18b20->getAddress(devAdr, i))
        {
          boolean found = false;
          String id = adrToId(devAdr);
          for (int j=0; (j < cntDev) && !found; j++)
            if (id == tempSensors[j].id)
              {
                tempSensors[j].found = true;
                found = true;
              }
          if (!found) 
            {
              /* Verschieben der bisherigen Sensoren eines nach hinten */
              for(int j=oneWireMax-1; j>=0; j--)
                tempSensors[j+1] = tempSensors[j];
              /* Speichern der aktuellen Daten in tempSensor[0] */
              for (int j=0; j<8; j++)  
                tempSensors[0].address[j] = devAdr[j];
              tempSensors[0].id = id;
              tempSensors[0].anschluss = ""; //unbekannt, sonst hätten wir ihn gefunden
              tempSensors[0].temperature = NOVAL;
              tempSensors[0].found = true;
              /* nun ein Sensor mehr */
              cntDev++;
              /* es hat sich was verändert*/
              changed = true;
            }
        }    
    }

  /* Inkative Sensoren hinter die aktiven schieben und zu altes fliegt raus */
  for (int i=cntDev-1; i>=0; i--)
    if (! tempSensors[i].found)  
      {
        Tdb18b20 inaktSensor = tempSensors[i];
        //die akvtiven von cntDev bis i eines hoch (gen 0)
        for (int j=i+1; j < cntDev; j++)
          tempSensors[j-1] = tempSensors[j];
        tempSensors[cntDev-1] = inaktSensor;
        cntDev--;
        changed = true;
      }
      
  if (changed) 
    {
      boolean first = true;
      writelog("  ** Changes");
      writelog("     cntDev:",false);
      writelog(String(cntDev));
      for (int i=0; i<oneWireMax; i++)
        {
          if (first && !tempSensors[i].found)
            writelog("  -- inaktiv");
          writelog(String(i),false);
          writelog(" Id",false);
          writelog(String(tempSensors[i].id),false);
          writelog(" Anschluss:",false);
          writelog(String(tempSensors[i].anschluss));

          tempSensors[i].id.toCharArray(anschluesse[i].id,41);
          tempSensors[i].anschluss.toCharArray(anschluesse[i].anschluss,16);

          writeAnschlusse();
        }
    }

  writelog("Check Devices - End");

  checkPinTime = millis();
}

boolean timeTest(int checkTime, int sec)
{
  int ms = millis();
  return (((ms - checkTime) / 1000) > sec) || ((ms - checkTime) < 0);
}

void messWerte()
{
  float newTemperatures[oneWireMax];
  for (int i=0; i < cntDev; i++)
    {
      tempSensors[i].temperature = 0;
      if (timeTest(temperatureTime,configs.delTime)) //Messwert älter als eine Stunde, kann weg
        temperatureTime = NOVAL;
    }

  if (cntDev > 0)
    for (int i=0; i < configs.messLoop; i++)
      {
        ds18b20->requestTemperatures();
        for(byte j=0 ;j < cntDev; j++) 
            {
              float temp =  ds18b20->getTempC(tempSensors[j].address);
              if (temp != DEVICE_DISCONNECTED_C) 
                tempSensors[j].temperature += temp;
            }
          delay(10);
        }

  checkMessTime = millis();          

  float value = 0;
  temperatureTime = checkMessTime;
  for(byte j=0; j< cntDev; j++) 
    {
      value = (tempSensors[j].temperature / configs.messLoop);
      if (abs( tempSensors[j].temperature - value) > configs.messDelta)
        messChanged = true;
      tempSensors[j].temperature = value;

      writelog("D1 GPIO ",false);
      writelog(String(oneWirePin),false);
      writelog("[",false);
      writelog(String(j),false);
      writelog("]: ",false);
      writelog(String(tempSensors[j].temperature),false);
      writelog(" 'C");
    }
}

void serverOnMain()
{
  String html = "<html><body>"; 
  html += "<h1>Temperatursensor Heizung</h1>";
  html += "<p>Version"+String(sensorVersion)+"</p>";
  html += "<p><a href=\"/config\">Konfiguration</a></br>";
  html += "<p><a href=\"/values\">aktuelle Werte</a></br>";
  html += "<p><a href=\"/valuesjson\">aktuelle Werte in JSON</a></br>";
  html += "<p><a href=\"/configjson\">aktuelle Konfiguration in JSON</a></br>";
  html += "<p><a href=\"/sendvalues\">sende Werte (JSON) zu Host</a></br>";
  html += "<p><a href=\"/sendconfig\">sende Config (JSON) zu Host</a></br>";
  html += "</body></html>"; 

  server.send(200, "text/html", html);
}

String getHtmlConfig()
{
  String html = "<html><body>"; 
  html += "<h1>Temperatursensor Heizung</h1>";
  html += "<h2>Konfiguration</h2>";
  html += "<form action=\"saveform\" method=\"POST\">";
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
  html += "<label>Pin-Time</label><input type=\"text\" name=\"pintime\"  size=10 value=\""+String(configs.pinTime)+"\"/><br/>";
  html += "Sekunden nach denen die Anschl&uuml;sse auf Sensoren getestet werden.<br/><br/>";
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
  html += "<input type=\"submit\" value=\"Werte &auml;ndern\">"; 
  html += "</form>";
  html += "<p><a href=\"/\">Main</a></br>";
  html += "<form action=\"writeconfig\">";
  html += "<input type=\"submit\" value=\"Config festschreiben\">"; 
  html += "</form>";
  html += "</body></html>"; 
  return html;
}
  
void serverOnConfig()
{
  server.send(200, "text/html", getHtmlConfig());
}

void serverOnSaveForm()
{
  for(int i=0; i<server.args(); i++)
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
      else if (fn == "pintime")
        configs.pinTime = server.arg(i).toInt();
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
  server.send(200, "text/html", getHtmlConfig());
}

void serverOnWriteConfig()
{
  writeConfig();
  server.send(200, "text/html", getHtmlConfig());
}

void serverOnValues()
{
  String html = "<html><body>"; 
  html += "<h1>Temperatursensor "+String(configs.DeviceID)+"</h1>";
  html += "<h2>aktuelle Werte Anschluss D1</h2>";
  html += "<table><tr><th>Address (ID)</th><th>Bez</th><th>vor Sekunden</th><th>Temp</th></tr>";
  int sec = (millis() - temperatureTime) / 1000;
  for (int i=0; i < cntDev; i++)
    if (temperatureTime != NOVAL)
      {
        html += "<tr><td>"+tempSensors[i].id+"</td>"; 
        html += "<tr><td>"+tempSensors[i].anschluss+"</td>"; 
        html += "<td>"+String(sec)+"</td>"; 
        html += "<td>"+String(tempSensors[i].temperature)+"</td>"; 
        html += "</tr>"; 
      }
  html += "</table>"; 
  html += "<p><a href=\"/\">Main</a></br>";
  html += "</body></html>"; 

  server.send(200, "text/html", html);
}

String getValuesJson()
{
  int sec = (millis() - temperatureTime) / 1000;
  
  String json = "{ \"Version\": \""+String(sensorVersion)+"\", \"DeviceID\": \""+String(configs.DeviceID)+"\", \"sec\" : \""+String(sec)+"\", \"values\" : { ";

  boolean first = true;
  for (int i=0; i < cntDev; i++)
    if (temperatureTime != NOVAL)
      {
        if (!first) json += ", ";
        json += "\"V"+String(i+1)+"\" : {"; 
        json += "\"id\" : \""+String(tempSensors[i].id)+"\""; 
        json += "\"anschluss\" : \""+String(tempSensors[i].anschluss)+"\""; 
        json += ", \"temp\" : \""+String(tempSensors[i].temperature)+"\""; 
        json += "} "; 
        first = false;
      }

  json += "} }";
  
  return json;
}

void serverOnValuesJson()
{
  server.send(200, "application/json", getValuesJson());
}

String getConfigJson(boolean withAll)
{
  String json = "{ \"Version\": \""+String(sensorVersion)+"\", \"DeviceID\": \""+String(configs.DeviceID)+"\", \"configs\" : { ";

  if (withAll)
    {
      json += "\"WIFISSID\": \""+String(configs.WIFISSID)+"\",";
      json += "\"WIFIPWD\": \""+String(configs.WIFIPWD)+"\",";
    }
  json += "\"ds18b20Aufloesung\": \""+String(configs.ds18b20Aufloesung)+"\",";
  json += "\"messLoop\": \""+String(configs.messLoop)+"\",";
  json += "\"pinTime\": \""+String(configs.pinTime)+"\",";
  json += "\"delTime\": \""+String(configs.delTime)+"\",";
  json += "\"messDelta\": \""+String(configs.messDelta)+"\",";
  json += "\"reportServer\": \""+String(configs.host)+"\",";
  json += "\"reportTime\": \""+String(configs.reportTime);
  json += "}  } }";
  return json;
}

void serverOnConfigJson()
{
  boolean withAll = false;
  if (server.args() > 0)
    if (server.argName(0) == "all");
      withAll = server.arg(0) == "Y";

  server.send(200, "application/json", getConfigJson(withAll));
}

//https://playground.arduino.cc/Code/EEPROMLoadAndSaveSettings
void writeAnschlusse()
{
  for (unsigned int i=0; i<sizeof(anschluesse); i++)
    EEPROM.write(configStart + sizeof(configs) + i, *((char*)&anschluesse + i));
  EEPROM.commit();    
}

void readAnschlusse()
{
  for (unsigned int i=0; i<sizeof(anschluesse); i++)
    *((char*)&anschluesse + i) = EEPROM.read(configStart + sizeof(configs) + i);  
}

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
      readAnschlusse();
    }
}

void writeConfig()
{
  for (unsigned int i=0; i<sizeof(configs); i++)
    EEPROM.write(configStart + i, *((char*)&configs + i));
  EEPROM.commit();    

  writeAnschlusse();
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

void serverOnSendValues()
{
  if (sendToHost(String(configs.host), getValuesJson()))
    checkReportTime = millis();
}

void serverOnSendConfig()
{
  sendToHost(String(configs.host), getConfigJson(true));
}

void serverOnGetConfig()
{
  //10.01.2019-tg
  //Hier braucht es einen schmalen JSON Parser, der unsere Werte liefert.
  //dabei ist aber zu beachten, dass die verschachtelten Strukturen richtig
  //geliefert werden.
  //In der Config ist zum Beispiel der komplexeste Wert: configs.korrWerte.D1.k1 = 4711 
}

void loop(void) 
{
  server.handleClient();
  
  //Prüfe de Pins nur alle 5 Minuten
  //beim Überlauf nach ca 50 Tagen prüft er ggf einmal zu wenig,
  if (timeTest(checkPinTime,configs.pinTime))
    checkPins();
  
  if (timeTest(checkMessTime,configs.messTime))
    messWerte();

  if (timeTest(checkReportTime,configs.reportTime) || messChanged)
    {
      serverOnSendValues();
      messChanged = false;
    }

  delay(50);
}
