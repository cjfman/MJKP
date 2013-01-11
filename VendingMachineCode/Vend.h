//Vending Machine Hardware Controller Library 
//Developed by Charles Franklin for RFID Vending Machine
//Assumes Serial have been initialized
//
//(c) Charles Franklin 2012

//Vend Buttons are locaded on latch 0 and latch 1 inputs 0, 1
//Sold Out Switches are locaded on latch 1 inputs 2, 3, 4, 5, 6, 7 
//   and latch 2 inputs 0, 1, 2, 3
//Vend Complete Switches are on latch 2 inputs 4, 5, 6, 7
//   and latch 3 inputs 0, 1, 2, 3, 4, 5

#ifndef VEND_h
#define VEND_H

#include "MDB.h"
#include "LCD.h"
#include "Log.h"
#include <inttypes.h>

namespace Vend
{
  // State Definitions
  const uint8_t SETUP = 0;
  const uint8_t IDLE = 1;
  const uint8_t MONEY = 2;
  const uint8_t VERIFY = 3;
  
  // Variables
  extern uint8_t state;
  extern boolean enabled;
  extern String sodas[10];
  extern unsigned long prices[10];
  extern uint8_t soda_enable[10];
  extern unsigned long max_price;
  extern uint8_t queue;
  
  // Functions
  void setup();
  void run();
  void enable();
  boolean disable();
  
  // State Functions
  void idle();
  void money();
  void verify();
  
  // Vend Function
  boolean vend(uint8_t);
  void vendFailue(uint8_t);
  
  // Utilities
  int buttonPush();
  int soldOut(int);
  void latch(uint8_t);
  void relatch();
  uint8_t timeout(unsigned long);
  void resetMotor(int);
}

#endif
