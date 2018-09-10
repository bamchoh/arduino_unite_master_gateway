#ifndef __UGW_UTILS__H__
#define __UGW_UTILS__H__

#include <avr/wdt.h>
#include <EEPROM.h>
#include "config.h"

void software_reset() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void load_serial_config(unsigned int addr, struct ser_conf *conf) {
  EEPROM.get(addr, *conf);
  if(conf->speed < 2400 || conf->speed > 115200) {
    conf->speed = 19200;
    conf->config = serial_config(8, 'E', 1);
    EEPROM.put(addr, *conf);
  }
}

#endif
