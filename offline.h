#ifndef __UGW_OFFLINE__H__
#define __UGW_OFFLINE__H__

#include <Arduino.h>

unsigned long parse_baud(String str_new_baud);
byte parse_parity(String new_parity);
byte parse_stopbit(String new_stopbit);
byte parse_length(String new_length);

#endif
