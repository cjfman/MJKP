//LCD Library Developed by Charles Franklin for RFID Vending Machine
//
//(c) Charles Franklin 2012

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
