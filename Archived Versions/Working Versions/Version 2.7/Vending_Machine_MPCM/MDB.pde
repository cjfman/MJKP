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
  funds_enable = 1;
  changer_enable = 1;
  reader_enable = 1;
  timeoutcount = 0;
  changer_jam = 0;
  start_time = 0;
  dispense = 0;
  stacker_full = 0;
  maxed_r = 0;
  maxed_c = 0;
  all_c = 0;
  all_r = 0;
  return_escrow = 0;
  stack_bill = 0;
  cancel = 0;
}

////////Public Methods/////////////////////////////////

void MDBSerial::check(void)
{
  cancel = 0;
  if (state == "START")
  {
    Serial.println("Bus Reset");
    MPCMSerial3.end();
    pinMode(14, OUTPUT);
    digitalWrite(14, LOW);
    delay(125);
    digitalWrite(14, HIGH);
    MPCMSerial3.begin(9600, 1);//Master
    MPCMSerial3.flush();
    this->command(0x30);
    this->command(0x08);
    reader_state = "RESET";
    changer_state = "RESET";
    state = "CHANGER";
    changer_enable = 1;
    reader_enable = 1;
    funds_enable = 1;
    return_escrow = 0;
    stack_bill = 0;
    maxed_r = 0;
    maxed_c = 0;
    all_c = 0;
    all_r = 0;
    timeoutcount = 0;
    return;
  }
  if (state == "CHANGER")
  {
    this->changer();
  }
  if (state == "READER")
  {
    this->reader();
  }
  //	if (state == "COMMAND")
  //	{
  //		this->commandCheck();
  //	}
}

void MDBSerial::setCommand(String command)
{
  if (command == "RETURN ESCROW")
  {
    return_escrow = 1;
    return;
  }
  if (command == "STACK BILL")
  {
    stack_bill = 1;
    return;
  }
  if (command == "RETURN COINS")
  {
    dispense = dispense + coin_funds;
    coin_funds = 0;
  }
  if (command == "MAX")
  {
    Serial.println("MAXED");
    all_c = 0;
    all_r = 0;
    maxed_r = 1;
    maxed_c = 1;
  }
  if (command == "ALL")
  {
    Serial.println("ALL");
    maxed_c = 0;
    maxed_r = 0;
    all_c = 1;
    all_r = 1;
  }
}

void MDBSerial::busReset(void)
{
  state = "START";
  escrow = 0;
  bill_funds = 0;
  coin_funds = 0;
}

