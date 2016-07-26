String split(String line, char splitter, int index) {
  int init_flag = -1;
  int last_flag = 0;
  int count = 0;

  for(int i = 0; i < line.length(); i++){
     if(line.charAt(i) == splitter) {
      count++;

      if((count == index)&&(init_flag == -1)) {
        init_flag = i;
      }

      if((count == index)&&(init_flag != -1)) {
        last_flag = i;
      }

     }
   }

  return line.substring(init_flag, last_flag);
}
