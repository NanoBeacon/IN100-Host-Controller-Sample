#include "windows.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "serial_if.h"

#define RX_BUFFER_SIZE (1024)

static int already_initialize = 0;
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static HANDLE hComm = NULL;
static OVERLAPPED writeOlap;
static OVERLAPPED readOlap;
static int rx_in_ptr;
static int rx_out_ptr;
static CRITICAL_SECTION cs;
static int write_tmo = INFINITE;
static int read_tmo = INFINITE;
static int rx_thread_exit = 0;

/******************************************************************************
**
**	Debug Function
**
*******************************************************************************/
static void debug_print(const char *fmt, ...)
{
	char buf[128];
	va_list args;
	int len;

	va_start(args, fmt);
	len = vsprintf_s(buf, sizeof(buf), fmt, args);
	va_end(args);
	OutputDebugString(buf);	

	return;
}

DWORD WINAPI uart_rx_thread(LPVOID pv)
{
	while (1) {
		uint8_t c;
		DWORD dwRet, bytesRead;

		dwRet = ReadFile(hComm, &c, 1, &bytesRead, &readOlap);
		if (!dwRet) {
			DWORD error;
			error = GetLastError();
			if (error == ERROR_IO_PENDING) {
				WaitForSingleObject(readOlap.hEvent, INFINITE);
			} 
		} 

		if (rx_thread_exit) {
			break;
		}

		BOOL result;
		result = GetOverlappedResult(hComm, &readOlap, &bytesRead, FALSE);
		if (result) {
			if (bytesRead == 1) {
				debug_print("[%02x]\r\n", c);
				rx_buffer[rx_in_ptr] = c;
				rx_in_ptr += 1;
				if (rx_in_ptr >= RX_BUFFER_SIZE)
					rx_in_ptr = 0;
			}
		}
	}
	
	return 1;
}

static void uart_flush()
{
	rx_out_ptr = rx_in_ptr;
}

static int uart_write(uint8_t *buffer, uint16_t buffer_len)
{
	DWORD error;
	DWORD bytesWritten;
	int ret = SERIAL_ERR_NO_ERROR;

	if (!WriteFile(hComm, buffer, buffer_len, &bytesWritten, &writeOlap)) {
		error = GetLastError();
		if (error == ERROR_IO_PENDING) {
			DWORD dwRet;
			dwRet = WaitForSingleObject(writeOlap.hEvent, write_tmo);
			if (dwRet == WAIT_TIMEOUT) {
				debug_print("Failed TX wait time out...\r\n"); 
				ret = SERIAL_ERR_WRITE_TMO;
				goto out;
			}
		} else {
			ret = SERIAL_ERR_DEVICE_ERROR;
			goto out;
		}
	}

	BOOL result;
	result = GetOverlappedResult(hComm, &writeOlap, &bytesWritten, FALSE);
	if (!result) {
		debug_print("Failed Overlapped result...\r\n"); 
		ret = SERIAL_ERR_DEVICE_ERROR;
		goto out;
	} else {
		if (bytesWritten == 0) { 
			debug_print("Failed TX write 0 byte...\r\n"); 
			ret = SERIAL_ERR_WRITE_ERROR;
		} else if (bytesWritten != buffer_len) {
			debug_print("Failed TX write %d bytes, but request %d bytes...\r\n", bytesWritten, buffer_len); 
			ret = SERIAL_ERR_WRITE_ERROR;
		}
	}

out:

	return ret;
}

static int uart_read(uint8_t *buffer, uint16_t buffer_len, int read_tmo)
{
	int count = 0;
	int ret = SERIAL_ERR_NO_ERROR;

	while (1) {

		DWORD availBytes;

		if (rx_in_ptr >= rx_out_ptr) {
			availBytes = rx_in_ptr - rx_out_ptr;
		} else {
			availBytes = RX_BUFFER_SIZE - rx_out_ptr;
			availBytes += rx_in_ptr;
		}

		if (availBytes >= buffer_len) {
			if (rx_out_ptr+ buffer_len <= RX_BUFFER_SIZE)
				memcpy(buffer, &rx_buffer[rx_out_ptr], buffer_len);
			else {
				int first_part_sz = RX_BUFFER_SIZE - rx_out_ptr;
				memcpy(buffer, &rx_buffer[rx_out_ptr], first_part_sz);
				memcpy(buffer + first_part_sz, &rx_buffer[0], buffer_len - first_part_sz);
			}
			if (rx_out_ptr + buffer_len >= RX_BUFFER_SIZE) {
				rx_out_ptr = (rx_out_ptr + buffer_len) - RX_BUFFER_SIZE;
			} else {
				rx_out_ptr += buffer_len;
			}
			break;
		} else {
			Sleep(1);
			count += 1;
			if (count >= read_tmo) {
				ret = SERIAL_ERR_READ_TMO;
				break;
			}		
		}
	}

	return ret;
}

/******************************************************************************
**
**	Exported Function
**
*******************************************************************************/
void serial_break(int on)
{
		if (on) {
				SetCommBreak(hComm);
		} else {
				ClearCommBreak(hComm);
		}
}

int serial_write(uint8_t *buffer, uint32_t buffer_len)
{
	int ret = SERIAL_ERR_NO_ERROR;

	if (!already_initialize)
		return SERIAL_ERR_NOT_READY;

	if (!buffer)
		return SERIAL_ERR_INVALID_PARAM;

	if (!buffer_len)
		return SERIAL_ERR_INVALID_PARAM;

	EnterCriticalSection(&cs);
	ret = uart_write(buffer, buffer_len); 
	LeaveCriticalSection(&cs);

	return ret;
}

