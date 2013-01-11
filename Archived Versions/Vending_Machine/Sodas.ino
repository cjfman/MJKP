/*
 *  Sodas.cpp
 *  
 *
 *  Created by Charles Franklin on 8/6/11.
 *  Copyright 2011 Massachusetts Institute of Technology. All rights reserved.
 *
 */

#include "Sodas.h"

const uint8_t ALL = 1;
//const uint8_t IDLE = 0;
const uint8_t VEND = 2;

/////////////////Constructor////////////////////////

SodaDispenser::SodaDispenser()
{
  state = IDLE;
  enabled = 1;
  queue = 10;
  vend_complete = 0;
  card = 0;
  mem = 10;
  free = 1;
  unsigned int i;
  for (i = 22; i < 26; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }
  for (i = 26; i < 34; i++)
  {
    pinMode(i, INPUT);
    digitalWrite(i, HIGH);
  }
  for (i = 34; i < 41; i = i + 2)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  for (i = 35; i < 46; i = i + 2)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
}

void SodaDispenser::begin()
{
  Serial.println("...Dispenser");
  int i;
  for (i = 0; i < 10; i++)
  {
    prices[i] = Accounts.getAccountIDBalance(i);
    sodas[i] = Accounts.getAccountName(i);
    soda_enable[i] = 1;
    if (prices[i] > max_price)
    {
      max_price = prices[i];
    }
    uint8_t done = 0;
    int temp = 0;
    Serial.print("...motor: ");
    Serial.println(i);
    if (i < 4)
    {
      this->latch(2);
      done = !digitalRead(i + 30);
      if (!done)
      {
        Serial.println("Activating");
        digitalWrite(i * 2 + 34, HIGH);
      }
      while (!done)
      {
        digitalWrite(25, LOW);
        digitalWrite(25, HIGH);
        temp = digitalRead(i + 30);
        digitalWrite(i * 2 + 34, temp);
        done = !temp;
        if (this->timeOut(7000))
        {
          digitalWrite(i * 2 + 34, LOW);
          done = 1;
          soda_enable[i] = 0;
          Serial.print("Soda Disabled: ");
          Serial.println(i, DEC);
        }
        if (!done)
        {
          digitalWrite(i * 2 + 34, HIGH);
        }
      }
      this->timeOut(0);
      done = 0;
      digitalWrite(i * 2 + 34, LOW);
    }
    else 
    {
      this->latch(3);
      done = !digitalRead(i + 22);
      if (!done)
      {
        Serial.println("Activating");
        digitalWrite(i * 2 + 27, HIGH); 
      }
      while (!done)
      {
        digitalWrite(25, LOW);
        digitalWrite(25, HIGH);
        temp = digitalRead(i + 22);
        digitalWrite(i * 2 + 27, temp);
        done = !temp; 
        if (this->timeOut(7000))
        {
          digitalWrite(i * 2 + 27, LOW);
          done = 1;
          soda_enable[i] = 0;
          Serial.print("Soda Disabled: ");
          Serial.println(i, DEC);
        }
        if (!done)
        {
          digitalWrite(i * 2 + 27, HIGH); 
        }
      }
      done = 0;
      this->timeOut(0);
      digitalWrite(i * 2 + 31, LOW);
    }
  }
}

//////////Public Methods///////////////////////////

void SodaDispenser::check(void)
{
  //  Serial.print("Bills: ");
  //  Serial.println(bill_funds);
  //  Serial.print("Coins:  ");
  //  Serial.println(coin_funds);
  //  Serial.print("Escrow: ");
  //  Serial.println(escrow);
  if (!enabled)
  {
    queue = 10;
    state = ALL;
    return;
  }
  if (state == ALL)
  {
    if ((bill_funds + coin_funds + escrow) != 0 && !this->timeOut(10000))
    {
      return;
    }
    else if ((bill_funds + coin_funds + escrow) != 0)
    {
      Serial.println("Took too long");
      received = 0;
      rfid_data = 0;
      maxed = 0;
      card = 0;
      state = IDLE;
      this->timeOut(0);
      MDB.busReset();
      return;
    }
    Serial.println("Dispenser Ready");
    MDB.setCommand("ALL");
    received = 0;
    rfid_data = 0;
    maxed = 0;
    card = 0;
    state = IDLE;
    this->timeOut(0);
    return;
  } 
  if (state == IDLE)
  {
    this->idle();
    return;
  }
  if (state == VEND)
  {
    this->vend();
  }
}

void SodaDispenser::enable(void)
{
  enabled = 1;
}

void SodaDispenser::disable(void)
{
  enabled = 0;
}

void SodaDispenser::cardPresent(int val)
{
  card = val;
}

uint8_t SodaDispenser::complete(void)
{
  uint8_t i;
  i = vend_complete;
  vend_complete = 0;
  return i;
}

//////////Private Methods//////////////////////////

uint8_t SodaDispenser::buttonPush(void)
{
  uint8_t i;
  this->latch(0);
  for (i = 0; i < 8; i++)
  {
    if (!digitalRead(i + 26))
    {
      return i;
    }
  }
  this->latch(1);
  for (i = 8; i < 10; i++)
  {
    if (!digitalRead(i + 18))
    {
      return i;
    }
  }
  return 10;
}

uint8_t SodaDispenser::soldOut(uint8_t soda)
{
  if (!soda_enable[soda])
  {
    return 1;
  }
  if (soda < 6)
  {
    this->latch(1);
    if (!digitalRead(soda + 28))
    {
      Serial.print("Sold Out: ");
      Serial.println(soda, DEC);
      return 1;
    }
  }
  else
  {
    this->latch(2);
    if (!digitalRead(soda + 20))
    {
      Serial.print("Sold Out: ");
      Serial.println(soda, DEC);
      return 1;
    }
  }
  return 0;
}

