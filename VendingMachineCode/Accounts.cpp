//Accounts Library Developed by Charles Franklin for RFID Vending Machine
//Assumes SD and Serial have been initialized

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

#include "Accounts.h"
#include "LCD.h"
#include "Log.h"

// Variables
boolean Accounts::cash_only;
unsigned long Accounts::account_total;
char Accounts::password[16];
  
// Files
char Accounts::accounts_path[20];
char Accounts::info_path[20];
char Accounts::account_files_path[64];
char Accounts::current_file[64];
unsigned long Accounts::open_accounts[16];

Accounts::Accounts()
{
  clearAccount();
}

Accounts::Accounts(unsigned long ID)
{
  // Opens the account by using account ID
  
  // Build file path
  String temp_path = String(account_files_path);
  temp_path += longToHexString(ID) + ".txt";
  temp_path.toCharArray(path, 64);
  //Log::print("Opening path: " + temp_path);
  if(!loadAccount() || isInList(account_ID))
  {
    Log::print("Account Read Failure");
    clearAccount();
  }
  else
  {
    addToList(account_ID);
    cache_entry = "NULL";
    edited = false;
  }
}

Accounts::Accounts(String name)
{
  // Opens the account by using account name
  
  fileLookup();
  if(!loadAccount() || isInList(account_ID))
  {
    clearAccount();
  }
  else
  {
    addToList(account_ID);
    cache_entry = "NULL";
    edited = false;
  }
}
  
/************************************************************************************
* * * * * * * * * * * * * * * * * *  Setup Functions * * * * * * * * * * * * * * * * 
************************************************************************************/

void Accounts::setup()
{
  // This Function sets up the SD card and account handling 
  // SD
  int tries;
  pinMode(4, OUTPUT);
  pinMode(53, OUTPUT);
  digitalWrite(53, HIGH);
  Log::print("Opening SD Card");
  for(tries = 0; !SD.begin(4) && tries < 10; tries++) {
    Log::print("Attempt " + String(tries + 1));
    delay(1000);
  }
  if(tries == 10)
  {
    //If SD card fails to initiate 10 times, enter cash only mode
    Log::print("SD Card Failure");
    LCD::alt("Out Of Order", "SD Card Failure");
    return;
  }
  else
  {
    cash_only = false;
  }
#ifndef CASH_ONLY
  cash_only = false;
#else
  cash_only = true;
#endif  // CASH_ONLY
  
  // Default File Paths
  setPath("/Accounts.txt", "/Info.txt", "/Accounts/");
  loadInfo();
  
  // Other variables
  
  int i;
  for (i = 0; i < 16; i++)
  {
    open_accounts[i] = -1;
  }
  if (cash_only) {
    Serial.println("Cash Only");
    LCD::alt("Michael Jackson The King of Pop", "Cash Only");
  }
}

void Accounts::setPath(char* new_accounts, char* new_info, char* new_files)
{
  // This function sets up the file paths
  // for the accounts file and the info file
  
  strcpy(accounts_path, new_accounts);
  strcpy(info_path, new_info);
  strcpy(account_files_path, new_files);
}

void Accounts::loadInfo()
{
  // This function parses the file at info_path for general information and settings
  
  if (cash_only) return;
  
  File file = SD.open(info_path);
  if (!file)
  {
    cash_only = true;
  }
  
  // First Entry : Password
  while (file.read() != ':');
  file.read(); // Clear space
  int i;
  for (i = 0; file.peek() != '\n' && i < 16; i++)
  {
    password[i] = file.read();
  }

  // Second Entry : Account Total    
  while (file.read() != ':');
  file.read(); // Clear space
  // Read line and convert to decimal at same time
  while (file.peek() != '\n')
  {
    account_total = account_total * 10 + asciiToInt(char(file.read()));
  }
  file.close();
}

/************************************************************************************
* * * * * * * * * * * * * * * Opening and Closes Accounts  * * * * * * * * * * * * *  
************************************************************************************/

boolean Accounts::save()
{
  // This function saves the currently open account
  
  if(!edited)
  {
    return true;
  }
  
  updateCache();
  
  // Open file
  File file = SD.open(path, FILE_WRITE);
  
  if (!file)
  {
    return false;
  }
  
  file.seek(0);
  
  file.print("Name: " + String(account_name));
  file.print('\n');
  file.print("ID: " + String(account_ID));
  file.print('\n');
  file.print("Balance: " + String(account_balance));
  file.print('\n');
  file.print("Purchase Count: " + String(purchase_count));
  file.print('\n');
  file.flush();
  file.close();
  
  return true;
}

boolean Accounts::close()
{
  // This function saves and closes the account that is open
  if(!save())
  {
    return false;
  }
  removeFromList(account_ID);
  return true;
}

void Accounts::cancel()
{
  // This function closes the account that is open
  // without saving
  
  removeFromList(account_ID);
}

