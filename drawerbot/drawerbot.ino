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
#define requestPin 2          //Pin to monitor for requests
#define motorDirA 4
#define motorDirB 5
#define drawerRelay 6
#define liftRelay 7
#define drawerLimitIn 8
#define drawerLimitOut 9
#define liftLimit 10
#define ledData 11
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
  pinMode(liftLimit, INPUT_PULLUP);

  //ADAFRUIT HARDWARE SETUP
  Adafruit_NeoPixel leds = Adafruit_NeoPixel(16, ledData, NEO_GRB + NEO_KHZ800);
  Adafruit_VS1053_FilePlayer musicPlayer = 
  // create breakout-example object!
  //Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);
  // create shield-example object!
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

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
      }
      break;
      case 99: {
        if(liftUp()){drawerIn();}
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
    Serial.print("DRAWER: Drawer already out, returning true.");
    return true;
  }
  //Set motion
  if(!setDirection("drawer", "forward")){
    Serial.println("DRAWER: Error, set direction failed.");
    return false;
  }
  //Confirm the lift is up
  if(!isLiftUp()){
    Serial.print("DRAWER: Motion canceled because the lift isn't up.");
    return false;
  }
  Serial.print("DRAWER: Opening... ");
  unsigned long current = millis();
  while(drawerLimitOut == HIGH){
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

bool drawerIn(){                                                            //Drawer in
  //Check to see if the drawer is already in
  if(isDrawerIn()){
    Serial.print("DRAWER: Drawer already out, returning true.");
    return true;
  }
  if(!isLiftUp()){
    Serial.print("DRAWER: Motion canceled because the lift isn't up.");
    return false;
  }
  //Set motion
  if(!setDirection("drawer", "reverse")){
    Serial.println("DRAWER: Error, set direction failed.");
    return false;
  }
  Serial.print("DRAWER: Closing... ");
  unsigned long current = millis();
  while(drawerLimitIn == HIGH){
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
  if(!setDirection("lift", "forward")){
    Serial.println("LIFT: Error, set direction failed.");
    return false;
  }
  Serial.print("LIFT: Lowering... ");
  unsigned long current = millis();
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
  if(!isLiftUp()){
    Serial.println("LIFT: Motion canceled because the lift is already up.");
    return false;
  }
  //Check to see if the drawer is out
  if(!isDrawerOut()){
    Serial.print("LIFT: Motion canceled because the drawer isn't out.");
    return false;
  }
  //Set motion
  if(!setDirection("lift", "reverse")){
    Serial.println("LIFT: Error, set direction failed.");
    return false;
  }
  Serial.print("LIFT: Raising... ");
  unsigned long current = millis();
  while(liftLimit == HIGH){
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

bool setDirection(char type, char dir){
  Serial.print("SETDIRECTION: ");
  if(type == "drawer" && dir == "forward"){           //DRAWER FORWARD
    digitalWrite(motorDirA, drawerMotorA);
    digitalWrite(motorDirB, !drawerMotorA);
    Serial.print("Drawer : Relay A ");
    Serial.print(drawerMotorA);
    Serial.print(", Relay B ");
    Serial.println(!drawerMotorA);
    return true;
  } else if(type == "drawer" && dir == "reverse"){    //DRAWER REVERSE
    digitalWrite(motorDirA, !drawerMotorA);
    digitalWrite(motorDirB, drawerMotorA);
    Serial.print("Drawer : Relay A ");
    Serial.print(!drawerMotorA);
    Serial.print(", Relay B ");
    Serial.println(drawerMotorA);
    return true;
  } else if(type == "lift" && dir == "forward"){      //LIFT FORWARD
    digitalWrite(motorDirA, liftMotorA);
    digitalWrite(motorDirB, !liftMotorA);
    Serial.print("Lift : Relay A ");
    Serial.print(liftMotorA);
    Serial.print(", Relay B ");
    Serial.println(!liftMotorA);
    return true;
  } else if(type == "lift" && dir == "reverse"){      //LIFT REVERSE
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
  if(drawerLimitIn == LOW){
    Serial.println("DRAWER: Status IN : TRUE");
    return true;
  } else {
    Serial.println("DRAWER: Status IN : FALSE");
    return false;
  }
}

bool isDrawerOut(){
  if(drawerLimitOut == LOW){
    Serial.println("DRAWER: Status OUT : TRUE");
    return true;
  } else {
    Serial.println("DRAWER: Status OUT : FALSE");
    return false;
  }
}

bool isLiftUp(){
  if(liftLimit == LOW){
    Serial.println("LIFT: Status UP : TRUE");
    return true;
  } else {
    Serial.println("LIFT: Status UP : FALSE");
    return false;
  }
}

void printStatus(){
  Serial.println("\r\r--------------------------");
  Serial.println("STATUS:\rRequest\tMotorDirA\tMotorDirB\tDrawer\tLift");
  Serial.print(requestPin, digitalRead(requestPin));
  Serial.print("\t");
  Serial.print(motorDirA, digitalRead(motorDirA));
  Serial.print("\t");
  Serial.print(motorDirB, digitalRead(motorDirB));
  Serial.print("\t");
  Serial.print(drawerRelay, digitalRead(drawerRelay));
  Serial.print("\t");
  Serial.print(liftRelay, digitalRead(liftRelay));
  Serial.println("\r\rLimit In\tLimit Out\tLimit Up\tDrawer Timeout\tLift Time");
  Serial.print(drawerLimitIn, digitalRead(drawerLimitIn));
  Serial.print("\t");
  Serial.print(drawerLimitOut, digitalRead(drawerLimitOut));
  Serial.print("\t");
  Serial.print(liftLimit, digitalRead(liftLimit));
  Serial.print("\t");
  Serial.print(drawerTimeout/1000);
  Serial.print("sec\t");
  Serial.println(lowerTime/1000);
  Serial.println("---------------------------------\r[O]pen, [C]lose, [D]rawer, [L]ift\rREADY.");
}
