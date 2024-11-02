// Tools header file.

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "link_layer.h"
#include "serial_port.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <linux/time.h>


#define FLAG 0x7E

#define A_RX 0x01
#define A_TX 0x03

#define C_SET 0x03
#define C_UA 0x07
#define C_RR0 0xAA
#define C_RR1 0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55
#define C_DISC 0x0B

#define C_I0 0x00
#define C_I1 0x80

#define ESC 0x7D
#define FLAG_SEQ 0x5E
#define ESC_SEQ 0x5D

#define T_SIZE 0x00
#define T_NAME 0x01

#define P_START 0x01
#define P_DATA 0x02
#define P_END 0x03

typedef enum {
    START_S = 0,
    FLAG_RCV_S,
    A_RCV_S,
    C_RCV_S,
    BCC_OK_S,
    STOP_S
} State;

extern int alarmEnabled;
extern int alarmCount;
extern int iFrame;
extern long fileSize;
extern double delta;

void arrayInsert(unsigned char arr[], int *n, int value, int pos);

void alarmHandler(int signal);

int writeResponse(int rr, int iFrame);

unsigned char* writeControl(long fileSize, const char* fileName, int* packetSize, int type);

unsigned char* writeData(unsigned char* data, int dataSize, int seqNum);

long power(int x, int y);

#endif // _TOOLS_H_
