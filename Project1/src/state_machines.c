#include "state_machines.h"

extern int iFrame;
extern int alarmEnabled;

int openStateMachine(State *state, unsigned char *buf, LinkLayerRole role) {
    
    switch (*state) {
        
        case START_S:
            // Initial state, looking for the start FLAG byte
            if (buf[0] == FLAG) {
                *state = FLAG_RCV_S; // Transition to next state if FLAG is found
            }
            break;

        case FLAG_RCV_S:
            // Waiting for the correct address byte (A_TX or A_RX) based on role
            if ((buf[0] == A_TX && role == LlRx) 
                || (buf[0] == A_RX && role == LlTx)) {
                *state = A_RCV_S; // Move to address received state
            }
            else if (buf[0] == FLAG) {
                *state = FLAG_RCV_S;
            }
            else {
                *state = START_S; // Invalid byte, reset to START state
            }
            break;

        case A_RCV_S:
            // Waiting for the correct control field (C_SET or C_UA) based on role
            if ((buf[0] == C_SET && role == LlRx) 
                || (buf[0] == C_UA && role == LlTx)) {
                *state = C_RCV_S; // Move to control received state
            }
            else if (buf[0] == FLAG) {
                *state = FLAG_RCV_S; // FLAG received instead, resync frame
            }
            else {
                *state = START_S; // Invalid byte, reset to START state
            }
            break;

        case C_RCV_S:
            // Waiting for BCC1 (Block Check Character), which is a XOR of address and control fields
            if ((buf[0] == (A_TX ^ C_SET) && role == LlRx) 
                || (buf[0] == (A_RX ^ C_UA) && role == LlTx)) {
                *state = BCC_OK_S; // BCC is correct, move to BCC_OK state
            }
            else if (buf[0] == FLAG) {
                *state = FLAG_RCV_S; // FLAG received, go back to re-sync
            }
            else {
                *state = START_S; // Invalid BCC, reset to START state
            }
            break;

        case BCC_OK_S:
            // Expecting final FLAG to complete the frame
            if (buf[0] == FLAG) {
                *state = STOP_S; // End of frame, transition to STOP state
                return 0; // Return 0 to indicate a successful frame reception
            }
            else {
                *state = START_S; // No FLAG received, reset to START state
            }
            break;

        default:
            printf("Unknown state encountered\n");
            break;
    }

    // If no successful transition to STOP state, return -1 by default
    return -1;
}