void SodaDispenser::idle()
{
  funds = bill_funds + coin_funds;
  unsigned long efunds = funds + escrow;
  static unsigned long previous = 0;
  static uint8_t look = 0;
  if (MDB.canceled())
  {
    state = ALL;
    previous = 0;
  }
  if (escrow && funds + escrow < max_price)
  {
    MDB.setCommand("STACK BILL");
  }
  uint8_t soda = buttonPush();
  if (mem != 10)
  {
    soda = mem;
  }
  if (efunds == 0 && !card)
  {
    previous = 0;
    if (soda != 10)
    {
      if (this->soldOut(soda))
      {
        Display.print(sodas[soda], "Sold Out", 3);
        return;
      }
      Display.price(sodas[soda], prices[soda], 3);
    }
    Serial.println("Preselection: " + String(soda));
    return;
  }
  if ((previous != efunds || this->timeOut(500)) && !card)
  {
    if (efunds < max_price)
    {
      Display.addFunds(efunds);
      if (maxed)
      {
        maxed = 0;
        MDB.setCommand("All");
        MDB.funds_enable = 1;
      }
    }
    else if (!maxed)
    {
      maxed = 1;
      MDB.setCommand("MAX");
      if (escrow <= 100)
      {
        Display.price("Reached MaxPrice", "Added: ", efunds);
      }
    }
    previous = efunds;
  }
  //soda = buttonPush();
  if (soda == 10)
  {
    return;
  }
  Serial.println("Selection: " + String(soda));
  if (this->soldOut(soda))
  {
    Serial.print("Sold Out: ");
    Serial.println(soda, DEC);
    Display.print("Sold Out");
    return;
  }
  if (efunds >= prices[soda] || (card && (efunds >= prices[soda] - 5)))
  {
    Serial.print("Vend: ");
    Serial.println(soda, DEC); 
    bill_funds = 0;
    if(!card)
    {
      Serial.println("Card Not Present. Stacking bill and returning change");
      coin_funds = efunds - prices[soda];
      MDB.setCommand("RETURN COINS");
      funds = 0;
    }
    else
    {
      Serial.println("Card Present. Not stacking bill or returning change");
      charge = -1 * (prices[soda] - 5);
      vend_complete = 1;
    }
    this-timeOut(0);
    queue = soda;
    previous = 0;
    state = VEND;
    return;
  }
  else 
  {
    if (card)
    {
      Display.insufficientFunds(efunds);
    }
    else
    {
      Serial.print("Not Enough Money: ");
      Serial.print(soda, DEC);
      Display.print(sodas[soda], String(prices[soda]), 3);
    }
  }
}

void SodaDispenser::vend(void)
{
  if (!card)
  {
    Display.print("VEND");
    Display.check();
  }
  static unsigned long time;
  if (queue == 10)
  {
    state = IDLE;
    return;
  }
  static uint8_t in_progress = 0;
  if (!in_progress)
  {
    in_progress = 1;
    if (queue < 4)
    {
//      Serial.println("Vending from set 1");
      digitalWrite(queue * 2 + 34, HIGH);
      this->latch(2);
    }
    else
    {
//      Serial.println("Vending from set 2");
      digitalWrite(queue * 2 + 27, HIGH);
      this->latch(3);
    }
    time = millis();
    Serial.print("Vend: ");
    Serial.println(queue, DEC);
    return;
  }
  uint8_t done, i, temp;
  done = 0;
  //  while(!digitalRead(queue + 30) || !this->timeOut(2000))
  //  {
  //    digitalWrite(25, LOW);
  //    digitalWrite(25, HIGH);
  //  }
  delay(300);
  while (!done || this->timeOut(5000))
  {
    if (queue < 4)
    {
      this->latch(2);
      temp = digitalRead(queue + 30);
      digitalWrite(queue * 2 + 34, temp);
      done = !temp;
    }
    else 
    {
      this->latch(3);
      temp = digitalRead(queue + 22);
      digitalWrite(queue * 2 + 27, temp);
      done = !temp;
    }
  }
  bill_funds = 0;
  if (!card)
  {
    Display.idle();
    MDB.no_add = 1;
    MDB.setCommand("STACK BILL");
  }
  received = 0;
  rfid_data = 0;
  state = ALL;
  in_progress = 0;
  if (done)
  {
    if (queue < 4)
    {
      digitalWrite(queue * 2 + 34, LOW);
    }
    else
    {
      digitalWrite(queue * 2 + 31, LOW);
    }
    Serial.println("Complete");
  }
  if (!done)
  {
    Serial.println("Jam");
    digitalWrite(queue * 2 + 34, LOW);
    soda_enable[queue] = 0;
    coin_funds = coin_funds + prices[queue];
    MDB.setCommand("RETURN COINS");
  }
  queue = 10;
}


void SodaDispenser::latch(uint8_t OE)
{
  digitalWrite(25, LOW);
  digitalWrite(25, HIGH);
  digitalWrite(22, OE & 1);
  digitalWrite(23, (OE >> 1) & 1);
  digitalWrite(24, (OE >> 2) & 1);
}

uint8_t SodaDispenser::timeOut(unsigned long time)
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

//uint8_t SodaDispenser::allOff(void)
//{
//  digitalWrite(34, LOW);
//  digitalWrite(36, LOW);
//  digitalWrite(38, LOW);
//  digitalWrite(40, LOW);
//  digitalWrite(35, LOW);
//  digitalWrite(37, LOW);
//  digitalWrite(39, LOW);
//  digitalWrite(41, LOW);
//  digitalWrite(43, LOW);
//  digitalWrite(45, LOW);
//  done = 1;
//}

SodaDispenser Vend;


