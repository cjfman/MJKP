//Vending Machine Hardware Controller Library 
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

#include "RFID.h"
#include "Vend.h"
#include "Accounts.h"
#include "MDB.h"
#include "LCD.h"
#include "Log.h"

namespace RFID
{
  // RFID Variables
  uint8_t state;
  volatile unsigned long rfid_data;
  volatile uint8_t received;
  
  Accounts account;
  unsigned long temp_id;
  
  uint8_t queue;
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * *  Main Functions  * * * * * * * * * * * * * * * * *
  ************************************************************************************/
  
  void setup()
  {
    received = 0;
    rfid_data = 0;
    queue = 10;
    state = IDLE;
  }
  
  void run()
  {
  // Main periodic fuction
  // This function should be called by the main loop
    switch(state)
    {
    case IDLE:
      idle();
      break;
    case CLOSE:
      close();
      break;
    case TAPPED:
      tapped();
      break;
    case DEPOSIT:
      deposit();
      break;
    case VERIFY:
      verify();
      break;
    }
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * *  State Functions  * * * * * * * * * * * * * * * * *
  ************************************************************************************/
  
  void idle()
  {
    // This function waits for a card to be tapped
    
    // If a bill is larger than than or equal to $5, it must be added to an account
    if (MDB::escrow >= 500)
    {
      Vend::disable();
      LCD::largeBill();
    }
    else
    {
      Vend::enable();
    }
    
    // Check to see if RFID data is ready
    if (!ready() || !Vend::disable())
    {
      return;
    }
    MDB::all();
    
    account = Accounts(temp_id);
    if (!account.exists())
    {
      Log::print("Account not found: " + String (temp_id));
      if (!Accounts::createAccount(temp_id))
      {
        Log::print("Failed to create account");
        Vend::enable();
        return;
      }
      account = Accounts(temp_id);
      Log::print("New Account Created");
      LCD::newAccountCreated(account.getName());
    }
    Log::print("Account found: " + String (temp_id));
    LCD::cardTapped(account.getBalance(), account.getName());
    state = TAPPED;
    return;
  }
  
  void close()
  {
    Log::print("Closing account: " + account.getName() + ", with balance: " + String(account.getBalance()));
    account.close();
    Vend::enable();
    LCD::balanceUpdated(account.getBalance());
    timeout(0);
    received = 0;
    MDB::money_hold = false;
    state = IDLE;
  }
  
  void tapped()
  {
    // This function runs when a card has been tapped
    // It waits for either money to be deposited or a soda to be selected
    
    // Check to see if money has been deposited
    if (MDB::escrow || MDB::coin_funds)
    {
      Log::print("Money inserted. Switching to deposit mode");
      MDB::money_hold = true;
      state = DEPOSIT;
      return;
    }
    
    // Check for timeout or cancel
    if(timeout(20000) || (received >= 32))
    {
      Log::print("Account timeout");
      state = CLOSE;
      return;
    }
  
    vend();
  }
  
  void deposit()
  {
    long funds = MDB::bill_funds + MDB::coin_funds;
    static long last_funds = funds;
    long escrow = MDB::escrow;
    LCD::addFunds(funds + escrow);
    
    // Check for escrow and stack it
    if (escrow)
    {
      MDB::stack();
      timeout(0);
      return;
    }
    
    // Check for additional funds
    if (funds != last_funds) {
      last_funds = funds;
      timeout(0);
    }
    
    // Check for timeout or cancel
    if(timeout(30000) || (received >= 32))
    {
      Log::print("Account deposit timeout / Account Closed by User");
      account.credit(funds);
      MDB::bill_funds = 0;
      MDB::coin_funds = 0;
      state = CLOSE;
      return;
    }
    
    vend();
  }
  
  void verify()
  {
    unsigned long funds = MDB::bill_funds + MDB::coin_funds;
    unsigned long escrow = MDB::escrow;
    
    Log::print("Verifying that the bill has been stacked");
    
    // If the escrow has been stacked, the value of escrow
    // should be 0;
    if (escrow)
    {
      if (!timeout(10000))
      {
        return;
      }
      else
      {
        Log::print("Bill Stack Error");
        MDB::stack();
        //MDB::money_hold = false;
        state = DEPOSIT;
      }
    }
    timeout(0);
    
    // If the escrow has been stacked, the value of funds will be enough to make the purchase
    if (account.getBalance() + funds < Vend::prices[queue])
    {
      Log::print("Money not stacked. Returning to acceptance mode");
      //MDB::money_hold = false;
      state = DEPOSIT;
      return;
    }
    
    Log::print("VEND");
    
    account.credit(funds);
    MDB::bill_funds = 0;
    MDB::coin_funds = 0;
    account.charge(Vend::prices[queue]);
    if (!Vend::vend(queue))
    {
      LCD::print("Vend Failure", "Account Not Charged", 7);
      account.credit(Vend::prices[queue]);
    }
    queue = 10;
    state = CLOSE;
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * * * *  RFID Functions  * * * * * * * * * * * * * * * 
  ************************************************************************************/
  
  void dataLow()
  {
    rfid_data <<= 1;
    received++;
  }
  
  void dataHigh()
  {
    rfid_data <<= 1;
    rfid_data |= 1;
    received++;
  }
  
  boolean ready()
  {
    // This function looks at the data received from the RFID reader
    // If 32 bits have been received within the time constraints
    // the funciton returns true and the account number is set
    
    // A valid data packet has 32 bits
    if (received == 32)
    {
      timeout(0); // Reset timer
      while (!timeout(10)); // Wait to see if more bits have come in
      if (received == 32)
      {
        temp_id = rfid_data;
        received = 0;
        return true;
      }
      else // Too many bits received
      {
        LCD::print("Error Reading", "Tap Again", 3);
        received = 0;
        return false;
      }
    }
    
    // Try and clear accidental bits after one second
    static uint8_t previously_received = 0;
    if (previously_received != received)
    {
      previously_received == received;
      timeout(0);
    }
    else if (!timeout(1000))
    {
      received = 0;
    }
    
    return false;
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * * * *  Utility Functions  * * * * * * * * * * * * * * 
  ************************************************************************************/
  
  boolean timeout(unsigned long time)
  {
    static uint8_t set = 0;
    static unsigned long start;
    unsigned long current = millis();
    if (!set)
    {
      start = current;
      set = 1;
      return false;
    }
    if (time == 0)
    {
      set = 0;
      return false;
    }
    if (time + start <= current)
    {
      set = 0;
      return true;
    }
    return false;
  }
  
  void vend()
  {
    unsigned long funds = MDB::bill_funds + MDB::coin_funds;
    unsigned long escrow = MDB::escrow;
    
    // Check for soda selection
    uint8_t soda = Vend::buttonPush();
    // No button was pushed
    if (soda == 10)
    {
      return;
    }
    
    // Check to see if sold out
    if (Vend::soldOut(soda))
    {
      LCD::print(Vend::sodas[soda], "Sold Out", 3);
      return;
    }
    
    // Check for appropriate funds
    if (account.getBalance() + funds + escrow < Vend::prices[soda])
    {
      LCD::insufficientFunds(account.getBalance());
      return;
    }
    
    //MDB::money_hold = true;
    
    if (escrow)
    {
      state = VERIFY;
      queue = soda;
      return;
    }
    
    Log::print("VEND");
    
    // Start Vend Sequence
    account.credit(funds);
    MDB::bill_funds = 0;
    MDB::coin_funds = 0;
    account.charge(Vend::prices[soda]);
    if (!Vend::vend(soda))
    {
      LCD::print("Vend Failure", "Account Not Charged", 7);
      account.credit(Vend::prices[soda]);
    }
    state = CLOSE;
  }
}
