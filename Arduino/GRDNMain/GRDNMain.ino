/*
 * 08.1o.2098 Kopiert aus dem HZLogger als GRDNMain
 *            Steuerung von Bewässerung, Pumpe und Hasenbeleuchtung
 *            
 *  TODO
 *    - Anschluss Verwaltung (Aus HZLogger)
 *    - Via URL Ein und Ausschalten
 *    - Dashborad mit Werten, Aktoren Status und Buttons
 *    - Ausschalten via EndTime -1 ?
 *    - Konfiguration gemäß Struktur konfigurieren, also alle Werte aufnehmen
 *    
 * (C) Andreas Tengicki 2019
*/

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

#define deviceVersion "tgGM01"

ESP8266WebServer server(80); 

#define NOVAL -9999

struct TSensor {
  String id;
  String anschluss;
  float value;
  float newValue;
  int messTime; 
  boolean changed;
  int reportTime; 
};

TSensor sensoren[3] = { {"DHT22-TEMP","GRDNSOUTH-TEMP", 0 , NOVAL, 0, false, 0}, {"DHT22-HUM","GRDNSOUTH-HUM", 0 , NOVAL, 0, false, 0}, {"FEUCHTE","GRDNSOUTH-FEUCHT", 0 , 0, NOVAL, false, 0} };

#define configStart 32

struct TConfig {
  char cfgVersion[7];
  char DeviceID[16];
  char WIFISSID[30];
  char WIFIPWD[30];
  char host[64];
  char urlgettimesec[64];
  char urlsensordata[64];
  int messLoop;    //[s]
  int messDelay;   //[ms]
  int messTime;    //[s]
  int delTime;     //[s]
  int reportTime;  //[s]
  int loopDelay;   //[ms]
  double messDeltaTemp;
  double messDeltaHum;
  double messDeltaFeuchte;
  int actorTime;    //[s]
  char urlactorlog[64];
  int waterStartTime;
  int waterVentil1;
  int waterVentil2;
  int waterVentil3;
  int waterVentil4;
  char urlextactor[64];
  int waterVentil5ext;
  int waterVentil6ext;
  int waterVentil7ext;
  int waterPercent;
};

TConfig configs =  {deviceVersion,"GRM001","BUISNESSZUM","FE1996#ag!2008","","measure/gettimesec.php","measure/sensordata.php",25,10,120,600,300,50};

struct TActor{
  String anschluss;
  String bez;
  int pin;
  boolean aktiv;
  boolean changed;
  int autoStart;
  int autoEnd;
  int endTime;
};

TActor actoren[9] = { {"WTR-HOF","Wasser Hof", 1, false, false, -1, -1, -1}, {"WTR-WEIDE","Wasser Weide", 2, false, false, -1, -1, -1}, {"WTR-SUED","Wasser Rasen Süd", 3, false, false, -1, -1, -1}, {"WTR-4","Wasser unbenutzt", 4, false, false, -1, -1, -1},
                      {"WTR-NORTH","Wasser Rasen Nord", 0, false, false, -1, -1, -1}, {"WTR-BEET","Wasser Beet", 0, false, false, -1, -1, -1}, {"WTR-7","Wasser unbenutzt (ext)", 0, false, false, -1, -1, -1},
                      {"HASEN-LIGHT","Light(24V)", 7, false, false, -1, -1, -1}, {"WATER-PUMP","Pumpe (220V)", 8, false, false, -1, -1, -1} 
                    };

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

