#include <Wire.h>
#include <SPI.h>

#define mySerial SerialUSB
#define ARRAYSIZE(a) (sizeof a/sizeof a[0])

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
  analogWrite(10, 55);
  analogWrite(12, 55);
  for (int j = 0; j < 6; j++)
    (void) analogRead(j);  // read from analog pins
  pinMode(13, OUTPUT);
}

void loop() {
  for (int pin = 0; pin < PINS_COUNT; pin++) {
    if (print_pinno(pin)) {  // Does pin exist?

      int bitno = g_APinDescription[pin].ulPin;
      PortGroup *port = digitalPinToPort(pin);
      int pmuxval;

      print_portname(pin);

      if (port->PINCFG[bitno].bit.PMUXEN) {  // Is pinmux enabled?
        // If pinmux is enabled, further report it's value
        if (bitno & 1) {
          // Odd pin numbers
          pmuxval = port->PMUX[bitno / 2].bit.PMUXO;
        } else {
          pmuxval = port->PMUX[bitno / 2].bit.PMUXE;
        }
	print_pinmux(pin, pmuxval);
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

void (*PMUX_decode_func[8])(int, int) = {
  decode_EIC,
  decode_ADC,
  decode_SERCOM,
  decode_SERCOMA,
  decode_TC,
  decode_TCC,
  decode_COMM,
  decode_CLK
};

void print_pinmux(int pin, int pmuxval)
{
  mySerial.print("PMUX");
  mySerial.print("(");
  mySerial.print(pmuxval);
  mySerial.print(") ");
  if (pmuxval < (sizeof PMUX_decode_func/sizeof PMUX_decode_func[0])) {
    PMUX_decode_func[pmuxval](pin, pmuxval);
  }
}


char *nullstr = "";

/*
 *   Pin multiplexer value names (PMUXn.PMUXE/O)
 */
const char *pinmux_names[8] = {
  "EIC",   // "A": External Interrupts
  "ADC",   // "B": REF, ADC, AC, PTC, DAC
  "SERCOM",   /* "C": SerCom */
  "SERCOM",  /* "D": Alternate SERCOM */
  "TC/TCC",     /* "E": TC or TCC Timer */
  "TCC",        /* "F": TCC Timer*/
  "COM",        /* "G": Other Comm - USB, I2S */
  "AC/GCLK"     /* "H": Analog Comparator or GCLKs */
};


void decode_EIC(int pin, int pmux)
{
  mySerial.print(pinmux_names[pmux]);
}
void decode_ADC(int pin, int pmux)
{
  mySerial.print(pinmux_names[pmux]);
}
const char *pin_to_SERCOMA_info[] = {
  /* PA00 */ nullstr,
  /* PA01 */ nullstr,
  /* PA02 */ nullstr,
/* PA03 */ nullstr,
/* PA04 */ nullstr,
/* PA05 */ nullstr,
/* PA06 */ nullstr,
/* PA07 */ nullstr,
/* PA08 */ "0 P0",
/* PA09 */ "0 P1",
/* PA10 */ "0 P2",
/* PA11 */ "0 P3",
/* PA12 */ "2 P0",
/* PA13 */ "2 P1",
/* PA14 */ "2 P2",
/* PA15 */ "2 P3",
/* PA16 */ "1 P0",
/* PA17 */ "1 P1",
/* PA18 */ "1 P2",
/* PA19 */ "1 P3",
/* PA20 */ "5 P2",
/* PA21 */ "5 P3",
/* PA22 */ "3 P0",
/* PA23 */ "3 P1",
/* PA24 */ "3 P2",
/* PA25 */ "3 P3"
};

const char *pin_to_SERCOMB_info[] = {
/* PB00 */ nullstr,
/* PB01 */ nullstr,
/* PB02 */ nullstr,
/* PB03 */ nullstr,
/* PB04 */ nullstr,
/* PB05 */ nullstr,
/* PB06 */ nullstr,
/* PB07 */ nullstr,
/* PB08 */ nullstr,
/* PB09 */ nullstr,
/* PB10 */ nullstr,
/* PB11 */ nullstr,
/* PB12 */ "4 P0",
/* PB13 */ "4 P1",
/* PB14 */ "4 P2",
/* PB15 */ "4 P3",
/* PB16 */ "5 P0",
/* PB17 */ "5 P1"
};

Sercom * sercomports[] = SERCOM_INSTS;

char *sercomtype_names[] = {
  "(USART)",
  "(UART)",
  "(SPI Slave)",
  "(SPI Master)",
  "(I2C Slave)",
  "(I2C Master)"
};

void decode_SERCOM(int pin, int pmux)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;
  int sercomno, sercomtype;
  Sercom *s;

  mySerial.print(pinmux_names[pmux]);
  if (port == &PORT->Group[0]) {
    if (bitno < (sizeof pin_to_SERCOMA_info/sizeof pin_to_SERCOMA_info[0])) {
      mySerial.print(pin_to_SERCOMA_info[bitno]);
      /*
       * Get sercom number from the string.  (Ugly, but saves space.)
       */
      sercomno = *pin_to_SERCOMA_info[bitno] - '0';
    }
  }
  if (port == &PORT->Group[1]) {
    if (bitno < (sizeof pin_to_SERCOMB_info/sizeof pin_to_SERCOMB_info[0])) {
      mySerial.print(pin_to_SERCOMB_info[bitno]);
      /*
       * Get sercom number from the string.  (Ugly, but saves space.)
       */
      sercomno = *pin_to_SERCOMB_info[bitno] - '0';
    }
  }
  if (sercomno > 0) {  // If the string was null, sercomno will be -48
    s = sercomports[sercomno];
    sercomtype = s->USART.CTRLA.bit.MODE;
    if (sercomtype < ARRAYSIZE(sercomtype_names)) {
      mySerial.print(sercomtype_names[sercomtype]);
    } 
  }
}

const char *pin_to_SERCOMALTA_info[] = {
/* PA00 */ "1 P0",
/* PA01 */ "1 P1",
/* PA02 */ nullstr,
/* PA03 */ nullstr,
/* PA04 */ "0 P0",
/* PA05 */ "0 P1",
/* PA06 */ "0 P2",
/* PA07 */ "0 P3",
/* PA08 */ "2 P0",
/* PA09 */ "2 P1",
/* PA10 */ "2 P2",
/* PA11 */ "2 P3",
/* PA12 */ "4 P0",
/* PA13 */ "4 P1",
/* PA14 */ "4 P2",
/* PA15 */ "4 P3",
/* PA16 */ "3 P0",
/* PA17 */ "3 P1",
/* PA18 */ "3 P2",
/* PA19 */ "3 P3",
/* PA20 */ "3 P2",
/* PA21 */ "3 P3",
/* PA22 */ "5 P0",
/* PA23 */ "5 P1",
/* PA24 */ "5 P2",
/* PA25 */ "5 P3",
/* PA26 */ nullstr,
/* PA27 */ nullstr,
/* PA28 */ nullstr,
/* PA29 */ nullstr,
/* PA30 */ "1 P2",
/* PA31 */ "1 P3",
};

const char *pin_to_SERCOMALTB_info[] = {
/* PB00 */ "5 P2",
/* PB01 */ "5 P3",
/* PB02 */ "5 P0",
/* PB03 */ "5 P1",
/* PB04 */ nullstr,
/* PB05 */ nullstr,
/* PB06 */ nullstr,
/* PB07 */ nullstr,
/* PB08 */ "4 P0",
/* PB09 */ "4 P1",
/* PB10 */ "4 P2",
/* PB11 */ "4 P3",
/* PB12 */ nullstr,
/* PB13 */ nullstr,
/* PB14 */ nullstr,
/* PB15 */ nullstr,
/* PB16 */ nullstr,
/* PB17 */ nullstr,
/* PB18 */ nullstr,
/* PB19 */ nullstr,
/* PB20 */ nullstr,
/* PB21 */ nullstr,
/* PB22 */ "5 P2",
/* PB23 */ "5 P3",
/* PB24 */ nullstr,
/* PB25 */ nullstr,
/* PB26 */ nullstr,
/* PB27 */ nullstr,
/* PB28 */ nullstr,
/* PB29 */ nullstr,
/* PB30 */ "5 P0",
/* PB31 */ "5 P1"
};

void decode_SERCOMA(int pin, int pmux)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;
  int sercomno, sercomtype;
  Sercom *s;

  mySerial.print(pinmux_names[pmux]);
  if (port == &PORT->Group[0]) {
    if (bitno < ARRAYSIZE(pin_to_SERCOMALTA_info)) {
      mySerial.print(pin_to_SERCOMALTA_info[bitno]);
      /*
       * Get sercom number from the string.  (Ugly, but saves space.)
       */
      sercomno = *pin_to_SERCOMALTA_info[bitno] - '0';
    }
  }
  if (port == &PORT->Group[1]) {
    if (bitno < ARRAYSIZE(pin_to_SERCOMALTB_info)) {
      mySerial.print(pin_to_SERCOMALTB_info[bitno]);
      /*
       * Get sercom number from the string.  (Ugly, but saves space.)
       */
      sercomno = *pin_to_SERCOMALTB_info[bitno] - '0';
    }
  }
  if (sercomno > 0) {  // If the string was null, sercomno will be -48
    s = sercomports[sercomno];
    sercomtype = s->USART.CTRLA.bit.MODE;
    if (sercomtype < ARRAYSIZE(sercomtype_names)) {
      mySerial.print(sercomtype_names[sercomtype]);
    }
  }
}

void decode_TC(int pin, int pmux)
{
  mySerial.print(pinmux_names[pmux]);
}
void decode_TCC(int pin, int pmux)
{
  mySerial.print(pinmux_names[pmux]);
}
void decode_COMM(int pin, int pmux)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;

  mySerial.print(pinmux_names[pmux]);
  // PA23..PA25 are for USB
  if (port == &PORT->Group[0] && (bitno >= 23 && bitno <= 25)) {
    mySerial.print("(USB)");
  }
  // PA30..PA31 are for SWD (Serial JTAG debug)
  if (port == &PORT->Group[0] && (bitno >= 30 && bitno <= 31)) {
    mySerial.print("(SWD)");
  }
}
void decode_CLK(int pin, int pmux)
{
  mySerial.print(pinmux_names[pmux]);
}

void print_portname(int pin)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;

  if (port == &PORT->Group[0]) {
    mySerial.print("PA");
  }
  if (port == &PORT->Group[1]) {
    mySerial.print("PB");
  }
  if (bitno < 10) {
    mySerial.print("0");
  }
  mySerial.print(bitno);
  mySerial.print("  ");
}
