#include <LiquidCrystal.h>
#include <Ethernet.h>
#include <SPI.h>
#include <SD.h>
#include "Controller.h"
#include "MDB.h"
#include "LCD.h"
#include "Sodas.h"
#include "WebServer.h"

long funds = 0;
unsigned long escrow = 0;
unsigned long bill_funds = 0;
unsigned long coin_funds = 0;
unsigned long max_price = 0;
volatile int done = 0;

void setup()
{
  Display.print("Setting up...");
  Display.check();
  //Pins
  //pinMode(2, OUTPUT); //RFID Reset
  // [0, 1, 4, 10, 11, 12, 13, 14, 15, 16, 17, 22, 23, 24, 25, 26, 27, 29, 31, 33, 35, 37, A8, A9, A10, A11, A12, A13, A14, A15] ~ In Use
  // [0, 1, 14, 15, 16, 17] ~ Serial, Serial2, MPCMSerial3 respectively
  // [4, 10, 11, 12, 13] ~ Ethernet Shield
  // [14, 15, 16, 17, 18, 19] ~ LCD
  // [22, 23, 24, 25] ~ Latch OE and CP
  // [26, 27, 28, 29, 30, 31, 32, 33] ~ Latch Output
  // [34, 36, 38, 40, 42, 44, 46, 48, 50, 52] ~ Motors
  
  //Serial
  Serial.begin(9600);
  Serial.println("Starting up...");
  MPCMSerial3.begin(9600, 1);//Master
  MPCMSerial3.flush();
  
  
  //Start Ethernet
  byte MAC[6] = {0x90, 0xA2, 0xDA, 0x00, 0x49, 0x9E};
  byte IP[4] = {18, 238, 1, 221};
  byte GATEWAY[4] = {18, 238, 0, 1};
  byte SUBNET[4] = {255, 255, 0, 0};
  Ethernet.begin(MAC, IP, GATEWAY, SUBNET);
  
  //SD
  Serial.println("...SD");
  while (!SD.begin(4)); //Starts SD. Must be pin 4
  
  //Accounts
  Serial.println("...accounts");
  Accounts.setPath("/Accounts.txt");
  
  //Dispenser
  Vend.begin();
  
  //Display
  Display.idle();

  Serial.println("Setup Complete");
}

void loop()
{
  MDB.check();
  //MainController.check();
  Vend.check();
  Display.check();
  Administrator.check();
  debugCheck();
}
