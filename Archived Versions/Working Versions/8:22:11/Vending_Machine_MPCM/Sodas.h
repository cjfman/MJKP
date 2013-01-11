/*
 *  Sodas.h
 *  
 *
 *  Created by Charles Franklin on 8/6/11.
 *  Copyright 2011 Massachusetts Institute of Technology. All rights reserved.
 *
 *
 *  Vend Buttons are locaded on latch 0 and latch 1 inputs 0, 1
 *  Sold Out Switches are locaded on latch 1 inputs 2, 3, 4, 5, 6, 7 
 *     and latch 2 inputs 0, 1, 2, 3
 *  Vend Complete Switches are on latch 2 inputs 4, 5, 6, 7
 *     and latch 3 inputs 0, 1, 2, 3, 4, 5
 *  
 *
 */
#ifndef SODA_h
#define SODA_h

#include <inttypes.h>
#include "Accounts.h"
#include "MDB.h"
#include "LCD.h"

class SodaDispenser
{
	public:
        uint8_t free;
	SodaDispenser();
        void begin(void);
	void check(void);
	void enable(void);
	void disable(void);
        void cardPresent(void);
	uint8_t complete(void);
        unsigned long prices[10];
	String sodas[10];
	
	private:
	String state;
	uint8_t enabled;
	uint8_t queue;
        uint8_t card;
	uint8_t vend_complete;
	uint8_t soda_enable[10];
	uint8_t buttonPush(void);
	uint8_t soldOut(uint8_t);
	void idle();
	void vend();
	void latch(uint8_t);
        void allOff(void);
	uint8_t timeOut(unsigned long);
};
extern SodaDispenser Vend;

#endif
