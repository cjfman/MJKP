//Micheal Jackson, The King of Pop
//Vending Machine code
//Developed by Charles Franklin for RFID Vending Machine

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


// Pin Usage
// [0, 1, 4, 10, 11, 12, 13, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30]
// [31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 43, 45] 
// [A5, A8, A9, A10, A11, A12, A13, A14] 
//
// LCD [A8, A9, A10, A11, A12, A13, A14]
// SD and Ethernet [4, 10, 11, 12, 13]
// Serial [0, 1]
// MPCMSerial3 [16, 17]
// Relay array [34, 35, 36, 37, 38, 39, 40, 41, 43, 45]
// Input Latch [22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33]
// Buttons [21, A5]

#include <inttypes.h>
#include <LiquidCrystal.h>
#include <SD.h>
#include "MDB.h"
#include "Log.h"
#include "LCD.h"
#include "Vend.h"
#include "Accounts.h"
#include "RFID.h"

void coinReturn();

void setup()
{
  // Serial
  Serial.begin(9600);
  Serial.println("Starting up...");
  
  // LCD
  LCD::setup();
  LCD::print("Settign up...");
  LCD::run();
  
  // Accounts
  Serial.println("Begin Accounts Setup");
  LCD::print("Settign up...", "...Accounts");
  LCD::run();
  Accounts::setup();
  Serial.println("Accounts Setup");
 
  // Log
  //Log::start("log.txt");
  
  // MDB
  LCD::print("Settign up...", "...MDB");
  LCD::run();
  MDB::reset();
  
  // Vend
  LCD::print("Settign up...", "...Hardware");
  LCD::run();
  Vend::setup();
  
  // RFID
  LCD::print("Settign up...", "...RFID");
  LCD::run();
  RFID::setup();
  attachInterrupt(0, RFID::dataLow, FALLING);
  attachInterrupt(1, RFID::dataHigh, FALLING);
  
  LCD::print("Setup done");
  LCD::run();
  
  LCD::idle();
  Log::print("Setup Complete");
  
  //Coin Return
  pinMode(21, INPUT);
  digitalWrite(21, HIGH);
  attachInterrupt(2, coinReturn, FALLING);
}

void loop()
{
  MDB::run();
  RFID::run();
  Vend::run();
  LCD::run();
  Log::run();
}


/************************************************************************************
* * * * * * * * * * * * * * * * * * * Support Functions * * * * * * * * * * * * * * *  
************************************************************************************/

void coinReturn()
{
  static unsigned long time = 0;
  unsigned long current = millis();
  if (time + 2000 < current && !MDB::money_hold)
  {
    MDB::money_hold = true;
    time = current;
    MDB::coinReturn();
    Log::print("Coin Return");
    //LCD::print("Returning Money", 3);
  }
}
