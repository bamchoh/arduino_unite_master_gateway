#ifndef __UGW_CONFIG__H__
#define __UGW_CONFIG__H__

#include "Arduino.h"
#include "serial.h"

struct ser_conf {
  unsigned long speed;
  byte config;
};

byte serial_config(int length, char parity, int stopbit);
byte get_parity(byte config);
byte get_stopbit(byte config);
byte get_length(byte config);
bool change_speed(struct ser_conf *conf, unsigned long new_speed);
bool change_parity(struct ser_conf *conf, byte new_parity);
bool change_stopbit(struct ser_conf *conf, byte new_stopbit);
bool change_length(struct ser_conf *conf, byte new_length);
char parity_char(byte config);
char stopbit_char(byte config);
char length_char(byte config);

#endif