TConfConfig devConfig[9] = { {"version","S",7,true,false,configs.cfgVersion,NULL,NULL,""}, 
                             {"deviceID","S",16,false,false,configs.DeviceID,NULL,NULL,"Device ID, ist auch in der Server-Konfiguration hinterlegt (identifiziert)"},
                             {"wifissid","S",30,false,true,configs.WIFISSID,NULL,NULL,"WLAN zum reinh&auml;ngen"},
                             {"wifipwd","S",30,false,true,configs.WIFIPWD,NULL,NULL,"zugeh&ouml;riges WLAN Passwort"},
                             {"host","S",128,false,true,configs.host,NULL,NULL,"URL an die mit http gesendet wird"},
                             {"messLoop","I",0,false,false,NULL,&(configs.messLoop),NULL,"Ermittlung des Messwerts als Mittelwert aus n Werten"},
                             {"messTime","I",0,false,false,NULL,&(configs.messTime),NULL,"Sekunden nach denen ein Messwert ung&uuml;ltig wird"},
                             {"delTime","I",0,false,false,NULL,&(configs.delTime),NULL,"&Auml;nderung nach der ein Wert sofort reported wird"},
                             {"reportTime","I",0,false,false,NULL,&(configs.reportTime),NULL,"Sekunden nach denen die Messwerte sp&auml;testens reported wird"}
                           };

int lastTimeMS = -1;
int mainTimeMS = 0; //[ms] damit die Weiterschaltung in ms funnktioniert, aber
int mainTime = 0;   //[s]  Wir schalten im Garten unabhängig vom Wochentag, d.h. Verwaltung von 00:00:00 bis 23:59:59 reicht, also 0 bis 86399 Sekunden reicht.

int checkMessTime = 0;
int checkActorTime = 0;

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
  writelog("GRDNMain (Garten Bewässerung Main");
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
  
  writelog("initializiere DHT22");
  writelog("initializiere FeuchteSensor");

  writelog("init EEPROM");
  EEPROM.begin(sizeof(configs));
  
  writelog("load Config");
  readConfig();
  
  writelog("calculate Times");
  calculateTimes();

  writelog("init Server");
  server.on("/",serverOnMain);
  server.on("/config",serverOnConfig);
  server.on("/saveconfig",serverOnSaveConfig);
  server.on("/writeconfig",serverOnWriteConfig);
  server.on("/getconfig",serverOnGetConfig);
  server.on("/putconfig",serverOnPutConfig);
  server.on("/values",serverOnValues);
  server.on("/valuesjson",serverOnValuesJson);
  server.on("/actorsjson",serverOnActorsJson);

  writelog("Start Server");
  server.begin();

  writelog("wait 250");
  delay(250);

  writelog("ENDE Initialisierung");
}

boolean timeTest(int checkTime, int sec)
{
  int ms = millis();
  return (((ms - checkTime) / 1000) > sec) || ((ms - checkTime) < 0);
}

void timer()
{
  int ms = millis();
  mainTimeMS += (ms - lastTimeMS);
  mainTime = mainTimeMS / 1000;
  if ((lastTimeMS = -1) or (mainTime > 86399) or (lastTimeMS > ms))
    {
      String response;
      httpRequest(String(configs.urlgettimesec), "", response);
      //Fraglich ist was geben ich hier zurück, am einfachsten wären unverpackt die Sekunden, oder halt xml oder noch besser json
      mainTime = response.toInt();
      mainTimeMS = mainTime * 1000;
    }
  lastTimeMS = ms;
}

void loop(void) 
{
  timer();
  
  server.handleClient();
  
  if (timeTest(checkMessTime,configs.messTime))
    {
      messWerte();
      String response;
      httpRequest(String(configs.urlsensordata), getValuesJson(false), response);
    }

  if (timeTest(checkActorTime,configs.actorTime))
    {
      actorAction();
      String response;
      httpRequest(String(configs.urlactorlog), getActorsJson(false), response);
    }

  delay(configs.loopDelay);
}

