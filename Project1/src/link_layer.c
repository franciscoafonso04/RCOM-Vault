// Link layer protocol implementation

#include "states.h"

extern int alarmEnabled, alarmCount;
int iFrame = 0;
int timeout, nTries;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 256
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    alarmEnabled = FALSE;
    alarmCount = 0;
    timeout = connectionParameters.timeout;
    nTries = connectionParameters.nRetransmissions;
    (void)signal(SIGALRM, alarmHandler);
    unsigned char buf[6] = {0};

    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0) // && alarmCount == 0)
    {
        return -1;
    }

    State state = START_S;

    while (state != STOP_S) {

        if (alarmEnabled == FALSE && connectionParameters.role == LlTx)
        {
            buf[0] = FLAG;
            buf[1] = A_TX;
            buf[2] = C_SET;
            buf[3] = buf[1] ^ buf[2];
            buf[4] = FLAG;

            int bytes = writeBytesSerialPort(*buf, 5);

            printf("%d bytes written\n", bytes);

            alarm(timeout);
            alarmEnabled = TRUE;
        }

        int byte = readByteSerialPort(*buf);
        if (!byte) continue;
        printf("receivedByte = 0x%02X\n", buf[0]);

        if(openStateMachine(state, *buf, connectionParameters) == 0) state = STOP_S;

        if(alarmCount >= nTries){
            perror("reached limit of retransmissions\n");
            return -1;
        }
    }

    if(connectionParameters.role == LlRx) {
        buf[0] = FLAG;
        buf[1] = A_RX;
        buf[2] = C_UA;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;
    
        int bytes = writeBytesSerialPort(*buf, BUF_SIZE);
        printf("%d bytes written\n", bytes);
    } 

    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    if (bufSize <= 0) return -1;
    
    int maxFrameSize = 2*bufSize + 10;
    int currentSize;
    int ans = 0;
    
    alarmEnabled = FALSE;
    alarmCount = 0;
    
    while (alarmCount < nTries) {
        currentSize = 0;
        if (alarmEnabled == FALSE || ans == -1) {
            unsigned char frame[maxFrameSize];

            frame[currentSize++] = FLAG;
            frame[currentSize++] = A_TX;

            if (iFrame == 0) frame[currentSize++] = C_I0; // Information frame number 0
            else frame[currentSize++] = C_I1;  // Information frame number 1
            
            frame[currentSize++] = frame[1] ^ frame[2];
            
            unsigned char BCC2 = 0;
            
            while (currentSize < bufSize + 4) {
                BCC2 ^= *buf;
                frame[currentSize++] = *buf;
                buf++;
            }

            frame[currentSize++] = BCC2;
            
            int s = 4;
            while (s < currentSize) {
                if (frame[s] == FLAG) {
                    frame[s] = ESC;
                    insert(frame, maxFrameSize, FLAG_SEQ, s + 1);
                    currentSize++;
                    s++; // Move past the inserted byte to avoid re-checking it.
                } else if (frame[s] == ESC) {
                    frame[s] = ESC;
                    insert(frame, maxFrameSize, ESC_SEQ, s + 1);
                    currentSize++;
                    s++; // Move past the inserted byte to avoid re-checking it.
                }
                s++;
            }

            frame[currentSize] = FLAG;
            
            alarm(timeout);
            alarmEnabled = TRUE;
        }

        ans = writeStateMachine(&iFrame);
    } 
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
