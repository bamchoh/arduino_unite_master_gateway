#include "config.h"

byte serial_config(int length, char parity, int stopbit) {
  byte conf = 0;
  switch(length) {
    case 7:
      conf |= CS7;
      break;
    default:
      conf |= CS8;
      break;
  }

  switch(parity) {
    case 'E':
      conf |= PARENB;
      break;
    case 'O':
      conf |= PARENB | PARODD;
      break;
  }

  switch(stopbit) {
    case 2:
      conf |= CSTOPB;
      break;
  }

  return conf;
}

byte get_parity(byte config) {
  return config & (PARENB | PARODD);
}

byte get_stopbit(byte config) {
  return config & CSTOPB;
}

byte get_length(byte config) {
  return config & CS8;
}

bool change_speed(struct ser_conf *conf, unsigned long new_speed) {
  if(new_speed != 0 && new_speed != conf->speed) {
    conf->speed = new_speed;
    return true;
  }
  return false;
}

bool change_parity(struct ser_conf *conf, byte new_parity) {
  if(new_parity != get_parity(conf->config)) {
    conf->config &= ~(PARENB | PARODD);
    conf->config |= new_parity;
    return true;
  }
  return false;
}

bool change_stopbit(struct ser_conf *conf, byte new_stopbit) {
  if(new_stopbit != get_stopbit(conf->config)) {
    conf->config &= ~(CSTOPB);
    conf->config |= new_stopbit;
    return true;
  }
  return false;
}

bool change_length(struct ser_conf *conf, byte new_length) {
  if(new_length != get_length(conf->config)) {
    conf->config &= ~(CS8);
    conf->config |= new_length;
    return true;
  }
  return false;
}

char parity_char(byte config) {
  switch(get_parity(config)) {
    case PARENB:
      return 'E';
    case PARENB | PARODD:
      return 'O';
    default:
      return 'N';
  }
}

char stopbit_char(byte config) {
  switch(get_stopbit(config)) {
    case CSTOPB:
      return '2';
    default:
      return '1';
  }
}

char length_char(byte config) {
  switch(get_length(config)) {
    case CS7:
      return '7';
    default:
      return '8';
  }
}