void messWerte()
{
  for (int i=0; i<sizeof(sensoren); i++)
    {
      sensoren[i].newValue = 0;
      if (timeTest(sensoren[i].messTime,configs.delTime))
        sensoren[i].messTime = NOVAL;
    }

  for (int i=0; i<configs.messLoop; i++)
    for (int j=0; j<sizeof(sensoren); j++)
      {
        switch (i)
          { case 1: sensoren[i].newValue += 0; break;
            case 2: sensoren[i].newValue += 0; break;
            case 3: sensoren[i].newValue += 0; break;
          }
        delay(configs.messDelay);
      }
  
  checkMessTime = millis();          

  float value;
  float delta;
  for (int i=0; i<sizeof(sensoren); i++)
    {
      value = sensoren[i].newValue / configs.messLoop;  
      switch (i)
        { case 1: delta = configs.messDeltaTemp; break;
          case 2: delta = configs.messDeltaHum; break;
          case 3: delta = configs.messDeltaFeuchte; break;
        }
      if (abs(sensoren[i].newValue - value) > delta)
        sensoren[i].changed = true; 
      sensoren[i].value = value;
      sensoren[i].messTime = checkMessTime;
    }  
}

void calculateTimes()
{
  //Hof
  actoren[0].autoStart = configs.waterStartTime;
  actoren[0].autoEnd = actoren[0].autoStart + (configs.waterVentil1 * (configs.waterPercent / 100));

  //Weiden
  actoren[1].autoStart = actoren[0].autoEnd;
  actoren[1].autoEnd = actoren[1].autoStart + (configs.waterVentil2 * (configs.waterPercent / 100));

  //Rasen Nord
  actoren[4].autoStart = actoren[1].autoEnd;
  actoren[4].autoEnd = actoren[4].autoStart + (configs.waterVentil5ext * (configs.waterPercent / 100));

  //Beet
  actoren[5].autoStart = actoren[4].autoEnd;
  actoren[5].autoEnd = actoren[5].autoStart + (configs.waterVentil6ext * (configs.waterPercent / 100));

  //Rasen Süd
  actoren[2].autoStart = actoren[5].autoEnd;
  actoren[2].autoEnd = actoren[2].autoStart + (configs.waterVentil3 * (configs.waterPercent / 100));

  //unbenutzt
  actoren[3].autoStart = actoren[2].autoEnd;
  actoren[3].autoEnd = actoren[3].autoStart + (configs.waterVentil4 * (configs.waterPercent / 100));
  
  actoren[6].autoStart = actoren[3].autoEnd;
  actoren[6].autoEnd = actoren[6].autoStart + (configs.waterVentil7ext * (configs.waterPercent / 100));
}

void actorAction()
{
  boolean actor8Pump = false;
  for (int i=0; i<sizeof(actoren); i++)
    {
      boolean newValue;
      if (i<7) 
        {
          newValue = ((actoren[i].autoStart <= mainTime) and (mainTime < actoren[i].autoEnd)) or (mainTime < actoren[i].endTime);
          actor8Pump = actor8Pump or actoren[i].aktiv;  
        }
      else if (i==7)
        newValue = (mainTime < actoren[7].endTime);
      else if (i==8)
        newValue = actor8Pump or (mainTime < actoren[8].endTime);

      if (actoren[i].pin > 0)
        {
          actoren[i].changed = digitalRead(actoren[i].pin) != newValue;
          if (actoren[i].changed)
            digitalWrite(actoren[i].pin,actoren[i].aktiv);
        }
      else
        {
          actoren[i].changed = actoren[i].aktiv != newValue;      
          if (actoren[i].changed && newValue) //an externe Actoren, senden wir nur Start und Endezeit, der externe fragt ggf nach, wenn er rebootet oder so
            {
              if (String(configs.urlextactor) != "")
                {
                  String url = String(configs.urlextactor) + "?anschluss="+actoren[i].anschluss+"&endtime="+String(actoren[i].endTime);
                  String response;
                  httpRequest(url, "", response);
                }
            }
        }
      actoren[i].aktiv = newValue;  
    }

  checkMessTime = millis();              
}

