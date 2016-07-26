#include <ESP8266WiFi.h>
#include "tools.h"

String localIP;

bool initApp() {

  bool fs = SPIFFS.begin();
  bool exist = SPIFFS.exists("/network");

  String row;
  if(exist) {
    File config = SPIFFS.open("/network", "r");
    row = config.readString();
    config.close();
  } else {
    return false;
  }

  String ssid = split(row, ',', 7);
  String password = split(row, ',', 9);

  WiFi.begin(ssid.c_str(), password.c_str());
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
 
  localIP = WiFi.localIP().toString();

  return true;
}



