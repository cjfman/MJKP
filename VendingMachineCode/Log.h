//Log Library Developed by Charles Franklin for RFID Vending Machine
//Assumes SD and Serial have been initialized

//(c) 2014 Charles Franklin

//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

// This Program uses a cooperative processesing kernal scheem
// A process is called by calling the run function of the process
// The process returns control when it is done
// Processes should return quickly and not run endless loops, 
// because the main loop will never regain control

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
