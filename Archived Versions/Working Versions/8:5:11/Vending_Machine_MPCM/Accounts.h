#ifndef ACCOUNT_h
#define ACCOUNT_h

#include <SD.h>
#include <inttypes.h>

class VendingAccounts
{
   //AccountName|IDNumber|Balance|Purchases
  private:
    bool file_loaded;
    unsigned long cp;
    unsigned int prefix;
    File file;
    char path[20];
    String data;
    
    //Methods
    char peek(void);
    char read(void);
    unsigned long numberOfAccounts(void);
    unsigned long acsiiTolong(unsigned long);
    unsigned long readValue(void);
    unsigned long getBalance(void);
    unsigned long openAccount(String);
    unsigned long openAccountID(unsigned long);
    unsigned long openAccountID(String);
    void writeBalance(unsigned long);
    void nextValue(unsigned long);
    void nextLine(void);
    void beginingOfLine(void);
    void loadFile(void);
    void resetCursor(void);
    void write(unsigned long);
    void write(String);
    void removeValue(void);
        
  public:
    VendingAccounts();
    void setPath(char*);
    void save(void);
    void closeSession(void);
    void cancelSession(void);
    String createAccount(unsigned long);
    String createAccount(String);
    String createAccount(String, unsigned long);
    String createAccount(String, unsigned long, unsigned long);
    String changeAccountName(String, String);
    String changeAccountName(unsigned long, String);
    unsigned long changeAccountID(String, unsigned long);
    unsigned long getAccountID(String);
    long getAccountBalance(String);
    long getAccountIDBalance(unsigned long);
    long creditAccount(String, unsigned long);
    long creditAccount(unsigned long, unsigned long);
};

extern VendingAccounts Accounts;

#endif
