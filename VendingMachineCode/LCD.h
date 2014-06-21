//LCD Library Developed by Charles Franklin for RFID Vending Machine

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

#ifndef LCD_h
#define LCD_h

#include <Arduino.h>
#include <inttypes.h>

namespace LCD 
{
  extern unsigned int autoidle;
  extern uint8_t updated;
  extern uint8_t first;
  extern unsigned int lines1;
  extern unsigned int lines2;
  extern unsigned int wait;
  extern unsigned int restpoint;
  extern unsigned int scroll;
  extern unsigned int remain;
  extern unsigned int alt_txt;
  extern String alt1;
  extern String alt2;
  extern String line1;
  extern String line2;
  extern String old_line1;
  extern String old_line2;
  extern String subline1;
  extern String subline2;
      
  void clear(void);
  void alt(String, String);
  void def(void);
  void reset(void);
  String convertBalance(long);
  void setup(void);
  
  void idle(void);
  void sessionCanceled(void);
  void sessionCanceled(long);
  void newAccountCreated(String);
  void cardTapped(long);
  void cardTapped(long, String);
  void addFunds(long);
  void addCredit(long);
  void balanceUpdated(long);
  void insufficientFunds(long);
  void price(String, long);
  void price(String, String, long);
  void price(String, long, unsigned int);
  void price(String, String, long, unsigned int);
  void largeBill(void);
  void timeOut(void);
  void timeOut(long);
  void print(String);
  void print(String, int);
  void print(String, String);
  void print(String, String, int);
      
  void run(void);
}

#endif
