//Vending Machine Hardware Controller Library 
//Developed by Charles Franklin for RFID Vending Machine
//Assumes Serial have been initialized

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

// Because there are 10 soda types, each signal is ten bits wide
//Vend Buttons are locaded on all of latch 0 and latch 1 inputs 0, 1
//Sold Out Switches are locaded on latch 1 inputs 2, 3, 4, 5, 6, 7 
//   and latch 2 inputs 0, 1, 2, 3
//Vend Complete Switches are on latch 2 inputs 4, 5, 6, 7
//   and latch 3 inputs 0, 1, 2, 3, 4, 5

#ifndef VEND_h
#define VEND_H

#include <inttypes.h>
#include <Arduino.h>

namespace Vend
{
  // State Definitions
  const uint8_t SETUP = 0;
  const uint8_t IDLE = 1;
  const uint8_t MONEY = 2;
  const uint8_t VERIFY = 3;
  
  // Variables
  extern uint8_t state;
  extern bool enabled;
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
