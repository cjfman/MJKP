/*
 *  VMC.cpp
 *  
 *
 *  Created by Charles Franklin on 8/4/11.
 *  Copyright 2011 Massachusetts Institute of Technology. All rights reserved.
 *
 */


#include "MDB.h"

MDBSerial::MDBSerial()
{
	state = "START";
	coin_level = 2;
	changer_enable = 1;
	timeoutcount = 0;
	changer_jam = 0;
	start_time = 0;
	dispense = 0;
}

////////Public Methods/////////////////////////////////

void MDBSerial::check(void)
{
	if (state == "START")
	{
		MPCMSerial3.end();
		pinMode(14, OUTPUT);
		digitalWrite(14, LOW);
		delay(125);
		reader_state = "RESET";
		changer_state = "RESET";
		state = "CHANGER";
		changer_enable = 1;
		return;
	}
	if (state == "CHANGER")
	{
		this->changer();
	}
	if (state == "READER")
	{
		//this->reader();
	}
//	if (state == "COMMAND")
//	{
//		this->commandCheck();
//	}
}

////////Private Methods/////////////////////////////////

void MDBSerial::changer(void)
{
	//Changer States
	//Reset - a reset command was just sent
	//WAIT - waiting for changer to complete reset
	if (changer_state == "NONE")
	{
		this->command(0x0B); //Poll
		changer_state = "POLL";
		return;
	}
	if (changer_state == "Poll")
	{
		this->changerPoll();
		return;
	}
	if (changer_state == "RESET")
	{
		if (start_time == 0)
		{
			start_time = millis();
			return;
		}
		if (start_time + 500 > millis())
		{
			return;
		}
		start_time = 0;
		this->command(0x0B);
		changer_state = "WAIT";
		return;
	}
	if (changer_state == "WAIT")
	{
		this->changerWait();
		return;
	}
	if (changer_state == "SETUP")
	{
		this->changerSetup();
		return;
	}
	if (changer_state == "TUBE STATUS")
	{
		this->changerTubeStatus();
		return;
	}
	if (changer_state == "Coin Type")
	{
		uint8_t end = this->read();
		if (end == 0)
		{
			this->command(0x0C, 2); //Coin Type
			return;
		}
		changer_state = "None";
		state = "READER";
		return;
	}
	if (changer_state == "DISPENSE")
	{
		if (dispense)
		{	
			this->changerDispense();
		}
		else
		{
			changer_state = "NONE";
			state = "READER";
		}
		return;
	}
}

void MDBSerial::changerPoll()
{
	uint8_t end, i, manual, action, tube, coin;
	end = this->read();
	if (end == 0)
	{
		this->command(0x0B); //Poll
		return;
	}
	if (end == 1 && buffer[0] == 0x00) //ACK
	{
		changer_state = "COMMAND";
		state = "READER";
		return;
	}
	manual = buffer[0] >> 7;
	action = (buffer[0] >> 4) & 0x03;
	coin = buffer[0] & 0x0F;
	tube = buffer[1];
	for (i = 2; i < end - 1; i++)
	{
		if (this->changerErrorCheck(buffer[i]))
		{
			changer_state = "NONE";
			state = "READER";
			return;
		}
		if (buffer[i] == 1) //Escrow Request
		{
			this->escrowRequest();
			changer_state = "COMMAND";
			state = "READER";
			return;
		}
	}
	if (manual) //Coins Dispensed Manually
	{
		//Fill This Later
	}
	else
	{
		coin_funds = coin_funds + coin_value[coin];
	}
	coin_count[coin] = tube;
	changer_state = "COMMAND";
	state = "READER";
}
 

void MDBSerial::changerWait()
{
	uint8_t end, i, just_reset;
	just_reset = 0;
	end = this->read();
	if (end == 0)
	{
		this->command(0x0B); //Poll
		return;
	}
	if (buffer[0] == 0x00) //ACK
	{
		end = this->read();
		if (end == 0 || buffer[0] == 0x00)
		{
			//this->changerReset();
			return;
		}
	}
	for (i = 2; i < end; i++)
	{
		if (this->changerErrorCheck(buffer[i]))
		{
			return;
		}
		if (buffer[i] == 0x0B)
		{
			just_reset = 1;
		}
	}
	if(!just_reset)
	{
		this->changerReset();
		return;
	}
	this->command(0x09); //SETUP
	changer_state = "SETUP";
}

void MDBSerial::changerSetup()
{
	uint8_t end, i;
	end = this->read();
	if (end == 0 || buffer[0] == 0x00)
	{
		this->command(0x09); //SETUP
		return;
	}
	changer_level = buffer[0];
	changer_country = buffer[1] << 8;
	changer_country |= buffer[2];
	changer_scale = buffer[3];
	changer_decimal = buffer[4];
	coins_used = buffer[5] << 8;
	coins_used |= buffer[6];
	for (i = 0; i < 16; i++)
	{
		coin_value[i] = changer_scale * buffer[i+7];
	}
	this->command(0x0A); //Tube Status
	changer_state = "TUBE STATUS";
}

