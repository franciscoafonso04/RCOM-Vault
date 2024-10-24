// Link layer protocol implementation

#include <signal.h>
#include "serial_port.h"
#include "states.h"

extern int alarmEnabled, alarmCount;
extern int iFrame;
int timeout, nTries;
LinkLayerRole role;

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
    role = connectionParameters.role;

    (void)signal(SIGALRM, alarmHandler);
    unsigned char buf[6] = {0};

    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0) // && alarmCount == 0)
        return -1;

    State state = START_S;

    while (state != STOP_S) {

        if (alarmEnabled == FALSE && role == LlTx)
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

        if(openStateMachine(state, *buf, role) == 0) state = STOP_S;

        if(alarmCount >= nTries){
            perror("reached limit of retransmissions\n");
            return -1;
        }
    }

    if(role == LlRx) {
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

        ans = writeStateMachine();
    } 

    return 0; // return written characters?
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int size = readStateMachine(*packet);

    if (size == 0){
        printf("Didn't read anything, hoping to read now.\n");

        writeResponse(TRUE, iFrame);

        return -1;
    }

    size -= 1; //don't want to read BCC2
    unsigned char BCC2 = 0;
    for (int i = 0; i < size; i++)
        BCC2 ^= packet[i];
    
    if (BCC2 == packet[size]) {

        writeResponse(TRUE, iFrame);

        if (iFrame == 0)
            iFrame = 1;
        else if (iFrame == 1)
            iFrame = 0;

        return size;
    }
    else {
        printf("Rejected\n");

        writeResponse(FALSE, iFrame);

        return -1;
    }
    
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////

int llclose(int showStatistics)
{
    unsigned char buf[6] = {0};

    if (role == LlTx) {
        alarmEnabled = FALSE;
        alarmCount = 0;

        while (alarmCount < nTries) {
            if (alarmEnabled == FALSE)
            {
                buf[0] = FLAG;
                buf[1] = A_TX;
                buf[2] = C_DISC;
                buf[3] = buf[1] ^ buf[2];
                buf[4] = FLAG;

                int bytes = writeBytesSerialPort(*buf, 5);

                printf("%d bytes written\n", bytes);

                alarmEnabled = TRUE;

                alarm(timeout);
            }

            // Wait for a DISC frame to send a UA frame
            if (!discStateMachine())
            {
                buf[0] = FLAG;
                buf[1] = A_TX;
                buf[2] = C_UA;
                buf[3] = buf[1] ^ buf[2];
                buf[4] = FLAG;

                int bytes = writeBytesSerialPort(*buf, 5);                
                break;
            }
        }

        if (alarmCount >= nTries)
        {
            printf("Timed exeded!\n");
            return 1;
        }
    }

    else {

        while(discStateMachine());
        do {
            buf[0] = FLAG;
            buf[1] = A_TX;
            buf[2] = C_DISC;
            buf[3] = buf[1] ^ buf[2];
            buf[4] = FLAG;

            int bytes = writeBytesSerialPort(*buf, 5);

            printf("%d bytes written\n", bytes);
        } while (uaStateMachine());
    }
    
    int clstat = closeSerialPort();

    if (showStatistics)
    {
        printf("\n\n\nStatistics:\n");

        if (role == LlTx)
            printf("User: Transmiter \n");
        else
            printf("User: Receiver \n");

        printf("File size: %ld\n");

        if (role == LlTx)
        {
            printf("Frames sent: %d\n");
            printf("Total number of alarms: %d\n");
        }
        else
        {
            printf("Frames read: %d\n");
            printf("Number of rejection/ repeted information %d\n");
        }

        printf("Time spent: %g seconds\n");
        printf("Total time: %g seconds\n");
    }
    
    return clstat;
}
