#ifndef SERIO_H
#define SERIO_H

//#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include <fcntl.h>
//#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

//#include <SerialStream.h>
//using namespace LibSerial;

class SerIo
{
public:
	enum StatusCode {
		StatusOkay = 0,
		SerialTimeout = 1,
		SerialReadError = 2,
	};

	bool IsInitialized;
	std::vector<uint8_t> ReceiveBuffer;

	SerIo(char *devicePath);
	~SerIo();

	int Read(std::vector<uint8_t> &ReadBuffer);
	int Write(uint8_t *write_buffer, uint8_t bytes_to_write);
private:
	int SerialHandler;

	void Init(char *devicePath);
	int SetAttributes(int SerialHandler, int baud);
	int SetLowLatency(int SerialHandler);
};

extern SerIo* g_pSerIo;

#endif