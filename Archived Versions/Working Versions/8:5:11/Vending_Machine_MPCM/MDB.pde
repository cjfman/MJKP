#include "MDB.h"

class MPCMHardwareSerial;

MDBSerial::MDBSerial()
{
  state = "MONITOR";
  reading = "NONE";
  rsetup = 1;
  csetup = 1;
  escrow = 0;
  deposit = 0;
  timeoutcount = 0;
  largebill = 0;
  coinbalance = 0;
  billbalance = 0;
  billescrow = 0;
  cancel = 0;
  returned = 0;
  retnum = 0;
  returnescrow = 0;
  stackescrow = 0;
  returncoins = 0;
  liquidate = 0;
  //ret[] and retnum are used for RET
}

void MDBSerial::check(void)
{
  static long time = 0;
  if (MPCMSerial1.peek() == 0xFF || MPCMSerial2.peek() == 0xFF)
  {
    //Resets everything if a NAK is ever received
    MPCMSerial1.flush();
    MPCMSerial2.flush();
    MPCMSerial3.flush();
    this->resetChanger();
    this->resetReader();
    state = "MONITOR";
    return;
  }
  if (state == "MONITOR")
  {
    this->monitor();
  }
  if (state == "OVERRIDE")
  {
    this->override();
  }
}

void MDBSerial::setCommand(String command)
{
  if (command == "return escrow")
  {
    returnescrow = 1;
    return;
  }
  if (command == "stack escrow")
  {
    stackescrow = 1;
    return;
  }
  if (command == "cancel")
  {
    liquidate = 1;
    return;
  }
}

unsigned long MDBSerial::canceled(void)
{
  if (cancel)
  {
    if (escrow)
    {
      this->setCommand("return escrow");
    }
    cancel = 0;
    return 1;
  }
  else
  {
    return 0;
  }
}

void MDBSerial::setState(String newstate)
{
  if (newstate == "override")
  {
    state = "OVERRIDE";
  }
  else if (newstate == "monitor")
  {
    state = "MONITOR";
    if (MPCMSerial1.available())
    {
      MPCMSerial3.writeMode(MPCMSerial1.read());
    }
    while (MPCMSerial1.available())
    {
      MPCMSerial3.write(MPCMSerial1.read());
    }
    if (MPCMSerial2.available())
    {
      MPCMSerial3.writeMode(MPCMSerial2.read());
    }
    while (MPCMSerial2.available())
    {
      MPCMSerial3.write(MPCMSerial2.read());
    }
  }
}

