#include <ESP8266WiFi.h>

#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

#include "FS.h"
#include "config.h"
#include "control.h"

bool ok;
String _log;
ESP8266WebServer server(80);
bool reset = false;


const int buttonPin = 0;
int buttonState = 0;
String status_button;
int low = 0;


WebSocketsServer web_socket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
    break;
    case WStype_CONNECTED:
      {
        IPAddress ip = web_socket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        web_socket.sendTXT(num, "Connected");
      }
    break;
    case WStype_TEXT:
     Serial.printf("[%u] get Text: %s\r\n", num, payload);
    // send message to client
    // webSocket.sendTXT(num, "message here");
    // send data to all connected clients
    // webSocket.broadcastTXT("message here");
    break;
    case WStype_BIN:
     Serial.printf("[%u] get binary length: %u\r\n", num, length);
    break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
    break;
  }

}


void setup() {
  Serial.begin(9600);
  // Checking init mode 
  // can be AP_MODE
  // or CLIENT
  bool fs = SPIFFS.begin();
  if(fs) {
     ok = SPIFFS.exists("/ok");
     if(ok) {
      /* ***
       * if the configuration is ok
       * then open in device mode from control.h
       *** */
      bool start = initApp();
      if (start) {

       deviceWebServer();

       /* ****
        * Start Web Socket
        **** */
       web_socket.begin();
       web_socket.onEvent(webSocketEvent);

       /* ***
       * Initialize GPIO02 resetButton  
       *** */
       pinMode(buttonPin, INPUT);
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
  web_socket.loop();

  // read reset button
   buttonState = digitalRead(buttonPin);
  if(buttonState == HIGH) {
    status_button = "high";
  } else {
    status_button = "low";   
    if(low > 50) { 
      Serial.println("CLEAN!!");
      clean(); 
    }
    low++;
  }
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
  server.send(200, "text/html", layout("welcome"));
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
  bool fs = SPIFFS.begin();

  if(fs) {
   File net = SPIFFS.open("/network", "w");
   for(int i = 0; i < server.args(); i++) {
     if((i == 1) && (server.args() == 9)) {
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
    main.close();
    content.replace("{reset_status}", status_button);

    layout.replace("{content}", content);
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


