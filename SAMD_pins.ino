/*
 * SAMD_pins.ino  -  June 2016 by Bill Westfield (WestfW@westfw.info)
 * Copyright 2016 by Bill Westfield
 * Licensed under the terms of the "MIT License."
 */

#include <Wire.h>
#include <SPI.h>
#include "SAMD_pinmux_report.h"

/*
 * Adjust these as necessary
 */

/*
 * For Sparkfun SAMD21 boards, use SerialUSB device
 */

#define mySerial SerialUSB
static char buffer[80];

/*
 * Standard Arduio setup() function.
 * Turn on some of the interesting features so we can see how it works.
 */
void setup() {
  mySerial.begin(9600);
  while (!mySerial)
    ;
  mySerial.println("SAMD Pin State Report");
  mySerial.println();

  // Set some pins up doing something different, so that the display will be
  //   more ... interesting.
  Serial1.begin(9600);  // Start both our "hardware serial" ports.
  Serial.begin(9600);
  SPI.begin();			/* Start SPI */
  Wire.begin();			/* Start I2C */
  analogWrite(10, 55);		/* PWM on both types of timers */
  analogWrite(12, 55);
  for (int j = 0; j < 6; j++)
    (void) analogRead(j);  // read from analog pins - sets analog mode.
  pinMode(13, OUTPUT);	   /* and at least one output */
}

void loop() {
  pinmux_report(PIN_LED_RXL, buffer, "RXL");
  mySerial.println(buffer);
  for (int pin = 0; pin < PINS_COUNT; pin++) {  // For all defined pins
    pinmux_report(pin, buffer, 0);
    mySerial.println(buffer);
  } // "pin" for loop

  while (mySerial.read() < 0)
    ;  // wait for character to run again.
  mySerial.println("\nRunning again!\n");
}

