#include <Arduino.h>

#define PRINTS(...) print_s(__VA_ARGS__)
#define PRINTN(...) print_n2(__VA_ARGS__)

static boolean print_pinno(int i, char *name);
static void print_pinmux(int pin, int pmuxval);
static void decode_EIC(int pin, int pmux);
static void decode_ADC(int pin, int pmux);
static void decode_SERCOM(int pin, int pmux);
static void decode_SERCOMA(int pin, int pmux);
static void decode_TC(int pin, int pmux);
static void decode_TCC(int pin, int pmux);
static void decode_COMM(int pin, int pmux);
static void decode_CLK(int pin, int pmux);
static void print_portname(int pin);
static void print_s(const char *s);
static void print_n2(int n);

static char *bufp;
static const char nullstr[] = "";

/*
 * Print an Arduino pin number, and it's "name."
 * (in particular, check for the Analog pins and label them)
 */
static boolean print_pinno(int i, char *name)
{
  int pintype = g_APinDescription[i].ulPinType;
  if (pintype == PIO_NOT_A_PIN) {
    return false;
  }
  PRINTN(i);
  if (name && *name) {
    PRINTS(" [");
    PRINTS(name);
    PRINTS("] ");
  } else if (pintype == PIO_ANALOG) {  // try to detect and renumber Analog pins.
    PRINTS(" (A");
    PRINTN(i - PIN_A0);
    PRINTS(") ");
  } else {
    PRINTS("      ");
  }
  return true;
}


// Convenient macro
#define ARRAYSIZE(a) (sizeof a/sizeof a[0])

/*
 * Table of functions to do decoding of individual pinmux values.
 * In general, each PMUX value has a standard function "class", and then
 * you have to do further decoding based on port/bit and perhaps peripheral
 * internals to figure out the details.  (For example, a SERCOM PMUX value
 * can apply to one of six different SERCOM modules on a SAMD21, and an ANALOG
 * PMUX value could be ADC, REF, DAC, or TouchController.)
 */
static void (*PMUX_decode_func[8])(int, int) = {
  decode_EIC,
  decode_ADC,
  decode_SERCOM,
  decode_SERCOMA,
  decode_TC,
  decode_TCC,
  decode_COMM,
  decode_CLK
};

/*
 * Print the raw PMUX value
 * Call via the function table for name and details.
 */
static void print_pinmux(int pin, int pmuxval)
{
  PRINTS("PMUX");
  PRINTS("(");
  PRINTN(pmuxval);
  PRINTS(") ");
  if (pmuxval < ARRAYSIZE(PMUX_decode_func)) {
    PMUX_decode_func[pmuxval](pin, pmuxval);
  }
}


/*
 *   Pin multiplexer value names (PMUXn.PMUXE/O)
 */
static const char *pinmux_names[8] = {
  "EIC",   // "A": External Interrupts
  "ADC",   // "B": REF, ADC, AC, PTC, DAC
  "SERCOM",   /* "C": SerCom */
  "SERCOM",  /* "D": Alternate SERCOM */
  "TC/TCC",     /* "E": TC or TCC Timer */
  "TCC",        /* "F": TCC Timer*/
  "COM",        /* "G": Other Comm - USB, I2S */
  "AC/GCLK"     /* "H": Analog Comparator or GCLKs */
};


static void decode_EIC(int pin, int pmux)
{
  PRINTS(pinmux_names[pmux]);   // default action
}

static void decode_ADC(int pin, int pmux)
{
  PRINTS(pinmux_names[pmux]);   // default action
}

/*
 * Sercom and Sercom ALT decoding.
 * pins can have one of two sercom settings, which are similar but rather
 * randomly (?) assigned to sercom functions.  The "P0" part refers to the
 * "pad" within the sercom unit (each Sercom has up to 4 pads.)  Mapping
 * between "pad" and actual function is controlled by fields in the CTRLA reg
 */

