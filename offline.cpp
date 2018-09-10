#include "offline.h"
#include "serial.h"

unsigned long parse_baud(String str_new_baud) {
  if(str_new_baud.compareTo("2400") == 0) {
    return 2400;
  } else if(str_new_baud.compareTo("4800") == 0) {
    return 4800;
  } else if(str_new_baud.compareTo("9600") == 0) {
    return 9600;
  } else if(str_new_baud.compareTo("19200") == 0) {
    return 19200;
  } else if(str_new_baud.compareTo("38400") == 0) {
    return 38400;
  } else if(str_new_baud.compareTo("57600") == 0) {
    return 57600;
  } else if(str_new_baud.compareTo("115200") == 0) {
    return 115200;
  }
  return 0;
}

byte parse_parity(String new_parity) {
  if(new_parity.equals("even")) {
    return PARENB;
  }

  if(new_parity.equals("odd")) {
    return PARENB | PARODD;
  }

  return 0; // NONE
}

byte parse_stopbit(String new_stopbit) {
  if(new_stopbit.equals("2")) {
    return CSTOPB;
  }

  return 0; // 1 bit
}

byte parse_length(String new_length) {
  if(new_length.equals("7")) {
    return CS7;
  }

  return CS8; // 1 bit
}

