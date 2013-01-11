//Log Library Developed by Charles Franklin for RFID Vending Machine
//Assumes SD and Serial have been initialized
//
//(c) Charles Franklin 2012

#ifndef LOG_h
#define LOG_h

#include <Arduino.h>
#include <inttypes.h>
#include <string.h>
#include <SD.h>

namespace Log
{
  extern char *logdata;
  extern char file_name[14];
  extern int size;
  extern int max;
  extern int used;
  extern boolean enabled;
  
  void start(char*);
  void print(char*);
  void print(String);
  void run(void);
  void save(void);
  
  int strsize(char*);
}

#endif