uint8_t MDBSerial::canceled(void)
{
  if (cancel)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

////////Private Methods/////////////////////////////////

void MDBSerial::changer(void)
{
  //Changer States
  //Reset - a reset command was just sent
  //WAIT - waiting for changer to complete reset
//  Serial.print("Changer State: ");
//  Serial.println(changer_state);
  if (changer_state == "NONE")
  {
    if (coin_return)
    {
      coin_return = 0;
      Serial.println("Escrow Request...");
      this->escrowRequest();
    }
    else if (maxed_c)
    {
      Serial.println("Disable All Coins");
      write_buffer[0] = 0;
      write_buffer[1] = 0;
      write_buffer[2] = 0xFF;
      write_buffer[3] = 0xFF;
      this->command(0x0C, 4); //Coin Type
      changer_state = "COIN TYPE";
      maxed_c = 0;
      return;
    }
    else if (all_c)
    {
      Serial.println("Enable All Coins");
      all_c = 0;
      write_buffer[0] = 0;
      write_buffer[1] = 0x0F;
      write_buffer[2] = 0xFF;
      write_buffer[3] = 0xFF;
      this->command(0x0C, 4); //Coin Type
      changer_state = "COIN TYPE";
      return;
    }
    this->command(0x0B); //Poll
    changer_state = "POLL";
  }
  if (changer_state == "POLL")
  {
    this->changerPoll();
    return;
  }
  if (changer_state == "RESET")
  {
    if (start_time == 0)
    {
      start_time = millis();
//      Serial.print("Changer Setup Pause: ");
//      Serial.println(start_time);
      return;
    }
    if (start_time + 800 > millis())
    {
      return;
    }
//    unsigned long end;
//    end = this->read();
//    if (end == -1 || buffer[0] != 0x00)
//    {
//      this->changerReset();
//    }
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
  if (changer_state == "COIN TYPE")
  {
    long end = this->read();
    if (end == -1)
    {
      this->command(0x0C, 4); //Coin Type
      return;
    }
    changer_state = "NONE";
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
  //Serial.println("POLL");
  int end, deposited;
  uint8_t i, action, tube, coin;
  end = this->read();
  if (end == 0)
  {
    return;
  }
  if (end == -1)
  {
    this->command(0x0B); //Poll
    return;
  }
  if (end == 1 && buffer[0] == 0x00) //ACK
  {
    changer_state = "DISPENSE";
    state = "READER";
    return;
  }
  deposited = -1;
  for (i = 0; i < end - 1; i++)
  {
    Serial.print("Data: ");
    Serial.println(buffer[i], HEX);
    if (this->changerErrorCheck(buffer[i]))
    {
      changer_state = "NONE";
      state = "READER";
      return;
    }
//    if (buffer[i] == 0x0B) //Just reset
//    {
//      this->changerReset();
//      return;
//    }
    if (buffer[i] == 1) //Escrow Request
    {
      Serial.println("Escrow Request");
      this->escrowRequest();
      changer_state = "DISPENSE";
      state = "READER";
      return;
    }
    if ((buffer[i] >> 6) & 0x01)
    {
      deposited = i;
    }
  }
  if (deposited >= 0)
  {
    action = (buffer[deposited] >> 4) & 0x03;
    coin = buffer[deposited] & 0x0F;
    tube = buffer[deposited + 1];
    coin_count[coin] = tube;
//    Serial.print("Action: ");
//    Serial.println(action, DEC);
//    Serial.println(buffer[deposited], HEX);
    if (action != 0x03)
    {
      Serial.print("COIN: ");
      Serial.print(coin, DEC);
      Serial.print(", ");
      Serial.println(coin_value[coin], DEC);
      coin_funds = coin_funds + coin_value[coin];
    }
    else
    {
      Serial.println("Rejected");
    }
  }
  changer_state = "DISPENSE";
  state = "READER";
}


void MDBSerial::changerWait()
{
  uint8_t end, i, just_reset;
  just_reset = 0;
  end = this->read();
  if (end == 0)
  {
    //this->command(0x0B); //Poll
    return;
  }
  if (end == -1)
  {
    this->command(0x0B); //Poll
    return;
  }
//  if (buffer[0] == 0x00 && end == 1) //ACK
//  {
//    end = this->read();
//    if (end == -1 || (buffer[0] == 0x00 && end == 1))
//    {
//      this->changerReset();
//      return;
//    }
//  }
  for (i = 0; i < end; i++)
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
  if(!just_reset) //Just Reset
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
  if (end == -1)
  {
    this->command(0x09); //SETUP
    return;
  }
  if (end == 0)
  {
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
  if (end == -1)
  {
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
  delay(25);
  write_buffer[0] = 0;
  write_buffer[1] = 0x0F;
  write_buffer[2] = 0xFF;
  write_buffer[3] = 0xFF;
  this->command(0x0C, 4); //Coin Type
  changer_state = "COIN TYPE";
}

void MDBSerial::changerDispense(void)
{
  static uint8_t queue = 0;
  static uint8_t number = 0;
  static uint8_t count = 0;
  if (queue)
  {
    uint8_t end, i;
    end = this->read();
    if (end == -1)
    {
      this->command(0x0D, 1); //Dispense
      count++;
      return;
    }
    if (end == 0)
    {
      count++;
      return;
    }
    if (buffer[0] == 0x00) //ACK
    {
      dispense -= (number * coin_value[queue]);
      count = 0;
      queue = 0;
      number = 0;
    }
  }
  if (count > 2)
  {
    this->busReset();
    return;
  }
  if (dispense >= 25 && queue == 0)
  {
    queue = this->coinWithValue(25);
    number = dispense / 25;
    if (coin_count[queue] >= 5)
    {
      write_buffer[0] = queue;
      write_buffer[0] |= number << 4;
      this->command(0x0D, 1);
    }
  }
  if (dispense >= 10 && queue == 0)
  {
    queue = this->coinWithValue(10);
    number = dispense / 10;
    if (coin_count[queue] >= 5)
    {
      write_buffer[0] = queue;
      write_buffer[0] |= number << 4;
      this->command(0x0D, 1);
    }
  }
  if (dispense >= 5 && queue == 0)
  {
    queue = this->coinWithValue(5);
    number = dispense / 5;
    if (coin_count[queue] >= 5)
    {
      write_buffer[0] = queue;
      write_buffer[0] |= number << 4;
      this->command(0x0D, 1);
    }
  }
  if (queue)
  {
    uint8_t end;
    end = this->read();
    if (end == 0 || end == -1)
    {
      return;
    }
    if (buffer[0] == 0x00) //ACK
    {
      dispense -= (number * coin_value[queue]);
      queue = 0;
      number = 0;
    }
    return;
  }
  changer_state = "NONE";
  state = "READER";
  queue = 0;
}

void MDBSerial::changerReset(void)
{
  changer_enable = 1;
  changer_jam = 0;
  this->command(0x08);
  changer_state = "RESET";
  delay(200);
  MPCMSerial3.flush();
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

///////////////////Reader Methods////////////////////////////////

void MDBSerial::reader(void)
{
//  Serial.print("Reader State: ");
//  Serial.println(reader_state);
  //reader States
  //Reset - a reset command was just sent
  //WAIT - waiting for changer to complete reset
  if (reader_state == "NONE")
  {
    if (maxed_r)
    {
      Serial.println("No Bills");
      maxed_r = 0;
      funds_enable = 0;
      write_buffer[0] = 0;
      write_buffer[1] = 0;
      write_buffer[2] = write_buffer[0];
      write_buffer[3] = write_buffer[1];
      this->command(0x34, 4);
      reader_state = "BILL TYPE";
    }
    else if (all_r)
    {
      Serial.println("All Bills");
      all_r = 0;
      bill_funds = 0;
      escrow = 0;
      write_buffer[0] = bills_used >> 8;
      write_buffer[1] = bills_used & 0xFF;
      write_buffer[2] = write_buffer[0];
      write_buffer[3] = write_buffer[1];
      this->command(0x34, 4);
      reader_state = "BILL TYPE";
    }
    else
    {
      this->command(0x33); //Poll
      reader_state = "POLL";
    }
  }
  if (reader_state == "POLL")
  {
//    Serial.println("POLL");
    this->readerPoll();
    return;
  }
  if (reader_state == "RESET")
  {
    this->command(0x33); //Poll
    reader_state = "WAIT";
    return;
  }
  if (reader_state == "WAIT")
  {
    this->readerWait();
    return;
  }
  if (reader_state == "SETUP")
  {
    this->readerSetup();
    return;
  }
  if (reader_state == "STACKER")
  {
    this->readerStacker();
    return;
  }
  if (reader_state == "BILL TYPE")
  {
    long end = this->read();
    if (end == -1)
    {
      this->command(0x34, 4); //Bill Type
      return;
    }
    if (end == 0)
    {
      return;
    }
    reader_state = "NONE";
    state = "CHANGER";
    return;
  }
  if (reader_state == "COMMAND")
  {
    if (return_escrow && escrow)
    {
      write_buffer[0] = 0;
      this->command(0x35, 1);
      reader_state = "POLL";
    }
    else if (stack_bill && escrow)
    {
      Serial.println("Sending Stack Bill Command");
      write_buffer[0] = 0x01;
      this->command(0x35, 1);
      reader_state = "POLL";
    }
    else
    {
      reader_state = "NONE";
    }
  }
}

void MDBSerial::readerPoll()
{
  static unsigned long last_stack = 0;
  unsigned long current = millis();
  int end, i, routing, bill, accepted;
  end = this->read();
  if (end == -1)
  {
    this->command(0x33); //Poll
    return;
  }
  if (end == 0)
  {
    return;
  }
  if (end == 1 && buffer[0] == 0x00)
  {
    reader_state = "COMMAND";
    state = "CHANGER";
    return;
  }
  accepted = -1;
  for (i = 0; i < end; i++)
  {
//    Serial.print("Reader Data: ");
//    Serial.println(buffer[i], HEX);
    if (this->readerErrorCheck(buffer[i]))
    {
      reader_state = "NONE";
      state = "CHANGER";
      return;
    }
//    if (buffer[i] == 0x06) //Just reset
//    {
//      this->readerReset();
//      return;
//    }
    if (buffer[i] == 0x0A) //Invalid Escrow Request
    {
      escrow = 0;
      return_escrow = 0;
    }
    if (buffer[i] >> 7)
    {
      accepted = i;
    }
  }
  if (accepted >= 0)
  {
    bill = buffer[accepted] & 0x0F;
    routing = (buffer[accepted] >> 4) & 0x07;
    Serial.print("Bill: ");
    Serial.print(bill, DEC);
    Serial.print(", ");
    Serial.println(bill_value[bill], DEC);
    if (routing == 0x00) //Bill Stacked
    {
      Serial.println("Stacked");
      if (funds_enable)
      {
        if (last_stack + 500 < current)
        {
          Serial.println("Added to Funds");
          bill_funds = bill_funds + bill_value[bill];
          last_stack = current;
        }
        else
        {
          Serial.println("Double Count Ignored");
        }
      }
      else
      {
        Serial.println("Funds Reenabled");
        funds_enable = 1;
      }
      stack_bill = 0;
      escrow = 0;
    }
    else if (routing == 0x01) //Bill In Escrow
    {
      Serial.print("Escrow: ");
      escrow = bill_value[bill];
      Serial.println(escrow);
    }
    else if (routing == 0x02) //Escrow Returned
    {
      Serial.println("Returned");
      escrow = 0;
      stack_bill = 0;
      return_escrow = 0;
      funds_enable = 0;
      this->setCommand("ALL");
    }
  }
  reader_state = "COMMAND";
  state = "CHANGER";
}

void MDBSerial::readerWait()
{
  uint8_t end, i, just_reset;
  just_reset = 0;
  end = this->read();
  if (end == -1)
  {
    this->command(033); //Poll
    return;
  }
  if (end == 0)
  {
    return;
  }
  if (end == -1)
  {
    return;
  }
//  if (buffer[0] == 0x00) //ACK
//  {
//    end = this->read();
//    if (end == 0 || buffer[0] == 0x00)
//    {
//      return;
//    }
//  }
  for (i = 0; i < end; i++)
  {
    if (this->readerErrorCheck(buffer[i]))
    {
      return;
    }
    if (buffer[i] == 0x06)
    {
      just_reset = 1;
    }
  }
  if(!just_reset)
  {
    this->readerReset();
    return;
  }
  this->command(0x31); //SETUP
  reader_state = "SETUP";
}

void MDBSerial::readerSetup()
{
  uint8_t end, i;
  for (i = 0; i < 16; i++)
  {
    buffer[i] = 0;
  }
  end = this->read();
  if (end == 0 || buffer[0] == 0x00)
  {
    this->command(0x31); //SETUP
    return;
  }
  if (end == -1)
  {
    return;
  }
  reader_level = buffer[0];
  reader_country = buffer[1] << 8;
  reader_country |= buffer[2];
  reader_scale = buffer[3] << 8;
  reader_scale |= buffer[4];
  reader_decimal = buffer[5];
  reader_capacity = buffer[6] << 8;
  reader_capacity |= buffer[7];
  reader_security = buffer[8] << 8;
  reader_security |= buffer[9];
  reader_escrow = buffer[10];
  bills_used = 0;
  for (i = 0; i < 16 /*end - 11*/; i++)
  {
    bill_value[i] = reader_scale * buffer[i + 11];
    if (bill_value[i])
    {
      bills_used |= 1 << i;
    }
  }
  this->command(0x36); //Stacker Status
  reader_state = "STACKER";
}

void MDBSerial::readerStacker()
{
  uint8_t end, i;
  end = this->read();
  if (end == 0)
  {
    //this->command(0x31); //SETUP
    return;
  }
  if (end == -1)
  {
    this->command(0x36); //Stacker Statuss
    return;
  }
  stacker_full = buffer[0] >> 7;
  bill_count = (buffer[0] & 0xFE) << 8;
  bill_count |= buffer[1];
  write_buffer[0] = bills_used >> 8;
  write_buffer[1] = bills_used & 0xFF;
  write_buffer[2] = write_buffer[0];
  write_buffer[3] = write_buffer[1];
  this->command(0x34, 4);
  reader_state = "BILL TYPE";
}

void MDBSerial::readerReset(void)
{
  reader_enable = 1;
  this->command(0x30);
  return_escrow = 0;
  stack_bill = 0;
  reader_state = "RESET";
  delay(200);
  MPCMSerial3.flush();
}

uint8_t MDBSerial::readerErrorCheck(uint8_t data)
{
  if (data == 0x01) //Defective Motor
  {
    reader_enable = 0;
    return 1;
  }
  if (data == 0x02) //Sensor Broken
  {
    reader_enable = 0;
    return 1;
  }
  if (data == 0x0C) //Checksum Error
  {
    reader_enable = 0;
    return 1;
  }
  if (data == 0x05) //Bill Jam
  {
    reader_enable = 0;
    return 1;
  }
//  if (data == 0x09) //Unit Disabled
//  {
//    reader_enable = 0;
//    return 1;
//  }
  return 0;
}

/////////////////VMC Methods////////////////////////////////////////////

void MDBSerial::escrowRequest(void)
{
  if (!funds_enable)
  {
    coin_funds += bill_funds;
    bill_funds = 0;
    funds_enable = 1;
  }
  if (coin_funds || escrow)
  {
    Serial.println("Returning Money");
    dispense = coin_funds;
    coin_funds = 0;
    return_escrow = 1;
    cancel = 1;
    Display.print("Returning Money", "", 3);
    all_c = 1;
    all_r = 1;
  }
  else
  {
    Serial.println("No Money To Return");
  }
}

void MDBSerial::command(uint8_t command)
{
//  Serial.print("Command: ");
//  Serial.println(command, HEX);
  MPCMSerial3.writeMode(command);
  MPCMSerial3.write(command);
}

void MDBSerial::command(uint8_t command, uint8_t end)
{
//  Serial.print("Command + Data: ");
//  Serial.println(command, HEX);
//  Serial.print("Bytes: ");
//  Serial.println(end, DEC);
  uint8_t i;
  unsigned long CHK;
  CHK = command;
  for (i = 0; i < end; i++)
  {
//    Serial.print("Data ");
//    Serial.print(i);
//    Serial.print(": ");
//    Serial.println(write_buffer[i], HEX);
    CHK = CHK + write_buffer[i];
  }
  CHK &= 0xFF;
//  Serial.print("CHK: ");
//  Serial.println(CHK, HEX);
  if (end == 1)
  {
    MPCMSerial3.writeMode(command);
    MPCMSerial3.write(write_buffer[0]);
    MPCMSerial3.write(CHK);
  }
  else if (end == 2)
  {
    MPCMSerial3.writeMode(command);
    MPCMSerial3.write(write_buffer[0]);
    MPCMSerial3.write(write_buffer[1]);
    MPCMSerial3.write(CHK);
  }
  else if (end == 3)
  {
    MPCMSerial3.writeMode(command);
    MPCMSerial3.write(write_buffer[0]);
    MPCMSerial3.write(write_buffer[1]);
    MPCMSerial3.write(write_buffer[2]);
    MPCMSerial3.write(CHK);
  }
  else if (end == 4)
  {
    MPCMSerial3.writeMode(command);
    MPCMSerial3.write(write_buffer[0]);
    MPCMSerial3.write(write_buffer[1]);
    MPCMSerial3.write(write_buffer[2]);
    MPCMSerial3.write(write_buffer[3]);
    MPCMSerial3.write(CHK);
  }
  else if (end == 5)
  {
    MPCMSerial3.writeMode(command);
    MPCMSerial3.write(write_buffer[0]);
    MPCMSerial3.write(write_buffer[1]);
    MPCMSerial3.write(write_buffer[2]);
    MPCMSerial3.write(write_buffer[3]);
    MPCMSerial3.write(write_buffer[4]);
    MPCMSerial3.write(CHK);
  }
  else if (end == 6)
  {
    MPCMSerial3.writeMode(command);
    MPCMSerial3.write(write_buffer[0]);
    MPCMSerial3.write(write_buffer[1]);
    MPCMSerial3.write(write_buffer[2]);
    MPCMSerial3.write(write_buffer[3]);
    MPCMSerial3.write(write_buffer[4]);
    MPCMSerial3.write(write_buffer[5]);
    MPCMSerial3.write(CHK);
  }
}

int MDBSerial::read(void)
{
//  Serial.println("READ");
  MPCMSerial3.available();
  unsigned int i;
  unsigned long sum = 0;
  i = 0;
  if (this->timeOut())
  {
    return -1;
  }
  unsigned int temp = MPCMSerial3.read();
//  Serial.print("Byte ");
//  Serial.print(i);
//  Serial.print(": ");
//  Serial.println(temp, HEX);
  while (temp >> 8 != 1)
  {
    //Serial.println("Looping");			
    buffer[i] = temp;
    sum = sum + temp;
    //Serial.print("SUM: ");
    //Serial.println(sum, HEX);
    if (this->timeOut())
    {
      return -1;
    }
    i++;
    temp = MPCMSerial3.read();
//    Serial.print("Byte ");
//    Serial.print(i);
//    Serial.print(": ");
//    Serial.println(temp, HEX);
  }
  uint8_t CHK = temp & 0xFF;
  sum &= 0xFF;
  //Serial.print("CHK: ");
  //Serial.println(CHK, HEX);
  //Serial.print("SUM: ");
  //Serial.println(sum, HEX);
  if (i == 0)
  {
    if (CHK == 0xFF) //NAK
    {
      //Serial.println("NAK");
      return -1;
    }
    if (CHK == 0x00) //ACK
    {
      //Serial.println("ACK");
      buffer[0] = 0x00;
      return 1;
    }
  }
  if (sum != CHK)
  {
    //Serial.println("RET");
    MPCMSerial3.write(0xAA); //RET
    return 0;
  }
  MPCMSerial3.write(0x00); //ACK
  //Serial.print("Data recieved: ");
  //Serial.println(i + 1);
  return i;
}	

int MDBSerial::timeOut(void)
{
  unsigned long time = 0;
  //Serial.println("Checking");
  while(!MPCMSerial3.available())
  {
    time++;
    delay(1);
    if (time > 10)
    {
      timeoutcount++;
      if (timeoutcount % 21 == 20)
      {
        if (timeoutcount > 500)
        {
          state = "START";
          return 1;
        }
        timeoutcount = 0;
      }
      //Serial.println("Time Out");
      return 1;
    }
  }
  timeoutcount = 0;
  //Serial.println("Stuff is available");
  return 0;
}

MDBSerial MDB;

