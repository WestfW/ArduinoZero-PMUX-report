#ifndef SAMD_pinmux_h
#define SAMD_pinmux_h

#include <Arduino.h>


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

extern void pinmux_report(byte pin, char *p, char *name);

#ifdef __cplusplus
} // extern "C"
#endif
#endif