void Accounts::updateCache()
{
  // Updates the accounts cache file
  
  if (cache_entry == "NULL")
  {
    return;
  }
  
  // Delete Old Entry
  nameDisable(old_name);
  
  // Write new entry
  File file = SD.open(accounts_path, FILE_WRITE);
  file.print(cache_entry);
  file.print('\n');
  file.close();
  
  cache_entry = "NULL";
}

/************************************************************************************
* * * * * * * * * * * * * * * * * * Account Creation  * * * * * * * * * * * * * * * * 
************************************************************************************/

boolean Accounts::createAccount(String name)
{
  // This function creates a new account with name name,
  // 0 balance, and auto generates an ID
  
  unsigned long ID = 4346; // ID prefix
  int i;
  for(i = 0; account_total/(10^i) > 0; i++)
  {
    ID = 4346 * (10^i) + account_total; // Add prefix to account total to get ID
  }
  return createAccount(name, ID, 0);
}

boolean Accounts::createAccount(String name, unsigned long ID)
{
  // This function creates a new account with name name,
  // ID ID, and 0 balance
  
  return createAccount(name, ID, 0);
}

boolean Accounts::createAccount(unsigned long ID)
{
  // This function creates an account with ID ID
  // and auto generates a name
  
  String name = "MJKP" + String(account_total + 1);
  return createAccount(name, ID);
}

boolean Accounts::createAccount(String name, unsigned long ID, unsigned long balance)
{
  // This function creates a new account with the paramaters passed in
  
  Log::print("Creating file for: " + name + ", " + String(ID));
  
  String path = String(account_files_path);
  path += longToHexString(ID) + ".txt"; // use hexadecimal of ID for Account file name
  
  char path_array[64];
  path.toCharArray(path_array, 64);
  
  // Create new file
  File file = SD.open(path_array, FILE_WRITE);
  if (!file)
  {
    return false;
  }
  file.print("Name: " + name);
  file.print('\n');
  file.print("ID: " + String(ID));
  file.print('\n');
  file.print("Balance: " + String(balance));
  file.print('\n');
  file.print("Purchase Count: 0");
  file.print('\n');
  file.close();
  
  Log::print("File Created");
  
  // Update Account info file
  file = SD.open(accounts_path, FILE_WRITE);
  if (!file)
  {
    return false;
  }
  Log::print("Adding Entry to List");
  String entry = name + "/" + String(ID) + "/" + "1" + "/";
  file.print(entry);
  file.print('\n');
  file.close();
  account_total++;
  updateInfo();
  
  return true;
}
  
/************************************************************************************
* * * * * * * * * * * * * * * * Account Access Functions * * * * * * * * * * * * * *
************************************************************************************/

boolean Accounts::exists()
{
  return account_ID != -1;
}

unsigned long Accounts::getID()
{
  return account_ID;
}

String Accounts::getName()
{
  return account_name;
}

long Accounts::getBalance()
{
  return account_balance;
}

/************************************************************************************
* * * * * * * * * * * * * * * * Account Editing Functions  * * * * * * * * * * * * *
************************************************************************************/

long Accounts::credit(long change)
{
  edited = true;
  account_balance += change;
  return account_balance;
}

long Accounts::charge(long change)
{
  edited = true;
  account_balance -= change;
  return account_balance;
}

unsigned long Accounts::count()
{
  edited = true;
  return ++purchase_count;
}

unsigned long Accounts::count(long change)
{
  edited = true;
  purchase_count += change;
  return purchase_count;
}

void Accounts::setCount(unsigned long new_count)
{
  edited = true;
  purchase_count = new_count;
}

boolean Accounts::changeAccountName(String new_name)
{
  // Changes the name of account to new_name
  
  Log::print("Changing name: " + String(account_name) + " >> " + new_name);
  
  if (cache_entry != "NULL")
  {
    Log::print("Failed to make change. Save previous changes first");
    return false;
  }
  
  // Save old name
  strcpy(old_name, account_name);
  
  // Change name of file
  new_name.toCharArray(account_name, 16);
  
  // Add to account cache buffer
  cache_entry = new_name + "/" + String(account_ID) + "/" + "1" + "/";
  
  edited = true; 
  return true;
}

boolean Accounts::changeAccountID(unsigned long new_ID)
{
  // Changes the ID of account to new_name
  
  Log::print("Changing ID: " + String(account_ID) + " >> " + new_ID);
  
  if (cache_entry != "NULL")
  {
    Log::print("Failed to make change. Save previous changes first");
    return false;
  }
  
  // Save old name
  strcpy(old_name, account_name);
  
  // Change name of file
  account_ID = new_ID;
  
  // Add to account cache buffer
  cache_entry = String(account_name) + "/" + String(account_ID) + "/" + "1" + "/";
   
  edited = true;
  return true;
}

/************************************************************************************
* * * * * * * * * * * * * * * * * *  Info Functions  * * * * * * * * * * * * * * * * 
************************************************************************************/

