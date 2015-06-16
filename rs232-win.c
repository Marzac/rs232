/*
    Cross-platform serial / RS232 library
    Version 0.1, 16/06/2015
    -> WIN32 implementation
    -> rs232-win.c
    The MIT License (MIT)

    Copyright (c) 2007 - 2015 Frédéric Meslin
    Contact: fredericmeslin@hotmail.com, @marzacdev

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include "rs232.h"

#include <stdio.h>
#include <string.h>

/*****************************************************************************/
typedef int             bool;
#define true            -1
#define false           0

typedef enum {
	COM_UNKNOWN = 0,
	COM_ENUM,
	COM_CLOSED,
	COM_OPENED,
	COM_ERROR
} COM_STATUS;

typedef struct {
	int port;
	void * handle;
	COM_STATUS status;
} COMDevice;

/*****************************************************************************/
#define COM_MAXDEVICES		32
COMDevice comDevices[COM_MAXDEVICES];
int noDevices;

#define COM_MINDEVNAME	    16384
const char * comPtn = "COM???";

/*****************************************************************************/
const char * findPattern(const char * string, const char * pattern, int * value);
const char * portInternalName(int index);

/*****************************************************************************/
typedef struct _COMMTIMEOUTS {
	uint32_t ReadIntervalTimeout;
	uint32_t ReadTotalTimeoutMultiplier;
	uint32_t ReadTotalTimeoutConstant;
	uint32_t WriteTotalTimeoutMultiplier;
	uint32_t WriteTotalTimeoutConstant;
} COMMTIMEOUTS;

typedef struct _DCB {
	uint32_t DCBlength;
	uint32_t BaudRate;
	uint32_t fBinary  :1;
	uint32_t fParity  :1;
	uint32_t fOutxCtsFlow  :1;
	uint32_t fOutxDsrFlow  :1;
	uint32_t fDtrControl  :2;
	uint32_t fDsrSensitivity  :1;
	uint32_t fTXContinueOnXoff  :1;
	uint32_t fOutX  :1;
	uint32_t fInX  :1;
	uint32_t fErrorChar  :1;
	uint32_t fNull  :1;
	uint32_t fRtsControl  :2;
	uint32_t fAbortOnError  :1;
	uint32_t fDummy2  :17;
	uint16_t wReserved;
	uint16_t XonLim;
	uint16_t XoffLim;
	uint8_t  ByteSize;
	uint8_t  Parity;
	uint8_t  StopBits;
	int8_t  XonChar;
	int8_t  XoffChar;
	int8_t  ErrorChar;
	int8_t  EofChar;
	int8_t  EvtChar;
	uint16_t wReserved1;
} DCB;

/*****************************************************************************/
/** Windows system constants */
#define ERROR_INSUFFICIENT_BUFFER	122
#define INVALID_HANDLE_VALUE		((void *) -1)
#define GENERIC_READ				0x80000000
#define GENERIC_WRITE				0x40000000
#define OPEN_EXISTING				3
#define MAX_DWORD					0xFFFFFFFF

/*****************************************************************************/
/** Windows system functions */
void * __stdcall CreateFileA(const char * lpFileName, uint32_t dwDesiredAccess, uint32_t dwShareMode, void * lpSecurityAttributes, uint32_t dwCreationDisposition, uint32_t dwFlagsAndAttributes, void * hTemplateFile);
bool __stdcall WriteFile(void * hFile, const void * lpBuffer, uint32_t nNumberOfBytesToWrite, uint32_t * lpNumberOfBytesWritten, void * lpOverlapped);
bool __stdcall ReadFile(void * hFile, void * lpBuffer, uint32_t nNumberOfBytesToRead, uint32_t * lpNumberOfBytesRead, void * lpOverlapped);
bool __stdcall CloseHandle(void * hFile);

uint32_t  __stdcall GetLastError(void);
void __stdcall SetLastError(uint32_t dwErrCode);

uint32_t  __stdcall QueryDosDeviceA(const char * lpDeviceName, char * lpTargetPath, uint32_t ucchMax);

bool __stdcall GetCommState(void * hFile, DCB * lpDCB);
bool __stdcall GetCommTimeouts(void * hFile, COMMTIMEOUTS * lpCommTimeouts);
bool __stdcall SetCommState(void * hFile, DCB * lpDCB);
bool __stdcall SetCommTimeouts(void * hFile, COMMTIMEOUTS * lpCommTimeouts);
bool __stdcall SetupComm(void * hFile, uint32_t dwInQueue, uint32_t dwOutQueue);