static const char *pin_to_SERCOMA_info[] = {
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

static const char *pin_to_SERCOMB_info[] = {
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

/*
 * SERCOM_INSTS is defined in the Atmel chip-specific samXXXX.h file, as a
 * list of all the Sercom units available on that specific chip.  (some chips
 * have more than others.
 */

static Sercom * sercomports[] = SERCOM_INSTS;

/*
 * Sercom configuration type indexed by CTRLA.MODE
 */
static char *sercomtype_names[] = {
  "(USART)",
  "(UART)",
  "(SPI Slave)",
  "(SPI Master)",
  "(I2C Slave)",
  "(I2C Master)"
};


static void decode_SERCOM(int pin, int pmux)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;
  int sercomno, sercomtype;
  Sercom *s;

  PRINTS(pinmux_names[pmux]);
  if (port == &PORT->Group[0]) {
    if (bitno < ARRAYSIZE(pin_to_SERCOMA_info)) {
      PRINTS(pin_to_SERCOMA_info[bitno]);
      /*
       * Get sercom number from the string.  (Ugly, but saves space.)
       */
      sercomno = *pin_to_SERCOMA_info[bitno] - '0';
    }
  }
  if (port == &PORT->Group[1]) {
    if (bitno < ARRAYSIZE(pin_to_SERCOMB_info)) {
      PRINTS(pin_to_SERCOMB_info[bitno]);
      /*
       * Get sercom number from the string.  (Ugly, but saves space.)
       */
      sercomno = *pin_to_SERCOMB_info[bitno] - '0';
    }
  }
  if (sercomno >= 0) {  // If the string was null, sercomno will be -48
    s = sercomports[sercomno];
    sercomtype = s->USART.CTRLA.bit.MODE;
    if (sercomtype < ARRAYSIZE(sercomtype_names)) {
      PRINTS(sercomtype_names[sercomtype]);
    } 
  }
}

/*
 * Tables for SercomALT PMUX value
 */

static const char *pin_to_SERCOMALTA_info[] = {
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

static const char *pin_to_SERCOMALTB_info[] = {
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

static void decode_SERCOMA(int pin, int pmux)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;
  int sercomno, sercomtype;
  Sercom *s;

  PRINTS(pinmux_names[pmux]);
  if (port == &PORT->Group[0]) {
    if (bitno < ARRAYSIZE(pin_to_SERCOMALTA_info)) {
      PRINTS(pin_to_SERCOMALTA_info[bitno]);
      /*
       * Get sercom number from the string.  (Ugly, but saves space.)
       */
      sercomno = *pin_to_SERCOMALTA_info[bitno] - '0';
    }
  }
  if (port == &PORT->Group[1]) {
    if (bitno < ARRAYSIZE(pin_to_SERCOMALTB_info)) {
      PRINTS(pin_to_SERCOMALTB_info[bitno]);
      /*
       * Get sercom number from the string.  (Ugly, but saves space.)
       */
      sercomno = *pin_to_SERCOMALTB_info[bitno] - '0';
    }
  }
  if (sercomno >= 0) {  // If the string was null, sercomno will be -48
    s = sercomports[sercomno];
    sercomtype = s->USART.CTRLA.bit.MODE;
    if (sercomtype < ARRAYSIZE(sercomtype_names)) {
      PRINTS(sercomtype_names[sercomtype]);
    }
  }
}

/*
 * Decoding for Timer PMUX values.
 * SAMD has two types of timers a "Timer/Counter" (TC) and a
 * "Timer/Counter for Control (TCC).  Both can be used to implement the
 * Arduino PWM-based analogWrite() functionality.
 */

static const char *pin_to_tc_info_a[] = {
  /* PA00 */ "TCC2/WO0",
  /* PA01 */ "TCC2/WO1",
  /* PA02 */ nullstr,
  /* PA03 */ nullstr,
  /* PA04 */ "TCC0/WO0",
  /* PA05 */ "TCC0/WO1",
  /* PA06 */ "TCC1/WO0",
  /* PA07 */ "TCC1/WO1",
  /* PA08 */ "TCC0/WO0",
  /* PA09 */ "TCC0/WO1",
  /* PA10 */ "TCC1/WO0",
  /* PA11 */ "TCC1/WO1",
  /* PA12 */ "TCC2/WO0",
  /* PA13 */ "TCC2/WO1",
  /* PA14 */ "TC3/WO0",
  /* PA15 */ "TC3/WO1",
  /* PA16 */ "TCC2/WO0",
  /* PA17 */ "TCC2/WO1",
  /* PA18 */ "TC3/WO0",
  /* PA19 */ "TC3/WO1",
  /* PA20 */ "TC7/WO0",
  /* PA21 */ "TC7/WO1",
  /* PA22 */ "TC4/WO0",
  /* PA23 */ "TC4/WO1",
  /* PA24 */ "TC5/WO0",
  /* PA25 */ "TC5/WO1",
  /* PA26 */ nullstr,
  /* PA27 */ nullstr,
  /* PA28 */ nullstr,
  /* PA29 */ nullstr,
  /* PA30 */ "TCC1/WO0",
  /* PA31 */ "TCC1/WO0"
};

static const char *pin_to_tc_info_b[] = {
  /* PB00 */ "TC7/WO0",
  /* PB01 */ "TC7/WO1",
  /* PB02 */ "TC6/WO0",
  /* PB03 */ "TC6/WO1",
  /* PB04 */ nullstr,
  /* PB05 */ nullstr,
  /* PB06 */ nullstr,
  /* PB07 */ nullstr,
  /* PB08 */ "TC4/WO0",
  /* PB09 */ "TC4/WO1",
  /* PB10 */ "TC5/WO0",
  /* PB11 */ "TC5/WO1",
  /* PB12 */ "TC4/WO0",
  /* PB13 */ "TC4/WO1",
  /* PB14 */ "TC5/WO0",
  /* PB15 */ "TC5/WO1",
  /* PB16 */ "TC6/WO0",
  /* PB17 */ "TC6/WO1",
  /* PB18,19,20, 21 */ nullstr, nullstr, nullstr, nullstr, 
  /* PB22 */ "TC7/WO0",
  /* PB23 */ "TC7/WO1",
  /* PB24-29 */ nullstr,nullstr,nullstr,nullstr,nullstr,nullstr,
  /* PB30 */ "TCC0/WO0",
  /* PB31 */ "TCC0/WO1",
};

static void decode_TC(int pin, int pmux)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;

  if (port == &PORT->Group[0]) {
    if (bitno < ARRAYSIZE(pin_to_tc_info_a)) {
      PRINTS(pin_to_tc_info_a[bitno]);
    }
  }
  if (port == &PORT->Group[1]) {
    if (bitno < ARRAYSIZE(pin_to_tc_info_a)) {
      PRINTS(pin_to_tc_info_b[bitno]);
    }
  }
}

static const char *pin_to_tcc_info_a[] = {
  /* PA00 */ nullstr,
  /* PA01 */ nullstr,
  /* PA02 */ nullstr,
  /* PA03 */ nullstr,
  /* PA04 */ nullstr,
  /* PA05 */ nullstr,
  /* PA06 */ nullstr,
  /* PA07 */ nullstr,
  /* PA08 */ "TCC1/WO2",
  /* PA09 */ "TCC1/WO3",
  /* PA10 */ "TCC0/WO2",
  /* PA11 */ "TCC0/WO3",
  /* PA12 */ "TCC0/WO6",
  /* PA13 */ "TCC0/WO7",
  /* PA14 */ "TCC0/WO4",
  /* PA15 */ "TCC0/WO5",
  /* PA16 */ "TCC0/WO6",
  /* PA17 */ "TCC0/WO7",
  /* PA18 */ "TCC0/WO2",
  /* PA19 */ "TCC0/WO3",
  /* PA20 */ "TCCO/WO6",
  /* PA21 */ "TCC0/WO7",
  /* PA22 */ "TCC0/WO5",
  /* PA23 */ "TCC1/WO2",
  /* PA24 */ "TCC1/WO3",
};


static const char *pin_to_tcc_info_b[] = {
  /* PB00 */  nullstr,
  /* PB01 */  nullstr,
  /* PB02 */  nullstr,
  /* PB03 */  nullstr,
  /* PB04 */  nullstr,
  /* PB05 */  nullstr,
  /* PB06 */  nullstr,
  /* PB07 */  nullstr,
  /* PB08 */  nullstr,
  /* PB09 */  nullstr,
  /* PB10 */ "TCC0/WO4",
  /* PB11 */ "TCC0/WO5",
  /* PB12 */ "TCC0/WO6",
  /* PB13 */ "TCC0/WO7",
  /* PB14 */  nullstr,
  /* PB15 */  nullstr,
  /* PB16 */ "TCC0/WO4",
  /* PB17 */ "TCC0/WO5",
  /* 18-21 */  nullstr, nullstr, nullstr, nullstr,
  /* PB22-29 */  nullstr,nullstr,nullstr,nullstr,nullstr,nullstr,nullstr,nullstr,
  /* PB30 */ "TCC1/WO2",
  /* PB31 */ "TCC1/WO3"
};


static void decode_TCC(int pin, int pmux)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;

  if (port == &PORT->Group[0]) {
    if (bitno < ARRAYSIZE(pin_to_tc_info_a)) {
      PRINTS(pin_to_tcc_info_a[bitno]);
    }
  }
  if (port == &PORT->Group[1]) {
    if (bitno < ARRAYSIZE(pin_to_tc_info_a)) {
      PRINTS(pin_to_tcc_info_b[bitno]);
    }
  }
}

