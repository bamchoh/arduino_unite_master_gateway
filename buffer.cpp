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
      if(micros() - start > timeout) {
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
  noInterrupts();
  memset(buf, 0, sizeof(buf));
  tail = 0;
  interrupts();
}

void Buffer::read() {
  tail += serial->readBytes(buf+tail, serial->available());
}

int Buffer::get_tail() {
  return tail;
}

char Buffer::get_buf(int idx) {
  return buf[idx];
}

int Buffer::flush() {
	int i;
	for(i = 0;i<tail;i++) {
		serial->write(buf[i]);
	}
	serial->flush();
	tail = 0;
	return i;
}

char* Buffer::push(char c) {
	buf[tail++] = c;
	return buf;
}

Buffer::Buffer(HardwareSerial *sio) {
	serial = sio;
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

