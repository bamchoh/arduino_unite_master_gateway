#ifndef __UGW_BUFFER_H__
#define __UGW_BUFFER_H__

class Buffer {
  protected:
  int tail;
  char buf[512];
  int tx_tail;
  char tx_buf[512];
	HardwareSerial *serial;

  public:
	Buffer(HardwareSerial *sio);
  void trans(HardwareSerial *to, unsigned long timeout);
  void readAll();
  void clear_rx();
  void clear_tx();
  int get_tail();
  int available();
  char get_buf(int idx);
	int send();
	char* push_tx(char c);
	void flush_rx(unsigned long timeout);
};

#endif
