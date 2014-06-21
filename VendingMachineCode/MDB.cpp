//Multi Drop Bus Library Developed by Charles Franklin for RFID Vending Machine
//Assumes Serial have been initialized
//Not for use with generic MDB systems
//Assumes US periferals are being used
//
//(c) Charles Franklin 2012

#include <Arduino.h>
#include "MDB.h"
#include "Log.h"


namespace MDB
{
  unsigned long bill_funds;
  unsigned long coin_funds;
  unsigned long escrow;
  
  uint8_t state;
  uint8_t buffer[28];
  uint8_t write_buffer[8];
  
  uint8_t reader_state;
  uint8_t reader_enable;
  
  uint8_t exact_change;
  uint8_t coins_only;
  bool money_hold;
  
  unsigned long dispense;
  
  // Changer variables
  uint8_t changer_state;
  uint8_t changer_enable;
  uint8_t changer_jam;
  uint8_t changer_level;
  unsigned int changer_country;
  uint8_t changer_scale;
  uint8_t changer_decimal;
  unsigned int coins_used;
  unsigned int coin_value[16];
  unsigned int tubes;
  uint8_t coin_count[16];
  uint16_t changer_errors;
  
  // Changer Command Flags
  uint8_t changer_acceptance;
  
  // Reader variables
  uint8_t reader_jammed;
  uint8_t reader_level;
  int reader_country;
  int reader_scale;
  uint8_t reader_decimal;
  int reader_capacity;
  int reader_security;
  uint8_t reader_escrow;
  int bills_used;
  unsigned long bill_value[16];
  unsigned int bill_count;
  uint8_t stacker_full;
  uint8_t reader_acceptance;
  
  // Reader command flags
  uint8_t stack_bill;
  uint8_t return_bill;
  
  //uint8_t funds_enable;

  
  /************************************************************************************
  * * * * * * * * * * * * * * * * * Reset and Check Functions * * * * * * * * * * * * *
  ************************************************************************************/
  
  void reset(void)
  {
    Log::print("MDB Reset");
    MPCMSerial3.end();
    
    // Bus Reset
    pinMode(14, OUTPUT);
    digitalWrite(14, LOW);
    delay(125); //Bus reset by holding line low for over 100 ms
    digitalWrite(14, HIGH);
    
    MPCMSerial3.begin(9600, 1); //Master
    MPCMSerial3.flush();
    
    command(0x30); //Reader Reset
    command(0x08); //Changer Reset
    
    bill_funds = 0;
    coin_funds = 0;
    escrow = 0;
  
    changer_enable = 1;
    reader_enable = 1;
    exact_change = 0;
    //funds_enable = 1;
    state = CHANGER;
    reader_state = RESET;
    changer_state = RESET;
    changer_jam = 0;
    coins_only = 0;
    money_hold = false;
    reader_jammed = 0;
    stacker_full = 0;
    
    // Changer Variables
    changer_errors = 0;
    
    
    // Reader Command Flags
    stack_bill = 0;
    return_bill = 0;
  }
  
