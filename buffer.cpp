#include <Arduino.h>
#include <HardwareSerial.h>
#include "buffer.h"

void Buffer::trans(HardwareSerial *to, unsigned long timeout) {
  unsigned long start = micros();
  while(1) {
    if(serial->available() > 0) {
      buf[tail++] = serial->read();
      start = micros();
    } else {
      if(micros() - start > 2 * timeout) {
        break;
      }
    }
  }
  
  for(int i = 0; i < tail;i++) {
    to->write(buf[i]);
  }
  to->flush();
}

void Buffer::clear_rx() {
  memset(buf, 0, sizeof(buf));
  tail = 0;
}

void Buffer::clear_tx() {
  memset(tx_buf, 0, sizeof(tx_buf));
  tx_tail = 0;
}

void Buffer::readAll() {
  tail += serial->readBytes(buf+tail, serial->available());
}

int Buffer::get_tail() {
  return tail;
}

int Buffer::available() {
  return this->serial->available();
}

char Buffer::get_buf(int idx) {
  return buf[idx];
}

int Buffer::send() {
	int i;
	for(i = 0;i<tx_tail;i++) {
		serial->write(tx_buf[i]);
	}
	serial->flush();
	tx_tail = 0;
	return i;
}

char* Buffer::push_tx(char c) {
	tx_buf[tx_tail++] = c;
	return tx_buf;
}

Buffer::Buffer(HardwareSerial *sio) {
	serial = sio;
  this->clear_rx();
}

void Buffer::flush_rx(unsigned long timeout) {
  unsigned long start = micros();
  while(1) {
    if(serial->available() > 0) {
      start = micros();
    } else {
      if(micros() - start > timeout) {
        return;
      }
    }
  }
}
