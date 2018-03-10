#define version 0.1
/*
 * DrawerBot 0.1
 * Arduino code to operate a mechanical shelf that lets you store things away in places too hard to reach.
 * By Dave Latham
 */
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_VS1053.h>
#include <SD.h>


//PIN DEFINITIONS
#define requestPin 2          //Pin to monitor for requests - interrupt
#define motorDirA A0
#define motorDirB A1
#define drawerRelay A2
#define liftRelay A3
#define ledData 12
#define drawerLimitIn 8
#define drawerLimitOut 9
#define liftLimit 10
//#define CLK 13       // SPI Clock, shared with SD card
//#define MISO 12      // Input data, from VS1053/SD card
//#define MOSI 11      // Output data, to VS1053/SD card
// Connect CLK, MISO and MOSI to hardware SPI pins. 
// See http://arduino.cc/en/Reference/SPI "Connections"
#define SHIELD_RESET  -1     // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)
#define CARDCS 4             // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3               // VS1053 Data request, ideally an Interrupt pin

//SAFETY TIMEOUTS
#define drawerTimeout 10000   //Time in milliseconds before drawer motion times out
#define lowerTime 15000       //Time in milliseconds that the lift will lower when requested (no lower limit)

//MOTION DIRECTION CONFIGURATION
#define drawerMotorA LOW      //Set the output of direction relay A for the drawer in forward
#define liftMotorA HIGH       //Set the output of direction relay A for the lift in down
#define drawer 0
#define lift 1
#define forward 2
#define reverse 3
unsigned long current;


 
void setup() {
  //PINMODES
  pinMode(requestPin, INPUT_PULLUP);
  pinMode(motorDirA, OUTPUT);
  digitalWrite(motorDirA, HIGH);
  pinMode(motorDirB, OUTPUT);
  digitalWrite(motorDirB, HIGH);
  pinMode(drawerRelay, OUTPUT);
  digitalWrite(drawerRelay, HIGH);
  pinMode(liftRelay, OUTPUT);
  digitalWrite(liftRelay, HIGH);
  pinMode(drawerLimitIn, INPUT_PULLUP);
  pinMode(drawerLimitOut, INPUT_PULLUP);
  pinMode(liftLimit, INPUT_PULLUP);

  //ADAFRUIT HARDWARE SETUP
  Adafruit_NeoPixel leds = Adafruit_NeoPixel(16, ledData, NEO_GRB + NEO_KHZ800);
  leds.begin();
  leds.show();
  Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

  //SERIAL MONITOR SETUP
  Serial.begin(9600);
  Serial.print("SelfBot version ");
  Serial.println(version);
  printStatus();
}

