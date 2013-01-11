#include "Accounts.h"

//Constructor//////////////////////////////////////

VendingAccounts::VendingAccounts()
{
  file_loaded = 0;
  cp = 0;
  prefix = 4682;
}

//Private Methods/////////////////////////////////

unsigned long VendingAccounts::numberOfAccounts(void)
{
  unsigned long num, pos;
  pos = cp;
  this->loadFile();
  //this->openAccount("NumberOfAccounts");
  this->nextValue(5);
  num = this->readValue();
  cp = pos;
  return num;
} 

void VendingAccounts::loadFile(void)
{
  if(!file_loaded)
  {
    file = SD.open(path);
    while(file.available())
    {
      data = data + char(file.read());
    }
    file_loaded = 1;
    file.close();
  }
}

char VendingAccounts::peek(void)
{
  return data[cp];
}

char VendingAccounts::read(void)
{
  cp++;
  return data[cp - 1];
}

unsigned long VendingAccounts::openAccount(String name)
{
  unsigned int tested = 0;
  String found;
  this->resetCursor();
  while (found != name)
  {
    found = "";
    while (char(this->peek()) != '/')
    {
      if (char(this->peek()) == '\0')
      {
        return 0;
      }
      found += char(this->read());
    }
    tested++;
    if (found != name)
    {
      unsigned long num = this->numberOfAccounts();
      num = num + 2;
      if (tested > num)
      {
        return 0;
      }
      this->nextLine();
    }
  }
  return 1;
}

unsigned long VendingAccounts::openAccountID(unsigned long ID)
{
  return this->openAccountID(String(ID));
}

unsigned long VendingAccounts::openAccountID(String ID)
{
  unsigned long tested = 0;
  String found;
  long pos;
  this->resetCursor();
  while (found != ID)
  {
    found = "";
    pos = cp;
    this->nextValue(1);
    while (char(this->peek()) != '/')
    {
      if (char(this->peek()) == '\0')
      {
        return 0;
      }
      found += char(this->read());
    }
    tested++;
    if (found != ID)
    {
      if (tested > this->numberOfAccounts() + 2)
      {
        return 0;
      }
      this->nextLine();
    }
  }
  cp = pos;
  return 1;
}
  
unsigned long VendingAccounts::getBalance(void)
{
  this->nextValue(2);
  return this->readValue();
}

unsigned long VendingAccounts::writeCount(unsigned long count)
{
  unsigned long pos = cp;
  this->nextValue(3);
  this->write(count);
  cp = pos;
  this->nextValue(3);
  return this->readValue();
}

void VendingAccounts::writeBalance(unsigned long balance)
{
  this->nextValue(2);
  this->write(balance);
}

void VendingAccounts::nextLine(void)
{
  while (char(this->read()) != '\n');
}

void VendingAccounts::beginingOfLine(void)
{
  while(char(this->peek() != '\n'))
  {
    cp--;
  }
  cp++;
}

unsigned long VendingAccounts::readValue(void)
{
  unsigned long number;
  number = acsiiTolong(this->read());
  while (char(this->peek()) != '/')
  {
    number = number * 10;
    number += acsiiTolong(this->read());
  }
  //Serial.println(number);
  return number;
}

void VendingAccounts::nextValue(unsigned long times)
{
  long i;
  for (i = 0; i < times; i++)
  {
    while (char(this->read()) != '/');
  }
}
  
unsigned long VendingAccounts::acsiiTolong(unsigned long character)
{
  return character - 48;
}

void VendingAccounts::resetCursor(void)
{
  cp = 0;
}

void VendingAccounts::write(unsigned long i)
{
  String string = String(i);
  this->write(string);
}

void VendingAccounts::write(String new_data)
{
  unsigned long pos = cp;
  this->nextValue(1);
  data = data.substring(0, pos) + new_data + data.substring(cp-1);
  cp = pos;
} 

void VendingAccounts::removeValue(void)
{
  unsigned long pos = cp;
  this->nextValue(1);
  cp--;
  data = data.substring(0, pos) + data.substring(cp);
  cp = pos;
}
  
  
String VendingAccounts::readString(void)
{
  String string = "";
  while (char(this->peek()) != '/')
  {
    string = string + this->read();
  }
  return string;
}

//Public Methods////////////////////////////////////////

void VendingAccounts::setPath(char* new_path)
{
  strcpy(path, new_path);
}

void VendingAccounts::save(void)
{
  if (file_loaded)
  {
    file = SD.open(path, FILE_WRITE);
    file.seek(0);
    file.print(data);
    file.print('\0');
    file.close();
    file_loaded = 0;
  } 
}
    
void VendingAccounts::closeSession(void)
{
  this->save();
  data = "";
}

void VendingAccounts::cancelSession(void)
{
  file_loaded = 0;
  this->loadFile();
}

void VendingAccounts::deleteAccount(String name)
{
  this->openAccount(name);
  unsigned long pos = cp;
  this->nextLine();
  data = data.substring(0, pos) + data.substring(cp);
}

String VendingAccounts::allAccounts(void)
{
  int i;
  for (i = 0; i < 13; i++)
  {
    this->nextLine();
  }
  return String("Name/ID/Balance/Purcahse Count") + data.substring(cp);
}

