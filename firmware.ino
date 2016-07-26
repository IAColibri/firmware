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
  bool fs = SPIFFS.begin();

  bool ok;
  if(fs) {
    ok = SPIFFS.exists("/ok");
  }

  if(ok) {
    initApp();
    deviceServer();
  } else {
    initConfig();  
  }
}

void loop(void) {
  server.handleClient();
}

void deviceServer() {
  server.on("/", handleHome);
  server.on("/data.html", handleConfigurationData);
}

void handleHome() {}
void handleConfigurationData() {}
