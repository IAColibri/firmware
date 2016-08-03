#include <ESP8266WiFi.h>

#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <WiFiClient.h>

#include "FS.h"
#include "config.h"
#include "control.h"

#include <Wire.h>
#include "Adafruit_MCP23017.h"

bool ok;
String _log;
ESP8266WebServer server(80);

bool reset = false;

const int buttonPin = 0;
int relayPin = 2;

int buttonState = 0;
String status_button;
int low = 0;

bool configuration = true;

const int sensor = 3;
int sensorState;
/**
 WiFiServer TCPServer(81);
*/


Adafruit_MCP23017 mcp;

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  /* ***
  * Initialize GPIO00 resetButton  
  *** */
  pinMode(buttonPin, INPUT);

  /* ***
   * Initialize GPIO03 open/close sensor
   **** */
  pinMode(sensor, INPUT_PULLUP);

  /* ***
  * Initialize GPIO02 relayPin
  *** */
  pinMode(relayPin, OUTPUT);
  pinMode(relayPin, LOW);

  // Checking init mode 
  // can be AP_MODE
  // or CLIENT
  bool fs = SPIFFS.begin();
  if(fs) {
     ok = SPIFFS.exists("/ok");
     configuration = !ok;
     if(ok) {
      /* ***
       * if the configuration is ok
       * then open in device mode from control.h
       *** */
      bool start = initApp();
      if (start) {
       deviceWebServer();
      /**
      // my socket
       TCPServer.begin();
    */
      } else {
        Serial.begin(9600);
        Serial.println("ERROR - INIT");
        delay(1);
      }
    } else {
      /* ***
       * if configuration is not ok
       * then open configuration mode from config.h 
       *** */
      initConfig();  
      deviceConfigInterface();
    }
  } else {
    Serial.begin(9600);
    Serial.println("ERROR - open library FS.h");
    delay(1);
  }
}

void loop(void) {
  server.handleClient();

  /**
  WiFiClient client = TCPServer.available();
  if (client){
    Serial.println("Client connected");
    while (client.connected()){
      // Read the incoming TCP command
      String command = client.readStringUntil('\n');
      // Debugging display command
      command.trim();
      Serial.println(command);
    }
  }
  */

  // read sensor
  sensorState = digitalRead(sensor);
  Serial.print("sensor: ");
  Serial.println(sensorState);
  /***
  // read reset button */
  buttonState = digitalRead(buttonPin);
  if(buttonState == HIGH) {
    status_button = "high";
    Serial.println("0: " + status_button);
  } else {
    status_button = "low";   
    Serial.println("0: " + status_button);
    /**
    if(low > 5000) { 
      Serial.println("CLEAN!!");
      clean(); 
    }*/
    low++;
  }
  delay(1000);
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

  server.on("/on.html", handleOn);
  server.on("/off.html", handleOff);

  server.on("/sensor.html", handleSensor);

  server.onNotFound(handleNotFound);
  const char *headerkeys[] = {"User-Agent", "Cookie"};
  size_t headerkeyssize = sizeof(headerkeyssize)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
}

void handleIndex() {
  server.send(200, "text/html", layout("welcome"));
}

void handleSensor() {
  String layout;

  if(sensorState == HIGH) {
    layout = "CLOSED";
  } else {
    layout = "OPENED";
  }
  
  server.send(200, "text/html", layout);
}

void handleOn() {
  String layout = "on";
  pinMode(relayPin, HIGH);
  server.send(200, "text/html", layout);
}
void handleOff() {
  String layout = "off";
  pinMode(relayPin, LOW);
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
  server.on("/restart.html", handleRestart);

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

/* *************************
 * AP_MODE configuration mode
 * this section is for put Configuration mode pages.
 @url home.html / index.html / none
 @url save.html
 @url successfull page (successfull.html) / error page (error.html)
 *** */


void handleHome() {
    server.send(200, "text/html", layout("form"));
}

void handleRestart() {
  String layout = "restart";
  ESP.restart();
  server.send(200, "text/html", layout);
}

void handleConfigurationSave() {
  bool fs = SPIFFS.begin();

  if(fs) {
   File net = SPIFFS.open("/network", "w");
   for(int i = 0; i < server.args(); i++) {
     if((i == 1) && (server.args() == 10)) {
      net.print(",");
     }
     net.print(server.arg(i) + ",");
   }
   net.close();
  
   File ok_file = SPIFFS.open("/ok", "w");
   ok_file.print("true");
   ok_file.close();

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
/**
      if(file_name.equals("form")) {
        Serial.println("1");
        content = autocomplete(content);
      }*/

      main.close();

      String menu = "";
      if(configuration) {
        File menu_file = SPIFFS.open("/menu.html", "r");
        menu = menu_file.readString();
        menu_file.close();
      }

      layout.replace("{menu}", menu);
      layout.replace("{content}", content);

      if(sensorState == HIGH) {
        layout.replace("{status_door}", "CLOSED");
      } else {
        layout.replace("{status_door}", "OPENED");
      }

      layout.replace("{status_button}", status_button);
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

void clean() {
  bool fs = SPIFFS.begin();
  if(fs) {
   bool ok_file = SPIFFS.remove("/ok");
   if(ok_file) {
     reset = true;
   }
   ESP.restart();
  } else {
     error_open_file("ERROR - open SSPIFFS Library"); 
  }
}

/***
String autocomplete(String content) {
  bool fs = SPIFFS.begin();
  bool exist = SPIFFS.exists("/network");

  if(exist) {

    Serial.println("2");

    String row;
    File config = SPIFFS.open("/network", "r");
    row = config.readString();
    config.close();

    content.replace("{hostname}", "value=\"" + split(row, ',', 1)+ "\"");
    content.replace("{checked}", split(row, ',', 2));
    content.replace("{ip}", "value=\"" + split(row, ',', 3)+ "\"");
    content.replace("{subnet}", "value=\"" + split(row, ',', 4)+ "\"");
    content.replace("{gateway}", "value=\"" + split(row, ',', 5)+ "\"");
    content.replace("{dns}", "value=\"" + split(row, ',', 6)+ "\"");
    content.replace("{dns_2}", "value=\"" + split(row, ',', 7)+ "\"");
    content.replace("{ssid}", networks());
   // content.replace("{}", "value=\"" + split(row, ',', 9)+ "\""); 
    content.replace("{password}", "value=\"" + split(row, ',', 10)+ "\"");
    content.replace("{admin}", "value=\"" + split(row, ',', 11)+ "\"");

  } else {
    Serial.println("3");
    content.replace("{hostname}", "");
    content.replace("{checked}", "");
    content.replace("{ip}", "");
    content.replace("{subnet}", "");
    content.replace("{gateway}", "");
    content.replace("{dns}", "");
    content.replace("{dns_2}", "");
    content.replace("{ssid}", "");
   // content.replace("{}", "value=\"" + split(row, ',', 9)+ "\""); 
    content.replace("{password}", "");
    content.replace("{admin}", "");
  }
  return content;
}

String networks() {
 String ssids;
 int n = WiFi.scanNetworks();

  int indices[n];
  for (int i = 0; i < n; i++) {
    indices[i] = i;
  }

 for(int j=0; j < n; j++) {
   Serial.println(WiFi.SSID(indices[j]));
   Serial.println("\n");

   ssids += "<option>" + WiFi.SSID(indices[j]) + "</option>";
 }
 return ssids;
}
t*/