boolean Accounts::updateInfo(void)
{
  Log::print("Updating Account Info File");
  File file = SD.open(info_path, FILE_WRITE);
  if (!file)
  {
    Log::print("Did Not Update Account Info File");
    return false;
  }
  file.seek(0);
  file.print("Info");
  file.print('\n');
  file.print("Password: ");
  file.print(password);
  file.print('\n');
  file.print("NumberOfAccounts: ");
  file.print(account_total);
  file.print('\n');
  file.close();
}

/************************************************************************************
* * * * * * * * * * * * * * * * * * *  Account Functions  * * * * * * * * * * * * * * 
************************************************************************************/

boolean Accounts::loadAccount()
{
  // This function loads an account from a file
  
  if (!SD.exists(path))
  {
    return false;
  }
  
  File file = SD.open(path, FILE_WRITE);
  if (!file)
  {
    return false;
  }
  
  file.seek(0);
  
  // Load Account Name
  while (file.read() != ':');
  file.read();
  int i;
  for (i = 0; file.peek() != '\n' && i < 32; i++)
  {
    account_name[i] = file.read();
  }
  account_name[i] = '\0';  // Null terminate string
  
  // Load Account ID
  while (file.read() != ':');
  file.read();
  account_ID = 0;
  while (file.peek() != '\n')
  {
    account_ID = account_ID * 10 + this->asciiToInt(char(file.read()));
  }
  
  // Load Account Balance
  while (file.read() != ':');
  file.read();
  account_balance = 0;
  while (file.peek() != '\n')
  {
    account_balance = account_balance * 10 + this->asciiToInt(char(file.read()));
  }
  
  // Load Account Purcahse Count
  while (file.read() != ':');
  file.read();
  purchase_count = 0;
  while (file.peek() != '\n')
  {
    purchase_count = purchase_count * 10 + this->asciiToInt(char(file.read()));
  }

  return true;
}

boolean Accounts::fileLookup()
{
  // This function looks up the file path of an account using the name
  
  String name = String(account_name);
  Log::print("File Lookup: " + name);

  File file = SD.open(accounts_path);
  
  // Look for entry in lookup table
  String found;
  unsigned long pos;
  while (found != name)
  {
    found = "";
    while (file.peek() != '/')
    {
      if (file.available() == 0)
      {
        Log::print("End of File Reached");
        return false;
      }
      found += char(file.read());
    }
    pos = file.position();
    file.read();
    while (file.read() != '/');
    if (file.read() != '1')
    {
      Log::print("Account Name Disabled");
      found = "NULL";
    }
    if (found != name)
    {
      while (file.read() != '\n');
    }
  }
  
  // Read ID
  file.seek(pos);
  file.read();
  unsigned long ID = 0;
  while (file.peek() != '/')
  {
    ID = ID * 10 + asciiToInt(file.read());
  }
  file.close();
  longToHexString(ID).toCharArray(path, 64);
  return true;
}

void Accounts::clearAccount()
{
  strcpy(path, "");
  strcpy(account_name, "");
  account_ID = -1;
  purchase_count = 0;
  account_balance = 0;
  cache_entry = "NULL";
  edited = false;
}

/************************************************************************************
* * * * * * * * * * * * * * * * * * *  Utility Functions  * * * * * * * * * * * * * * 
************************************************************************************/

unsigned int Accounts::asciiToInt(unsigned char c)
{
  return (unsigned int)c - 48;
}

String Accounts::longToHexString(unsigned long num)
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

boolean Accounts::addToList(unsigned long id)
{
  int i;
  for (i = 0; i < 16; i++)
  {
    if (open_accounts[i] == -1)
    {
      open_accounts[i] = id;
      return true;
    }
  }
  return false;
}

void Accounts::removeFromList(unsigned long id)
{
  boolean found = false;
  int i;
  for (i = 0; i < 15; i++)
  {
    if (open_accounts[i] == id || found)
    {
      open_accounts[i] = open_accounts[i+1];
      found = true;
    }
  }
  open_accounts[15] = -1;
}

boolean Accounts::isInList(unsigned long id)
{
  int i;
  for (i = 0; i < 16; i++)
  {
    if (open_accounts[i] == id)
    {
      return true;
    }
  }
  return false;
}

boolean Accounts::nameDisable(String name)
{
  // This function disables an account entry in the cache file
  
  
  Log::print("Name Disable: " + name);

  File file = SD.open(accounts_path, FILE_WRITE);
  file.seek(0);
  
  // Parse file and look for name
  String found;
  while (found != name)
  {
    // Look for entry
    found = "";
    while (file.peek() != '/')
    {
      if (!file.available())
      {
        // Entry not found
        return false;
      }
      found += char(file.read());
    }
    
    // See if entry is enabled
    while (file.read() != '/' && file.available());
    while (file.read() != '/' && file.available());
    if (asciiToInt(file.read()) == 0)
    {
      // The found entry was already disabled
      // Continue search
      found = "";
    }
    
    if (found != name)
    {
      while (file.read() != '\n');
    }
  }
  
  // Disable entry
  file.write("0");
  file.flush();
  file.close();
  
  return true;
}

