//LCD Library Developed by Charles Franklin for RFID Vending Machine
//
//(c) Charles Franklin 2012

#include "LCD.h"
#include <LiquidCrystal.h>
#include <Arduino.h>

LiquidCrystal lcd(A8, A9, A10, A11, A12, A13, A14);
int backLight = A15;

namespace LCD
{
  unsigned int autoidle;
  uint8_t updated;
  uint8_t first;
  unsigned int lines1;
  unsigned int lines2;
  unsigned int wait;
  unsigned int restpoint;
  unsigned int scroll;
  unsigned int remain;
  unsigned int alt_txt;
  String alt1;
  String alt2;
  String line1;
  String line2;
  String old_line1;
  String old_line2;
  String subline1;
  String subline2;
  
  void clear(void)
  {
    lcd.clear();
    lcd.setCursor(0,0);
  }
  
  void alt(String str1, String str2)
  {
    alt1 = str1;
    alt2 = str2;
    alt_txt = 1;
  }
  
  void def(void)
  {
    alt_txt = 0;
  }
  
  void reset(void)
  {
    autoidle = 1;
    updated = 1;
    first = 1;
    idle();
  }
  
  String convertBalance(long balance)
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
  
  void setup(void)
  {
    alt_txt = 0;
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
  
  /******************************************************************************/
  
  void idle(void)
  {
    if (remain)
    {
      return;
    }
    if(alt_txt)
    {
      line1 = alt1;
      line2 = alt2;
      return;
    }
    updated = 0;
    remain = 1;
    line1 = "Michael Jackson The King of Pop";
    line2 = "Tap card to make purchase or add credit";
    restpoint = 17;
    wait = 3;
  }
  
  void sessionCanceled(void)
  {
    updated = 0;
    line1 = "Session Canceled";
    line2 = "";
    autoidle = 3;
  }
  
  void sessionCanceled(long balance)
  {
    updated = 0;
    line1 = "Session Canceled";
    line2 = "Balance " + convertBalance(balance);
    autoidle = 7;
  }
  
  void newAccountCreated(String name)
  {
    wait = 3;
    updated = 0;
    line1 = "New Account Made";
    line2 = "Name: " + name;
  }
  
  void cardTapped(long balance)
  {
    updated = 0;
    wait = 2;
    line1 = "Select Soda or Add Funds";
    line2 = "Balance " + convertBalance(balance);
  }
  
  void cardTapped(long balance, String name)
  {
    updated = 0;
    wait = 2;
    line1 = "Hello " + name + ". Select Soda or Add Funds";
    line2 = "Balance " + convertBalance(balance);
  }
  
  void addFunds(long balance)
  {
    updated = 0;
    wait = 2;
    line1 = "Deposit Money";
    line2 = "Added: " + convertBalance(balance);
  }
  
  void addCredit(long balance)
  {
    updated = 0;
    wait = 2;
    line1 = "Credit:";
    line2 = convertBalance(balance);
  }
  
  void balanceUpdated(long balance)
  {
    updated = 0;
    line1 = "Thank You!";
    line2 = "Balance " + convertBalance(balance);
    autoidle = 7;
  }
  
  void insufficientFunds(long balance)
  {
    updated = 0;
    line1 = "Inadequate Funds";
    line2 = "Balance " + convertBalance(balance);
    autoidle = 7;
  }
  
  void price(String string1, long price)
  {
    if(string1 == line1)
    {
      return;
    }
    updated = 0;
    line1 = string1;
    line2 = convertBalance(price);
  }
  
  void price(String string1, String string2, long price)
  {
    if(string1 == line1)
    {
      return;
    }
    updated = 0;
    line1 = string1;
    line2 = string2 + convertBalance(price);
  }
  
  void price(String string1, long price, unsigned int time)
  {
    if(string1 == line1)
    {
      return;
    }
    updated = 0;
    line1 = string1;
    line2 = convertBalance(price);
    autoidle = time;
  }
  
  void price(String string1, String string2, long price, unsigned int time)
  {
    if(string1 == line1)
    {
      return;
    }
    updated = 0;
    line1 = string1;
    line2 = string2 + convertBalance(price);
    autoidle = time;
  }
  
  void largeBill(void)
  {
    updated = 0;
    wait = 2;
    line1 = "Bill Over $1";
    line2 = "Tap Card to Add to Account";
  }
  
  void timeOut(void)
  {
    updated = 0;
    line1 = "Session Time Out";
    line2 = "";
    autoidle = 7;
  }
  
  void timeOut(long balance)
  {
    updated = 0;
    line1 = "Session Time Out";
    line2 = "Balance " + convertBalance(balance);
    autoidle = 7;
  }
  
  void print(String string)
  {
    print(string, "");
  }
  
  void print(String string, int i)
  {
    print(string, "", i);
  }
  
  void print(String string1, String string2)
  {
    updated = 0;
    wait = 3;
    line1 = string1;
    line2 = string2;
  } 
  
  void print(String string1, String string2, int i)
  {
    updated = 0;
    wait = 3;
    line1 = string1;
    line2 = string2;
    autoidle = i;
  } 
  
  /************************************************************************************/
  
  void run(void)
  {
    static unsigned int i, j, len1, len2;
    static unsigned long time, time1, time2;
    unsigned long ctime = millis();
    unsigned int reprint = 0;
    if(!updated)
    {
      if (old_line1 == line1 && old_line2 == line2)
      {
        updated = 1;
        return;
      }
      old_line1 = line1;
      old_line2 = line2;
      clear();
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
      idle();
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
      clear();
      lcd.print(subline1);
      lcd.setCursor(0,1);
      lcd.print(subline2);
    }
  }
}
