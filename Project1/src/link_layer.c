// Link layer protocol implementation

#include <signal.h>
#include "link_layer.h"
#include "serial_port.h"

int alarmEnabled = FALSE;
int alarmCount = 0;

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
    (void)signal(SIGALRM, alarmHandler);
    unsigned char buf[BUF_SIZE] = {0};

    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0) // && alarmCount == 0)
    {
        return -1;
    }

    State state = START_S;

    while (state != STOP_S) {

        if (alarmEnabled == FALSE && connectionParameters.role == LlTx)
        {
            buf[0] = 0x7E;
            buf[1] = 0x03;
            buf[2] = 0x03;
            buf[3] = buf[1] ^ buf[2];
            buf[4] = 0x7E;

            int bytes = write(fd, buf, 5);
            printf("%d bytes written\n", bytes);

            alarm(connectionParameters.timeout);
            alarmEnabled = TRUE;
        }

        int bytes = read(fd, buf, 1);
        if (!bytes) continue;
        printf("receivedByte = 0x%02X\n", buf[0]);

        switch (state) {
            case START_S:
                if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                }
                break;
            case FLAG_RCV_S:
                if ((buf[0] == ADDRESS_TX && connectionParameters.role == LlRx) 
                    || (buf[0] == ADDRESS_RX && connectionParameters.role == LlTx)) {
                    state = A_RCV_S;
                }
              
                else if (buf[0] == FLAG) break;
                else state = START_S;
                break;
            case A_RCV_S:
                if ((buf[0] == CONTROL_SET && connectionParameters.role == LlRx) 
                    || (buf[0] == CONTROL_UA && connectionParameters.role == LlTx)) {
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
                if ((buf[0] == (ADDRESS_TX ^ CONTROL_SET) && connectionParameters.role == LlRx)
                    || (buf[0] == (ADDRESS_RX ^ CONTROL_UA) && connectionParameters.role == LlTx)) {
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
        buf[0] = 0x7E;
        buf[1] = 0x01;
        buf[2] = 0x07;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = 0x7E;

        int bytes = write(fd, buf, BUF_SIZE);
        printf("%d bytes written\n", bytes);
    } 

    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
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
