#include "states.h"

extern int iFrame;

int openStateMachine(State state, unsigned char *buf, LinkLayerRole role){
    switch (state) {
        case START_S:
            if (buf[0] == FLAG) {
                state = FLAG_RCV_S;
            }
            break;
        case FLAG_RCV_S:
            if ((buf[0] == A_TX && role == LlRx) 
                || (buf[0] == A_RX && role == LlTx)) {
                state = A_RCV_S;
            }
            
            else if (buf[0] == FLAG) break;
            else state = START_S;
            break;
        case A_RCV_S:
            if ((buf[0] == C_SET && role == LlRx) 
                || (buf[0] == C_UA && role == LlTx)) {
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
            if ((buf[0] == (A_TX ^ C_SET) && role == LlRx)
                || (buf[0] == (A_RX ^ C_UA) && role == LlTx)) {
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

int writeStateMachine(){
    State state = START_S;
    unsigned char buf[6] = {0};
    int ans = 0;

    while (state != STOP_S && alarmEnabled) {
        readByteSerialPort(buf);

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
                if (buf[0] == FLAG) {
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

unsigned char readStateMachine(unsigned char *packet){
    State state = START_S;

    // buf = information being read, that will be destuffed and placed in packet  
    unsigned char buf[256] = {0}; // NS QUE TAMANHO POR (eu sei :) muito engraçado mas tens um ( e dois ) dava erro de compilação
    int ans = 0;
    int deStuff = FALSE;
    int size = 0;

    while (state != STOP_S) {
        if(readByteSerialPort(buf) == -1){
            perror("error reading the byte");
            return -1;
        }
        if (deStuff) {
            if (buf[0] == FLAG_SEQ)
                packet[size] = FLAG;
            else if (buf[0] == ESC_SEQ)
                packet[size] = ESC;
            else
                continue;
            size++;
            deStuff = FALSE;
        }
        else{
            switch (state) {
                case START_S:
                    if (buf[0] == FLAG)
                        state = FLAG_RCV_S;
                    break;
                case FLAG_RCV_S:
                    if (buf[0] == A_TX)
                        state = A_RCV_S;
                    else if (buf[0] == FLAG)
                        state = state;
                    else
                        state = START_S;
                    break;
                case A_RCV_S:
                    if (buf[0] == C_I0 || buf[0] == C_I1) {
                        state = C_RCV_S;
                        ans = buf[0];
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
                    if (buf[0] == FLAG) 
                        state = STOP_S;             
                    else if (buf[0] == ESC)
                        deStuff = TRUE;
                    else {
                        packet[size] = buf[0];
                        size++;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    if (iFrame != ans)
        return -1;

    return size;
}

unsigned char discStateMachine() {

    alarmEnabled = TRUE;
    State state = START_S;
    unsigned char buf[6] = {0};
    while (state != STOP_S)
    {
        // Returns after 5 chars have been input
        readByteSerialPort(buf);
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
                if (buf[0] == C_DISC)
                    state = C_RCV_S;
                else if (buf[0] == FLAG)
                    state = FLAG_RCV_S;
                else
                    state = START_S;
                break;
            case C_RCV_S:
                if (buf[0] == (A_TX ^ C_DISC))
                    state = BCC_OK_S;
                else if (buf[0] == FLAG)
                    state = FLAG_RCV_S;
                else
                    state = START_S;
                break;
            case BCC_OK_S:
                if (buf[0] == FLAG){
                    alarm(0);
                    return 0;
                }
                else
                    state = START_S;
                break;
            default:
                break;
        }
    }
    return 1;
}

unsigned char uaStateMachine() {

    State state = START_S;
    unsigned char buf[6] = {0};
    while (state != STOP_S)
    {
        // Returns after 5 chars have been input
        readByteSerialPort(buf);
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
                if (buf[0] == (C_UA))
                    state = C_RCV_S;
                else if (buf[0] == FLAG)
                    state = FLAG_RCV_S;
                else
                    state = START_S;
                break;
            case C_RCV_S:
                if (buf[0] == (A_TX ^ C_DISC))
                    state = BCC_OK_S;
                else if (buf[0] == FLAG)
                    state = FLAG_RCV_S;
                else
                    state = START_S;
                break;
            case BCC_OK_S:
                if (buf[0] == FLAG)
                    return 0;
                else
                    state = START_S;
                break;
            default:
                break;
        }
    }
    return 1;
}
