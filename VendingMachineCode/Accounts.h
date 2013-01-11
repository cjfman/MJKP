//Accounts Library Developed by Charles Franklin for RFID Vending Machine
//
//(c) Charles Franklin 2012

#ifndef ACCOUNTS_h
#define ACCOUNTS_h

#include <SD.h>
#include <inttypes.h>
#include "LCD.h"
#include "Log.h"

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
