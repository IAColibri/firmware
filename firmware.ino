#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FS.h"

//////////////////////
// WiFi Definitions //
//////////////////////
String ssid = "blabla";
String WiFiAPPSK = "123123123";

ESP8266WebServer server(80);

void setupWiFi()
{
  WiFi.mode(WIFI_AP);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = ssid;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  WiFi.softAP(AP_NameChar, WiFiAPPSK);
}

void setup(){
  initHardware();
  setupWiFi();

  server.on("/", handleHome);
  server.on("/save.html", handleSave);
  server.onNotFound(handleNotFound);

  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize );
  server.begin();
}

void initHardware){
  Serial.begin(115200);
}

void loop(void){
 server.handleClient();
}

void handleSave() {
  String layout;
  String content;
  bool ok =  SPIFFS.begin();
  if(ok) {

    File net = SPIFFS.open("/network", "w");
    for(int i = 0; i < server.args();i++) {
      net.print(server.arg(i) + ",");
      content += server.arg(i) + ",";
    }
    net.close();

    Serial.println("ok");
    bool exist = SPIFFS.exists("/save.html");
    if(exist) {

      File template_file = SPIFFS.open("/template.html", "r");
      layout = template_file.readString();
      template_file.close();

      Serial.println("The file exists!");
      File save = SPIFFS.open("/save.html", "r");

      if(!save) {
        Serial.println("Something went wrong trying to open the file ...");
      }

      int s = save.size();
      Serial.printf("Size = %d\r\n", s);
      String data = save.readString();

      Serial.println(data);
      content = data;
      save.close();
    }
  } else {
    Serial.println("No such file found.");
  }

  layout.replace("{content}", content);
  server.send(200, "text/html", layout);
}

void handleHome() {
  String header;
  String content;
  String layout;

  bool ok =  SPIFFS.begin();
  if(ok) {
    Serial.println("ok");
    bool exist = SPIFFS.exists("/form.html");
    if(exist) {

      File template_file = SPIFFS.open("/template.html", "r");
      layout = template_file.readString();
      template_file.close();

      Serial.println("The file exists!");
      File form = SPIFFS.open("/form.html", "r");

      if(!form) {
        Serial.println("Something went wrong trying to open the file ...");
      }

      int s = form.size();
      Serial.printf("Size = %d\r\n", s);
      String data = form.readString();

      Serial.println(data);
      content = data;
      form.close();
    }
  } else {
    Serial.println("No such file found.");
  }

  content += "<pre>";
  bool e = SPIFFS.exists("/network");
  if(e) {
      Serial.println("The file exists!");
      content += "The file exists!";
      File net = SPIFFS.open("/network", "r");
      if(!net) {
        Serial.println("Something went wrong trying to open the file ...");
        content += "Something went wrong trying to open the file ...";
      }
      String dataNet = net.readString();
      content += dataNet + "<br />";
      net.close();

  } else {
    Serial.println("No such file found.");
    content += "No such file found.";
  }
  content += "</pre>";

  layout.replace("{content}", content);

  server.send(200, "text/html", layout);

}

bool loadFromSpiffs(String path){
  Serial.println(path);
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
    else if(path.endsWith(".htm")) dataType = "text/html";
    else if(path.endsWith(".html")) dataType = "text/html";
    else if(path.endsWith(".css")) dataType = "text/css";
    else if(path.endsWith(".js")) dataType = "application/javascript";
    else if(path.endsWith(".png")) dataType = "image/png";
    else if(path.endsWith(".gif")) dataType = "image/gif";
    else if(path.endsWith(".jpg")) dataType = "image/jpeg";
    else if(path.endsWith(".ico")) dataType = "image/x-icon";
    else if(path.endsWith(".xml")) dataType = "text/xml";
    else if(path.endsWith(".pdf")) dataType = "application/pdf";
    else if(path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";
  server.streamFile(dataFile, dataType);
  dataFile.close();
  return true;
}

void handleNotFound() {
   if(loadFromSpiffs(server.uri())) return;
}

