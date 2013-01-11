#ifndef LCD_h
#define LCD_h

#include <WString.h>
#include <inttypes.h>

class LCDPanel
{
  private:
    unsigned int autoidle;
    uint8_t updated;
    uint8_t first;
    unsigned int lines1;
    unsigned int lines2;
    unsigned int wait;
    unsigned int restpoint;
    unsigned int scroll;
    unsigned int remain;
    String line1;
    String line2;
    String subline1;
    String subline2;
    void reset(void);
    String convertBalance(long);
    
  public:
    LCDPanel();
    void clear(void);
    void check(void);
    void idle(void);
    void sessionCanceled(void);
    void sessionCanceled(long);
    void newAccountCreated(String);
    void cardTapped(long);
    void cardTapped(long, String);
    void addFunds(long);
    void balanceUpdated(long);
    void balanceUpdated(long, long);
    void insufficientFunds(long);
    void largeBill(void);
    void timeOut(void);
    void timeOut(long);
    void print(String);
    void print(String, String);
    void print(String, String, int);
};

extern LCDPanel Display; 

#endif
