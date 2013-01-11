#ifndef WEBSERVER_h
#define WEBSERVER_h

#define PASSWORD "Quincy Jones"
#define TRUE 1
#define FALSE 0

#include <SD.h>
#include "Accounts.h"

void debugCheck(void);
void debug(String);
void debugln(String);
void debugHex(int);
void debugCheck(void);

class AdminWebServer
{
private:
  uint8_t first_login;
  //int first_authenticated;
  uint8_t wrong;
  uint8_t authenticated;
  uint8_t pos;
  uint8_t comstep;
  unsigned long start_time;
  unsigned long up_time;
  unsigned long time_out;
  long balance;
  String name;
  String data;
  //int balance;
  unsigned long ID;

  //Methods
  String getPrompt(void);
  void authenticate(String);
  void about(void);
  void mainMenu(String);
  void disconnect(void);
  void addAccount(String);
  //void deleteAccount(String);
  void editAccount(String);
  void listAllAccounts(String);
  void checkBalance(String);
  void creditAccount(String);
  void adminTools(String);
  void chargeAccount(String);
  void upTime(void);
  void timeOut(void);
  long convertStringToLong(String);
public:
  AdminWebServer();
  void check(void);
};

extern AdminWebServer Administrator;

#endif
