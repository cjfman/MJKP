//Vending Machine RFID Controller Library 
//Developed by Charles Franklin for RFID Vending Machine
//Assumes Serial has been initialized
//
//(c) Charles Franklin 2012

#ifndef RFID_h
#define RFID_h

#include <inttypes.h>
#include "Accounts.h"

namespace RFID
{
  // State Declarations
  const uint8_t IDLE = 1;
  const uint8_t CLOSE = 2;
  const uint8_t TAPPED = 3;
  const uint8_t DEPOSIT = 4;
  const uint8_t VERIFY = 5;
  
  // RFID Variables
  extern uint8_t state;
  extern volatile unsigned long rfid_data;
  extern volatile uint8_t received;
  
  extern Accounts account;
  extern unsigned long temp_id;
  
  extern uint8_t queue;
  
  // Main Functions
  void setup();
  void run();
  
  // State Functions
  void idle();
  void close();
  void tapped();
  void deposit();
  void verify();
  
  // RFID Functions
  void dataLow();
  void dataHigh();
  boolean ready();
  
  // Utility Functions
  boolean timeout(unsigned long);
  void vend();
}

#endif
