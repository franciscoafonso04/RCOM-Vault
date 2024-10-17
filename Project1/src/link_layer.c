// Link layer protocol implementation

#include "link_layer.h"
#include "tools.h"

int alarmEnabled, alarmCount;
int Ns, Nr;

void alarmHandler(int signal) {
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

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

            alarm(connectionParameters.timeout);
            alarmEnabled = TRUE;
        }

        int bytes = readByteSerialPort(*buf);
        if (!bytes) continue;
        printf("receivedByte = 0x%02X\n", buf[0]);

        switch (state) {
            case START_S:
                if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                }
                break;
            case FLAG_RCV_S:
                if ((buf[0] == A_TX && connectionParameters.role == LlRx) 
                    || (buf[0] == A_RX && connectionParameters.role == LlTx)) {
                    state = A_RCV_S;
                }
              
                else if (buf[0] == FLAG) break;
                else state = START_S;
                break;
            case A_RCV_S:
                if ((buf[0] == C_SET && connectionParameters.role == LlRx) 
                    || (buf[0] == C_UA && connectionParameters.role == LlTx)) {
                    state = C_RCV_S;
                    break;
                }
                else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                    break;
                }
                else state = START_S;
                break;
            case C_RCV_S:
                if ((buf[0] == (A_TX ^ C_SET) && connectionParameters.role == LlRx)
                    || (buf[0] == (A_RX ^ C_UA) && connectionParameters.role == LlTx)) {
                    state = BCC_OK_S;
                    break;
                }
                else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                    break;
                }
                else {
                    state = START_S;
                    break;
                }
            case BCC_OK_S:
                if (buf[0] == FLAG) {
                    state = STOP_S;
                    printf("sucesso!\n");
                    break;
                }
                else {
                    state = START_S; 
                    break;
                }
            default:
                break;
        }

        if(alarmCount >= connectionParameters.nRetransmissions){
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
    
    alarmEnabled = FALSE;
    alarmCount = 0;
    Ns = 0;

    int answer = 0;
    
    while (alarmCount != 3) {

        if (alarmEnabled == FALSE || answer == -1) {
            unsigned char frame[BUF_SIZE] = {0};

            frame[0] = FLAG;
            frame[1] = A_TX;

            if (Ns == 0) frame[2] = C_I0; // Information frame number 0
            else  frame[2] = C_I1;  // Information frame number 1
            
            frame[3] = frame[1] ^ frame[2];
            
            unsigned char BCC2 = 0;
            
            for (int i = 4; i < bufSize + 4; i++) {
                BCC2 ^= *buf;
                frame[i] = *buf;
                buf++;
            }

            frame[bufSize + 4] = BCC2;

            for (int i = 4; i < bufSize + 4; i++) {
                if(frame[i] == FLAG){
                    frame[i] = ESC;
                    insert(frame, FLAG_SEQ, i+1);
                }
                else if (frame[i] == ESC){
                    insert(frame, ESC_SEQ, i+1);
                }
            }

            frame[bufSize + 5] = FLAG;
            
            
            alarm(4); // ver variavel
            alarmEnabled = TRUE;
        }

    } // depois vê-se a var

    
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
