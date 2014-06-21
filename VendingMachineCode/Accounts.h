//Accounts Library Developed by Charles Franklin for RFID Vending Machine

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

#ifndef ACCOUNTS_h
#define ACCOUNTS_h

#include <SD.h>
#include <inttypes.h>


//#define CASH_ONLY

class Accounts
{
public:
  Accounts();
  Accounts(unsigned long);
  Accounts(String);

  // Setup Functions
  static void setup();
  static void setPath(char*, char*, char*);
  static void loadInfo();
  
  // Saving and Closing Accounts
  boolean save();
  boolean close();
  void cancel();
  void updateCache();
  
  // Creation of Accounts
  static boolean createAccount(String name);
  static boolean createAccount(String name, unsigned long ID);
  static boolean createAccount(unsigned long ID);
  static boolean createAccount(String name, unsigned long ID, unsigned long balance);
  
  // Account Access Fuctions
  boolean exists();
  unsigned long getID();
  String getName();
  long getBalance();
  
  // Account Editing Functions
  long credit(long);
  long charge(long);
  unsigned long count();
  unsigned long count(long);
  void setCount(unsigned long);
  boolean changeAccountName(String);
  boolean changeAccountID(unsigned long);
  
  // Info Functions
  static boolean updateInfo(void);
  
private:
  // Account Fuctions
  boolean loadAccount();
  boolean fileLookup();
  void clearAccount();

  // Utility Functions
  static unsigned int asciiToInt(unsigned char);
  static String longToHexString(unsigned long);
  static boolean addToList(unsigned long);
  static void removeFromList(unsigned long);
  static boolean isInList(unsigned long);
  static boolean nameDisable(String);

  // Variables
  static boolean cash_only;
  static unsigned long account_total;
  static char password[16];
  
  // Files
  static char accounts_path[20];
  static char info_path[20];
  static char account_files_path[64];
  static char current_file[64];
  static unsigned long open_accounts[16];
  
  // Accounts
  char path[64];
  char account_name[32];
  char old_name[32];
  unsigned long account_ID;
  unsigned long purchase_count;
  long account_balance;
  String cache_entry;
  boolean edited;
};

#endif
