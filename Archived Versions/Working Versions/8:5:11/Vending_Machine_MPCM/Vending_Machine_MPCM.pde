#include <LiquidCrystal.h>
#include <Ethernet.h>
#include <SPI.h>
#include <SD.h>
#include "Controller.h"
#include "MDB.h"
#include "WebServer.h"

void setup()
{
  //Pins
  pinMode(2, OUTPUT); //RFID Reset
  // [0, 1, 2, 14, 15, 16, 17, 18, 19, A8, A9, A10, A11, A12, A13, A14, 15] ~ In Use
  // [0, 1, 14, 15, 16, 17, 18, 19] ~ Serial, MPCMSerial1, MPCMSerial2, MPCMSerial3 respectively
  // [14, 15, 16, 17, 18, 19] ~ LCD
  
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);
  delay(125);
  //Serial
  Serial.begin(9600);
  Serial.println("Starting up...");
  MPCMSerial1.begin(9600);
  MPCMSerial1.setAddress(0x08, 0xF8);//Changer
  MPCMSerial2.begin(9600);
  MPCMSerial2.setAddress(0x30, 0xF8);//Bill Reader
  MPCMSerial3.begin(9600, 1);//Master
  MPCMSerial1.flush();
  MPCMSerial2.flush();
  MPCMSerial3.flush();
  
  
  //Start Ethernet
  byte MAC[6] = {0x90, 0xA2, 0xDA, 0x00, 0x49, 0x9E};
  byte IP[4] = {18, 238, 1, 221};
  byte GATEWAY[4] = {18, 238, 0, 1};
  byte SUBNET[4] = {255, 255, 0, 0};
  Ethernet.begin(MAC, IP, GATEWAY, SUBNET);
  
  //SD
  //while (!SD.begin(4)); //Starts SD. Must be pin 4
  
  //Accounts
  Accounts.setPath("/Accounts.txt");
  
  //Display.sessionCanceled(54308);
  //Display.idle();
  //Display.newAccountCreated("Charles Franklin");
  //Display.cardTapped(2309);
  //Display.largeBill();
  
  Serial.println("Setup Complete");
}

void loop()
{
  debugCheck();
  MainController.check();
  MDB.check();
  Administrator.check();
  Display.check();
}
