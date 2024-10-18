// Tools header file.

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <stdio.h>

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

typedef enum
{
    START_S = 0,
    FLAG_RCV_S,
    A_RCV_S,
    C_RCV_S,
    BCC_OK_S,
    STOP_S

} State;

int alarmEnabled, alarmCount;
int iFrame = 0;

void insert(int arr[], int *n, int value, int pos);
void remove(int arr[], int *n, int pos);

void alarmHandler(int signal);

int writeResponse(int rr, int iFrame);

#endif // _TOOLS_H_
