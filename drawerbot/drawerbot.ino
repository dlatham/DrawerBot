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
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);
Adafruit_NeoPixel leds = Adafruit_NeoPixel(16, ledData, NEO_GRB + NEO_KHZ800);
 
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
  leds.begin();
  leds.show();

  //SERIAL MONITOR SETUP
  Serial.begin(9600);
  Serial.print("SelfBot version ");
  Serial.println(version);

  //SETUP THE SOUND PLAYER
  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  musicPlayer.setVolume(10,10);
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  SD.open("/");
  printStatus();
}

void loop() {
  if(Serial.available() > 0){
    uint8_t incomingByte = Serial.read();
    switch(incomingByte){
      case 111: {
        if(drawerOut()){if(liftDown()){musicPlayer.playFullFile("end.mp3");}}
        printStatus();
      }
      break;
      case 99: {
        if(liftUp()){if(drawerIn()){musicPlayer.playFullFile("end.mp3");}}
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
  musicPlayer.playFullFile("start.mp3");
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
  musicPlayer.playFullFile("next.mp3");
  current = millis();
  while(digitalRead(drawerLimitIn) == HIGH){
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

bool liftDown(){                                                                //Lift Down
  //Check to see if the drawer is out
  if(!isDrawerOut()){
    Serial.println(F("LIFT: Motion canceled because drawer isn't out."));
    return false;
  }
  //Check to see if the lift is up
  if(!isLiftUp()){
    Serial.println(F("LIFT: Motion canceled because lift isn't up - can't ensure proper lower time."));
    return false;
  }
  //Set motion
  if(!setDirection(lift, forward)){
    Serial.println(F("LIFT: Error, set direction failed."));
    return false;
  }
  Serial.print(F("LIFT: Lowering... "));
  musicPlayer.playFullFile("next.mp3");
  current = millis();
  while((millis()-current) < lowerTime){
    digitalWrite(liftRelay, LOW);
  }
  digitalWrite(liftRelay, HIGH);
  Serial.print(F("OK (Completed in "));
  Serial.print(millis()-current);
  Serial.println("ms)");
  return true;
}

bool liftUp(){                                                                        //Lift Up
  //Check to see if the lift is up already
  if(isLiftUp()){
    Serial.println(F("LIFT: Motion canceled because the lift is already up."));
    return false;
  }
  //Check to see if the drawer is out
  if(!isDrawerOut()){
    Serial.print(F("LIFT: Motion canceled because the drawer isn't out."));
    return false;
  }
  //Set motion
  if(!setDirection(lift, reverse)){
    Serial.println(F("LIFT: Error, set direction failed."));
    return false;
  }
  Serial.print(F("LIFT: Raising... "));
  musicPlayer.playFullFile("start.mp3");
  current = millis();
  while(digitalRead(liftLimit) == HIGH){
    digitalWrite(liftRelay, LOW);
    if((millis() - current) >= (lowerTime + 1000)){
      digitalWrite(liftRelay, HIGH);
      Serial.print(F("Failed. Lift timed out."));
      return false;
    }
  }
  digitalWrite(liftRelay, HIGH);
  Serial.print(F("OK (Completed in "));
  Serial.print(millis()-current);
  Serial.println("ms)");
  return true;
}

bool setDirection(uint8_t type, uint8_t dir){
  Serial.print("SETDIRECTION: ");
  if(type == 0 && dir == 2){           //DRAWER FORWARD
    digitalWrite(motorDirA, drawerMotorA);
    digitalWrite(motorDirB, !drawerMotorA);
    Serial.print(F("Drawer : Relay A "));
    Serial.print(drawerMotorA);
    Serial.print(F(", Relay B "));
    Serial.println(!drawerMotorA);
    return true;
  } else if(type == 0 && dir == 3){    //DRAWER REVERSE
    digitalWrite(motorDirA, !drawerMotorA);
    digitalWrite(motorDirB, drawerMotorA);
    Serial.print(F("Drawer : Relay A "));
    Serial.print(!drawerMotorA);
    Serial.print(F(", Relay B "));
    Serial.println(drawerMotorA);
    return true;
  } else if(type == 1 && dir == 2){      //LIFT FORWARD
    digitalWrite(motorDirA, liftMotorA);
    digitalWrite(motorDirB, !liftMotorA);
    Serial.print(F("Lift : Relay A "));
    Serial.print(liftMotorA);
    Serial.print(F(", Relay B "));
    Serial.println(!liftMotorA);
    return true;
  } else if(type == 1 && dir == 3){      //LIFT REVERSE
    digitalWrite(motorDirA, !liftMotorA);
    digitalWrite(motorDirB, liftMotorA);
    Serial.print(F("Lift : Relay A "));
    Serial.print(!liftMotorA);
    Serial.print(F(", Relay B "));
    Serial.println(liftMotorA);
    return true;
  } else {
    Serial.println(F("SETDIRECTION: Error, incorrect type or direction provided."));
    return false;
  }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! STATUS FUNCTIONS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool isDrawerIn(){
  if(digitalRead(drawerLimitIn) == 0){
    Serial.println(F("DRAWER: Status IN : TRUE"));
    return true;
  } else {
    Serial.println(F("DRAWER: Status IN : FALSE"));
    return false;
  }
}

bool isDrawerOut(){
  if(digitalRead(drawerLimitOut) == LOW){
    Serial.println(F("DRAWER: Status OUT : TRUE"));
    return true;
  } else {
    Serial.println(F("DRAWER: Status OUT : FALSE"));
    return false;
  }
}

bool isLiftUp(){
  if(digitalRead(liftLimit) == LOW){
    Serial.println(F("LIFT: Status UP : TRUE"));
    return true;
  } else {
    Serial.println(F("LIFT: Status UP : FALSE"));
    return false;
  }
}

void printStatus(){
  Serial.println("\n\n--------------------------");
  Serial.println(F("STATUS:\nRequest\tMotorDirA\tMotorDirB\tDrawer\tLift"));
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
  Serial.println(F("\n\nLimit In\tLimit Out\tLimit Up\tDrawer Timeout\tLift Time"));
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
  Serial.println(F("---------------------------------\n[O]pen, [C]lose, [D]rawer, [L]ift\nREADY."));
}


void ledRun(){
  //Rotate the LEDs in a loading circle given the current millis of the action
  
}

void ledError(){
  
}
void ledRest(){
  
}

