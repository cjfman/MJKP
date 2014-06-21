//Vending Machine RFID Controller Library 
//Developed by Charles Franklin for RFID Vending Machine
//Assumes Serial has been initialized

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
