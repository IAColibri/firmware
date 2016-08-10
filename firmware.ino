#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "FS.h"
#include "config.h"
#include "control.h"

bool ok;
String _log;
ESP8266WebServer server(80);

const int buttonPin = 0;
int relayPin = 3;

int buttonState = 0;
int low = 0;
bool configuration = true;

const int sensor = 2;
int sensorState;

void setup() {
  Serial.begin(9600);
  // Checking init mode 
  // can be AP_MODE
  // or CLIENT
  bool fs = SPIFFS.begin();
  if(fs) {
     ok = SPIFFS.exists("/ok");
     configuration = !ok;
     if(ok) {
      /* ***
      * Initialize GPIO00 resetButton  
      *** */
      pinMode(buttonPin, INPUT_PULLUP);

      /* ***
       * Initialize GPIO02 open/close sensor
       **** */
      pinMode(sensor, INPUT);

      /* ***
      * Initialize GPIO03 relayPin
      *** */
      pinMode(relayPin, OUTPUT);
      pinMode(relayPin, LOW);

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

  // read sensor
  sensorState = digitalRead(sensor);
  
  /***
  // read reset button */
  buttonState = digitalRead(buttonPin);
  if(buttonState == HIGH) {
  } else {
    if(low > 5000) { 
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
String admin_password() {
  String row;
  bool fs = SPIFFS.begin();
  bool exist = SPIFFS.exists("/network");
  if(exist) {
    File config = SPIFFS.open("/network", "r");
    row = config.readString();
    config.close();
  } else {
    return "Error";
  }
  String password = split(row, ',', 11);
  _log += password + "<br />";
  return password;
}

bool is_authentified(){
  if (server.hasHeader("Cookie")){
    String cookie = server.header("Cookie");
    _log += "~~ " + cookie + " ~~ <br />";
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      _log += "auth true";
      return true;
    }
  }
  _log += "auth false";
  return false;
}

bool auth(String username, String password) {
  String row;

  if((username == "admin") && (password == admin_password())) {
    _log = "true <br />";
    return true;
  } else {
    File users = SPIFFS.open("/users", "r");
     if(!users) {
        _log = "false <br />";
        return false;
     } else {
      while(users.available()) {
        row =  users.readString();
        _log += username + " -> "+ split(row, ',', 1) + " :: " + password + " -> " + split(row, ',', 2) + "<br />";
        if((username == split(row, ',', 1)) && (password == split(row, ',', 2))) {
          _log += "~~>" +  split(row, ',', 1)+" && " + split(row, ',', 2) + "<br />";
          _log = "true <br />";
          return true;
        }
      }
    } 
    users.close();
  }
  _log = "false <br />";
  return false;
}

void deviceWebServer() {

  server.on("/", handleWelcome);
  server.on("/login.html", handleLogin);
//  server.on("/status.html", handleStatus);
//  server.on("/network.html", handleConfigNetwork);
//  server.on("/update.html", handleConfigUpdate);
  server.on("/user-manager.html", handleUserManager);
  server.on("/user-save.html", handleUserSave);
  server.on("/push_button.html", handlePushButton);
  server.on("/on.html", handleOn);
  server.on("/off.html", handleOff);
  server.on("/sensor.md", handleSensor);
  server.on("/button.md", handleButton);
  server.onNotFound(handleNotFound);

  const char *headerkeys[] = {"User-Agent", "Authorization"};
  size_t headerkeyssize = sizeof(headerkeyssize)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);

  server.begin();
}

void handleLogin() {
  String msg;

  const char *headerkeys[] = {"User-Agent", "Cookie", "Authorization"};
  size_t headerkeyssize = sizeof(headerkeyssize)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);
  for (int i = 0; i < 10; ++i) {
     _log += "-- " + server.headerName(i) + " :: " + server.header(i) +"-- <br />";
   }

  if (server.hasHeader("Cookie")){
    String cookie = server.header("Cookie");
    _log += "<>" + cookie + "<br />";
  }

  if (server.hasArg("DISCONNECT")){
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n"; 
    server.sendContent(header);
    return;
  }

  if (server.hasArg("username") && server.hasArg("password")){
    if (auth(server.arg("username"), server.arg("password"))){
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      return;
    }
    msg = "Wrong username/password! try again.";
  }
  server.send(200, "text/html", layout("login"));
}

bool validate() {
  String header;
  if (!is_authentified()){
    server.sendHeader("Location","/login.html");
    _log += "(validate) Error<br />";
    server.sendHeader("Cache-Control","no-cache");
    server.send(301);
    return false;
  }
  return true;
}

void handleWelcome() {
  if(!validate()) { return; }

  _log += "(handleWelcome) Good<br />";
  server.send(200, "text/html", layout("welcome"));
}

void handleUserManager() {
  if(!validate()) { return; }

  _log += "(handleUserManager) Good<br />";
  server.send(200, "text/html", layout("user-manager"));
}

void handleUserSave() {
  if(!validate()) { return; }

  bool fs = SPIFFS.begin();
  String line = "";
  if(fs) {
   File users = SPIFFS.open("/users", "a");
   for(int i = 0; i < server.args(); i++) {
    line += server.arg(i) + ",";
   }
   users.print(line + "\n");
   users.close();
   server.send(200, "text/html", layout("user-save"));
  } else {
     error_open_file("ERROR - open SSPIFFS Library"); 
  }
}

String listUsers() {
  bool fs = SPIFFS.begin();
  String listUsers = "";
  int count = 0;
  String auxiliar;
  String row;

  File html = SPIFFS.open("/list-users.html", "r");
  String design = html.readString(); 
  html.close();

  File users = SPIFFS.open("/users", "r");
  if(!users) {
    return "<li>NO USERS.</li>";
  } else {
    while(users.available()) {
      auxiliar = design;
      row =  users.readStringUntil('\n');

      auxiliar.replace("{id}", String(count)); 
      auxiliar.replace("{name}", split(row, ',', 1));
      auxiliar.replace("{mac}", split(row, ',', 4));

      listUsers += auxiliar + "\n";
    count++;
    }
  }
  users.close();
  return listUsers;
}

void handlePushButton() {
  if(!validate()) { return; }
  
  pinMode(relayPin, HIGH);
  delay(1000);
  pinMode(relayPin, LOW);
  delay(1000);
  String layout;
  layout = "CONFIRM\n";

  server.send(200, "text/html", layout);
}

void handleButton() {
  if(!validate()) { return; }

  String layout;
  if(buttonState == HIGH) {
    layout = "NO PRESSED";
  } else {
    layout = "PRESSED";
  }
  server.send(200, "text/html", layout);
}

void handleSensor() {
  if(!validate()) { return; }

  String layout;

  if(sensorState == HIGH) {
    layout = "CLOSED";
  } else {
    layout = "OPENED";
  }
  
  server.send(200, "text/html", layout);
}

void handleOn() {
  if(!validate()) { return; }

  String layout = "on";
  pinMode(relayPin, HIGH);
  server.send(200, "text/html", layout);
}
void handleOff() {
  if(!validate()) { return; }
   
  String layout = "off";
  pinMode(relayPin, LOW);
  server.send(200, "text/html", layout);
}

/**
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
*/

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
    else if(path.endsWith(".md")) dataType = "text/html";

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

      if(file_name.equals("user-manager")) {
        content.replace("{list-users}", listUsers());
      }

      main.close();

      String menu = "";
      if(!configuration) {
        File menu_file = SPIFFS.open("/menu.html", "r");
        menu = menu_file.readString();
        menu_file.close();
      }

      if(file_name.equals("login")) {
        layout.replace("{menu}", "");
        layout.replace("{status_door}", "-");
        layout.replace("{status_button}", "-");
        for(int i = 0; i < server.args(); i++) {
           _log += "~> " + server.argName(i) + ": " + server.arg(i)+ "<br />";
        }
        content = _log +"<br />"+ server.arg("username")+","+ server.arg("password") + "\n" + content;
      }

      layout.replace("{menu}", menu);
      layout.replace("{content}", content);

      if(sensorState == HIGH) {
        layout.replace("{status_door}", "CLOSED");
      } else {
        layout.replace("{status_door}", "OPENED");
      }

      if(buttonState == HIGH) {
        layout.replace("{status_button}", "NO PRESSED");
      } else {
        layout.replace("{status_button}", "PRESSED");
      }
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
   }
   ESP.restart();
  } else {
     error_open_file("ERROR - open SSPIFFS Library"); 
  }
}

/*
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
*/
  /**
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
