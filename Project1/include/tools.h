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

#define FLAG 0x7E // frame boundary flag byte

#define A_RX 0x01 // address byte for receiver
#define A_TX 0x03 // address byte for transmitter

#define C_SET 0x03 // SET control byte for initiating connection
#define C_UA 0x07 // UA control byte for acknowledgment
#define C_RR0 0xAA // RR0 control byte for acknowledgment (frame 0)
#define C_RR1 0xAB // RR1 control byte for acknowledgment (frame 1)
#define C_REJ0 0x54 // REJ0 control byte for rejection (frame 0)
#define C_REJ1 0x55 // REJ1 control byte for rejection (frame 1)
#define C_DISC 0x0B // DISC control byte for disconnect request

#define C_I0 0x00 // information control byte for frame 0
#define C_I1 0x80 // information control byte for frame 1

#define ESC 0x7D // escape byte for special characters in frame data
#define FLAG_SEQ 0x5E // replacement sequence for FLAG within data
#define ESC_SEQ 0x5D // replacement sequence for ESC within data

#define T_SIZE 0x00 // file size type for control packet
#define T_NAME 0x01 // file name type for control packet

#define P_START 0x01 // packet type for start of transmission
#define P_DATA 0x02 // packet type for data transmission
#define P_END 0x03 // packet type for end of transmission

typedef enum {
    START_S = 0, // initial state for state machine
    FLAG_RCV_S, // state after receiving FLAG byte
    A_RCV_S, // state after receiving address byte
    C_RCV_S, // state after receiving control byte
    BCC_OK_S, // state after receiving BCC byte without error
    STOP_S // final state indicating successful end of reception
} State;

extern int alarmEnabled; // alarm status flag
extern int alarmCount; // counter for active alarms
extern int iFrame; // current frame index for transmissions
extern long fileSize; // total size of the file in bytes
extern double delta; // elapsed time since start of connection session

void arrayInsert(unsigned char arr[], int *n, int value, int pos);

void alarmHandler(int signal);

int writeResponse(int rr, int iFrame);

unsigned char* writeControl(long fileSize, const char* fileName, int* packetSize, int type);

unsigned char* writeData(unsigned char* data, int dataSize, int seqNum);

long power(int x, int y);

#endif // _TOOLS_H_
