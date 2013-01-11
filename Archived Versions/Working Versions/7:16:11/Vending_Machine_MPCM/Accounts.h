#ifndef ACCOUNT_h
#define ACCOUNT_h

#include <SD.h>
#include <inttypes.h>

class VendingAccounts
{
private:
    File file;
    uint8_t file_loaded;
    int prefix;
    String account_loaded;
    String account_name;
    unsigned long account_ID;
    unsigned long account_balance;
    unsigned long account_count;
    unsigned long account_total;
    char accounts_path[20];
    char info_path[20];
    unsigned int asciiToInt(unsigned int);
//    void updateInfo(String, String);
    void updateInfo(void);
    int loadAccount(String);
    String longToHEXString(unsigned long);
    String fileLookup(String);
public:	
    VendingAccounts();
    void setPath(char*, char*);
    void save(void);
    void closeSession(void);
    void cancelSession(void);
    void deleteAccount(String); ////////DO THIS LATER
    String allAccounts(void); //////////DO THIS LATER
    String createAccount(unsigned long);
    String createAccount(String);
    String createAccount(String, unsigned long);
    String createAccount(String, unsigned long, unsigned long);
    String changeAccountName(String, String);
    String changeAccountName(unsigned long, String);
    unsigned long changeAccountID(String, unsigned long);
    unsigned long getAccountID(String);
    String getAccountName(unsigned long);
    long getAccountBalance(String);
    long getAccountIDBalance(unsigned long);
    long creditAccount(String, long);
    long creditAccount(unsigned long, long);
    unsigned long count(String);
    unsigned long count(unsigned long);
    unsigned long count(String, long);
    unsigned long count(unsigned long, long);
    int exists(unsigned long);
    int exists(String);
};

extern VendingAccounts Accounts;

#endif