/*****************************************************************************/
int comEnumerate()
{
// Get devices information text
	size_t size = COM_MINDEVNAME;
	char * list = (char *) malloc(size);
	SetLastError(0);
	QueryDosDeviceA(NULL, list, size);
	while (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		char * nlist = realloc(list, size *= 2);
		if (!nlist) {
			free(list);
			return 0;
		}
		list = nlist;
		SetLastError(0);
		QueryDosDeviceA(NULL, list, size);
	}
// Gather all COM ports
	int port;
	const char * nlist = findPattern(list, comPtn, &port);
	noDevices = 0;
	while(port > 0 && noDevices < COM_MAXDEVICES) {
		COMDevice * com = &comDevices[noDevices ++];
		com->port = port;
		com->status = COM_ENUM;
		nlist = findPattern(nlist, comPtn, &port);
	}
	free(list);
	return noDevices;
}

int comGetNoPorts()
{
	return noDevices;
}

/*****************************************************************************/
#define COM_MAXNAME		16
char comName[COM_MAXNAME];
const char * comGetPortName(int index)
{
	sprintf(comName, "COM%i", comDevices[index].port);
	return comName;
}

int comFindPort(const char * name)
{
	int p;
	for (p = 0; p < noDevices; p++)
		if (strcmp(name, comGetPortName(p)) == 0)
			return p;
	return -1;
}

const char * comGetInternalName(int index)
{
	sprintf(comName, "//./COM%i", comDevices[index].port);
	return comName;
}

/*****************************************************************************/
int comOpen(int index, int baudrate)
{
    DCB config;
    COMMTIMEOUTS timeouts;
// Check the port status
	COMDevice * com = &comDevices[index];
	if (com->status != COM_ENUM) return 0;
// Open COM port
    void * handle = CreateFileA(comGetInternalName(index), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) return 0;
	com->status = COM_OPENED;
	com->handle = handle;
// Prepare read / write timeouts
	SetupComm(handle, 64, 64);
	timeouts.ReadIntervalTimeout = MAX_DWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(handle, &timeouts);
// Prepare serial communication format
	GetCommState(handle, &config);
    config.BaudRate = baudrate;
    config.fBinary = true;
	config.fParity = 0;
	config.fErrorChar = 0;
    config.fNull = 0;
	config.fAbortOnError = 0;
	config.ByteSize = 8;
    config.Parity = 0;
	config.StopBits = 0;
	config.EvtChar = '\n';
// Set the port state
    if (SetCommState(handle, &config) == 0) {
        CloseHandle(handle);
        return 0;
    }
	return 1;
}

void comClose(int index)
{
// Check the port status
	COMDevice * com = &comDevices[index];
	if (com->status != COM_OPENED) return;
	CloseHandle(com->handle);
	com->status = COM_CLOSED;
}

/*****************************************************************************/
int comWrite(int index, const char * buffer, size_t len)
{
	COMDevice * com = &comDevices[index];
    uint32_t bytes = 0;
    WriteFile(com->handle, buffer, len, &bytes, NULL);
    return bytes;
}

int comRead(int index, char * buffer, size_t len)
{
	COMDevice * com = &comDevices[index];
    uint32_t bytes = 0;
    ReadFile(com->handle, buffer, len, &bytes, NULL);
    return bytes;
}

/*****************************************************************************/
const char * findPattern(const char * string, const char * pattern, int * value)
{
	char c, n = 0;
	const char * sp = string;
	const char * pp = pattern;
// Check for the string pattern
	while (1) {
		c = *sp ++;
		if (c == '\0') {
			if (*pp == '?') break;
			if (*sp == '\0') break;
			n = 0;
			pp = pattern;
		}else{
			if (*pp == '?') {
			// Expect a digit
				if (c >= '0' && c <= '9') {
					n = n * 10 + (c - '0');
					if (*pp ++ == '\0') break;
				}else{
					n = 0;
					pp = comPtn;
				}
			}else{
			// Expect a character
				if (c == *pp) {
					if (*pp ++ == '\0') break;
				}else{
					n = 0;
					pp = comPtn;
				}
			}
		}
	}
// Return the value
	* value = n;
	return sp;
}