int serial_read(uint8_t *buffer, uint32_t buffer_len, int tmo)
{
	int ret = SERIAL_ERR_NO_ERROR;

	if (!already_initialize)
		return SERIAL_ERR_NOT_READY;

	if (!buffer)
		return SERIAL_ERR_INVALID_PARAM;

	if (!buffer_len)
		return SERIAL_ERR_INVALID_PARAM;

	EnterCriticalSection(&cs);
	ret = uart_read(buffer, buffer_len, tmo);
	LeaveCriticalSection(&cs);

	return ret;
}

int serial_flush(void)
{
	int ret = SERIAL_ERR_NO_ERROR;

	EnterCriticalSection(&cs);
	uart_flush();
	LeaveCriticalSection(&cs);

	return ret;
}

int serial_open(const char * portname, DWORD baud)
{
	DCB		Dcb ;
	COMMTIMEOUTS  CommTimeOuts = {2, 1, 1, 0, 0};
	int ret = SERIAL_ERR_NO_ERROR;
	HANDLE h;
	if (already_initialize)
		return ret;

	hComm = CreateFileA(portname,
										GENERIC_READ | GENERIC_WRITE,
										0,
										0,
										OPEN_EXISTING,
										FILE_FLAG_OVERLAPPED,0);

	if(hComm == INVALID_HANDLE_VALUE)
	{
		debug_print("Failed to Open Com Port...");
		return SERIAL_ERR_DEVICE_ERROR;
	}

	SetCommMask(hComm,0);
	SetupComm(hComm,4096,4096) ;
	PurgeComm(hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	//GetCommTimeouts(m_hComm,&CommTimeOuts);
	CommTimeOuts.ReadIntervalTimeout = 0/*MAXDWORD*/;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0 /*MAXDWORD*/;
	CommTimeOuts.ReadTotalTimeoutConstant = 0 /*20000*/;
	SetCommTimeouts(hComm, &CommTimeOuts);

  ZeroMemory(&Dcb, sizeof(Dcb));
	Dcb.DCBlength = sizeof(DCB);
	//GetCommState(m_hComm,&Dcb) ;
	Dcb.BaudRate	= baud /*CBR_115200*/;
	Dcb.ByteSize	= 8;
	Dcb.Parity		= NOPARITY;
	Dcb.StopBits	= ONESTOPBIT;
	Dcb.fOutxCtsFlow = FALSE;
	Dcb.fOutxDsrFlow = FALSE;
	Dcb.fDtrControl = DTR_CONTROL_DISABLE;
	Dcb.fRtsControl = RTS_CONTROL_DISABLE;
	Dcb.fInX		= FALSE;
	Dcb.fOutX		= FALSE;
	Dcb.fBinary		= TRUE ;
	SetCommState(hComm,&Dcb);

	/* Overlapped */
	writeOlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (writeOlap.hEvent == NULL) {
		debug_print("Failed to create OVERLAPPER...");
		ret = SERIAL_ERR_DEVICE_ERROR;
		goto out;
	}
	writeOlap.Offset = 0;
	writeOlap.OffsetHigh = 0;

	readOlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (readOlap.hEvent == NULL) {
		debug_print("Failed to create OVERLAPPER...");
		ret = SERIAL_ERR_DEVICE_ERROR;
		goto out;
	}
	readOlap.Offset = 0;
	readOlap.OffsetHigh = 0;
	rx_in_ptr = 0;
	rx_out_ptr = 0;

	h = CreateThread(NULL, 0, uart_rx_thread, NULL, 0, NULL);
	if (h == NULL) {
		debug_print("Failed to create RX thread...");
		ret = SERIAL_ERR_DEVICE_ERROR;
		goto out;
	}

	InitializeCriticalSection(&cs);

	already_initialize = 1;

  return SERIAL_ERR_NO_ERROR;

out:

	if (hComm) { 
		CloseHandle(hComm);
		hComm = NULL;
	}

	if (writeOlap.hEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(writeOlap.hEvent);
		writeOlap.hEvent = INVALID_HANDLE_VALUE;
	}	

	if (readOlap.hEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(readOlap.hEvent);
		readOlap.hEvent = INVALID_HANDLE_VALUE;
	}	

  return SERIAL_ERR_DEVICE_ERROR;
}

void serial_close(void)
{
	if(already_initialize)  {

		EnterCriticalSection(&cs);

		rx_thread_exit = 1;
		SetEvent(readOlap.hEvent);
		Sleep(100);

		if (hComm != INVALID_HANDLE_VALUE) {
			CloseHandle(hComm);
			hComm = INVALID_HANDLE_VALUE;
		}

		if (writeOlap.hEvent != INVALID_HANDLE_VALUE) {
			CloseHandle(writeOlap.hEvent);
			writeOlap.hEvent = INVALID_HANDLE_VALUE;
		}	

		if (readOlap.hEvent != INVALID_HANDLE_VALUE) {
			CloseHandle(readOlap.hEvent);
			readOlap.hEvent = INVALID_HANDLE_VALUE;
		}	

		LeaveCriticalSection(&cs);

		already_initialize = 0;
		rx_thread_exit = 0;

		rx_in_ptr = 0; 
		rx_out_ptr = 0;
		write_tmo = INFINITE;
		read_tmo = INFINITE;

		DeleteCriticalSection(&cs);
	}

	return;
}
