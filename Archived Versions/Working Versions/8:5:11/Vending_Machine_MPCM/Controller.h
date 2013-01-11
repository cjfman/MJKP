#ifndef CONTROLLER_h
#define CONTROLLER_h

#include <inttypes.h>
#include "MDB.h"
#include "Accounts.h"
#include "LCD.h"

class Controller
{
  private:
    uint8_t state;
    uint8_t first;
    byte code[6];
    unsigned long account;
    unsigned long balance;
    unsigned long funds;
    //Methods
    void idle(uint8_t);
    void deposit(uint8_t);
    void large(uint8_t);
    void tapped(uint8_t);
    void autoidle(void);
    uint8_t timeout(unsigned long);
    unsigned long getAccount();
  public:
    Controller(void);
    void check(void);
};

extern Controller MainController;

#endif
