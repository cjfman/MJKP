#include "Controller.h"

#define IDLE 0
#define DEPOSIT 1
#define LARGE 2
#define TAPPED 3
#define AUTOIDLE 4

uint8_t RFIDreset = 2;

Controller::Controller()
{
  state = IDLE;
  account = 0;
  balance = 0;
  funds = 0;
  first = 1;
  digitalWrite(RFIDreset, HIGH);
  Display.idle();
}

void Controller::check(void)
{
  funds = bill_funds + coin_funds;
  if (state == IDLE)
  {
    this->idle();
    return;
  }
  if (state == DEPOSIT)
  {
    this->deposit();
    return;
  }
  if (state == LARGE)
  {
    this->large();
    return;
  }
  if (state == TAPPED)
  {
    this->tapped();
    return;
  }
}

void Controller::idle(void)
{
  Vend.enable();
  if (first)
  {
    Display.idle();
    first = 0;
  }
  digitalWrite(RFIDreset, HIGH);
  if (escrow > 1) //large bill
  {
    state = LARGE;
    Display.largeBill();
    return;
  }
  if (!Serial.available())
  {
    return;
  }
  account = this->getAccount();
  if (!account)
  {
    return;
  }
  digitalWrite(RFIDreset, LOW); //Turn off RFID Reader
  Serial.flush();
  balance = Accounts.getAccountIDBalance(account);
  if (balance == -1)
  {
    String name = Accounts.createAccount(account);
    Display.newAccountCreated(name);
    balance = 0;
    state = DEPOSIT;
    first = 1;
    return;
  }
  Display.cardTapped(balance);
  state = TAPPED;
  first = 1;
  return;
}

void Controller::deposit(void)
{
  Vend.disable();
  static unsigned long change = 0;
  if (change != funds)
  {
    change = funds;
    Display.addFunds(funds);
  }
  if (escrow) //Bill in escrow
  {
    this->timeout(0);
    MDB.setCommand("STACK BILL");
    return;
  }
  if (this->timeout(10) || MDB.canceled())
  {
    this->timeout(0);
    balance = Accounts.creditAccount(account, funds);
    Display.balanceUpdated(balance);
    balance = 0;
    funds = 0;
    bill_funds = 0;
    coin_funds = 0;
    MDB.setCommand("RETURN ESCROW");
    first = 1;
    state = IDLE;
  }
}

void Controller::large(void)
{
  Vend.disable();
  if (this->timeout(20000))
  {
    timeout(0);
    MDB.setCommand("RETURN ESCROW");
    Display.sessionCanceled();
    first = 1;
    state = IDLE;
    return;
  }
  if (MDB.canceled())
  {
    timeout(0);
    Display.sessionCanceled();
    first = 1;
    state = IDLE;
    return;
  }
  if (escrow) //Bill in escrow
  {
    account = this->getAccount();
    if (!account)
    {
      return;
    }
    timeout(0);
    MDB.setCommand("stack escrow");
    return;
  }
  if (funds == 0)
  {
    if (this->timeout(5000))
    {
      Display.print("ERROR", "Bill Feader");
      first = 1;
      state = IDLE;
    }
    return;
  }
  balance = Accounts.getAccountIDBalance(account);
  balance = Accounts.creditAccount(account, funds);
  Display.balanceUpdated(balance);
  balance = 0;
  funds = 0;
  first = 1;
  state = IDLE;
}

void Controller::tapped(void)
{
  Vend.cardPresent();
  if (escrow) //Bill in Escrow
  {
    state = DEPOSIT;
    return;
  }
  funds = balance;
  if(!(this->timeout(3000) || Vend.complete()))
  {
    return;
  }
  balance = Accounts.creditAccount(account, funds);
  Display.balanceUpdated(balance);
  first = 1;
  account = 0;
  balance = 0;
  funds = 0;
  state = IDLE;
  return;
}

uint8_t Controller::timeout(unsigned long time)
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

unsigned long Controller::getAccount()
{
  byte i = 0;
  byte val = 0;
  byte code[6];
  byte checksum = 0;
  byte bytesread = 0;
  byte tempbyte = 0;
  unsigned long result;
  
  if(Serial.available() > 0) 
  {
    if((val = Serial.read()) == 2) 
    {                  // check for header 
      bytesread = 0; 
      while (bytesread < 12) 
      {                        // read 10 digit code + 2 digit checksum
        if( Serial.available() > 0) 
        { 
          val = Serial.read();
          if((val == 0x0D)||(val == 0x0A)||(val == 0x03)||(val == 0x02)) 
          { // if header or stop bytes before the 10 digit reading 
            break;                                    // stop reading
          }
  
          // Do Ascii/Hex conversion:
          if ((val >= '0') && (val <= '9')) 
          {
            val = val - '0';
          } 
          else if ((val >= 'A') && (val <= 'F'))
          {
            val = 10 + val - 'A';
          }
  
          // Every two hex-digits, add byte to code:
          if (bytesread & 1 == 1) 
          {
            // make some space for this hex-digit by
            // shifting the previous hex-digit with 4 bits to the left:
            code[bytesread >> 1] = (val | (tempbyte << 4));
  
            if (bytesread >> 1 != 5) 
            {                // If we're at the checksum byte,
              checksum ^= code[bytesread >> 1];       // Calculate the checksum... (XOR)
            };
          } 
          else 
          {
            tempbyte = val;                           // Store the first hex digit first...
          };
          bytesread++;                                // ready to read next digit
        } 
      }
      if (bytesread == 12) 
      {                          // if 12 digit read is complete
        for (i=0; i < 4; i++) 
        {
          if (code[i] < 16)
          { 
            i++;
          }
          result |= (code[i] << (i * 4));
        }
        if (code[5] != checksum)
        {
          result = 0;
        }
      }
      bytesread = 0;
      return result;
    }
  }
  return 0;
}
 
Controller MainController;
