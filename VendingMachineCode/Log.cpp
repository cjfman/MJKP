//Log Library Developed by Charles Franklin for RFID Vending Machine
//Assumes SD and Serial have been initialized
//
//(c) Charles Franklin 2012


#include <stdlib.h>
#include "Log.h"

namespace Log
{
  char *logdata;
  char file_name[14];
  int size;
  int max;
  int used;
  boolean enabled = false;
  
  void start(char* name)
  {
    size = 512;
    max = 480; //The max size before auto save
    used = 0;
    logdata = (char*)malloc( size);
    strcpy(name,  file_name);
    File logfile = SD.open(file_name, FILE_WRITE);
    if (!logfile.size())
    {
      //Create Log If It Doesn't Exist and print header
      logfile.println("MJKP Activity Log\n");
    }
    logfile.println("Starting up...");
    logfile.close();
    enabled = true;
  }
  
  void print(char* entry)
  {
    // Takes entry and adds it to the log buffer
    
    Serial.println(entry);
    
    if (!enabled)
    {
      return;
    }
    
    int len = strlen(entry) + 1; // Incude space for newline char
    
    // Forces log to save if not enough space
    if(len +  used  >  size)
    {
      save();
    }
    
    // Save to SD card now
    if (len > size)
    {
      File logfile = SD.open(file_name, FILE_WRITE);
      logfile.println(entry);
      logfile.close();
      return;
    }
    
    strcat(logdata, entry);
    strcat(logdata, "\n");
    used =  used + len; // Updates log length
  }
  
  void print(String entry)
  {
    // Gets c type string from Arduino string and calls normal
    // log function
    
    int i = entry.length() + 1;
    char temp[i];
    entry.toCharArray(temp, i);
    print(temp);
  }
  
  void save(void)
  {
    File logfile = SD.open(file_name, FILE_WRITE);
    logfile.println(logdata);
    logfile.close();
    logdata[0] = '\0';
    used = 0;
  }
  
  void run(void)
  {
    if( used >  max)
    {
      save();
    }
  }
  
  int strlen(char* str)
  {
    // Returns length of string
    
    int i;
    for(i = 0; str[i] != '\0'; i++);
    return i + 1;
  }
}