void MDBSerial::changerTubeStatus(void)
{
	uint8_t end, i;
	end = this->read();
	if (end == 0 || (end == 1 && buffer[0] == 0x00))
	{
		this->command(0x0A); //Tube Status
		return;
	}
	tubes = buffer[0] << 8;
	tubes |= buffer[1];
	for (i = 0; i < 16; i++)
	{
		if (i + 2 < end)
		{
			coin_count[i] = buffer[i + 2];
			if (coin_count[i] == 0 && (tubes >> i) == 1)
			{
				coin_count[i] = 0;
				changer_jam = 1;
			}
		}
		else
		{
			coin_count[i] = 0;
		}
	}
	write_buffer[0] = tubes >> 8;
	write_buffer[1] = tubes & 0xFF;
	this->command(0x0C, 2); //Coin Type
	changer_state = "COIN TYPE";
}

void MDBSerial::changerDispense(void)
{
	static uint8_t queue = 0;
	if (queue)
	{
		uint8_t end, i;
		end = this->read();
		if (end == 0 || (end == 1 && buffer[0] == 0x00))
		{
			this->command(0x0D, 1); //Dispense
			return;
		}
		if (buffer[0] == 0x00) //ACK
		{
			coin_funds = coin_funds - coin_value[queue];
			queue = 0;
		}
	}
	if (dispense >= 25)
	{
		queue = this->coinWithValue(25);
		if (coin_count[queue] >= 5)
		{
			write_buffer[0] = queue;
			this->command(0x0D, 1);
			return;
		}
	}
	if (dispense >= 10)
	{
		queue = this->coinWithValue(10);
		if (coin_count[queue] >= 5)
		{
			write_buffer[0] = queue;
			this->command(0x0D, 1);
			return;
		}
	}
	if (dispense >= 5)
	{
		queue = this->coinWithValue(5);
		if (coin_count[queue] >= 5)
		{
			write_buffer[0] = queue;
			this->command(0x0D, 1);
			return;
		}
	}
	changer_state = "NONE";
	state = "READER";
}

void MDBSerial::changerReset(void)
{
	changer_enable = 1;
	changer_jam = 0;
	this->command(0x08);
	changer_state = "RESET";
}

uint8_t MDBSerial::coinWithValue(uint8_t value)
{
	uint8_t i;
	for (i = 0; i < 16; i++)
	{
		while ((coins_used >> i) & 0x01 == 0)
		{
			i++;
		}
		if (coin_value[i] == value)
		{
			return i;
		}
	}
	return 16;
}
		
		
uint8_t MDBSerial::changerErrorCheck(uint8_t data)
{
	if (data == 0x05) //Acceptor Unplugged
	{
		changer_enable = 0;
		return 1;
	}
	if (data == 0x06) //Tube Jam
	{
		changer_enable = 0;
		changer_jam = 1;
		return 1;
	}
	if (data == 0x0C) //Tube Jam
	{
		changer_enable = 0;
		changer_jam = 1;
		return 1;
	}
	return 0;
}

void MDBSerial::command(uint8_t command)
{
	MPCMSerial3.writeMode(command);
	MPCMSerial3.write(command);
}

void MDBSerial::command(uint8_t command, uint8_t end)
{
	uint8_t i;
	unsigned long CHK;
	CHK = command;
	MPCMSerial3.writeMode(command);
	for (i = 0; i < end; i++)
	{
		MPCMSerial3.write(write_buffer[i]);
		CHK = CHK + write_buffer[i];
	}
	MPCMSerial3.write(CHK);
}


int MDBSerial::read(void)
{
	unsigned int i;
	unsigned long sum = 0;
	i = 0;
	while (MPCMSerial3.peek() >> 8 != 1)
	{			
		if (this->timeOut())
		{
			return 0;
		}
		buffer[i] = MPCMSerial3.read();
		sum = sum + buffer[i];
		i++;
	}
	uint8_t CHK = MPCMSerial3.read() & 0xFF;
	if (CHK = 0xFF) //NAK
	{
		return 0;
	}
	if (CHK = 0x00) //ACK
	{
		buffer[0] = 0x00;
		return 1;
	}
	if (sum & 0xFF != CHK)
	{
		MPCMSerial3.write(0xAA); //RET
		return 0;
	}
	MPCMSerial3.write(0x00); //ACK
	return i;
}
	

int MDBSerial::timeOut(void)
{
	unsigned long time = millis();
	while(!MPCMSerial3.available())
	{
		delay(1);
		if (time + 6 < millis())
		{
			timeoutcount++;
			return 1;
		}
	}
	timeoutcount = 0;
	return 0;
}

void MDBSerial::escrowRequest(void)
{
  dispense = coin_funds;
  coin_funds = 0;
}

MDBSerial MDB;