String VendingAccounts::createAccount(String name)
{
  unsigned long num = this->numberOfAccounts();
  unsigned int i;
  unsigned long ID = prefix;
  for(i = 0; num/(10^i) > 0; i++)
  {
    ID = prefix*(10^i) + num;
  }
  return this->createAccount(name, ID, 0);
}

String VendingAccounts::createAccount(String name, unsigned long ID)
{
  return this->createAccount(name, ID, 0);
}

String VendingAccounts::createAccount(String name, unsigned long ID, unsigned long balance)
{
  this->loadFile();
  Serial.println(data);
  String info = '\n'+ name + "/" + String(ID) + "/" + String(balance) + "/" + "0" + "/";
  Serial.println(info);
  data += info;
  Serial.println(data);
  unsigned long num = this->numberOfAccounts();
  Serial.print("Number of Accounts: ");
  Serial.println(num);
  uint8_t successful = this->openAccount(name);
  if (successful)
  {
    Serial.println("Saving Account...");
//    unsigned long num = this->numberOfAccounts();
//    num++;
//    this->resetCursor();
//    this->nextValue(5);
//    this->removeValue();
//    this->write(num);
//    this->resetCursor();
    this->save();
    return name;
  }
  else
  {
    return String("NULL");
  }
}

String VendingAccounts::createAccount(unsigned long ID)
{
    this->loadFile();
    long num = this->numberOfAccounts();
    Serial.print("Number of Accounts: ");
    Serial.println(num, DEC);
    unsigned int i;
    String name = String(prefix) + String(num);
    Serial.print("Name Generated");
    Serial.println(name);
    return this->createAccount(name, ID);
}

String VendingAccounts::changeAccountName(String name, String new_name)
{
  this->loadFile();
  this->openAccount(name);
  this->beginingOfLine();
  this->removeValue();
  this->write(new_name);
  this->beginingOfLine();
  return this->readString();
}

String VendingAccounts::changeAccountName(unsigned long ID, String new_name)
{
  this->loadFile();
  this->openAccountID(ID);
  this->beginingOfLine();
  this->removeValue();
  this->write(new_name);
  this->beginingOfLine();
  return this->readString();
}

unsigned long VendingAccounts::getAccountID(String name)
{
  this->loadFile();
  this->openAccount(name);
  this->nextValue(1);
  long ID = this->readValue();
  this->resetCursor();
  return ID;
}

String VendingAccounts::getAccountName(unsigned long ID)
{
  this->loadFile();
  this->openAccountID(ID);
  return this->readString();
}

unsigned long VendingAccounts::changeAccountID(String name, unsigned long ID)
{
  this->loadFile();
  this->openAccount(name);
  this->nextValue(1);
  this->removeValue();
  this->write(ID);
  this->openAccount(name);
  return this->getAccountID(name);
}

long VendingAccounts::getAccountBalance(String name)
{
  this->loadFile();
  if (!openAccount(name))
  {
    return -1;
  }
  unsigned long balance = getBalance();
  this->resetCursor();
  return balance;
}

long VendingAccounts::getAccountIDBalance(unsigned long ID)
{
  this->loadFile();
  if (!this->openAccountID(ID))
  {
    return -1;
  }
  unsigned long balance = this->getBalance();
  this->resetCursor();
  return balance;
}

long VendingAccounts::creditAccount(String name, unsigned long credit)
{
  this->loadFile();
  this->resetCursor();
  unsigned long balance = credit;
  if (!this->openAccount(name))
  {
    return -1;
  }
  balance += this->getBalance();
  this->openAccount(name);
  this->writeBalance(balance);
  //file.close();
  this->resetCursor();
  return this->getAccountBalance(name);
}

long VendingAccounts::creditAccount(unsigned long ID, unsigned long credit)
{
  this->loadFile();
  this->resetCursor();
  long balance = credit;
  if (!this->openAccountID(ID))
  {
    return -1;
  }
  balance += this->getBalance();
  this->openAccountID(ID);
  this->writeBalance(balance);
  return this->getAccountIDBalance(ID);
}

unsigned long VendingAccounts::count(String name, long change)
{
  this->loadFile();
  this->resetCursor();
  unsigned long count = change;
  if (!this->openAccount(name))
  {
    return -1;
  }
  count = count + this->count(name);
  if (count < 0)
  {
    count = 0;
  }
  this->openAccount(name);
  return this->writeCount(count);
}
  
unsigned long VendingAccounts::count(unsigned long ID, long change)
{
  this->loadFile();
  this->resetCursor();
  unsigned long count = change;
  if (!this->openAccountID(ID))
  {
    return -1;
  }
  count = count + this->count(ID);
  if (count < 0)
  {
    count = 0;
  }
  this->openAccountID(ID);
  return this->writeCount(count);
}

unsigned long VendingAccounts::count(String name)
{
  this->loadFile();
  this->resetCursor();
  if (!this->openAccount(name))
  {
    return -1;
  }
  this->nextValue(3);
  return this->readValue();
}

unsigned long VendingAccounts::count(unsigned long ID)
{
  this->loadFile();
  this->resetCursor();
  if (!this->openAccountID(ID))
  {
    return -1;
  }
  this->nextValue(3);
  return this->readValue();
}

VendingAccounts Accounts;
