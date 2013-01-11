/*
 *  Accounts.cpp
 *  
 *
 *  Created by Charles Franklin on 8/26/11.
 *  Copyright 2011 Massachusetts Institute of Technology. All rights reserved.
 *
 */


#include "Accounts.h"

VendingAccounts::VendingAccounts()
{
//  if (!SD.exists("/Accounts/"))
//  {
//    SD.mkdir("/Accounts");
//  }
  prefix = 4682;
  account_total = 0;
  file_loaded = 0;
  account_ID = 13;
  account_name = "";
}

//////////////Public Methods////////////////////

void VendingAccounts::setPath(char* new_path, char* new_path2)
{
  strcpy(accounts_path, new_path);
  strcpy(info_path, new_path2);
  file = SD.open(info_path);
  while (file.read() != ':');
  while (file.read() != ':');
  file.read();
  while (file.peek() != '\n')
  {
    account_total = account_total * 10 + this->asciiToInt(char(file.read()));
  }
  file.close();
}

void VendingAccounts::save(void)
{
  if (file_loaded)
  {
    file.seek(0);
    file.print("Name: " + account_name);
    file.print('\n');
    file.print("ID: " + String(account_ID));
    file.print('\n');
    file.print("Balance: " + String(account_balance));
    file.print('\n');
    file.print("Purchase Count: " + String(account_count));
    file.print('\n');
    file.flush();
  }
}

void VendingAccounts::closeSession(void)
{
  if (file_loaded)
  {
    this->save();
    account_ID = 13;
    account_name = "";
    file.close();
    file_loaded = 0;
  }
}

void VendingAccounts::cancelSession(void)
{
  if (file_loaded)
  {
    file_loaded = 0;
    file.close();
  }
}

String VendingAccounts::createAccount(String name)
{
  unsigned int i;
  unsigned long ID = prefix;
  for(i = 0; account_total/(10^i) > 0; i++)
  {
    ID = prefix*(10^i) + account_total;
  }
  return this->createAccount(name, ID, 0);
}

String VendingAccounts::createAccount(String name, unsigned long ID)
{
  return this->createAccount(name, ID, 0);
}

String VendingAccounts::createAccount(String name, unsigned long ID, unsigned long balance)
{
  String path = "/Accounts/";
  int i;
  path += this->longToHEXString(ID);
  path += ".txt";
  Serial.print("Creating file: ");
  char path_array[100];
  path.toCharArray(path_array, 100);
  Serial.println(path_array);
  file = SD.open(path_array, FILE_WRITE);
  if (file)
  {
    file.print("Name: " + name);
    file.print('\n');
    file.print("ID: " + String(ID));
    file.print('\n');
    file.print("Balance: " + String(balance));
    file.print('\n');
    file.print("Purchase Count: 0");
    file.print('\n');
    file.close();
    String entry = name + "/" + String(ID) + "/" + "1" + "/";
    Serial.println("Adding Entry to List: ");
    Serial.println(entry);
    Serial.println(accounts_path);
    file = SD.open(accounts_path, FILE_WRITE);
    if (!file)
    {
      Serial.println("Entry Not Added");
    }
    file.print(entry);
    file.print('\n');
    file.close();
    Serial.println("File Created");
    account_total++;
    this->updateInfo("NumberOfAccounts", String(account_total));
    return name;
  }
  else
  {
    return "NULL";
  }
}

String VendingAccounts::createAccount(unsigned long ID)
{
  String name = "MJKP" + String(account_total);
  Serial.print("Name Generated: ");
  Serial.println(name);
  return this->createAccount(name, ID);
}

String VendingAccounts::changeAccountName(String name, String new_name)
{
  if (account_name != name)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->fileLookup(name) + ".txt";
    this->loadAccount(path);
  }
  account_name = new_name;
}

String VendingAccounts::changeAccountName(unsigned long ID, String new_name)
{
  if (account_ID != ID)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->longToHEXString(ID) + ".txt";
    this->loadAccount(path);
  }
  account_name = new_name;
}

unsigned long VendingAccounts::getAccountID(String name)
{
  if (account_name != name)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->fileLookup(name) + ".txt";
    this->loadAccount(path);
    this->cancelSession();
  }
  unsigned long res = account_ID;
  account_name = "";
  account_ID = 13;
  return account_ID;
}

String VendingAccounts::getAccountName(unsigned long ID)
{
  if (account_ID != ID)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->longToHEXString(ID) + ".txt";
    this->loadAccount(path);
    this->cancelSession();
  }
  String res = account_name;
  account_name = "";
  account_ID = 13;
  return res;
}

unsigned long VendingAccounts::changeAccountID(String name, unsigned long new_ID)
{
  if (account_name != name)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->fileLookup(name) + ".txt";
    this->loadAccount(path);
  }
  account_ID = new_ID;	
}

long VendingAccounts::getAccountBalance(String name)
{
  if (account_name != name)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->fileLookup(name) + ".txt";
    if (!this->loadAccount(path))
    {
      return -1;
    }
    this->cancelSession();
  }
  account_name = "";
  account_ID = 13;
  return account_balance;
}

long VendingAccounts::getAccountIDBalance(unsigned long ID)
{
  if (account_ID != ID)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->longToHEXString(ID) + ".txt";
    if (!this->loadAccount(path))
    {
      return -1;
    }
    this->cancelSession();
  }
  account_name = "";
  account_ID = 13;
  return account_balance;
}

