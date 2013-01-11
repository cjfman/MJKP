#include "LCD.h"
#include <LiquidCrystal.h>
#include <Arduino.h>

LiquidCrystal lcd(A8, A9, A10, A11, A12, A13, A14);
int backLight = A15;

LCDPanel::LCDPanel()
{
  updated = 1;
  first = 1;
  autoidle = 0;
  scroll = 500;
  remain = 0;
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH); // turn backlight on. Replace 'HIGH' with 'LOW' to turn it off.
  lcd.begin(16,2);              // columns, rows.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                  // start with a blank screen
  lcd.setCursor(0,0);
}

//private///////////////////////////////////////////////////

void LCDPanel::reset(void)
{
  autoidle = 1;
  updated = 1;
  first = 1;
  this->idle();
}

String LCDPanel::convertBalance(long balance)
{
  String new_balance = "$" + String(balance/100) + ".";
  if (balance%100 == 0)
  {
    new_balance = new_balance + "00";
  }
  else if (balance%100 < 10)
  {
    new_balance = new_balance + "0" + balance%100;
  }
  else
  {
    new_balance = new_balance + String(balance%100);
  }
  return new_balance;
}
  
//public///////////////////////////////////////////////////////////////////  

void LCDPanel::clear(void)
{
  lcd.clear();
  lcd.setCursor(0,0);
}

void LCDPanel::check(void)
{
  static unsigned int i, j, len1, len2;
  static unsigned long time, time1, time2;
  unsigned long ctime = millis();
  unsigned int reprint = 0;
  if(!updated)
  {
    this->clear();
    lcd.print(line1);
    lcd.setCursor(0,1);
    lcd.print(line2);
    updated = 1;
    i = 1;
    j = 1;
    time = ctime;
    time1 = ctime;
    time2 = ctime;
    wait = wait * 1000;
    autoidle = autoidle*1000;
    len1 = line1.length();
    len2 = line2.length();
    subline1 = line1.substring(0, 16);
    subline2 = line2.substring(0, 16);
    remain = 0;
  }
  if ((ctime >= time + autoidle) && autoidle > 0)
  {
    this->idle();
  }
  if (ctime >= time1 + 300)
  {
    if (len1 > 16)
    {
      if (i > len1)
      {
        i = 0;
      }
      if(!(i == restpoint || i == 1) || (ctime >= time1 + wait))
      {
        subline1 = line1.substring(i);
        time1 = ctime;
        i++;
        reprint |= 1;
      }
    }
  }
  if (ctime >= time2 + 400)
  {
    if (len2 > 16)
    {
      if (j > len2)
      {
        j = 0;
      }
      if(!(j == 1) || (ctime >= time2 + wait))
      {
        subline2 = line2.substring(j);
        time2 = ctime;
        j++;
        reprint |= 1;
      }
    }
  }
  if (reprint)
  {
    this->clear();
    lcd.print(subline1);
    lcd.setCursor(0,1);
    lcd.print(subline2);
  }
}

void LCDPanel::idle(void)
{
  if (remain)
  {
    return;
  }
  updated = 0;
  remain = 1;
  line1 = "Michael Jackson The King of Pop";
  line2 = "Tap card to make purchase or add credit";
  restpoint = 17;
  wait = 3;
}

void LCDPanel::sessionCanceled(void)
{
  updated = 0;
  line1 = "Session Canceled";
  line2 = "";
  autoidle = 3;
}

void LCDPanel::sessionCanceled(long balance)
{
  updated = 0;
  line1 = "Session Canceled";
  line2 = "Balance " + this->convertBalance(balance);
  autoidle = 7;
}

void LCDPanel::newAccountCreated(String name)
{
  wait = 3;
  updated = 0;
  line1 = "New Account Made";
  line2 = "Name: " + name;
}

void LCDPanel::cardTapped(long balance)
{
  updated = 0;
  wait = 2;
  line1 = "Select Soda or Add Funds";
  line2 = "Balance " + this->convertBalance(balance);
}

void LCDPanel::cardTapped(long balance, String name)
{
  updated = 0;
  wait = 2;
  line1 = "Hello " + name + ". Select Soda or Add Funds";
  line2 = "Balance " + this->convertBalance(balance);
}

void LCDPanel::addFunds(long balance)
{
  updated = 0;
  wait = 2;
  line1 = "Deposit Money";
  line2 = "Added: " + this->convertBalance(balance);
}

void LCDPanel::balanceUpdated(long balance)
{
  updated = 0;
  line1 = "Thank You!";
  line2 = "Balance " + this->convertBalance(balance);
  autoidle = 7;
}

void LCDPanel::insufficientFunds(long balance)
{
  updated = 0;
  line1 = "Inadequate Funds";
  line2 = "Balance " + this->convertBalance(balance);
  autoidle = 7;
}

void LCDPanel::price(String string1, long price)
{
  updated = 0;
  line1 = string1;
  line2 = this->convertBalance(price);
}

void LCDPanel::price(String string1, String string2, long price)
{
  updated = 0;
  line1 = string1;
  line2 = string2 + this->convertBalance(price);
}

void LCDPanel::price(String string1, long price, unsigned int time)
{
  updated = 0;
  line1 = string1;
  line2 = this->convertBalance(price);
  autoidle = time;
}

void LCDPanel::price(String string1, String string2, long price, unsigned int time)
{
  updated = 0;
  line1 = string1;
  line2 = string2 + this->convertBalance(price);
  autoidle = time;
}

void LCDPanel::largeBill(void)
{
  updated = 0;
  wait = 2;
  line1 = "Bill Over $1";
  line2 = "Tap Card to Add to Account";
}

void LCDPanel::timeOut(void)
{
  updated = 0;
  line1 = "Session Time Out";
  line2 = "";
  autoidle = 7;
}

void LCDPanel::timeOut(long balance)
{
  updated = 0;
  line1 = "Session Time Out";
  line2 = "Balance " + this->convertBalance(balance);
  autoidle = 7;
}

void LCDPanel::print(String string)
{
  updated = 0;
  wait = 3;
  line1 = string;
  line2 = "";
}

void LCDPanel::print(String string1, String string2)
{
  updated = 0;
  wait = 3;
  line1 = string1;
  line2 = string2;
} 

void LCDPanel::print(String string1, String string2, int i)
{
  updated = 0;
  wait = 3;
  line1 = string1;
  line2 = string2;
  autoidle = i;
} 

LCDPanel Display;