uint8_t MDBSerial::ready(void)
{
  if (masterstate == "NONE")
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//Private//////////////////////////////////////////

void MDBSerial::monitor(void)
{
  //Serial.println(reading);
  unsigned int data;
//  static uint8_t first = 1;
  //Changer
  if (MPCMSerial1.available())
  {
    data = MPCMSerial1.read();
    if (data >> 8) //Mode bit set
    {
      reading = "CHANGER"; //Marks session start with changer
      lastcommand = command;
      command = data; //Sets the command
      Serial.print("Changer Command: ");
      Serial.println(data, HEX);
    }
    MPCMSerial3.write(data);
    while (MPCMSerial1.available())
    {
      Serial.println(MPCMSerial1.peek(), HEX);
      MPCMSerial3.write(MPCMSerial1.read()); //Forwards data to changer
    }
  }
  else if (MPCMSerial2.available())
  {
    data = MPCMSerial2.read();
    if (data >> 8) //mode bit
    {
      Serial.print("ReaderCommand: ");
      Serial.println(data, HEX);
      reading = "READER"; //Marks session start with reader
      lastcommand = command;
      command = data; //Sets the command
    }
    MPCMSerial3.write(data);
    while (MPCMSerial1.available())
    {
      Serial.println(MPCMSerial2.peek(), HEX);
      MPCMSerial3.write(MPCMSerial2.read()); //Forwards data to changer
    }
  }
  while (MPCMSerial3.available()) //Receives all data from periferals
  {
    Serial.print("Periferal: ");
    data = MPCMSerial3.read();
    MPCMSerial1.write(data);
    Serial.println(data, HEX);
  }
}
//    mdata = MPCMSerial3.read();
//    Serial.print("Periferal: ");
//    Serial.println(mdata, HEX);
//    mdata = intercept(mdata); //Extracts important info and intercepts commands if nessessary
//    if (mdata == -1) //intercept will return -1 upon override
//    {
//      Serial.println("Intercept");
//      return;
//    }
//    if (MPCMSerial3.peek() == 0xEE) //Stop byte received
//    {
//      Serial.println("Stop Byte");
//      received = 0;
//      MPCMSerial3.read(); //Gets rid of stop byte
//      MPCMSerial3.writeMode(mdata); //Writes the last data received with the mode bit set
//      return;
//    }
//    else
//    {
//      Serial.print("Relay: ");
//      Serial.print(mdata, HEX);
//      MPCMSerial1.write(mdata); //Forwards the data to the VMC
//    }

void MDBSerial::override(void)
{
  uint8_t mdata, cdata, bdata;
//  static uint8_t first = 1;
  //Acts as Changer
  if (MPCMSerial1.available())
  {
    cdata = MPCMSerial1.read();
    this->changer(cdata);
  }
  //Acts as Reader
  else if (MPCMSerial2.available())
  {
    bdata = MPCMSerial2.read();
    this->reader(bdata);
  }
  //Acts as Master
  this->master();
}

int MDBSerial::intercept(uint8_t data)
{
  if (reading == "CHANGER" && command == 0xAA) //RET
  {
    csetup = 1;
    received = 0;
    command == lastcommand;
    return data;
  }
  if (reading == "CHANGER" && command == 0x09) //Setup
  {
    if (csetup == 4)
    {
      cscale = data;
    }
    else if (csetup == 5)
    {
      cdecimal = data;
    }
    else if (csetup >= 6 && csetup <= 23)
    {
      coins[csetup - 6] = data * cscale;
    }
    csetup++;
    return data;
  }
  if (reading == "READER" && command == 0xAA and lastcommand == 0x31) //RET
  {
    rsetup = 1;
    return data;
  }
  if (reading == "READER" && command == 0x31) //Setup
  {
    if (rsetup == 4)
    {
      rscale = data;
    }
    else if (rsetup == 6)
    {
      rdecimal = data;
    }
    else if (rsetup == 11)
    {
      if (data == 0x00)
      {
        rescrow = 1;
      }
      else
      {
        rescrow = 0;
      }
    }
    else if (rsetup >= 12 && rsetup <= 27)
    {
      rbills[rsetup - 12] = data * rscale;
    }
    rsetup++;
    return data;
  }
  if (reading == "READER" && command == 0x33 && received == 0) //Poll
  {
    if (data == 0x00) //ACK
    {
      return data;
    }
    uint8_t bill = data & 0x0F; //Gets type of bill
    uint8_t action = (data >> 4) & 0x07; //Sees what happen 
    if (rbills[bill] > 100) //looks to see if the value of the bill is over $1
    {
      if (action == 0x00 || action == 0x01) //If the bill is in escrow or was stacked
      {
        state = "OVERRIDE";
        reading = "NONE";
        received = 0;
        masterstate = "NONE";
        MPCMSerial1.writeMode(0x00); //ACK
        while(MPCMSerial3.available() && MPCMSerial3.read() != 0xEE); //Waits for stop byte
        MPCMSerial3.write(0x00); //ACK
        MPCMSerial3.flush();
        largebill = rbills[bill];
        return -1;
      }
    }
    received++;
  }
  if (reading == "READER" && command == 0x36) //Stacker
  {
    stacker[received] = data;
    received++;
  }
  return data;
}

void MDBSerial::changer(uint8_t data)
{
  uint8_t CHK = 0; //Used to generate Checksum
  if (data == 0x08) //Reset
  {
    this->resetChanger();
    return;
  }
  if (data == 0x0A) //Tube Status
  {
    uint8_t i;
    retnum = 0;
    MPCMSerial1.write(0x00); //Tubes not full
    ret[retnum] = 0x00;
    retnum++; 
    MPCMSerial1.write(0x00); //Tubes not full
    ret[retnum] = 0x00;
    retnum++;
    for (i = 0; i < 16; i++)
    {
      if (coins[i] != 0) //Reports that all tubes in use to 100 coins
      {
        MPCMSerial1.write(0x64);
        CHK = CHK + 0x64;
        ret[retnum] = 0x64;
        retnum++;
      }
      else
      {
        MPCMSerial1.write(0x00); //Reports that tubes not in use have no coins
        ret[retnum] = 0x64;
        retnum++;
      }
    }
    MPCMSerial1.writeMode(CHK); //Checksum
    return;
  }
  if (data == 0x0B) //Poll
  {
    retnum = 0;
    if (deposit)
    {
      MPCMSerial1.write(B01010000); //Says that a coin of type 0 was deposited
      ret[retnum] = B01010000;
      retnum++;
      CHK = CHK + B01010000;
      MPCMSerial1.write(0x64); //Says that the tube has 100 coins
      ret[retnum] = 0x64;
      retnum++;
      CHK = CHK + 0x64;
      MPCMSerial1.writeMode(CHK); //Checksum
      return;
    }
    if (liquidate)
    {
      MPCMSerial1.write(B01100000);
      CHK = B01100000;
      MPCMSerial1.write(0x01);
      CHK = CHK + 0x01;
      MPCMSerial1.writeMode(CHK);
      return;
    }
    else
    {
      MPCMSerial1.writeMode(0x00); //ACK
    }
    return;
  }
  if (data == 0x0D) //Disense
  {
    this->timeout();
    data = MPCMSerial1.read();
    uint8_t value, number;
    value = coins[data & 0x0F]; //value of coin
    if (value == 0)
    {
      MPCMSerial1.writeMode(0xFF); //NAK
      return;
    }
    MPCMSerial1.writeMode(0x00); //ACK
    number = data >> 4; //number of coins to be dispensed
    coinbalance = coinbalance + value * number;
    command = 0;
    received = 0;
    return;
  }
  else if  (data == 0xAA) //RET
  {
    if (retnum == 0)
    {
      MPCMSerial1.writeMode(0x00); //ACK
      return;
    }
    uint8_t i, CHK;
    CHK = 0;
    for (i = 0; i < retnum; i++) //Retransmits previous data
    {
      MPCMSerial1.write(ret[retnum]);
      CHK = CHK + ret[retnum];
    }
    MPCMSerial1.writeMode(CHK); //Checksum
    return;
  }
  else if (data = 0x00) //ACK
  {
    return;
  }
  else
  {
    MPCMSerial1.writeMode(0xFF); //NAK
    return;
  }
}

void MDBSerial::resetChanger(void)
{
  deposit = 0;
  csetup = 0;
  cancel = 0;
  state = "MONITOR";
  MPCMSerial3.writeMode(0x08); //Reset
  MPCMSerial3.flush();
}

void MDBSerial::reader(uint8_t data)
{
  uint8_t CHK = 0;
  if (data == 0x30) //reset
  {
    state == "MONITOR";
    this->resetReader();
    return;
  }
  if (data == 0x33) //POLL
  {
    if (stacked)
    {
      MPCMSerial2.write(B10000000);
      MPCMSerial2.writeMode(B10000000); //CHK
      stacked = 0;
      return;
    }
    else if (inescrow)
    {
      MPCMSerial2.write(B10010000);
      MPCMSerial2.writeMode(B10010000);
      return;
    }
    else
    {
      MPCMSerial2.writeMode(0x00); //ACK
    }
    return;
  }
  else if (data == 0x35) //Escrow
  {
    this->timeout();
    data = MPCMSerial2.read();
    if (!escrow)
    {
      MPCMSerial2.writeMode(0xFF); //NAK
      return;
    }
    data &= 0x01;
    if (data == 0x01) //Stack Command
    {
      stacked = 1;
      escrow--;
      command = 0;
    }
    if (data == 0x00)
    {
      stacked = 0;
      returned = escrow;
      escrow = 0;
      command = 0;
    }
    received = 0;
    command = 0;
    return;
  }
  if (data == 0x36) //Stakcer
  {
    MPCMSerial2.write(stacker[0]);
    CHK = CHK + stacker[0];
    MPCMSerial2.write(stacker[1]);
    CHK = CHK + stacker[1];
    MPCMSerial2.writeMode(CHK);
    return;
  }
  else if (data = 0x00) //ACK
  {
    return;
  }
  else
  {
    MPCMSerial1.writeMode(0xFF); //NAK
    return;
  }
}

void MDBSerial::resetReader(void)
{
  state = "MONITOR";
  rsetup = 1;
  reading = "NONE";
  cancel = 1;
  MPCMSerial3.writeMode(0x30); //Reset
  MPCMSerial3.write(0x30); //CHK
  MPCMSerial3.flush();
}

void MDBSerial::master(void)
{
  if (masterstate == "NONE")
  {
    readercommand = 0;
    changercommand = 0;
    masterstate = "CHANGER";
  }
  if (masterstate == "CHANGER")
  {
    this->changerMaster();
  }
  if (masterstate == "READER")
  {
    this->readerMaster();
  }
  if (masterstate == "COMMAND")
  {
    this->commandCheck();
  }
}

void MDBSerial::changerMaster(void)
{
  unsigned long data, CHK;
  if (timeoutcount >= 20)
  {
    timeoutcount = 0;
    masterstate = "NONE";
    state = "MONITOR";
    this->resetChanger();
    return;
  }
  if (changercommand == 0)
  {
    MPCMSerial3.writeMode(0x0B); //Poll
    changercommand = 0x0B;
    return;
  }
  if (changercommand == 0x0B) //Poll
  {
    uint8_t escrow_return = 0;
    if (this->timeout())
    {
      MPCMSerial3.writeMode(0x0B);
      return;
    }
    data = MPCMSerial3.read() & 0xFF; //First Byte
    CHK = data; //Checksum
    if (data == 0x00) //ACK
    {
      changercommand = 0;
      masterstate = "READER";
      return;
    }
    if (data >> 7 == 1)
    {
      uint8_t value, amount;
      value = coins[data & 0x0F];
      amount = (data >> 4) & 0x07;
      if (this->timeout())
      {
        MPCMSerial3.write(0xAA); //RET
        return;
      }
      data = MPCMSerial3.read(); //Clears out number of coins in tube byte
      if (this->timeout())
      {
        MPCMSerial3.write(0xAA); //RET
        return;
      }
      while(!(data >> 8)) //Clears out the rest of the data
      {
        if (this->timeout())
        {
          MPCMSerial3.write(0xAA); //RET
          return;
        }
        CHK = CHK + (data & 0xFF);
        data = MPCMSerial3.read();
        if ((data & 0xFF) == 0x04) //Escrow return request
        {
          escrow_return = 1;
        }
      }
      if(CHK != (0xFF & data)) //Checksum
      {
        MPCMSerial3.write(0xAA); //RET
        return;
      }
      MPCMSerial3.write(0x00); //ACK
      if (escrow_return)
      {
        cancel = 1;
      }
      return;
    }
    uint8_t routing, value, tube;
    routing = (data >> 4) & 0x03; //finds out where coin went
    value = coins[data & 0x0F]; //gets value of coin
    if (this->timeout())
    {
      MPCMSerial3.write(0xAA); //RET
      return;
    }
    data = MPCMSerial3.read();
    tube =  data & 0xFF; //Byte 2//Gets number of coins in tube
    if (this->timeout())
    {
      MPCMSerial3.write(0xAA); //RET
      return;
    }
    while(!(data >> 8))
    {
      CHK = CHK + data; //Checksum
      data = MPCMSerial3.read(); //Recovers CHK byte on last iteration
      if (data == 0x04) //Escrow return request
      {
        escrow_return = 1;
      }
      if (this->timeout())
      {
        MPCMSerial3.write(0xAA); //RET
        return;
      }
    }
    if ((data & 0xFF) != CHK) //Checks the checksum
    {
      MPCMSerial3.write(0xAA); //RET
      return;
    }
    else
    {
      MPCMSerial3.write(0x00); //ACK
    }
    if (escrow_return)
    {
      cancel = 1;
    }
    if (routing == 0x00 || routing == 0x01) //Checks to see if coin was sent to tubes or box
    {
      coinbalance = coinbalance + value; //Update balance
    }
    changercommand = 0;
    masterstate = "READER";
  }
}

void MDBSerial::readerMaster(void)
{
  unsigned long data, CHK;
  if (timeoutcount >= 20)
  {
    timeoutcount = 0;
    masterstate = "NONE";
    state = "MONITOR";
    this->resetChanger();
    return;
  }
  if (readercommand == 0)
  {
    MPCMSerial3.writeMode(0x33); //Poll
    readercommand = 0x33;
    return;
  }
  if (readercommand == 0x33) //Poll
  {
    if (this->timeout())
    {
      MPCMSerial3.writeMode(0x33);
      return;
    }
    data = MPCMSerial3.read() & 0xFF; //First Byte
    CHK = 0; //data; //Checksum
    if (data == 0x00) //ACK
    {
      changercommand = 0;
      masterstate = "COMMAND";
      return;
    }
    uint8_t value, routing;
    value = rbills[data & 0x0F];
    routing = (data >> 4) & 0x07;
    if (this->timeout())
    {
      MPCMSerial3.write(0xAA); //RET
      return;
    }
    while(!(data >> 8))
    {
      CHK = CHK + (data & 0xFF); //Checksum
      data = MPCMSerial3.read(); //Recovers CHK byte on last iteration
      if (this->timeout())
      {
        MPCMSerial3.write(0xAA); //RET
        return;
      }
    }
    if ((data & 0xFF) != CHK) //Checks the checksum
    {
      MPCMSerial3.write(0xAA); //RET
      return;
    }
    else
    {
      MPCMSerial3.write(0x00); //ACK
    }
    if (routing == 0x00) //Stacked
    {
      billbalance = billbalance + value;
    }
    else if (routing == 0x01) //Escrow
    {
      billescrow = value;
    }
    else if (routing == 0x02) //Returned
    {
      billescrow = 0;
    }
    masterstate == "NONE";
    readercommand == 0;
  }
}

int MDBSerial::timeout(void)
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
  return 0;
}
    
