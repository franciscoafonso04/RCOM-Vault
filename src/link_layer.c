// Link layer protocol implementation

#include <signal.h>
#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 256
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    int alarmEnabled = FALSE;
    int alarmCount = 0;

    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0 && alarmCount == 0)
    {
        return -1;
    }

    State state = START_S;

    unsigned char buf[BUF_SIZE] = {0};
    while (state != STOP_S) {

        if (alarmEnabled == FALSE && connectionParameters.role == LlTx)
        {
            alarm(connectionParameters.timeout);
            alarmEnabled = TRUE;
        }

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
            break;
        }
    }
    // TODO 

    return 1;
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
