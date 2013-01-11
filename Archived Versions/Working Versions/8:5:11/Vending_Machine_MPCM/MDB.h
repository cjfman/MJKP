#ifndef MDB_h
#define MDB_h

#include <inttypes.h>

class MDBSerial
{
  private:
    String state;
    uint8_t command;
    uint8_t lastcommand;
    String reading;
    String masterstate;
    uint8_t readercommand;
    uint8_t changercommand;
    uint8_t liquidate;
    unsigned int timeoutcount;
    //External Flags
    unsigned int largebill;
    unsigned int coinbalance;
    unsigned long billbalance;
    unsigned int billescrow;
    uint8_t cancel;
    //Changer Info
    uint8_t csetup;
    uint8_t cscale;
    uint8_t cdecimal;
    uint8_t coins[16];
    uint8_t deposit;
    uint8_t ret[32];
    uint8_t retnum;
    uint8_t returned;
    uint8_t returnescrow;
    uint8_t stackescrow;
    uint8_t returncoins;
    //Reader Info
    unsigned int rscale;
    uint8_t rdecimal;
    uint8_t rescrow;
    unsigned int rbills[16];
    uint8_t rsetup;
    uint8_t received;
    uint8_t escrow;
    unsigned int stacker[2];
    uint8_t stacked;
    unsigned int inescrow;
    //Methods
    void monitor(void);
    void override(void);
    int intercept(uint8_t);
    void changer(uint8_t);
    void resetChanger(void);
    void reader(uint8_t);
    void resetReader(void);
    void master(void);
    void readerMaster(void);
    void changerMaster(void);
    void commandCheck(void);
    int timeout(void);
    //void waitForStop(void);
  public:
    MDBSerial();
    void check(void);
    unsigned long canceled(void);
    unsigned long status(void);
    void setEscrow(unsigned int);
    void setCommand(String);
    void setState(String);
    uint8_t ready(void);
};

extern MDBSerial MDB;

#endif
