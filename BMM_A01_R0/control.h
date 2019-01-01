#ifndef CONTROL_H_INCLUDED
#define CONTROL_H_INCLUDED
#include <stdint.h>

void SendMsg(const char *);			// Send String Back to Host via USB
void SendCode(int8_t code);
void SendCodeVal(int8_t, uint16_t);
void SendErrorMsg(const char *msg);
void poll_control(void);

#endif