String htmlHeader()
{
  String html = "<html><body>"; 
  html += "<h1>GRDBMain (Garten (-Bewässerungs) Main)</h1>";
  html += "<p>Device ID:"+String(configs.DeviceID)+"</br>";
  html += "<p>Version"+String(deviceVersion)+"</p>";
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
  html += "<p><a href=\"/values\">aktuelle Werte</a></br>";
  html += "<p><a href=\"/valuesjson\">aktuelle Werte in JSON</a></br>";
  html += "<p><a href=\"/actorsjson\">aktuelle Zustände der Aktoren in JSON</a></br>";
  html += "<p><a href=\"/getconfig\">aktuelle Konfiguration in JSON</a></br>";
  html += htmlFooter(); 

  server.send(200, "text/html", html);
}

int getConfigIndex(String fieldname)
{
  int erg = -1;
  for (int i=0; i<sizeof(devConfig); i++)
    if (devConfig[i].fieldname = fieldname)
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
    if (devConfig[i].typ = "S")
      erg = String(devConfig[i].psval);
    else if (devConfig[i].typ = "I")
      erg = String(*(devConfig[i].pival));
    else if (devConfig[i].typ = "D")
      erg = String(*(devConfig[i].pdval));
  return erg;
}

void setConfigValue(String fieldname, String value)
{
  int i = getConfigIndex(fieldname);
  if (i>=0)
    if (devConfig[i].typ = "S")
      value.toCharArray(devConfig[i].psval,devConfig[i].groesse);
    else if (devConfig[i].typ = "I")
      *(devConfig[i].pival) = value.toInt();
    else if (devConfig[i].typ = "D")
      *(devConfig[i].pdval) = value.toFloat();
}

String getHtmlConfig()
{
  String html = htmlHeader(); 
  html += "<h2>Konfiguration</h2>";
  html += "<form action=\"saveconfig\" method=\"POST\">";
  for (int i=0; i < sizeof(devConfig); i++)
    {
      html += "<label>"+devConfig[i].fieldname+"</label><input type=\"text\" name=\""+devConfig[i].fieldname+"\"  size="+String(devConfig[i].groesse);
      html += " value=\""+getConfigValue(devConfig[i].fieldname)+"\"/><br/>";
      html += devConfig[i].description+"<br/><br/>";
    }
  /**/
  html += "<button type=\"submit\" name=\"action\">Werte &auml;ndern</button>";
  html += "</form>";
  html += "<form action=\"writeconfig\">";
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
  calculateTimes();
  server.send(200, "text/html", getHtmlConfig());
}

void serverOnWriteConfig()
{
  writeConfig();
  server.send(200, "text/html", getHtmlConfig());
}

void serverOnValues()
{
  String html = htmlHeader(); 
  html += "<h2>aktuelle Werte</h2>";
  html += "<table><tr><th>ID</th><th>Anschluss</th><th>vor Sekunden</th><th>Value</th></tr>";
  for (int i=0; i<sizeof(sensoren); i++)
    if (sensoren[i].messTime != NOVAL)
      {
        html += "<tr><td>"+sensoren[i].id+"</td>"; 
        html += "<tr><td>"+sensoren[i].anschluss+"</td>"; 
        long sec = (millis() - sensoren[i].messTime) / 1000;
        html += "<td>"+String(sec)+"</td>"; 
        html += "<td>"+String(sensoren[i].value)+"</td>"; 
        html += "</tr>"; 
      }  
  html += "</table>"; 
  html += htmlFooter();

  server.send(200, "text/html", html);
}

