#ifndef __UGW_BUFFER_H__
#define __UGW_BUFFER_H__

class Buffer {
  protected:
  char buf[512];
  int tail;
	HardwareSerial *serial;

  public:
	Buffer(HardwareSerial *sio);
  void trans(HardwareSerial *to, unsigned long timeout);
  void read();
  void clear_rx();
  int get_tail();
  char get_buf(int idx);
	int flush();
	char* push(char c);
	void flush_rx(unsigned long timeout);
};

#endif

