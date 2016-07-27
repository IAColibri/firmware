String split(String line, char splitter, int index) {
  int init_flag = -1;
  int last_flag = 0;
  int buffer = 0;
  int count = 0;

  for(int i = 0; i < line.length(); i++){
     if(line.charAt(i) == splitter) {
      count++;
      if(count == index)  {
        init_flag = buffer+1;
        last_flag = i;
      }
      buffer = i;
     }
   }

  return line.substring(init_flag, last_flag);
}
