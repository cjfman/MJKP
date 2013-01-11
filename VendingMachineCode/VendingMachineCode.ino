//Micheal Jackson, The King of Pop
//Vending Machine code
//Developed by Charles Franklin for RFID Vending Machine
//
//(c) Charles Franklin 2012

// This Program uses a cooperative processesing kernal scheem
// A process is called by calling the run function of the process
// The process returns control when it is done
// Processes should return quickly and not run endless loops, 
// because the main loop will never regain control


// Pin Usage
// [0, 1, 4, 10, 11, 12, 13, 22, 23, 24, 25, 26, 27, 28, 29, 30]
// [31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 43, 45] 
// [A8, A9, A10, A11, A12, A13, A14] 
//
// LCD [A8, A9, A10, A11, A12, A13, A14]
// SD and Ethernet [4, 10, 11, 12, 13]
// Serial [0, 1]
// MPCMSerial3 [16, 17]
// Relay array [34, 35, 36, 37, 38, 39, 40, 41, 43, 45]
// Input Latch [22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33]

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
  Accounts::setup();
 
  // Log
  Log::start("log.txt");
  
  // MDB
  MDB::reset();
  
  // Vend
  Vend::setup();
  
  // RFID
  RFID::setup();
  attachInterrupt(0, RFID::dataLow, FALLING);
  attachInterrupt(1, RFID::dataHigh, FALLING);
  
  LCD::idle();
  Log::print("Setup Complete");
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
  if (time + 2000 < current)
  {
    time = current;
    MDB::coinReturn();
    Log::print("Coin Return");
  }
}
