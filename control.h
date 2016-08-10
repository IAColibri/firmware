#include "tools.h"

String localIP;

bool initApp() {
  const int buttonPin = 0;
  int buttonState = HIGH;

  WiFi.mode(WIFI_STA);

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

  String hostname = split(row, ',', 1);

  IPAddress ip = getIp(split(row, ',', 3));
  IPAddress gateway = getIp(split(row, ',', 5));
  IPAddress subnet = getIp(split(row, ',', 4));

  String ssid = split(row, ',', 8);
  String password = split(row, ',', 10);

  // detail verbosity
  Serial.begin(9600);
  Serial.println(row + "\n");
  Serial.println("\n" + hostname + "\n");
  Serial.println("\n" + ip.toString() + "\n");
  Serial.println("\n" + gateway.toString() + "\n");
  Serial.println("\n" + subnet.toString() + "\n");
  Serial.println("\n" + ssid + "\n");
  Serial.println("\n" + password + "\n"); 
  
  WiFi.hostname(hostname);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid.c_str(), password.c_str());

  int times = 0;
  Serial.begin(9600);
  while((WiFi.status() != WL_CONNECTED) && (buttonState == HIGH)) {
    delay(500);
    Serial.println("<<"+ String(times) + ">>");
    buttonState = digitalRead(buttonPin);
    times++;
  }
 
  localIP = WiFi.localIP().toString();

  /** detail verbosity
  Serial.println("\n" + localIP +  "\n");
  */
  return true;
}