long VendingAccounts::creditAccount(String name, unsigned long credit)
{
  if (account_name != name)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->fileLookup(name) + ".txt";
    if (!this->loadAccount(path))
    {
      return -1;
    }
  }
  account_balance += credit;
  return account_balance;
}	

long VendingAccounts::creditAccount(unsigned long ID, unsigned long credit)
{
//  Serial.println("Crediting Account");
  if (account_ID != ID)
  {
//    Serial.print("Opening File: ");
    this->closeSession();
    String path = "/Accounts/";
    path += this->longToHEXString(ID) + ".txt";
//    Serial.println(path);
    if (!this->loadAccount(path))
    {
//      Serial.println("Failed");
      return -1;
    }
  }
  account_balance += credit;
  return account_balance;
}

unsigned long VendingAccounts::count(String name, long change)
{
  if (account_name != name)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->fileLookup(name) + ".txt";
    if (!this->loadAccount(path))
    {
      return -1;
    }
  }
  account_count += change;
  if (account_count < 0)
  {
    account_count = 0;
  }
  this->closeSession();
  return account_count;
}

unsigned long VendingAccounts::count(unsigned long ID, long change)
{
  if (account_ID != ID)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->longToHEXString(ID) + ".txt";
    if (!this->loadAccount(path))
    {
      return -1;
    }
  }
  account_count += change;
  if (account_count < 0)
  {
    account_count = 0;
  }
  this->closeSession();
  return account_count;
}

unsigned long VendingAccounts::count(String name)
{
  if (account_name != name)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->fileLookup(name) + ".txt";
    if (!this->loadAccount(path))
    {
      return -1;
    }
    this->cancelSession();
  }
  account_name = "";
  account_ID = 13;
  return account_count;
}

unsigned long VendingAccounts::count(unsigned long ID)
{
  if (account_ID != ID)
  {
    this->closeSession();
    String path = "/Accounts/";
    path += this->longToHEXString(ID) + ".txt";
    if (!this->loadAccount(path))
    {
      return -1;
    }
    this->cancelSession();
  }
  account_name = "";
  account_ID = 13;
  return account_count;
}

/////////////Private Methods///////////////////////

unsigned int VendingAccounts::asciiToInt(unsigned int character)
{
  return character - 48;
}

void VendingAccounts::updateInfo(String value, String info)
{
  file = SD.open(info_path, FILE_WRITE);
  file.seek(0);
  String data = "";
  while (file.available())
  {
    data += char(file.read());
  }
  value += ": ";
  unsigned long pos;
  pos = data.indexOf(value);
  pos = pos + value.length();
  unsigned long end = pos;
  while (data[end] != '\n')
  {
    end++;
  }
  data = data.substring(0, pos) + info + data.substring(end);
  file.seek(0);
  file.print(data);
}

int VendingAccounts::loadAccount(String path)
{
  file.close();
  char path_array[100];
  path.toCharArray(path_array, 100);
  if (!SD.exists(path_array))
  {
    return 0;
  }
  file = SD.open(path_array, FILE_WRITE);
  if (!file)
  {
    return 0;
  }
  file.seek(0);
  while (file.read() != ':');
  file.read();
  account_name = "";
  while (file.peek() != '\n')
  {
    account_name += char(file.read());
  }
  while (file.read() != ':');
  file.read();
  account_ID = 0;
  while (file.peek() != '\n')
  {
    account_ID = account_ID * 10 + this->asciiToInt(char(file.read()));
  }
  while (file.read() != ':');
  file.read();
  account_balance = 0;
  while (file.peek() != '\n')
  {
    account_balance = account_balance * 10 + this->asciiToInt(char(file.read()));
  }
  while (file.read() != ':');
  file.read();
  account_count = 0;
    while (file.peek() != '\n')
  {
    account_count = account_count * 10 + this->asciiToInt(char(file.read()));
  }
  file_loaded = 1;
  return 1;
}

String VendingAccounts::longToHEXString(unsigned long num)
{
  String res = "";
  int i;
  for (i = 0; i < 8; i++)
  {
    int half_byte = (num >> (i * 4)) & 0xF;
    if (half_byte < 10)
    {
      half_byte += 48;
    }
    else
    {
      half_byte += 55;
    }
    res = char(half_byte) + res;
  }
  while (res[0] == '0' && res.length() != 1)
  {
    res = res.substring(1);
  }
  return res;
}

String VendingAccounts::fileLookup(String name)
{
  file = SD.open(accounts_path);
  unsigned int tested = 0;
  String found;
  while (found != name)
  {
    found = "";
    while (file.peek() != '/')
    {
      if (file.peek() == '\0')
      {
        return 0;
      }
      found += file.read();
    }
    tested++;
    if (found != name)
    {
      unsigned long num = account_total;
      num++;
      if (tested > num)
      {
        return 0;
      }
      while (file.read() != '\n');
    }
  }
  file.read();
  unsigned long ID = 0;
  while (file.peek() != '/')
  {
    ID = ID * 10 + this->asciiToInt(file.read());
  }
  file.close();
  return this->longToHEXString(ID);
}

VendingAccounts Accounts;
