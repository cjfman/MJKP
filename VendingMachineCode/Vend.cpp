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

#include "Vend.h"
#include "Accounts.h"
#include "MDB.h"
#include "LCD.h"
#include "Log.h"

namespace Vend
{
  uint8_t state; 
  bool enabled; 
  String sodas[10];
  unsigned long prices[10];
  uint8_t soda_enable[10];
  unsigned long max_price;
  uint8_t queue;
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * *  Setup Functions  * * * * * * * * * * * * * * * * *
  ************************************************************************************/
  
  
  void setup()
  {
    // This function sets up the Vend hardware and functions
    Log::print("Setting up Vend");
    
    // Setup Variables
    state = IDLE;
    max_price = 0;
    uint8_t queue = 10;
    enabled = true;
    queue = 10;
    
    // Setup Free switch
    pinMode(A5, INPUT);
    digitalWrite(A5, HIGH);
    
    // Setup the Digital I/O pins
    int i;
    for (i = 22; i < 26; i++)
    {
      // Decoder and latch clock pin
      pinMode(i, OUTPUT);
      digitalWrite(i, HIGH);
    }
    for (i = 26; i < 34; i++)
    {
      // 8-bit buffer for input latches
      pinMode(i, INPUT);
      digitalWrite(i, HIGH);
    }
    for (i = 34; i < 41; i = i + 2)
    {
      // First bank of relays
      pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
    }
    for (i = 35; i < 46; i = i + 2)
    {
      // Second back of relays
      pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
    }
    
    // Loop through all ten sodas and run setup
    for (i = 0; i < 9; i++)
    {
      Accounts soda = Accounts(i);
      if (soda.getName() == "") {
        prices[i] = 0;
        sodas[i] = "Soda " + String(i + 1);
        soda_enable[i] = 0;
      }
      else {
        prices[i] = soda.getBalance();
        sodas[i] = soda.getName();
        soda_enable[i] = 1;
      }
      // Hunt for the soda with the greatest price
      if (prices[i] > max_price)
      {
        max_price = prices[i];
      }
      
      // Test motors. Reset them to their default position
      // If a motor fails to reset, disable that soda
      Log::print("...motor: " + String(i));
      resetMotor(i);
    }
    soda_enable[9] = false;
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * *  Main Function  * * * * * * * * * * * * * * * * *
  ************************************************************************************/
  
  void run()
  {
    if (!enabled)
    {
      return;
    }
    
    switch (state)
    {
    case SETUP:
      setup();
      break;
    case IDLE:
      idle();
      break;
    case MONEY:
      money();
      break;
    case VERIFY:
      verify();
      break;
    }
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * *  Enable Functions * * * * * * * * * * * * * * * * *
  ************************************************************************************/
  
  void enable()
  {
    if (enabled) return;
    enabled = true;
    LCD::idle();
    MDB::all();
  }
  
  boolean disable()
  {
    if (state == VERIFY)
    {
      return false;
    }
    enabled = false;
    state = IDLE;
    return true;
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * *  State Functions  * * * * * * * * * * * * * * * * *
  ************************************************************************************/
  
  void idle()
  {
    // This Function represents the idle state
    // This state runs when the vending machine is waiting for input
    //LCD::idle();
    
    unsigned long funds = MDB::bill_funds + MDB::coin_funds;
    boolean escrow = MDB::escrow;
    
    if (MDB::escrow >= 500)
    {
      Vend::disable();
      LCD::largeBill();
      return;
    }
    else if (funds || escrow)
    {
      state = MONEY;
      timeout(0);
      return;
    }
    
    uint8_t soda = buttonPush();
    
    if (soda != 10)
    {
      // A button was pushed
      if (!digitalRead(A5)) 
      {
        vend(soda);
        LCD::idle();
      }
      if (soldOut(soda))
      {
        LCD::print(sodas[soda], "Sold Out", 3);
        return;
      }
      LCD::price(sodas[soda], prices[soda], 3);
    }
    
    return;
  }
  
  void money()
  {
    // This function runs when money has been depositied into the machine
    // It can accept more money, or request a vend.
    // If a bill is still in escrow, the state will transistion to VERIFY
    // first.
    
    unsigned long funds = MDB::bill_funds + MDB::coin_funds;
    boolean escrow = MDB::escrow;
    
    LCD::addFunds(funds + escrow);
    
    // Block further money input if current funds is >= max_price
    if (funds + escrow >= max_price)
    {
      MDB::none();
      LCD::price("Reached MaxPrice", "Added: ", funds + escrow);
    }
    
    // Stack the bill if total money < most expensive soda
    else if (escrow && funds + escrow < max_price)
    {
      MDB::stack();
    }
    
    // Check to see if no money
    if (funds + escrow == 0)
    {
      MDB::all();
      state = IDLE;
      LCD::idle();
      return;
    }
    
    uint8_t soda = buttonPush();
    // No button was pushed
    if (soda == 10)
    {
      return;
    }
    
    // Check to see if sold out
    if (soldOut(soda))
    {
      LCD::print(sodas[soda], "Sold Out", 3);
      return;
    }
    
    // Check to see if enough money
    if (funds + escrow < prices[soda])
    {
      LCD::price(sodas[soda], prices[soda], 3);
      return;
    }
    
    MDB::money_hold = true;
    
    // Check again because of coin return
    if (funds + escrow < prices[soda])
    {
      LCD::price(sodas[soda], prices[soda], 3);
      return;
    }
    
    // If money in escrow, state >> VERIFY
    // set queue and stack bill
    if (escrow)
    {
      Log::print("Verify Escrow");
      MDB::stack();
      queue = soda;
      state = VERIFY;
      return;
    }
    
    Log::print("Return Change");
    if (!vend(soda)) {
      MDB::giveChange(prices[soda]);        // Refund Money
    }
    else {
      MDB::giveChange(funds - prices[soda]); // Calculate Change
    }
    MDB::money_hold = false;
    MDB::all();
    state = IDLE;
    LCD::idle();
  }
  
  void verify()
  {
    // This function waits for the escrow to be stacked
    // When the escrow is stacked, it calls vend
    // If the escrow is not stacked, state >> Money
    
    unsigned long funds = MDB::bill_funds + MDB::coin_funds;
    unsigned long escrow = MDB::escrow;
    
    // If the escrow has been stacked, the value of escrow
    // should be 0;
    if (escrow)
    {
      if (timeout(10000))
      {
        MDB::money_hold = false;
        Log::print("Bill Stack Error");
        MDB::stack();
        state = MONEY;
        timeout(0);
      }
      return;
    }
    timeout(0);
    
    if (funds < prices[queue])
    {
      MDB::money_hold = false;
      state = MONEY;
      return;
    }
    
    Log::print("Verified\nReturn Coins");
    if (!vend(queue)) {
      MDB::giveChange(prices[queue]);        // Refund Money
    }
    else {
      MDB::giveChange(funds - prices[queue]); // Calculate Change
    }
    MDB::money_hold = false;
    queue = 10;
    MDB::all();
    state = IDLE;
    LCD::idle();
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * *  Vend Function * * * * * * * * * * * * * * * * * *
  ************************************************************************************/
  
  boolean vend(uint8_t soda)
  {
    // This function takes a soda and controls the hardware to vend the soda
        
    LCD::print("VEND");
    LCD::run();
    
    // Check for valid value
    if (soda >= 10)
    {
      return false;
    }
    
    int l;        // Latch to be used
    int m;        // Motor to run
    int r;        // Input to read       
    
    // Vending from set 1
    if (soda < 4)
    {
      //digitalWrite(queue * 2 + 34, HIGH);
      //latch(2);
      m = soda * 2 + 34;
      l = 2;
      r = soda + 30;
    }
    // Vending from set 2
    else
    {
      //digitalWrite(queue * 2 + 27, HIGH);
      //latch(3);
      m = soda * 2 + 27;
      l = 3;
      r = soda + 22;
    }
    
    digitalWrite(m, HIGH); // Start the motor
    delay(200);            // Give motor chance to start
    latch(l);              // Latch to get the mode of the motor
    
    
    /*
    if(!digitalRead(r))
    {
      vendFailue(soda);
      digitalWrite(m, LOW);
      Serial.println(__LINE__);
      return false;
    }
    //*/
    //*
    timeout(0);
    while (!digitalRead(r))
    {
      if (timeout(4000))
      {
        vendFailue(soda);
        digitalWrite(m, LOW);
        timeout(0);
        Serial.println(__LINE__);
        return false;
      }
      
      
      delay(200);   // Debounce
      relatch();    // Latch and try again
    }
    timeout(0);
    //*/
    
    // Turn motor off when it is back to default position
    int done = 0;
    int temp;
    delay(200);   // Debounce
    while (!done || timeout(5000))
    {
      relatch();
      temp = digitalRead(r);
      digitalWrite(m, temp);  // Set motor to it's next state now
      done = !temp;
      if (timeout(7000))
      {
        digitalWrite(m, LOW);  // Turn off motor
        break;
      }
    }
    timeout(0);
    
    // If timeout, report motor jam and disable soda
    if (!done)
    {
      vendFailue(soda);
      return false;
    }
    return true;
  }
  
  void vendFailue(uint8_t soda)
  {
    Log::print("Motor Jam: " + String(soda));
    LCD::print("Vend Failure", 7);
    soda_enable[soda] = 0;
    /*
    if (!digitalRead(A5)) {
      MDB::giveChange(prices[soda]);
    }
    //*/
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * * * *  Utility Functions  * * * * * * * * * * * * * * 
  ************************************************************************************/
  
  int buttonPush()
  {
    // This function checks the array of front buttons and returns which button is being pushed
    // If non, an invalid number is returned
    uint8_t i;
    
    // Try the buttons on latch 0
    latch(0);
    for (i = 0; i < 8; i++)
    {
      if (!digitalRead(i + 26))
      {
        // Active low, return the button
        return i;
      }
    }
    
    // Now try the buttons on latch 1
    latch(1);
    for (i = 8; i < 10; i++)
    {
      if (!digitalRead(i + 18))
      {
        // Active low, return the button
        return i;
      }
    }
    return 10;
  }
  
  int soldOut(int soda)
  {
    // This function takes a soda number
    // checks to see if it is sold out
    // and returns true if it is
    if (!soda_enable[soda])
    {
      // If the soda is disabled, return true
      return 1;
    }
    
    // Check bank one first
    if (soda < 6)
    {
      latch(1);
      if (!digitalRead(soda + 28))
      {
        Log::print("Sold Out: " + String(soda));
        return 1;
      }
    }
    
    // Check Bank 2
    else
    {
      latch(2);
      if (!digitalRead(soda + 20))
      {
        Log::print("Sold Out: " + String(soda));
        return 1;
      }
    }
    return 0;
  }

  void latch(uint8_t OE)
  {
    // This function activates the specified latch
    digitalWrite(25, LOW);
    digitalWrite(25, HIGH);
    digitalWrite(22, OE & 1);
    digitalWrite(23, (OE >> 1) & 1);
    digitalWrite(24, (OE >> 2) & 1);
  }
  
  void relatch()
  {
    // This function relatches the same latch as before
    digitalWrite(25, LOW);
    digitalWrite(25, HIGH);
  }
  
  uint8_t timeout(unsigned long time)
  {
    static uint8_t set = 0;
    static unsigned long start;
    unsigned long current = millis();
    if (!set)
    {
      start = current;
      set = 1;
      return 0;
    }
    if (time == 0)
    {
      set = 0;
      return 0;
    }
    if (time + start <= current)
    {
      set = 0;
      return 1;
    }
    return 0;
  }
  
  void resetMotor(int motor)
  {
    // This function takes a motor number,
    // determins its location on the relay bank
    // and resets it to its default position.
    // If a motor fails to reset, the corresponding soda
    // is disabled
    
    // Set up variables based on bank
    int l; // The latch to read from
    int r; // The start location to read from on the latch bus
    int w; // The start location to write to on the relay bank
    int done, temp;
    
    // Change the variables based on which relay bank the motor is on
    if (motor < 4)
    { 
      l = 2;
      r = 30;
      w = 34;
    }
    else
    {
      l = 3;
      r = 22;
      w = 27;
    }
    
    latch(l);                             // Latch motor reset signals
    done = !digitalRead(motor + r);        // Read current motor status
    if (!done)
    {
      // Reset the motor
      Log::print("Activating");
      digitalWrite(motor * 2 + w, HIGH);
    }
    
    while (!done)
    {
      // Continually check to see if motor has reset
      digitalWrite(25, LOW);
      digitalWrite(25, HIGH);
      temp = digitalRead(motor + r);      // Get motor status
      digitalWrite(motor * 2 + w, temp);  // Set motor mode to the current status
      done = !temp;                      // When motor status is 1, it is not reset
      
      if (timeout(7000))
      {
        // The motor has taken too long to reset, possible jam
        // Disable that soda
        digitalWrite(motor * 2 + w, LOW);
        done = 1;
        soda_enable[motor] = 0;
        Log::print("Soda Disabled: " + String(motor));
      }
      
      if (!done)
      {
        // Keep the motor on if it has not reset
        digitalWrite(motor * 2 + w, HIGH);
      }
    }
    
    timeout(0); // Reset the timeout counter
    done = 0;
    digitalWrite(motor * 2 + 34, LOW);
  }
}