  void run(void)
  {
    // Main periodic fuction
    // This function should be called by the main loop
    switch(state)
    {
    case CHANGER:
      changer();
      break;
    case READER:
      reader();
      break;
    }
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * * * External Commands * * * * * * * * * * * * * * * 
  ************************************************************************************/
  
  void stack()
  {
    stack_bill |= 1;
  }
  
  void returnEscrow()
  {
    return_bill |= 1;
  }
  
  void giveChange(long change)
  {
    dispense = dispense + change;
    coin_funds = 0;
    bill_funds = 0;
    money_hold = false;
  }
  
  void coinReturn()
  {
    dispense = dispense + coin_funds + bill_funds;
    coin_funds = 0;
    bill_funds = 0;
    returnEscrow();
  }
  
  void all()
  {
    // This function verifies that the reader
    // and changer are accepting all money.
    // Bit one of xxx_acceptance is the mode
    // that should be active. The second bit
    // is the current mode.
    // mode0: reject all money
    // mode1: accept all money

    if (!(reader_acceptance & 0x02))
    {
      reader_acceptance |= 0x01;        // Set command bit
    }
    if (!(changer_acceptance & 0x02))
    {
      changer_acceptance |= 0x01;      // Set command bit
    }
  }
  
  void none()
  {
    // This function verifies that the reader
    // and changer are rejecting all money.
    if (reader_acceptance & 0x02)
    {
      reader_acceptance &= 0xFE;    // Clear command bit
    }
    if (changer_acceptance & 0x02)
    {
      changer_acceptance &= 0xFE;  // Clear command bit
    }
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * * * *  Changer Functions * * * * * * * * * * * * * * 
  ************************************************************************************/
  
  void changer(void)
  {
    // Main switching funciton for the changer
    // Calls proper funciton based on state
    // Main loop is POLL>>DISPENSE>>COMMAND
    switch(changer_state)
    {
      case POLL:
        changerPoll();
        break;
      case DISPENSE:
        changerDispense();
        break;
      case COMMAND:
        changerCommand();
        //changer_state = POLL;
        break;
      case RESET:
        changerReset();
        break;
      case WAIT:
        changerWait();
        break;
      case SETUP:
        changerSetup();
        break;
      case TUBE_STATUS:
        changerTubeStatus();
        break;
      case COIN_TYPE:
        changerCoinType();
        break;
      default:
        command(0x0B);  // POLL command
        changer_state = POLL;
    }
  }
  
  void changerPoll()
  {
    uint8_t end = read();
    switch(end)
    {
    case -1: //No data received
      command(0x0B); // Resend POLL command
      return;
    case 0:
      return; // Retransmit requested
    }
    if(end == 1 && buffer[0] == 0x00)
    {
      // If only one byte is sent and it is 0x00 it must be an ACK
      // If an ACK is sent, there is nothing to do
      changer_state = DISPENSE;
      state = READER;
      return;
    }
    uint8_t error = changerErrorCheck(end);
    if((error >> 1) & 0x01)
    {
      // Error that is cause for return
      // All nessessary actions are handled
      // outside of the scope of this function
      return;
    }
    /*
    // This code causes dimes to cause crashes
    if(!(error & 0x01) && !changer_enable)
    {
      // An error has been cleared. Reset Changer
      resetChanger();
      return;
    }
    */
    // If an action has been taken involving a coin the first byte
    // Indicates the action and the coin type
    // Look at MDB 4.0 spec for specifics on coding scheme
    if((buffer[0] >> 6) & 0x01)
    {
      // If the first byte has prefix that indicates coin deposited
      uint8_t action = (buffer[0] >> 4) & 0x03; // Indicates coin routing
      uint8_t coin = buffer[0] & 0x0F; // Type of coin
      coin_count[coin] = buffer[1]; // The number of coins in the tube for this type
      Log::print("Coin: " + String(coin) + ", " + String(coin_value[coin]));
      if(action != 0x03)
      {
        // All actions other than 0x03 indicate that the coin was accepted and good
        coin_funds = coin_funds + coin_value[coin];
      }
    }
    if((buffer[0] >> 7) & 0x01)
    {
      // If prefix indicates that coins were dispensed manually
      uint8_t coin = buffer[0] & 0x0F; // Type of coin
      coin_count[coin] = buffer[1]; // The number of coins in the tube for this type
    } 
    changer_state = DISPENSE;
    state = READER;
  }
  
  void changerCommand(void)
  {
    // Set acceptance to all bills
    // Only if not already set
    if (changer_acceptance == 0x01)
    {
      Log::print("Set Acceptance: All Coins");
      changer_acceptance = 0x03;
      write_buffer[0] = 0;      // Types 15-8 disabled
      write_buffer[1] = 0x0F;   // Types 0-3 enabled
      write_buffer[2] = 0xFF;   // All types dispense enable
      write_buffer[3] = 0xFF;
      command(0x0C, 4); //Coin Type
      changer_state = COIN_TYPE;
    }
    
    // Set acceptance to reject all bills
    // Only if not already set
    else if (changer_acceptance == 0x02)
    {
      Log::print("Set Acceptance: No Coins");
      changer_acceptance = 0x00;      
      write_buffer[0] = 0;      // Types 15-8 disabled
      write_buffer[1] = 0;      // Types 7-0  disabled
      write_buffer[2] = 0xFF;   // All types dispense enable
      write_buffer[3] = 0xFF;
      command(0x0C, 4); //Coin Type
      changer_state = COIN_TYPE;
    }
    
    // Nothing to do
    else {
      changer_state = NONE;
      state = READER;
    }
  }

  void changerDispense(void)
  {
    // This funciton Calculates what coins to dispense
    // The coin is selected, its index is put in the queue
    // and the changer is told to dispense that coin
    // The function then waits for confirmation from that changer
    // that the coin was dispensed
    // The amount of money to be dispensed is stored in the dispense variable
    // that is set by another function
    static uint8_t queue = 16; // 15 is the highest valid coin index
    static uint8_t number = 0; // The number of coins that the dispenser has been asked to dispense
    static uint8_t wait = 0; //Set when waiting for payout to finish
    if(queue != 16)
    {
      //If the index in queue is valid, check to see if it was dispensed
      uint8_t end;
      end = read();
      if(end == -1)
      {
        // No data available, or NAK
        command(0x0D, 1); // Resend Dispense command with same data
        return;
      }
      if(buffer[0] == 0x00)
      {
        // If ACK was received, then the coin was dispensed
        dispense -= (number * coin_value[queue]); // Calculate amount left to be dispensed
        queue = 16; // Invalidate queue
        number = 0;
        wait = 1;
        command(0x0B); //Poll to look for busy signal
      }
    }
    if(wait)
    {
      uint8_t end;
      end = read();
      if(end <= 0)
      {
        return;
      }
      uint8_t error = changerErrorCheck(end);
      if((error >> 2) & 0x01)
      {
        //If payout is busy
        command(0x0B); //Poll to look for busy signal
        return;
      }
      else
      {
        //Changer is no longer busy
        wait = 0;
      }
    }
    // Assumes that changer is using american currency and that only 
    // quarters, dimes, and nickles are used
    if (dispense >= 25)
    {
      // If there is more than 25 cents to vend and nothing is currently being vended
      // Vend quarters
      queue = coinWithValue(25); // Look up the quarter index
      number = dispense / 25; // Calculate how many quarters can be dispensed
      if (coin_count[queue] >= number)
      {
        // If there are enough coins to dispense change
        write_buffer[0] = queue; // Prepare the data buffer
        write_buffer[0] |= number << 4; // Look up in MDB documentation for format
        command(0x0D, 1); // Send DISPENSE command with one data byte
        return;
      }
    }  
    if (dispense >= 10)
    {
      //Dimes, see commenting for quarters
      queue = coinWithValue(10);
      number = dispense / 10;
      if (coin_count[queue] >= number)
      {
        write_buffer[0] = queue;
        write_buffer[0] |= number << 4;
        command(0x0D, 1);
        return;
      }
    }  
    if (dispense >= 5) //Nickel Dispenser Broken. Give Dimes Instead
    {
      //Nickles, see commeting for quarters
      queue = coinWithValue(5);
      number = dispense / 5;
      if (coin_count[queue] >= number)
      {
        write_buffer[0] = queue;
        write_buffer[0] |= number << 4;
        command(0x0D, 1);
        return;
      }
    }
    // No more coins to dispensed
    changer_state = COMMAND;
    state = READER;
    queue = 16;
  }
    
  void changerReset(void)
  {
    //Must wait 800 ms before attempting to initialize the changer after reset
    static unsigned int time = 0; 
    if (time == 0)
    {
      //Start timer
      time = millis();
      return;
    }
    if (time + 800 > millis())
    {
      return;
    }
    Log::print("Initiating Changer");
    command(0x0B); //POLL
    changer_state = WAIT;
  }
  
  void changerWait(void)
  {
    //Makes sure that the changer reset properly
    //If not, signals a reset
    uint8_t end = read();
    switch(end)
    {
      case -1: //No data received
        command(0x0B); //Resend POLL command
        return;
      case 0: //Retransmit requested
        return;
    }
    if(!(changerErrorCheck(end) & 0x02))
    {
      //If changer did not reset
      resetChanger();
      return;
    }
  }
  
  void changerSetup()
  {
    //Gets setup data from the changer
    uint8_t  end, i;
    end = read();
    switch(end)
    {
    case -1: //No data received
      command(0x09); //Resend SETUP command
      return;
    case 0:
      return; //Retransmit requested
    }
    // When multiple bytes arrive, they are placed in the buffer
    // The first byte is at the 0 index of the buffer
    // Setup data arrives in the order and orginization below
    // 23 data bytes are expected
    changer_level = buffer[0];
    changer_country = buffer[1] << 8;
    changer_country |= buffer[2];
    changer_scale = buffer[3];
    changer_decimal = buffer[4];
    coins_used = buffer[5] << 8; // Says which coins can be put
    coins_used |= buffer[6];     // in tubes
    for (i = 0; i < 16; i++)
    {
      // The next 16 bytes say the value of the coins that are accepted
      // Up to 16 types of coins can be accepted
      coin_value[i] = changer_scale * buffer[i+7];
    }
    command(0x0A); //Tube Status
    changer_state = TUBE_STATUS;
    Log::print("Changer Initialized");
  }
  
  void changerTubeStatus(void)
  {
    uint8_t end, i;
    end = read();
    switch(end)
    {
     case -1: // No data received
       command(0x0A); // Resend Tube Status command
       return;
     case 0:
       return;
    }
    tubes = buffer[0] << 8; // Retrieve Tubes that are full
    tubes |= buffer[1];
    for (i = 0; i < 16; i++)
    {
      // The following data bytes are the coin count for each tube
      // Consecutive empty tubes at the end of the list are not sent
      if (i + 2 < end)
      {
        // If data was sent for this tube
        coin_count[i] = buffer[i + 2];
        if (coin_count[i] == 0 && (tubes >> i) == 1)
        {
          // A tube jam is represented by a tube being set as full
          // But having a coin coint of 0
          coin_count[i] = 0;
          changer_jam = 1;
        }
      }
      else
      {
        // If there is no data on the coin, set count to 0
        coin_count[i] = 0;
      }
    }
    delay(25); //Wait before sending data
    write_buffer[0] = 0;    // Prep the write buffer for data to be send
    write_buffer[1] = 0x0F; // The first two bytes indicate which coins are to be accepted
    write_buffer[2] = 0xFF; // The second two bytes indicate which coins can be manually dispensed
    write_buffer[3] = 0xFF; // This data is specific to a changer that accepts 4 types of coins
    command(0x0C, 4); //Coin Type
    changer_acceptance = 0x03;
    changer_state = COIN_TYPE;
  }
  
  void changerCoinType(void)
  {
    uint8_t end = read();
    switch(end)
    {
      case -1: // No data received
        command(0x0C, 4); // Resend Coin Type command
      return;
    }
    changer_state = COMMAND;
    state = READER;
  }
  
  uint8_t coinWithValue(uint8_t value)
  {
    uint8_t i;
    for (i = 0; i < 16; i++)
    {
      while ((coins_used >> i) & 0x01 == 0)
      {
        i++;
      }
      if (coin_value[i] == value)
      {
        return i;
      }
    }
    return 16;
  }

  int changerErrorCheck(uint8_t end)
  {
    // Parses buffer and looks for error codes
    // Sets the first bit if calling fuction can
    // continue as normal. Sets second bit if calling
    // function should return without other actions
    // The third bit is set when the changer is busy
    int result = 0;
    for(int i = 0; i < end; i++)
    {
      //*
      if(buffer[i] == 0x06) //Acceptor Unplugged
      {
        //Log::print("Acceptor Unplugged");
        changer_enable = 0;
        result |= 1;
        changer_errors++;
      }
      else if(buffer[i] == 0x07) //Tube Jam
      {
        // Can accept coins but can't dispense
        //Log::print("Tube Jam");
        exact_change = 1;
        changer_jam = 1;
        result |= 1;
        changer_errors++;
      }
      else if(buffer[i] == 0x0C) //Coin Jam
      {
        // Can still dispense, but can't accept coins
        //Log::print("Coin Jam");
        changer_enable = 0;
//        exact_change = 1;
//        changer_jam = 1;
        result |= 1;
        //result |= (1 << 1);
        changer_state = DISPENSE;
        changer_errors++;
      }
      // */
      else if(buffer[i] == 0x0B) //Just Reset
      {
        Log::print("Changer: Just Reset");
        changer_state = SETUP;
        command(0x09); //SETUP
        result |= (1 << 1);
      }
      else if(buffer[i] == 0x02 || buffer[i] == 0x0A) //Busy
      {
        //changer_state = COMMAND;
        result |= (1 << 2);
      }
    }
    static unsigned long ctime = millis();
    static unsigned long last_error = ctime;
    if (changer_errors > 100) {
      resetChanger();
    }
    if (last_error + 3000 >= ctime) {
      changer_errors = 0;
    }
    return result;
  }  
  
  void resetChanger(void)
  {
    changer_enable = 1;
    changer_jam = 0;
    changer_errors = 0;
    command(0x08);
    changer_state = RESET;
    Log::print("Changer Reset");
    //delay(200);
    //MPCMSerial3.flush();
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * * * *  Reader Functions * * * * * * * * * * * * * * * 
  ************************************************************************************/
  
  void reader(void)
  {
    // Main switching funciton for the Bill Reader
    // Calls proper funciton based on state
    // Main loop is POLL >> COMMAND
    if (!reader_enable) {
      state = CHANGER;
      return;
    }
    
    switch(reader_state)
    {
      case POLL:
        readerPoll();
        break;
      case COMMAND:
        readerCommand();
        break;
      case RESET:
        readerReset();
        break;
      case WAIT:
        readerWait();
        break;
      case SETUP:
        readerSetup();
        break;
      case STACKER:
        readerStacker();
        break;
      case BILL_TYPE:
        readerBill();
      default:
        command(0x33); // POLL command
        reader_state = POLL;
        break;
    }
  }
  
  void readerPoll()
  {
    int8_t end = read();
    static unsigned long last_stack = 0;
    unsigned long current = millis();
    
    switch(end)
    {
      case -1: //No data received
        command(0x33); //Resend POLL command
        return;
      case 0: //Retransmit requested
        return;
    }
    
    // Check for errors
    if(readerErrorCheck(end) & 0x01)
    {
      return;
    }
    
    // Check for sole ACK
    if (end == 1 && buffer[0] == 0x00)
    {
      reader_state = COMMAND;
      state = CHANGER;
      return;
    }
    // Look through read buffer for bill accepted byte
    int accepted = -1;
    int i;
    for (i = 0; i < end; i++)
    {
      if (buffer[i] >> 7)
      {
        accepted = i;
      }
    }
    
    if (accepted >= 0)
    {
      // A bill was accepted
      // Parse the input buffer for info
      int bill = buffer[accepted] & 0x0F;
      int routing = (buffer[accepted] >> 4) & 0x07;
      Log::print("Bill: " + String(bill) + ", " + String(bill_value[bill]));
      switch (routing)
      {
      case 0x00: // Stacked
        Log::print("Stacked");
        // Make sure that no double counts are counted
        if (last_stack + 500 < current)
        {
          bill_funds += bill_value[bill];
          last_stack = current;
          escrow = 0;
        }
        break;
      case 0x01: // Escrow
        Log::print("Escrow");
        escrow = bill_value[bill];
        break;
      case 0x02: // Escrow Returned
        Log::print("Returned");
        escrow = 0;
        break;
      }
    }
    
    reader_state = COMMAND;
    state = CHANGER;
  }
  
  void readerCommand()
  {
    // This function looks at flags that are set externally
    // Only one flag is handled at a time
    // All flags are cleared by and only this function
    
    // Return bill command
    // Only valid if there is a bill in escrow
    if (return_bill) // & 0x01 && (return_bill & 0x02))
    {
      if (escrow)
      {
        Log::print("Sending Return Bill Command");
        //return_bill |= 0x02;
        write_buffer[0] = 0x00; // Return
        command(0x35, 1);       // Bill command
        escrow = 0;
        reader_state = POLL;
      }
      else
      {
        Log::print("Invalid Return Escrow Request");
      }
      return_bill = 0;
      money_hold = false;
    }
    
    
    // Stack bill command
    // Only valid if there is a bill in escrow
    else if (stack_bill) // & 0x01 && !(stack_bill & 0x02))
    {
      if (escrow)
      {
        Log::print("Sending Stack Bill Command: " + String(escrow));
        //stack_bill |= 0x02;
        write_buffer[0] = 0x01; // Stack
        command(0x35, 1); // Bill command
        reader_state = POLL;
      }
      else
      {
        Log::print("Invalid Stack Escrow Request");
      }
      stack_bill = 0;
    }
    
    // Set acceptance to all bills
    // Only if not already set
    else if (reader_acceptance == 0x01)
    {
      Log::print("Set Acceptance: All Bills");
      reader_acceptance = 0x03;
      write_buffer[0] = bills_used >> 8;
      write_buffer[1] = bills_used & 0xFF;
      write_buffer[2] = write_buffer[0];
      write_buffer[3] = write_buffer[1];
      command(0x34, 4);
      reader_state = BILL_TYPE;
    }
    
    // Set acceptance to reject all bills
    // Only if not already set
    else if (reader_acceptance == 0x02)
    {
      Log::print("Set Acceptance: No Bills");
      reader_acceptance = 0x00;
      write_buffer[0] = 0;
      write_buffer[1] = 0;
      write_buffer[2] = write_buffer[0];
      write_buffer[3] = write_buffer[1];
      command(0x34, 4);
      reader_state = BILL_TYPE;
    }
    
    else {
      //command(0x33); // POLL command
      reader_state = NONE;
      state = CHANGER;
    }
  }
  
  void readerReset()
  {
    command(0x33); //Poll
    reader_state = WAIT;
    return;
  }
  
  void readerWait()
  {
    //This function makes sure that the reader has reset
    //If not, signals a reset
    uint8_t end = read();
    switch(end)
    {
      case -1: //No data received
        command(0x33); //Resend POLL command
        return;
      case 0: //Retransmit requested
        return;
    }
    if(!(readerErrorCheck(end) & 0x02))
    {
      if (!reader_enable) return;
      //If changer did not reset
      resetReader();
      return;
    }
  }
  
  void readerSetup()
  {
    // This function gets the setup values from the reader
    uint8_t end = read();
    switch(end)
    {
      case -1: //No data received
        command(0x31); //Resend SETUP command
        return;
      case 0: //Retransmit requested
        return;
    }
    // Populate reader variables from read buffer
    reader_level = buffer[0];
    reader_country = buffer[1] << 8;
    reader_country |= buffer[2];
    reader_scale = buffer[3] << 8;
    reader_scale |= buffer[4];
    reader_decimal = buffer[5];
    reader_capacity = buffer[6] << 8;
    reader_capacity |= buffer[7];
    reader_security = buffer[8] << 8;
    reader_security |= buffer[9];
    reader_escrow = buffer[10];
    bills_used = 0;
    int i;
    for (i = 0; i < 16; i++)
    {
      // The next 16 values in the read buffer dictate
      // which bill codes will be used
      bill_value[i] = reader_scale * buffer[i + 11];
      if (bill_value[i])
      {
        bills_used |= 1 << i;
      }
    }
    command(0x36); //Stacker Status
    Log::print("Reader Initialized");
    reader_state = STACKER;
  }
  
  void readerStacker()
  {
    // This function checks on the status of the stacker
    uint8_t end = read();
    switch(end)
    {
      case -1: //No data received
        command(0x36); //Resend Stacker Status command
        return;
      case 0: //Retransmit requested
        return;
    }
    // Populate reader variables from read buffer
    stacker_full = buffer[0] >> 7;
    bill_count = (buffer[0] & 0xFE) << 8;
    bill_count |= buffer[1];
    
    // Prepare write buffer to tell reader that
    // all bills will be used
    write_buffer[0] = bills_used >> 8;
    write_buffer[1] = bills_used & 0xFF;
    write_buffer[2] = write_buffer[0];
    write_buffer[3] = write_buffer[1];
    command(0x34, 4); //Bill Type
    changer_acceptance = 0x03;
    reader_state = BILL_TYPE;
  }
  
  void readerBill()
  {
    // This function makes sure that the Bill Type command
    // was processed by the reader
    uint8_t end = read();
    switch(end)
    {
      case -1: //No data received
        command(0x34, 4);; //Resend Bill Type command
        return;
      case 0: //Retransmit requested
        return;
    }
    reader_state = COMMAND;
    state = CHANGER;
    return;
  }
  
  int readerErrorCheck(uint8_t end)
  {
    // Parses buffer and looks for error codes
    // Sets the first bit if calling fuction can
    // continue as normal. Sets second bit if calling
    // function should return without other actions.
    int result = 0;
    int i = 0;
    for(i = 0; i < end; i++)
    {
      if(buffer[i] == 0x01) //Defective Motor
      {
        Log::print("Bill Feeder Error: Defective Motor");
        //reader_enable = 0;
        //coins_only = 1;
        result |= 1;
      }
      if(buffer[i] == 0x02) //Defective Sensor
      {
        Log::print("Bill Feeder Error: Defective Sensor");
        //reader_enable = 0;
        //coins_only = 1;
        result |= 1;
      }
      if(buffer[i] == 0x03) //Busy
      {
        reader_state = COMMAND;
        result |= 1 << 2;
      }
      if(buffer[i] == 0x05) //Jam
      {
        reader_jammed = 1;
        reader_state = COMMAND;
        result |= 1;
      }
      if(buffer[i] == 0x06) //Just Reset
      {
        command(0x31); //SETUP
        reader_state = SETUP;
        result |= 1 << 1;
      }
      /*if (buffer[i] == 0x0A) //Invalid Escrow Request
      {
        escrow = 0;
      }*/
      /*
      else if(buffer[i] == 0x09) //Validator Disabled
      {
        Log::print("Bill Feeder Error: Validator Disabled");
        reader_enable = 0;
        coins_only = 1;
        result = result & 1;
      }
      */
    }
    return result;
  }  
  
  void resetReader(void)
  {
    reader_enable = 1;
    command(0x30);
    reader_state = RESET;
    //delay(200);
    //MPCMSerial3.flush();
  }
  
  /************************************************************************************
  * * * * * * * * * * * * * * * * * *  Serial Functions * * * * * * * * * * * * * * * *
  ************************************************************************************/
  
  void command(uint8_t command)
  {
    MPCMSerial3.writeMode(command); //Send command
    MPCMSerial3.write(command); //Send ChK byte
  }
  
  void command(uint8_t command, uint8_t end)
  {
    //Sends the command and data from buffer
    //Length of buffer is end parameter
    uint8_t i;
    unsigned long CHK;
    CHK = command;
    for (i = 0; i < end; i++)
    {
      //Calculate Check Byte
      CHK = CHK + write_buffer[i];
    }
    CHK &= 0xFF; //Mask Byte
    //Timing restraints require that bytes be sent 
    //one after the other without other code interupting
    //I.E. for loops cause too long breaks between bytes
    switch(end)
    {
    case 1:
      MPCMSerial3.writeMode(command); //Send Command
      MPCMSerial3.write(write_buffer[0]); //First byte in buffer
      MPCMSerial3.write(CHK); //Check byte
      break;
    case 2:
      MPCMSerial3.writeMode(command); //Rinse and Repeat
      MPCMSerial3.write(write_buffer[0]);
      MPCMSerial3.write(write_buffer[1]);
      MPCMSerial3.write(CHK);
      break;
    case 3:
      MPCMSerial3.writeMode(command);
      MPCMSerial3.write(write_buffer[0]);
      MPCMSerial3.write(write_buffer[1]);
      MPCMSerial3.write(write_buffer[2]);
      MPCMSerial3.write(CHK);
      break;
    case 4:
      MPCMSerial3.writeMode(command);
      MPCMSerial3.write(write_buffer[0]);
      MPCMSerial3.write(write_buffer[1]);
      MPCMSerial3.write(write_buffer[2]);
      MPCMSerial3.write(write_buffer[3]);
      MPCMSerial3.write(CHK);
      break;
    case 5:
      MPCMSerial3.writeMode(command);
      MPCMSerial3.write(write_buffer[0]);
      MPCMSerial3.write(write_buffer[1]);
      MPCMSerial3.write(write_buffer[2]);
      MPCMSerial3.write(write_buffer[3]);
      MPCMSerial3.write(write_buffer[4]);
      MPCMSerial3.write(CHK);
      break;
    case 6:
      MPCMSerial3.writeMode(command);
      MPCMSerial3.write(write_buffer[0]);
      MPCMSerial3.write(write_buffer[1]);
      MPCMSerial3.write(write_buffer[2]);
      MPCMSerial3.write(write_buffer[3]);
      MPCMSerial3.write(write_buffer[4]);
      MPCMSerial3.write(write_buffer[5]);
      MPCMSerial3.write(CHK);
      break;
    }
  }
  
  int read(void)
  {
    //This function parses incoming data from the MDB line
    //It returns -1 if no data is available or NAK
    //Data should be retransmited if -1 is returned
    //If 0 is returned, function should wait for data
    //  to be retransmited
    
    //MPCMSerial3.available();
    unsigned long sum = 0;
    if (timeout())
    {
      //No data within the time constraint
      return -1;
    }
    unsigned int temp = MPCMSerial3.read();
    unsigned int i;
    for (i = 0; temp >> 8 != 1; i++)
    {
      //Loop while the 9th bit is not set
      //Seting the 9th bit signifies the end of a package
      buffer[i] = temp;
      sum = sum + temp; //Calculate CHK byte
      if (timeout())
      {
        //No data within the time constraint even though more data expected
        return -1;
      }
      temp = MPCMSerial3.read();
    }
    uint8_t CHK = temp & 0xFF; //Check calculated CHK byte with received one
    //Received CHK byte always comes at end of data package
    sum &= 0xFF; //Mask
    if (i == 0)
    {
      //If only one byte was sent, check or ACK or NAK
      if (CHK == 0xFF) //NAK
      {
        return -1;
      }
      if (CHK == 0x00) //ACK
      {
        buffer[0] = 0x00;
        return 1;
      }
    }
    if (sum != CHK)
    {
      //CHK failure
      MPCMSerial3.write(0xAA); //RET
      return 0;
    }
    MPCMSerial3.write(0x00); //ACK
    //Send ACK if all data received properly
    return i;
  }	
  
  int timeout(void)
  {
    unsigned long time = 0;
    static unsigned int timeoutcount = 0;
    while(!MPCMSerial3.available())
    {
      time++;
      delay(1);
      if (time > 10)
      {
        timeoutcount++;
        if (timeoutcount % 21 == 20)
        {
          if (timeoutcount > 500)
          {
            Log::print("Bus timeout");
            state = RESET;
            return 1;
          }
          timeoutcount = 0;
        }
        return 1;
      }
    }
    timeoutcount = 0;
    return 0;
  }
}
