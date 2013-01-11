#ifndef MDB_h
#define MDB_h

#include <inttypes.h>

class MDBSerial
{
	private:
	//VMC Attributes
	String state;
	unsigned long dispense;
        uint8_t cancel;
        uint8_t maxed;
        uint8_t all;
	uint8_t timeoutcount;
	uint8_t buffer[36];
	uint8_t write_buffer[36];
	String reader_state;
	unsigned long start_time;
	//Changer Attributes
	String changer_state;
	uint8_t coin_level;
	uint8_t changer_enable;
	uint8_t changer_jam;
	uint8_t changer_level;
	unsigned int changer_country;
	uint8_t changer_scale;
	uint8_t changer_decimal;
	unsigned int coins_used;
	unsigned int coin_value[16];
	unsigned int tubes;
	unsigned int coin_count[16];
	//Reader Attributes
	uint8_t reader_enable;
	uint8_t reader_level;
	unsigned int reader_country;
	unsigned int reader_scale;
	uint8_t reader_decimal;
	unsigned int reader_capacity;
	unsigned int reader_security;
	unsigned int bills_used;
	uint8_t reader_escrow;
	uint8_t bill_value[16];
	uint8_t stacker_full;
	unsigned int bill_count;
	uint8_t return_escrow;
	uint8_t stack_bill;
	//Changer Methods
	void changer(void);
	void changerPoll(void);
	void changerWait(void);
	void changerSetup(void);
	void changerTubeStatus(void);
	void changerDispense(void);
	void changerReset(void);
	uint8_t coinWithValue(uint8_t);
	uint8_t changerErrorCheck(uint8_t);
	//Reader Methods
	void reader(void);
	void readerPoll(void);
	void readerWait(void);
	void readerSetup(void);
        void readerStacker(void);
	void readerReset(void);
	uint8_t readerErrorCheck(uint8_t);
	//VMC Methods
	void escrowRequest();
	void command(uint8_t);
	void command(uint8_t, uint8_t);
	int read(void);
	int timeOut(void);
	
	public:
        MDBSerial();
	void check(void);
        void setCommand(String);
        uint8_t canceled(void);
	
};
extern MDBSerial MDB;

#endif