void loop() {
  if(Serial.available() > 0){
    uint8_t incomingByte = Serial.read();
    switch(incomingByte){
      case 111: {
        if(drawerOut()){liftDown();}
        printStatus();
      }
      break;
      case 99: {
        if(liftUp()){drawerIn();}
        printStatus();
      }
      break;
    }
  }
  delay(500); //make sure there's no weirdness on the Serial.available()
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MOTION FUNCTIONS >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool drawerOut(){                                                           //Drawer out
  //Check status
  if(isDrawerOut()){
    Serial.println(F("DRAWER: Drawer already out, returning true."));
    return true;
  }
  //Set motion
  if(!setDirection(drawer, forward)){
    Serial.println(F("DRAWER: Error, set direction failed."));
    return false;
  }
  //Confirm the lift is up
  if(!isLiftUp()){
    Serial.println(F("DRAWER: Motion canceled because the lift isn't up."));
    return false;
  }
  Serial.print(F("DRAWER: Opening... "));
  current = millis();
  while(digitalRead(drawerLimitOut) == HIGH){
    digitalWrite(drawerRelay, LOW);
    if((millis() - current) >= drawerTimeout){
      digitalWrite(drawerRelay, HIGH);
      Serial.print(F("Failed. Drawer timed out."));
      return false;
    }
  }
  digitalWrite(drawerRelay, HIGH);
  Serial.print(F("OK (Completed in "));
  Serial.print(millis()-current);
  Serial.println("ms)");
  return true;
}

bool drawerIn(){                                                            //Drawer in
  //Check to see if the drawer is already in
  if(isDrawerIn()){
    Serial.println(F("DRAWER: Drawer already out, returning true."));
    return true;
  }
  if(!isLiftUp()){
    Serial.println(F("DRAWER: Motion canceled because the lift isn't up."));
    return false;
  }
  //Set motion
  if(!setDirection(drawer, reverse)){
    Serial.println(F("DRAWER: Error, set direction failed."));
    return false;
  }
  Serial.print(F("DRAWER: Closing... "));
  current = millis();
  while(digitalRead(drawerLimitIn) == HIGH){
    digitalWrite(drawerRelay, LOW);
    if((millis() - current) >= drawerTimeout){
      digitalWrite(drawerRelay, HIGH);
      Serial.print("Failed. Drawer timed out.");
      return false;
    }
  }
  digitalWrite(drawerRelay, HIGH);
  Serial.print("OK (Completed in ");
  Serial.print(millis()-current);
  Serial.println("ms)");
  return true;
}

bool liftDown(){                                                                //Lift Down
  //Check to see if the drawer is out
  if(!isDrawerOut()){
    Serial.println("LIFT: Motion canceled because drawer isn't out.");
    return false;
  }
  //Check to see if the lift is up
  if(!isLiftUp()){
    Serial.println("LIFT: Motion canceled because lift isn't up - can't ensure proper lower time.");
    return false;
  }
  //Set motion
  if(!setDirection(lift, forward)){
    Serial.println("LIFT: Error, set direction failed.");
    return false;
  }
  Serial.print("LIFT: Lowering... ");
  current = millis();
  while((millis()-current) < lowerTime){
    digitalWrite(liftRelay, LOW);
  }
  digitalWrite(liftRelay, HIGH);
  Serial.print("OK (Completed in ");
  Serial.print(millis()-current);
  Serial.println("ms)");
  return true;
}

bool liftUp(){                                                                        //Lift Up
  //Check to see if the lift is up already
  if(isLiftUp()){
    Serial.println("LIFT: Motion canceled because the lift is already up.");
    return false;
  }
  //Check to see if the drawer is out
  if(!isDrawerOut()){
    Serial.print("LIFT: Motion canceled because the drawer isn't out.");
    return false;
  }
  //Set motion
  if(!setDirection(lift, reverse)){
    Serial.println("LIFT: Error, set direction failed.");
    return false;
  }
  Serial.print("LIFT: Raising... ");
  current = millis();
  while(digitalRead(liftLimit) == HIGH){
    digitalWrite(liftRelay, LOW);
    if((millis() - current) >= (lowerTime + 1000)){
      digitalWrite(liftRelay, HIGH);
      Serial.print("Failed. Lift timed out.");
      return false;
    }
  }
  digitalWrite(liftRelay, HIGH);
  Serial.print("OK (Completed in ");
  Serial.print(millis()-current);
  Serial.println("ms)");
  return true;
}

bool setDirection(uint8_t type, uint8_t dir){
  Serial.print("SETDIRECTION: ");
  if(type == 0 && dir == 2){           //DRAWER FORWARD
    digitalWrite(motorDirA, drawerMotorA);
    digitalWrite(motorDirB, !drawerMotorA);
    Serial.print("Drawer : Relay A ");
    Serial.print(drawerMotorA);
    Serial.print(", Relay B ");
    Serial.println(!drawerMotorA);
    return true;
  } else if(type == 0 && dir == 3){    //DRAWER REVERSE
    digitalWrite(motorDirA, !drawerMotorA);
    digitalWrite(motorDirB, drawerMotorA);
    Serial.print("Drawer : Relay A ");
    Serial.print(!drawerMotorA);
    Serial.print(", Relay B ");
    Serial.println(drawerMotorA);
    return true;
  } else if(type == 1 && dir == 2){      //LIFT FORWARD
    digitalWrite(motorDirA, liftMotorA);
    digitalWrite(motorDirB, !liftMotorA);
    Serial.print("Lift : Relay A ");
    Serial.print(liftMotorA);
    Serial.print(", Relay B ");
    Serial.println(!liftMotorA);
    return true;
  } else if(type == 1 && dir == 3){      //LIFT REVERSE
    digitalWrite(motorDirA, !liftMotorA);
    digitalWrite(motorDirB, liftMotorA);
    Serial.print("Lift : Relay A ");
    Serial.print(!liftMotorA);
    Serial.print(", Relay B ");
    Serial.println(liftMotorA);
    return true;
  } else {
    Serial.println("SETDIRECTION: Error, incorrect type or direction provided.");
    return false;
  }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! STATUS FUNCTIONS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool isDrawerIn(){
  if(digitalRead(drawerLimitIn) == 0){
    Serial.println("DRAWER: Status IN : TRUE");
    return true;
  } else {
    Serial.println("DRAWER: Status IN : FALSE");
    return false;
  }
}

bool isDrawerOut(){
  if(digitalRead(drawerLimitOut) == LOW){
    Serial.println("DRAWER: Status OUT : TRUE");
    return true;
  } else {
    Serial.println("DRAWER: Status OUT : FALSE");
    return false;
  }
}

bool isLiftUp(){
  if(digitalRead(liftLimit) == LOW){
    Serial.println("LIFT: Status UP : TRUE");
    return true;
  } else {
    Serial.println("LIFT: Status UP : FALSE");
    return false;
  }
}

void printStatus(){
  Serial.println("\n\n--------------------------");
  Serial.println("STATUS:\nRequest\tMotorDirA\tMotorDirB\tDrawer\tLift");
  Serial.print(requestPin, digitalRead(requestPin));
  Serial.print("\t");
  Serial.print(motorDirA);
  Serial.print("-");
  Serial.print(digitalRead(motorDirA));
  Serial.print("\t\t");
  Serial.print(motorDirB);
  Serial.print("-");
  Serial.print(digitalRead(motorDirB));
  Serial.print("\t\t");
  Serial.print(drawerRelay);
  Serial.print("-");
  Serial.print(digitalRead(drawerRelay));
  Serial.print("\t");
  Serial.print(liftRelay);
  Serial.print("-");
  Serial.print(digitalRead(liftRelay));
  Serial.println("\n\nLimit In\tLimit Out\tLimit Up\tDrawer Timeout\tLift Time");
  Serial.print(drawerLimitIn);
  Serial.print("-");
  Serial.print(digitalRead(drawerLimitIn));
  Serial.print("\t\t");
  Serial.print(drawerLimitOut);
  Serial.print("-");
  Serial.print(digitalRead(drawerLimitOut));
  Serial.print("\t\t");
  Serial.print(liftLimit);
  Serial.print("-");
  Serial.print(digitalRead(liftLimit));
  Serial.print("\t\t");
  Serial.print(drawerTimeout/1000);
  Serial.print("sec\t\t");
  Serial.print(lowerTime/1000);
  Serial.println("sec");
  Serial.println("---------------------------------\n[O]pen, [C]lose, [D]rawer, [L]ift\nREADY.");
}


void ledRun(){
  
}

void ledError(){
  
}
void ledRest(){
  
}

