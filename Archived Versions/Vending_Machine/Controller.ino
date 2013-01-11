#include "Controller.h"

const uint8_t IDLE = 0;
const uint8_t DEPOSIT = 1;
const uint8_t LARGE = 2;
const uint8_t TAPPED = 3;


Controller::Controller()
{
  state = IDLE;
  account = 0;
  balance = 0;
  funds = 0;
  first = 1;
  Display.idle();
}

void Controller::check(void)
{
  //  Serial.print("Bill Funds: ");
  //  Serial.println(bill_funds);
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
  if (first)
  {
    Vend.enable();
    rfid_data = 0;
    received = 0;
    //Display.idle();
    first = 0;
//    Vend.cardPresent(0);
    MDB.setCommand("ALL");
    Serial.println("Account Closed");
  }
  if (escrow > 100) //large bill
  {
    Serial.print("Large: ");
    Serial.println(escrow);
    state = LARGE;
    Display.largeBill();
    return;
  }
  if (funds || escrow)
  {
    return;
  }
  account = this->getAccount();
  if (account == 0)
  {
    return;
  }
  Serial.println("Looking for Account");
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
    state = DEPOSIT;
    //MDB.setCommand("ALL");
    first = 1;
    Vend.cardPresent(1);
    return;
  }
  Serial.println("Account Found");
  Display.cardTapped(balance, Accounts.getAccountName(account));
  state = TAPPED;
  //MDB.setCommand("ALL");
  first = 1;
  Vend.cardPresent(1);
  return;
}

void Controller::deposit(void)
{
  MDB.funds_enable = 1;
  Vend.disable();
  static uint8_t wait = 1;
  static unsigned long change = 0;
  if (change != funds)
  {
    wait = 0;
    this->timeout(0);
    change = funds;
    Display.addFunds(funds);
  }
  if (escrow) //Bill in escrow
  {
    Serial.println("Bill Inserted");
    this->timeout(0);
    MDB.setCommand("STACK BILL");
    wait = 1;
    return;
  }
  uint8_t vend = Vend.buttonPush();
  if (vend != 10 && !wait)
  {
    Serial.println("Returning to Tapped");
    Accounts.creditAccount(account, funds);
    Accounts.closeSession();
    funds = 0;
    wait = 1;
    change = 0;
    coin_funds = 0;
    Vend.mem = vend;
    Vend.enable();
    balance = Accounts.getAccountIDBalance(account);
    bill_funds = balance;
    state = TAPPED;
    return;
  }
  if (this->timeout(30000) || (MDB.canceled() && !wait) || received >= 32)
  {
    Serial.println("Session Time Out");
    this->timeout(0);
    balance = Accounts.creditAccount(account, funds);
    Accounts.closeSession();
    Display.balanceUpdated(balance);
    balance = 0;
    funds = 0;
    change = 0;
    wait = 1;
    bill_funds = 0;
    coin_funds = 0;
    //MDB.setCommand("RETURN ESCROW");
    first = 1;
    state = IDLE;
    Vend.enable();
  }
}

void Controller::large(void)
{
  static uint8_t step = 0;
  Vend.disable();
  if (this->timeout(20000) || MDB.canceled() || (escrow == 0 && step == 0))
  {
    timeout(0);
    MDB.setCommand("RETURN ESCROW");
    Accounts.cancelSession();
    Display.sessionCanceled();
    first = 1;
    state = IDLE;
    step = 0;
    return;
  }
  if (step == 0) //Bill in escrow
  {
    account = this->getAccount();
    if (!account)
    {
      return;
    }
    step = 1;
    timeout(0);
    MDB.funds_enable = 1;
    MDB.setCommand("STACK BILL");
    return;
  }
  if (funds == 0 && step == 1)
  {
    if (this->timeout(8000))
    {
      Display.print("ERROR", "Bill Feader", 3);
      first = 1;
      step = 0;
      state = IDLE;
    }
    return;
  }
  this->timeout(0);
//  balance = Accounts.getAccountIDBalance(account);
  balance = Accounts.creditAccount(account, funds);
  Accounts.closeSession();
  Display.balanceUpdated(balance);
  balance = 0;
  funds = 0;
  first = 1;
  step = 0;
  state = IDLE;
}

void Controller::tapped(void)
{
  if (escrow || coin_funds > 0)
  {
    bill_funds = 0;
    state = DEPOSIT;
    return;
  }
  bill_funds = balance;
  Vend.cardPresent(1);
  if(!(this->timeout(30000) || Vend.complete()) && !(received >= 32))
  {
    return;
  }
  MDB.setCommand("RETURN ESCROW");
  balance = Accounts.creditAccount(account, charge);
  Accounts.closeSession();
  Display.balanceUpdated(balance);
  Vend.mem = 10;
  charge = 0;
  first = 1;
  account = 0;
  balance = 0;
  funds = 0;
  bill_funds = 0;
  Vend.cardPresent(0);
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
  static unsigned int ready = 0;
  static unsigned int over = 0;
  //  if (received != 0)
  //  {
  //    Serial.println("Receiving RFID data...");
  //  }
  if (received == 32)
  {
//    Serial.println("Verifying RFID data");
    if (!ready)
    {
      this->timeout(0);
      ready = 1;
    }
    if (this->timeout(10))
    {
//      Serial.println("Verified");
//      Serial.println(rfid_data, DEC);
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
  if (over)
  {
    if (this->timeout(10))
    {
//      Serial.println("");
//      Serial.println("Over");
      Display.print("Error Reading", "Tap Again", 3);
      over = 0;
      rfid_data = 0;
      received = 0;
    }
  }
  else
  {
    static uint8_t bits = 0;
    if (this->timeout(100))
    {
      rfid_data = 0;
      received = 0;
    }
    else if (bits != received)
    {
      bits = received;
      this->timeout(0);
    }
  }
  return 0;
}

Controller MainController;