unsigned long MDBSerial::status(void)
{
  unsigned long results;
  uint8_t data;
  results = billbalance + coinbalance;
  billbalance = 0;
  coinbalance = 0;
  if (billescrow)
  {
    data = 1;
  }
  if (largebill)
  {
    results |= (1 << 1);
  }
  if (stacked)
  {
     results |= (1 << 2);
  }
  if (returned)
  {
    data |= (1 << 3);
    data |= (returned << 4);
    returned = 0;
  }
  else
  {
    data |= (escrow << 4);
  }
  results |= (data << 24);
  return results;
}

void MDBSerial::setEscrow(unsigned int i)
{
  escrow = i;
}

void MDBSerial::commandCheck(void)
{
  uint8_t CHK;
  if (stackescrow)
  {
    MPCMSerial3.writeMode(0x35); //ESCROW
    CHK = 0x35;
    MPCMSerial3.write(0x07); //Stack bill. Lowest bit must be 1. Others don't matter
    CHK = CHK + 0x07;
    MPCMSerial3.write(CHK);
    stackescrow = 0;
    return;
  }
  if (returnescrow)
  {
    if (billescrow)
    {
      MPCMSerial3.writeMode(0x35); //ESCROW
      CHK = 0x35;
      MPCMSerial3.write(0x00); //Stack bill. Lowest bit must be 1. Others don't matter
      CHK = CHK + 0x00;
      MPCMSerial3.write(CHK);
      returnescrow = 0;
      return;
    }
  }
  else
  {
    masterstate = "NONE";
  }
//  if (returncoins)
//  {
//    
}

MDBSerial MDB;
