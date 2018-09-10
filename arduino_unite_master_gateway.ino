#include <EEPROM.h>
#include <avr/wdt.h>
#include "buffer.h"
#include "config.h"
#include "utils.h"
#include "offline.h"

#define DLE 0x10
#define EOT 0x04
#define ENQ 0x05

#define BOOTING_LED 22
#define EMISSION_LED 23
#define RECEPTION_LED 24
#define TRANSDATA_LED 25
#define EMIT_MESSAGE_LED 26
#define CONF_MODE_LED 27

#define MODE_CHNG_PIN 2

unsigned long tbit;

enum mode_state_enum {
  ONLINE = 0,
  OFFLINE = 1
};
volatile byte mode_state = ONLINE;

volatile Buffer* rx1;
volatile Buffer* rx2;
HardwareSerial* slave_serial;
HardwareSerial* master_serial;

enum comm_state {
  BOOTING,
  EMISSION,
  RECEPTION,
  TRANS_DATA
};

struct ser_conf ser1_conf = {0};
#define EEP_ADDR1 0
#define EEP_ADDR2 (EEP_ADDR1 + sizeof(ser1_conf))

void serialEvent1() {
  rx1->read();
}

void serialEvent2() {
  rx2->read();
}

void print_serial_conf(struct ser_conf *conf) {
  master_serial->print(conf->speed);
  master_serial->print(",");
  master_serial->print(length_char(conf->config));
  master_serial->print(",");
  master_serial->print(parity_char(conf->config));
  master_serial->print(",");
  master_serial->print(stopbit_char(conf->config));
  master_serial->println();
}

void serialEvent() {
  if(mode_state != OFFLINE) {
    return;
  }

  bool changed = false;
  struct ser_conf new_conf = ser1_conf;

  while(1) {
    if(master_serial->available() == 0) {
      delay(100);
      continue;
    }
    
    String line = master_serial->readString();
  
    const char *key_speed = "speed:";
    const char *key_parity = "parity:";
    const char *key_stopbit = "stopbit:";
    const char *key_length = "length:";
    const char *key_save = "save";

    if(line.startsWith(key_speed)) {
      changed = change_speed(&new_conf, parse_baud(line.substring(strlen(key_speed))));
    } else if(line.startsWith(key_parity)) {
      changed = change_parity(&new_conf, parse_parity(line.substring(strlen(key_parity))));
    } else if(line.startsWith(key_stopbit)) {
      changed = change_stopbit(&new_conf, parse_stopbit(line.substring(strlen(key_stopbit))));
    } else if(line.startsWith(key_length)) {
      changed = change_length(&new_conf, parse_length(line.substring(strlen(key_length))));
    } else if(line.equals(key_save)) {
      break;
    } else {
      master_serial->print("invalid text!! : ");
      master_serial->println(line);
    }
    
    master_serial->print("current settings:");
    print_serial_conf(&new_conf);
  }

  if(changed) {
    master_serial->println("save");
    master_serial->print("old settings:");
    print_serial_conf(&ser1_conf);
    master_serial->print("new settings:");
    print_serial_conf(&new_conf);
    EEPROM.put(0, new_conf);
    on_change_mode();
    software_reset();
  }
}

byte read_mode() {
  byte mode = EEPROM.read(EEP_ADDR2);
  switch(mode) {
    case ONLINE:
    case OFFLINE:
      mode_state = mode;
      break;
    default:
      mode_state = ONLINE;
  }
  return mode;
}

void on_change_mode() {
  if(mode_state == OFFLINE) {
    EEPROM.write(EEP_ADDR2, ONLINE);
  } else {
    EEPROM.write(EEP_ADDR2, OFFLINE);
  }
  software_reset();
}

void offline_setup() {
  master_serial->setTimeout(200);
  master_serial->print("current settings:");
  print_serial_conf(&ser1_conf);
}

void online_setup() {
  slave_serial->begin(ser1_conf.speed, ser1_conf.config);
  rx1 = new Buffer(slave_serial);
  rx2 = new Buffer(master_serial);
}

void setup() {
  pinMode(BOOTING_LED, OUTPUT);
  pinMode(EMISSION_LED, OUTPUT);
  pinMode(RECEPTION_LED, OUTPUT);
  pinMode(TRANSDATA_LED, OUTPUT);
  pinMode(EMIT_MESSAGE_LED, OUTPUT);
  pinMode(CONF_MODE_LED, OUTPUT);
  pinMode(MODE_CHNG_PIN, INPUT_PULLUP);

  slave_serial = &Serial1;
  master_serial = &Serial;
  master_serial->begin(115200);

  attachInterrupt(digitalPinToInterrupt(MODE_CHNG_PIN), on_change_mode, FALLING);
  
  load_serial_config(EEP_ADDR1, &ser1_conf);

  tbit = ceil(10000000 / ser1_conf.speed);

  switch(read_mode()) {
    case OFFLINE:
      master_serial->println("offline");
      master_serial->print("tbit : ");
      master_serial->print(tbit);
      master_serial->println(" us");
      offline_setup();
      break;
    case ONLINE:
    default:
      online_setup();
      break;    
  }
}

void sendEmission(unsigned char slvno) {
  rx1->push(DLE);
  rx1->push(ENQ);
  rx1->push(slvno);
  rx1->flush();
  delayMicroseconds(11 * tbit);
}

enum comm_state recv_reception() {
  unsigned long first = millis();
  unsigned char c;
  while(1) {
    if(rx1->get_tail() > 0) {
      switch(rx1->get_buf(0)) {
      case DLE:
        return TRANS_DATA;
      default:
        delayMicroseconds(11 * tbit);
        return EMISSION;
      }
    }

    if(millis() - first > 30) {
      return  EMISSION;
    }
  }
}

void offline_loop() {
  // NOP for now;
}

void online_loop() {
  static enum comm_state state = BOOTING;
  switch(state) {
    case BOOTING:
      // Serial.println("BOOTING");
      digitalWrite(BOOTING_LED, HIGH);
      rx1->flush_rx(11 * tbit);
      rx2->flush_rx(11 * tbit);
      state = EMISSION;
      digitalWrite(BOOTING_LED, LOW);
      break;
    case EMISSION:
      // Serial.println("EMISSION");
      if(rx2->get_tail() > 0) {
        digitalWrite(EMIT_MESSAGE_LED, HIGH);
        // Serial.println("trans_to_serial2");
        rx2->trans(master_serial, 11 * tbit);
        rx2->clear_rx();
        state = EMISSION;
        digitalWrite(EMIT_MESSAGE_LED, LOW);
      } else {
        digitalWrite(EMISSION_LED, HIGH);
        // Serial.println("emission");
        rx1->clear_rx();
        sendEmission(0x01);
        state = RECEPTION;
        digitalWrite(EMISSION_LED, LOW);
      }
      break;
    case RECEPTION:
      // Serial.println("RECEPTION");
      digitalWrite(RECEPTION_LED, HIGH);
      state = recv_reception();
      digitalWrite(RECEPTION_LED, LOW);
      break;
    case TRANS_DATA:
      // Serial.println("TRANS_DATA");
      digitalWrite(TRANSDATA_LED, HIGH);
      rx1->trans(slave_serial, 11 * tbit);
      state = EMISSION;
      digitalWrite(TRANSDATA_LED, LOW);
      break;
    default:
      // Serial.println("default");
      break;
  }
}

void loop() {
  switch(mode_state) {
    case ONLINE:
      online_loop();
      break;
    case OFFLINE:
      offline_loop();
      break;
  }
}
