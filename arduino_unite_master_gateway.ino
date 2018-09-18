#include <EEPROM.h>
#include <avr/wdt.h>
#include "buffer.h"
#include "config.h"
#include "utils.h"
#include "offline.h"

#define DLE 0x10
#define EOT 0x04
#define ENQ 0x05
#define ACK 0x06

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

Buffer* rx1;
Buffer* rx2;
HardwareSerial* slave_serial;
HardwareSerial* master_serial;

enum comm_state {
  BOOTING,
  EMISSION,
  RECEPTION,
  TRANS_DATA
};

struct ser_conf ser1_conf = {0};
byte slvno = 0;
#define EEP_ADDR1 0
#define EEP_ADDR2 (EEP_ADDR1 + sizeof(ser1_conf))
#define EEP_ADDR3 (EEP_ADDR2 + sizeof(byte))

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

void print_slvno(byte no) {
  master_serial->print("slvno : ");
  master_serial->print(no);
  master_serial->println();
}

enum CONF_KEY {
  KEY_SPEED,
  KEY_PARITY,
  KEY_STOPBIT,
  KEY_LENGTH,
  KEY_SLAVE_ADDR,
  KEY_SAVE,
  KEY_ARRAY_SIZE,
};

const String keys[KEY_ARRAY_SIZE] = {
  "speed:",
  "parity:",
  "stopbit:",
  "length:",
  "slvno:",
  "save"
};

void serialEvent() {
  if(mode_state != OFFLINE) {
    rx2->readAll();
    return;
  }

  bool changed = false;
  struct ser_conf new_conf = ser1_conf;
  byte new_slvno = slvno;

  while(1) {
    if(master_serial->available() == 0) {
      delay(100);
      continue;
    }
    
    String line = master_serial->readString();
  
    if(line.startsWith(keys[KEY_SPEED])) {
      changed = change_speed(&new_conf, parse_baud(line.substring(keys[KEY_SPEED].length())));
    } else if(line.startsWith(keys[KEY_PARITY])) {
      changed = change_parity(&new_conf, parse_parity(line.substring(keys[KEY_PARITY].length())));
    } else if(line.startsWith(keys[KEY_STOPBIT])) {
      changed = change_stopbit(&new_conf, parse_stopbit(line.substring(keys[KEY_STOPBIT].length())));
    } else if(line.startsWith(keys[KEY_LENGTH])) {
      changed = change_length(&new_conf, parse_length(line.substring(keys[KEY_LENGTH].length())));
    } else if(line.startsWith(keys[KEY_SLAVE_ADDR])) {
      new_slvno = line.substring(keys[KEY_SLAVE_ADDR].length()).toInt();
      if(new_slvno == 0) {
        new_slvno = slvno;
      } else {
        changed = true;
      }
    } else if(line.equals(keys[KEY_SAVE])) {
      break;
    } else {
      master_serial->print("invalid text!! : ");
      master_serial->println(line);
    }
    
    master_serial->print("current settings:");
    print_serial_conf(&new_conf);
    print_slvno(new_slvno);
  }

  if(changed) {
    master_serial->println("save");
    master_serial->print("old settings:");
    print_serial_conf(&ser1_conf);
    print_slvno(slvno);
    master_serial->print("new settings:");
    print_serial_conf(&new_conf);
    print_slvno(new_slvno);
    EEPROM.put(EEP_ADDR1, new_conf);
    EEPROM.put(EEP_ADDR3, new_slvno);
  } else {
    master_serial->println("no any changes. did not save.");
  }
  master_serial->println("restaring...");
  on_change_mode();
  software_reset();
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

  master_serial->println("commands:");
  for(int i = 0; i < (sizeof(keys) / sizeof(keys[0])); i++) {
    master_serial->print("  ");
    master_serial->println(keys[i]);
  }
}

void online_setup() {
  slave_serial = &Serial1;
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

  master_serial = &Serial;
  master_serial->begin(115200);

  attachInterrupt(digitalPinToInterrupt(MODE_CHNG_PIN), on_change_mode, FALLING);
  
  load_serial_config(EEP_ADDR1, &ser1_conf);
  
  load_slave_addr(EEP_ADDR3, &slvno);
  
  tbit = ceil(1000000 / ser1_conf.speed);

  switch(read_mode()) {
    case OFFLINE:
      master_serial->println("offline");
      master_serial->print("tbit : ");
      master_serial->print(tbit);
      master_serial->println(" us");
      master_serial->print("slave addr : ");
      master_serial->println(slvno);
      offline_setup();
      digitalWrite(BOOTING_LED, HIGH);
      break;
    case ONLINE:
    default:
      online_setup();
      break;    
  }
}

void sendEmission(Buffer *rx) {
  rx->clear_tx();
  rx->push_tx(DLE);
  rx->push_tx(ENQ);
  rx->push_tx(slvno);
  rx->send();
}

void sendAck(Buffer *rx) {
  rx->clear_tx();
  rx->push_tx(ACK);
  rx->send();
}

enum comm_state recv_reception(Buffer *rx) {
  unsigned long first = micros();
  unsigned char c;
  rx->clear_rx();
  while(1) {
    if(rx->available() > 0) {
      rx->readAll();
      switch(rx->get_buf(0)) {
      case DLE:
        return TRANS_DATA;
      default:
        delayMicroseconds(11 * tbit);
        return EMISSION;
      }
    }

    if(micros() - first >= 30000) {
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
      // master_serial->println("BOOTING");
      digitalWrite(BOOTING_LED, HIGH);
      rx1->flush_rx(11 * tbit);
      rx2->flush_rx(11 * tbit);
      state = EMISSION;
      digitalWrite(BOOTING_LED, LOW);
      break;
    case EMISSION:
      // master_serial->println("EMISSION");
      if(rx2->get_tail() > 0) {
        digitalWrite(EMIT_MESSAGE_LED, HIGH);
        // master_serial->println("trans_to_serial2");
        rx2->trans(slave_serial, 11 * tbit);
        rx2->clear_rx();
        state = EMISSION;
        digitalWrite(EMIT_MESSAGE_LED, LOW);
      } else {
        digitalWrite(EMISSION_LED, HIGH);
        // master_serial->println("emission");
        sendEmission(rx1);
        state = RECEPTION;
        digitalWrite(EMISSION_LED, LOW);
      }
      break;
    case RECEPTION:
      // master_serial->println("RECEPTION");
      digitalWrite(RECEPTION_LED, HIGH);
      state = recv_reception(rx1);
      digitalWrite(RECEPTION_LED, LOW);
      break;
    case TRANS_DATA:
      // master_serial->println("TRANS_DATA");
      digitalWrite(TRANSDATA_LED, HIGH);
      rx1->trans(master_serial, 11 * tbit);
      rx1->clear_rx();
      sendAck(rx1);
      state = EMISSION;
      digitalWrite(TRANSDATA_LED, LOW);
      break;
    default:
      // master_serial->println("default");
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