String getValuesJson(boolean angefordert)
{
  String json = "{ \"Version\": \""+String(deviceVersion)+"\", \"DeviceID\": \""+String(configs.DeviceID)+"\", \"values\" : { ";

  boolean first = true;
  long now = millis();

  for (int i; i<sizeof(sensoren); i++)
    if ((angefordert or timeTest(sensoren[i].reportTime,configs.reportTime)) and (sensoren[i].messTime != NOVAL))
      {
        if (!first) json += ", ";
        json += "\"V"+String(i)+"\" : {"; 
        json += "\"id\" : \""+sensoren[i].id+"\","; 
        json += "\"anschluss\" : \""+sensoren[i].anschluss+"\",";  
        long sec = (millis() - sensoren[i].messTime) / 1000;
        json += "\"sec\" : \""+String(sec)+"\",";
        json += ", \"value\" : \""+String(sensoren[i].value)+"\""; 
        json += "}, "; 
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

String getActorsJson(boolean angefordert)
{
  String json = "{ \"Version\": \""+String(deviceVersion)+"\", \"DeviceID\": \""+String(configs.DeviceID)+"\", \"actors\" : { ";

  boolean first = true;

  for (int i=0; i<sizeof(actoren); i++)
    if (angefordert || actoren[i].changed)
      {
        if (!first) json += ", ";
        json += "\"A"+String(i)+"\" : {"; 
        json += "\"anschluss\" : \""+actoren[i].anschluss+"\",";  //Er darf icht die gleichen Anschluss Identifikatoren wir der Heizsensor schicken, also ggf konfiguriertbar
        if (actoren[i].aktiv)
          json += ", \"aktiv\" : \"Y\""; 
        else
          json += ", \"aktiv\" : \"N\""; 
        json += "\"autostart\" : \""+String(actoren[i].autoStart)+"\",";
        json += "\"autoend\" : \""+String(actoren[i].autoEnd)+"\",";
        json += "\"endtime\" : \""+String(actoren[i].endTime)+"\",";
        json += "}, "; 

        if (!angefordert) actoren[i].changed = false;
      } 

  json += "} }";

  if (!first)
    return json;
  else 
    return "";
}

void serverOnActorsJson()
{
  server.send(200, "application/json", getActorsJson(true));
}

String getConfigJson(boolean withAll)
{
  String json = "{ \"Version\": \""+String(deviceVersion)+"\", \"DeviceID\": \""+String(configs.DeviceID)+"\", \"configs\" : { ";

  boolean first = true;
  for (int i=0; i<sizeof(devConfig); i++)
    {
      if (withAll || !devConfig[i].secure)
        {
          if (!first) json += ",";
          json += "\""+devConfig[i].fieldname+"\": \""+getConfigValue(devConfig[i].fieldname)+"\"";
          first = false;
        }  
    }
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
  for (int i=0; i<sizeof(deviceVersion); i++)
    if (EEPROM.read(configStart + i) != deviceVersion[i])
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
boolean httpRequest(String url, String values, String response)
{
  boolean erg = false;
  
  if (String(configs.host) != "")
    {
      url = String(configs.host)+"\"+"+url;
      writelog("Value Request to: \""+url+"\n");
      HTTPClient http;    //Declare object of class HTTPClient

      writelog("Values: \""+values+"\"\n");

      http.begin(url);   //Specify request destination

      int httpCode = 0;
      if (values != "")
        httpCode = http.GET(); 
      else
        {
          http.addHeader("Content-Type", "application/json");  //Specify content-type header
          httpCode = http.POST(values);
        }
        
      writelog("httpCode:"+String(httpCode)+":\n");
      if (httpCode < 0)
        writelog("[HTTP] ... failed, error: "+http.errorToString(httpCode)+"\n");

      response = http.getString();          //Get the response payload
      writelog("Response:"+response+":\n");

      http.end();  //Close connection

      erg = true;
    }
  return erg;
}

//Quatsch, wenn wir vom aussen die Konfig wollen, dann holen wir sie über getConfig
//void serverOnSendConfig()

void serverOnPutConfig()
{
  /*  
  * 07.10.2019
  * Damit (also ohne Wiederholgruppen, können wir das lesen vereinfachen
  * { setzt auf Modus 1, dann kommt ein Feldname, dann :, dann ein Wert und ggf ein ,
  * } können wir überlesen, wenn ein feld1 : { feld2 kommt, führt das reset zum lesen von feld2
  */
  String json = ""; //hier muss der POST-Body der Abfrage rein
  int modus = 0;
  String fieldname = "";
  String fieldvalue = "";
  for (int i=0; i<sizeof(json); i++)
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
  calculateTimes();
}
