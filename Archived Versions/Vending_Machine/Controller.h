#ifndef CONTROLLER_h
#define CONTROLLER_h

#include <inttypes.h>
#include "MDB.h"
#include "Accounts.h"
#include "LCD.h"
#include "Sodas.h"

class Controller
{
  private:
    uint8_t state;
    uint8_t first;
    byte code[6];
    unsigned long account;
    unsigned long balance;
    //Methods
    void idle(void);
    void deposit(void);
    void large(void);
    void tapped(void);
    void autoidle(void);
    uint8_t timeout(unsigned long);
    unsigned long getAccount();
  public:
    Controller(void);
    void check(void);
};

extern Controller MainController;

#endif