/*
 * "COM" are special communications peripherals like USB, SWD, or ISS
 */
static void decode_COMM(int pin, int pmux)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;

  PRINTS(pinmux_names[pmux]);
  // PA23..PA25 are for USB
  if (port == &PORT->Group[0] && (bitno >= 23 && bitno <= 25)) {
    PRINTS("(USB)");
  }
  // PA30..PA31 are for SWD (Serial JTAG debug)
  if (port == &PORT->Group[0] && (bitno >= 30 && bitno <= 31)) {
    PRINTS("(SWD)");
  }
}

static void decode_CLK(int pin, int pmux)
{
  PRINTS(pinmux_names[pmux]);   // default action
}

/*
 * Given an Arduino pin number, print the datasheet-style Port and Bit numbers
 */
static void print_portname(int pin)
{
  PortGroup *port = digitalPinToPort(pin);
  int bitno = g_APinDescription[pin].ulPin;

  if (port == &PORT->Group[0]) {
    PRINTS("PA");
  }
  if (port == &PORT->Group[1]) {
    PRINTS("PB");
  }
  PRINTN(bitno);
  PRINTS("  ");
}

/*
 * copy ("print") a string to the temporary buffer
 */
static void print_s(const char *s)
{
  while (*s) {
    *bufp++ = *s++;
  }
  *bufp = 0;
}

/*
 * copy ("print") a two digit decimal number to the temporary buffer
 */
static void print_n2(int n)
{
  if (n <= 99) {
    *bufp++ = (n/10) + '0';
    *bufp++ = (n % 10) + '0';
    *bufp = 0;
  } else {
    print_s("***");
  }
}

/*
 * generate a pinmux report for a single pin.  Pass in a buffer of sufficient size,
 * and an optional name.
 */

void pinmux_report(byte pin, char *p, char *name)
{
  bufp = p;
  *bufp = 0;
  if (print_pinno(pin, name)) {  // Does pin exist?

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
      PRINTS("GPIO ");
      if (port->DIR.reg & (1 << bitno)) { // Decode pinmode (direction.)
	PRINTS("O");
      } else {
	PRINTS("I");
      }
    } // Pinmux or GPIO
  } // Pin existance
}
