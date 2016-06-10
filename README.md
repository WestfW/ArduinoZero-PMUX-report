# ArduinoZero-PMUX-report 
Generate a user-readable report of the current state of the pins (what
they're doing) on Arduino Zero and other SAMD21 based systems

In the forum discussion here:
http://forum.arduino.cc/index.php?topic=403674.msg2776323 the question is
asked

"How can I tell how the Arduino core has configured the various multi-purpose pins of the SAMD21 chip?"

This is a good question, and one that CAN be answered without too much
trouble, since the SAMD21 controls the pins from a central "Pin
Multiplexing" datastructure.  Each pin has a bit saying whether it is just
GPIO or "something else", and a separate field describing the "something
else."  It's just a SMOP to decode this information.  (and invitingly, while
a full decode is possible, even a relatively minor effort can yield useful
information.)

Sample Output

````
SAMD Pin State Report

0       PA11  PMUX(2) SERCOM0 P3(UART)
1       PA10  PMUX(2) SERCOM0 P2(UART)
2       PA14  GPIO I
3       PA09  GPIO I
4       PA08  GPIO I
5       PA15  GPIO I
6       PA20  GPIO I
7       PA21  GPIO I
8       PA06  GPIO I
9       PA07  GPIO I
10      PA18  PMUX(4) TC3/WO0
11      PA16  GPIO I
12      PA19  PMUX(5) TCC0/WO3
13      PA17  GPIO O
14 (A0) PA02  PMUX(1) ADC
15 (A1) PB08  PMUX(1) ADC
16 (A2) PB09  PMUX(1) ADC
17 (A3) PA04  PMUX(1) ADC
18 (A4) PA05  PMUX(1) ADC
19 (A5) PB02  PMUX(1) ADC
20      PA22  PMUX(2) SERCOM3 P0(I2C Master)
21      PA23  PMUX(2) SERCOM3 P1(I2C Master)
22      PA12  PMUX(3) SERCOM4 P0(SPI Master)
23      PB10  PMUX(3) SERCOM4 P2(SPI Master)
24      PB11  PMUX(3) SERCOM4 P3(SPI Master)
25      PB03  GPIO I
26      PA27  GPIO I
27      PA28  GPIO I
28      PA24  PMUX(6) COM(USB)
29      PA25  PMUX(6) COM(USB)
30      PB22  PMUX(3) SERCOM5 P2(UART)
31      PB23  PMUX(3) SERCOM5 P3(UART)
32      PA22  PMUX(2) SERCOM3 P0(I2C Master)
33      PA23  PMUX(2) SERCOM3 P1(I2C Master)
34      PA19  PMUX(5) TCC0/WO3
35      PA16  GPIO I
36      PA18  PMUX(4) TC3/WO0
37      PA17  GPIO O
38      PA13  GPIO I
````
