#include "states.h"

int openStateMachine(State state, unsigned char *buf, LinkLayer connectionParameters){
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
                return 0;
            }
            else {
                state = START_S; 
                break;
            }
        default:
            break;
    }

    return -1;
}

int writeStateMachine(int *iFrame){
    State state = START_S;
    unsigned char buf[6] = {0};
    int ans = 0;

    while (state != STOP_S && alarmEnabled) {
        readByteSerialPort(*buf);

        switch (state) {
            case START_S:
                if (buf[0] == FLAG)
                    state = FLAG_RCV_S;
                break;
            case FLAG_RCV_S:
                if (buf[0] == A_TX) 
                    state = A_RCV_S;
                else if (buf[0] == FLAG) 
                    break;
                else 
                    state = START_S;
                break;
            case A_RCV_S:
                if (buf[0] == C_RR0 || buf[0] == C_RR1 || buf[0] == C_REJ0 || buf[0] == C_REJ1) {
                    ans = buf[0];
                    state = C_RCV_S;
                }
                else if (buf[0] == FLAG) 
                    state = FLAG_RCV_S;
                else 
                    state = START_S;
                break;
            case C_RCV_S:
                if (buf[0] == (A_TX ^ ans)) 
                    state = BCC_OK_S;
                else if (buf[0] == FLAG) 
                    state = FLAG_RCV_S;
                else 
                    state = START_S;
                break;
            case BCC_OK_S:
                if (buf[0] == FLAG){
                    state = STOP_S;
                    alarm(0);
                    if (ans == C_REJ0 || ans == C_REJ1)
                        return -1;
                    if ((ans == C_RR0 && iFrame == 0) || (ans == C_RR1 && iFrame == 1)) 
                        return -1;
                    if (ans == C_RR0) 
                        iFrame = 0;
                    if (ans == C_RR1) 
                        iFrame = 1;
                    return 0;
                }
                else state = START_S;
                break;
            default:
                break;
        }
    }
    return -1;
}