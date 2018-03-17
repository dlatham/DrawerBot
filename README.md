# DrawerBot

A program for arduino that controls the motion of a robotic drawer enabling storage in places out of reach. This program controls 2-axis motion - linear motion (tested with a lead screw) and lifting / lowering motion using a winding drum (tested using a motor connected via worm gear). Motion start/stop is controlled using 2 limit switched for linear movement and a single (up position) limit switch for the lifting motion. Lowering motion is controlled using adjustable active state time.

----------------------

## PINOUTS

All pinouts are configureable in the constant definitions except for hardware SPI control of the VS1053 sound shield that I am using from Adafruit (pins 11,12,13). Originally I attempted to move these to the ICSP pins through altering the VS1053 but I was still getting noise on GPIO 11,12 and 13. The configureable pinouts are as follows:

* requestPin (default 2) - Used as the main interrupt to request drawer service. The pin is pulled high by default for closed so HIGH = closed and LOW = open. This pin is configureable but should be attached to an interrupt pin (2 or 3 on the Uno).

* motorDirA (default A0) - One of two relays used to control the motor direction. HIGH or LOW configuration is available below in the CONFIGURATION section.

* motorDirB (default A1) - The second of two relays to control motor direction. HIGH or LOW configuration will be opposite of the motorDirA config.

* drawerRelay (default A2) - The relay controlling power to the drawer motor.

* liftRelay (default A3) - The relay controlling power to the lift motor.

* ledData (default 5) - The data pin used to control Adafruit's Neopixel product which provides visual notification of status.

* drawerLimitIn (default 8) - Input pin pulled high that monitors the limit switch indicating drawer in (closed) maximum.

* drawerLimitOut (default 9) - Input pin pulled high that monitors the limit switch indicating drawer out (open) maximum.

* liftLimit (default 10) - Input pin pulled high that monitors the limit switch indicating lift up (closed) maximum.

* SHIELD_CS (default 7 for the Adafruit shield) - The chip select pin being used for the VS1053

* SHIELD_DCS (default 6 for the Adafruit shield) - VS1053 Data/command select pin (output)

* CARDCS (default 4 for the Adafruit shield) - Card chip select used for the SD card reader on the VS1053

* DREQ (default 3 for the Adafruit shield) - Interrupt pin used to run sound in the background (requires interrupt pin)