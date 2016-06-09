#include <Wire.h>
#include <SPI.h>

#define mySerial SerialUSB

/*
   Pin multiplexer value names (PMUXn.PMUXE/O)
*/
const char *pinmux_names[8] = {
  "EIC",   // "A": External Interrupts
  "ADC",   // "B": REF, ADC, AC, PTC, DAC
  "SERCOM",   /* "C": SerCom */
  "SERCOMALT",  /* "D": Alternate SERCOM */
  "TC/TCC",     /* "E": TC or TCC Timer */
  "TCC",        /* "F": TCC Timer*/
  "COM",        /* "G": Other Comm - USB, I2S */
  "AC/GCLK"     /* "H": Analog Comparator or GCLKs */
};

boolean print_pinno(int i)
{
  int pintype = g_APinDescription[i].ulPinType;
  if (pintype == PIO_NOT_A_PIN) {
    return false;
  }
  mySerial.print(i);
  if (i < 10)
    mySerial.print(" ");
  if (pintype == PIO_ANALOG) {  // try to detect and renumber Analog pins.
    mySerial.print(" (A");
    mySerial.print(i - PIN_A0);
    mySerial.print(") ");
  } else {
    mySerial.print("      ");
  }
  return true;
}

void print_pinmux(int pmuxval)
{
  mySerial.print("PMUX");
  mySerial.print("(");
  mySerial.print(pmuxval);
  mySerial.print(") ");
  mySerial.print(pinmux_names[pmuxval]);
}

void setup() {
  mySerial.begin(9600);
  while (!mySerial)
    ;
  mySerial.println("SAMD Pin State Report");
  mySerial.println();

  // Set some pins up doing something different, so that the display will be
  //   more ... interesting.
  Serial1.begin(9600);
  Serial.begin(9600);
  SPI.begin();
  Wire.begin();
  for (int j = 0; j < 6; j++)
    (void) analogRead(j);  // read from analog pins
}

void loop() {
  for (int pin = 0; pin < PINS_COUNT; pin++) {
    if (print_pinno(pin)) {  // Does pin exist?

      int bitno = g_APinDescription[pin].ulPin;
      PortGroup *port = digitalPinToPort(pin);
      int pmuxval;

      if (port->PINCFG[bitno].bit.PMUXEN) {  // Is pinmux enabled?
        // If pinmux is enabled, further report it's value
        if (bitno & 1) {
          // Odd pin numbers
          pmuxval = port->PMUX[bitno / 2].bit.PMUXO;
        } else {
          pmuxval = port->PMUX[bitno / 2].bit.PMUXE;
        }
        print_pinmux(pmuxval);
      } else {
        // Pinmux not enabled - port is GPIO
        mySerial.print("GPIO ");
        if (port->DIR.reg & (1 << bitno)) { // Decode pinmode (direction.)
          mySerial.print("O");
        } else {
          mySerial.print("I");
        }
      } // Pinmux or GPIO
    } // Pin existance
    mySerial.println();
  } // "pin" for loop

  while (mySerial.read() < 0)
    ;  // wait for character to run again.
  mySerial.println("\nRunning again!\n");
}
