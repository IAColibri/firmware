String split(String line, char splitter, int index) {
  int init_flag = -1;
  int last_flag = 0;
  int buffer = 0;
  int count = 0;

  for(int i = 0; i < line.length(); i++){
     if(line.charAt(i) == splitter) {
      count++;
      if(count == index)  {
        if(buffer == 0) {
          init_flag = 0;
        } else {
          init_flag = buffer+1;
        }
        last_flag = i;
      }
      buffer = i;
     }
     if((i==(line.length()-1)) && (last_flag == 0)) {
      init_flag = buffer + 1;
      last_flag = i+1;
     }
   }
  return line.substring(init_flag, last_flag);
}

int counter(String line, char element) {
  int times = 0;
  for(int i=0; i<line.length(); i++) {
    if(line.charAt(i) == element) {
      times++;
    }
  }
  return times;
}

String user(String users, int id) {
  return split(users, '\n', id + 1);
}

IPAddress getIp(String ip_string){
  IPAddress ip ((uint8_t) split(ip_string, '.', 1).toInt(), 
          (uint8_t) split(ip_string, '.', 2).toInt(),  
          (uint8_t) split(ip_string, '.', 3).toInt(),  
          (uint8_t) split(ip_string, '.', 4).toInt());
  return ip;
}

bool remove_user(int id) {
 File users = SPIFFS.open("/users", "r");
 int count = 1;
 String content = users.readString();
 users.close();
 String user;

// bool remove = SPIFFS.remove("/users");
 bool remove = SPIFFS.remove("/users_1");
 File new_user = SPIFFS.open("/users_1", "a");
  while(count <= counter(content, '\n')) {
    user = split(content, '\n', count);
    if(count != (id+1)) {
      new_user.print(user + "\n");
    }
    count++;
  }
  new_user.close();
}

String file_log(String name) {
  String content;
  bool fs = SPIFFS.begin();
  if(SPIFFS.exists("/"+ name)) {
    File file = SPIFFS.open("/" + name, "r");
    content = file.readString();
    file.close();
  } else {
    content = "none\n";
  }
  return content;
}

void reset_device() {
  bool fs = SPIFFS.begin();
  if(fs) {
   bool ok_file = SPIFFS.remove("/ok");
   ESP.restart();
  } else {
    Serial.begin(9600);
    Serial.println("ERROR - open SSPIFFS Library"); 
  }
}



