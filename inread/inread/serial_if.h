#ifndef SERIAL_IF_H
#define SERIAL_IF_H

#include <stdint.h>

enum {
	SERIAL_ERR_NO_ERROR,
	SERIAL_ERR_NOT_READY,
	SERIAL_ERR_INVALID_PARAM,
	SERIAL_ERR_WRITE_ERROR,
	SERIAL_ERR_READ_ERROR,
	SERIAL_ERR_DEVICE_ERROR,
	SERIAL_ERR_WRITE_TMO,
	SERIAL_ERR_READ_TMO,
	SERIAL_ERR_OUT_OF_SYNC,
};

#if defined(__cplusplus)
  extern "C" {                // Make sure we have C-declarations in C++ programs
#endif

void serial_break(int on);
int serial_write(uint8_t *buffer, uint32_t buffer_len);
int serial_read(uint8_t *buffer, uint32_t buffer_len, int tmo);
int serial_flush(void);
int serial_open(const char* portname, DWORD baud);
void serial_close(void);


#if defined(__cplusplus)
}     /* Make sure we have C-declarations in C++ programs */
#endif

#endif
