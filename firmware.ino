#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "FS.h"
#include "config.h"
#include "control.h"


ESP8266WebServer server(80);

void setup() {
  // Checking init mode 
  // can be AP_MODE
  // or CLIENT
  Serial.begin(115200);
  bool fs = SPIFFS.begin();
  if(fs) {
    bool ok = SPIFFS.exists("/ok");
     if(ok) {
      /* ***
       * if the configuration is ok
       * then open in device mode from control.h
       *** */
      initApp();
      deviceWebServer();
    } else {
      /* ***
       * if configuration is not ok
       * then open configuration mode from config.h 
       *** */
      initConfig();  
      deviceConfigInterface();
    }
  } else {
    Serial.begin(115200);
    Serial.println("ERROR - open library FS.h");
  }
}

void loop(void) {
  server.handleClient();
}

/** 
* Server device application
* @url "/" and "status.html" and "network.html"
* @url "update.html"
*/

void deviceWebServer() {
  server.on("/", handleIndex);
  server.on("/status.html", handleStatus);
  server.on("/network.html", handleConfigNetwork);
  server.on("/update.html", handleConfigUpdate);

  server.onNotFound(handleNotFound);
  const char *headerkeys[] = {"User-Agent", "Cookie"};
  size_t headerkeyssize = sizeof(headerkeyssize)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
}

void handleIndex() {
  String layout = "index";
  server.send(200, "text/html", layout);
}
void handleStatus() {
  String layout = "status";
  server.send(200, "text/html", layout);
}
void handleConfigNetwork() {
  String layout = "config network";
  server.send(200, "text/html", layout);
}
void handleConfigUpdate() {  
  String layout = "config update";
  server.send(200, "text/html", layout);
}


/** 
* Server the configurations urls
* @url "/" or "index.html"
* @url "save.html"
*/
void deviceConfigInterface() {
  server.on("/", handleHome);
  server.on("/save.html", handleConfigurationSave);

  server.onNotFound(handleNotFound);
  const char *headerkeys[] = {"User-Agent", "Cookie"};
  size_t headerkeyssize = sizeof(headerkeyssize)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
}

bool loadFromSpiffs(String path){
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


/**************************
 * AP_MODE configuration mode
 * this section is for put Configuration mode pages.
 @url home.html / index.html / none
 @url save.html
 @url successfull page (successfull.html) / error page (error.html)
 *** */


void handleHome() {
    server.send(200, "text/html", layout("form"));
}

void handleConfigurationSave() {
  bool ok = SPIFFS.begin();
  if(ok) {
   File net = SPIFFS.open("/network", "w");
   for(int i = 0; i < server.args(); i++) {
     net.print(server.arg(i) + ",");
   }
   net.close();
   
   server.send(200, "text/html", layout("save"));
  } else {
     error_open_file("ERROR - open SSPIFFS Library"); 
  }
}

String layout(String file_name) {
  String layout;
  String content;
  bool check = false;

  bool ok = SPIFFS.begin();
  if(ok) {
  check = SPIFFS.exists("/"+ file_name +".html");

  if(check) {
    File template_file = SPIFFS.open("/template.html", "r");
    layout = template_file.readString();
    template_file.close();

    File main =  SPIFFS.open("/"+ file_name + ".html", "r");

    int size = main.size();
    content = main.readString();
    main.close();

    layout.replace("{content}", content);
  } else {
      return "Exception - No such file found. ["+file_name+"]" ;
  }

  return layout;
  } else {  
    return "ERROR - open SSPIFFS Library"; 
  }

}


void error_open_file(String text) {
  server.send(200, "text/html", text);
}