int writeStateMachine() {
    State state = START_S;
    unsigned char buf[1] = {0};  // Buffer to hold received bytes
    int res = 0;

    // Start of the main loop, runs until state is STOP_S or alarmEnabled is false
    while (state != STOP_S && alarmEnabled == TRUE) {

        // Attempt to read a byte from the serial port
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;  // No byte received, loop continues


        // Handle each state and transition
        switch (state) {
            case START_S:
                if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                }
                break;

            case FLAG_RCV_S:
                if (buf[0] == A_TX) {
                    state = A_RCV_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;

            case A_RCV_S:
                if (buf[0] == C_RR0 || buf[0] == C_RR1 || buf[0] == C_REJ0 || buf[0] == C_REJ1) {
                    res = buf[0];
                    state = C_RCV_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;

            case C_RCV_S:
                if (buf[0] == (A_TX ^ res)) {
                    state = BCC_OK_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;

            case BCC_OK_S:
                if (buf[0] == FLAG) {
                    state = STOP_S;
                    alarm(0);  // Disable the alarm

                    if (res == C_REJ0 || res == C_REJ1) {
                        alarmCount = 0;
                        printf("Received REJ, returning -2\n");
                        return -2;
                    }
                    if ((res == C_RR0 && iFrame == 0) || (res == C_RR1 && iFrame == 1)) {
                        printf("Received duplicate RR, returning -3\n");
                        return -3;
                    }
                    if (res == C_RR0) {
                        iFrame = 0;
                        printf("Received RR0, iFrame set to 0\n");
                    }
                    if (res == C_RR1) {
                        iFrame = 1;
                        printf("Received RR1, iFrame set to 1\n");
                    }
                    return 0;
                } else {
                    state = START_S;
                }
                break;

            default:
                printf("Unexpected state encountered. State: %d\n", state);
                break;
        }
    }

    // Loop ended without reaching STOP_S, returning -1 for timeout or alarmDisabled
    printf("writeStateMachine exited with state: %d, alarmEnabled: %d\n", state, alarmEnabled);
    return -1;
}


int readStateMachine(unsigned char *packet) {
    State state = START_S;

    // Buffer to hold incoming bytes temporarily
    unsigned char buf[1] = {0};  
    int res = -1;          // Holds the control field (C_I0 or C_I1) if found
    int deStuff = FALSE;  // Flag for handling byte-stuffing
    int size = 0;         // Tracks the size of the packet data


    while (state != STOP_S) {
        
        // Read a byte from the serial port
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;


        // If we are in byte-stuffing mode, handle the next byte accordingly
        if (deStuff) {
            if (buf[0] == FLAG_SEQ) {
                packet[size] = FLAG;
            } else if (buf[0] == ESC_SEQ) {
                packet[size] = ESC;
            } else {
                continue;
            }
            size++;
            deStuff = FALSE; // Reset deStuff mode
        } else {
            // Normal state handling without byte-stuffing
            switch (state) {
                case START_S:
                    if (buf[0] == FLAG) {
                        state = FLAG_RCV_S;
                    }
                    break;

                case FLAG_RCV_S:
                    if (buf[0] == A_TX) 
                        state = A_RCV_S;
                    else if (buf[0] == FLAG)
                        state = FLAG_RCV_S;
                    else 
                        state = START_S;                    
                    break;

                case A_RCV_S:
                    if (buf[0] == C_I0 || buf[0] == C_I1) {
                        state = C_RCV_S;
                        res = buf[0]; // Store the control field for later checks
                    } else if (buf[0] == FLAG)
                        state = FLAG_RCV_S;
                    else 
                        state = START_S;
                    
                    break;

                case C_RCV_S:
                    if (buf[0] == (A_TX ^ res)) {
                        state = BCC_OK_S;
                    } else if (buf[0] == FLAG) {
                        state = FLAG_RCV_S;
                    } else { 
                        state = START_S; 
                    }
                    break;

                case BCC_OK_S:
                    if (buf[0] == FLAG) {
                        state = STOP_S;  // End of frame detected
                    } else if (buf[0] == ESC) {
                        deStuff = TRUE; // Start byte-stuffing mode
                    } else {
                        packet[size] = buf[0]; // Add normal byte to packet
                        size++;
                    }
                    break;

                default:
                    printf("Unknown state encountered.\n");
                    break;
            }
        }
    }


    // Verify if iFrame matches the received control field (ans)
    if ((iFrame == 1 && res != C_I1) || (iFrame == 0 && res != C_I0)) {
        printf("iFrame: %d, res: 0x%02X\n", iFrame, res);
        return -1;
    }

    printf("Frame received successfully with size: %d\n", size);
    return size;
}


unsigned char discStateMachine() {
    alarmEnabled = TRUE;
    State state = START_S;
    unsigned char buf[5] = {0};

    while (state != STOP_S) {
        // Read a byte from the serial port
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;

        switch (state) {
            case START_S:
                if (buf[0] == FLAG) { 
                    state = FLAG_RCV_S;
                }
                break;
            case FLAG_RCV_S:
                if (buf[0] == A_TX) {
                    state = A_RCV_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;
            case A_RCV_S:
                if (buf[0] == C_DISC) {
                    state = C_RCV_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;
            case C_RCV_S:
                if (buf[0] == (A_TX ^ C_DISC)) {
                    state = BCC_OK_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;
            case BCC_OK_S:
                if (buf[0] == FLAG) {
                    alarm(0);
                    return 0;
                } else {
                    state = START_S;
                }
                break;
            default:
                printf("Unknown state encountered.\n");
                break;
        }
    }
    return 1;
}


unsigned char uaStateMachine() {
    State state = START_S;
    unsigned char buf[5] = {0};

    while (state != STOP_S) {
        // Returns after 5 chars have been input
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;

        switch (state) {
            case START_S:
                if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                }
                break;

            case FLAG_RCV_S:
                if (buf[0] == A_TX) {
                    state = A_RCV_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;

            case A_RCV_S:
                if (buf[0] == C_UA) {
                    state = C_RCV_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;

            case C_RCV_S:
                if (buf[0] == (A_TX ^ C_UA)) {
                    state = BCC_OK_S;
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                } else {
                    state = START_S;
                }
                break;

            case BCC_OK_S:
                if (buf[0] == FLAG) {
                    return 0;
                } else {
                    state = START_S;
                }
                break;

            default:
                printf("Unknown state encountered, exiting.\n");
                return 1;
        }
    }

    return 1;
}

