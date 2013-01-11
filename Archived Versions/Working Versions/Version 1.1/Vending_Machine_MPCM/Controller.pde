#include "Controller.h"


Controller::Controller()
{
  state = "IDLE";
  account = 0;
  balance = 0;
  funds = 0;
  first = 1;
  Display.idle();
}

void Controller::check(void)
{
  funds = bill_funds + coin_funds;
  if (state == "IDLE")
  {
    this->idle();
    return;
  }
  if (state == "DEPOSIT")
  {
    this->deposit();
    return;
  }
  if (state == "LARGE")
  {
    this->large();
    return;
  }
  if (state == "TAPPED")
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
  if (escrow > 1) //large bill
  {
    state = "LARGE";
    Display.largeBill();
    return;
  }
  account = this->getAccount();
  if (account == 0)
  {
    return;
  }
  balance = Accounts.getAccountIDBalance(account);
  if (balance == -1)
  {
    Serial.println("Account Not Found");
    String name = Accounts.createAccount(account);
    if (name == "NULL")
    {
      Serial.println("Failed to create account");
      rfid_data = 0;
      received = 0;
      return;
    }
    Serial.println("New Account Created");
    Display.newAccountCreated(name);
    balance = 0;
    state = "DEPOSIT";
    first = 1;
    return;
  }
  Serial.println("Account Found");
  Display.cardTapped(balance);
  state = "TAPPED";
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
    state = "IDLE";
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
    state = "IDLE";
    return;
  }
  if (MDB.canceled())
  {
    timeout(0);
    Display.sessionCanceled();
    first = 1;
    state = "IDLE";
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
      state = "IDLE";
    }
    return;
  }
  balance = Accounts.getAccountIDBalance(account);
  balance = Accounts.creditAccount(account, funds);
  Display.balanceUpdated(balance);
  balance = 0;
  funds = 0;
  first = 1;
  state = "IDLE";
}

void Controller::tapped(void)
{
  Vend.cardPresent();
  if (escrow) //Bill in Escrow
  {
    state = "DEPOSIT";
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
  state = "IDLE";
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
  static unsigned int ready = 0;
  static unsigned int over = 0;
//  if (received != 0)
//  {
//    Serial.println("Receiving RFID data...");
//  }
  if (received == 32)
  {
    Serial.println("Verifying RFID data");
    if (!ready)
    {
      this->timeout(0);
      ready = 1;
    }
    if (this->timeout(10))
    {
      Serial.println("Verified");
      Serial.println(rfid_data, DEC);
      unsigned long id = rfid_data;
      rfid_data = 0;
      received = 0;
      ready = 0;
      return id;
    }
    return 0;
  }
  if (received > 32 && !over)
  {
    this->timeout(0);
    over = 1;
    ready = 0;
  }
//  if (this->timeout(10))
//  {
//    rfid_data = 0;
//    received = 0;
//  }
  return 0;
}
 
Controller MainController;
