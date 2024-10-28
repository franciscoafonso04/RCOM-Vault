#include "states.h"

extern int iFrame;
extern int alarmEnabled;

int openStateMachine(State *state, unsigned char *buf, LinkLayerRole role) {
    
    switch (*state) {
        
        case START_S:
            // Initial state, looking for the start FLAG byte
            //printf("State: START_S\n");
            if (buf[0] == FLAG) {
                *state = FLAG_RCV_S; // Transition to next state if FLAG is found
                printf("Transitioned to FLAG_RCV_S\n");
            }
            break;

        case FLAG_RCV_S:
            // Waiting for the correct address byte (A_TX or A_RX) based on role
            //printf("State: FLAG_RCV_S, Received Byte: 0x%02X\n", buf[0]);
            if ((buf[0] == A_TX && role == LlRx) 
                || (buf[0] == A_RX && role == LlTx)) {
                *state = A_RCV_S; // Move to address received state
                //printf("Transitioned to A_RCV_S\n");
            }
            else if (buf[0] == FLAG) {
                //printf("Received FLAG again, staying in FLAG_RCV_S\n");
            }
            else {
                *state = START_S; // Invalid byte, reset to START state
                //printf("Invalid byte in FLAG_RCV_S, resetting to START_S\n");
            }
            break;

        case A_RCV_S:
            // Waiting for the correct control field (C_SET or C_UA) based on role
            //printf("State: A_RCV_S, Received Byte: 0x%02X\n", buf[0]);
            if ((buf[0] == C_SET && role == LlRx) 
                || (buf[0] == C_UA && role == LlTx)) {
                *state = C_RCV_S; // Move to control received state
                //printf("Transitioned to C_RCV_S\n");
            }
            else if (buf[0] == FLAG) {
                *state = FLAG_RCV_S; // FLAG received instead, resync frame
                //printf("Received FLAG, transitioning back to FLAG_RCV_S\n");
            }
            else {
                *state = START_S; // Invalid byte, reset to START state
                //printf("Invalid byte in A_RCV_S, resetting to START_S\n");
            }
            break;

        case C_RCV_S:
            // Waiting for BCC1 (Block Check Character), which is a XOR of address and control fields
            //printf("State: C_RCV_S, Received Byte: 0x%02X\n", buf[0]);
            if ((buf[0] == (A_TX ^ C_SET) && role == LlRx) 
                || (buf[0] == (A_RX ^ C_UA) && role == LlTx)) {
                *state = BCC_OK_S; // BCC is correct, move to BCC_OK state
                //printf("Transitioned to BCC_OK_S\n");
            }
            else if (buf[0] == FLAG) {
                *state = FLAG_RCV_S; // FLAG received, go back to re-sync
                //printf("Received FLAG, transitioning back to FLAG_RCV_S\n");
            }
            else {
                *state = START_S; // Invalid BCC, reset to START state
                //printf("Invalid BCC in C_RCV_S, resetting to START_S\n");
            }
            break;

        case BCC_OK_S:
            // Expecting final FLAG to complete the frame
            //printf("State: BCC_OK_S, Received Byte: 0x%02X\n", buf[0]);
            if (buf[0] == FLAG) {
                *state = STOP_S; // End of frame, transition to STOP state
                //printf("Transitioned to STOP_S - Success!\n");
                return 0; // Return 0 to indicate a successful frame reception
            }
            else {
                *state = START_S; // No FLAG received, reset to START state
                //printf("Expected FLAG in BCC_OK_S, but got 0x%02X. Resetting to START_S\n", buf[0]);
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

        //printf("Received Byte: 0x%02X, Current State: %d\n", buf[0], state);

        // Handle each state and transition
        switch (state) {
            case START_S:
                printf("State: START_S\n");
                if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                    printf("Transitioned to FLAG_RCV_S\n");
                }
                break;

            case FLAG_RCV_S:
                printf("State: FLAG_RCV_S\n");
                if (buf[0] == A_TX) {
                    state = A_RCV_S;
                    printf("Transitioned to A_RCV_S\n");
                } else if (buf[0] == FLAG) {
                    printf("Received another FLAG in FLAG_RCV_S, remaining in this state\n");
                } else {
                    state = START_S;
                    printf("Invalid byte in FLAG_RCV_S, resetting to START_S\n");
                }
                break;

            case A_RCV_S:
                printf("State: A_RCV_S\n");
                if (buf[0] == C_RR0 || buf[0] == C_RR1 || buf[0] == C_REJ0 || buf[0] == C_REJ1) {
                    res = buf[0];
                    state = C_RCV_S;
                    printf("Transitioned to C_RCV_S, ans set to: 0x%02X\n", res);
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                    printf("Received FLAG in A_RCV_S, transitioning back to FLAG_RCV_S\n");
                } else {
                    state = START_S;
                    printf("Invalid byte in A_RCV_S, resetting to START_S\n");
                }
                break;

            case C_RCV_S:
                printf("State: C_RCV_S\n");
                if (buf[0] == (A_TX ^ res)) {
                    state = BCC_OK_S;
                    printf("Transitioned to BCC_OK_S\n");
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                    printf("Received FLAG in C_RCV_S, transitioning back to FLAG_RCV_S\n");
                } else {
                    state = START_S;
                    printf("Invalid BCC or unexpected byte in C_RCV_S, resetting to START_S\n");
                }
                break;

            case BCC_OK_S:
                printf("State: BCC_OK_S\n");
                if (buf[0] == FLAG) {
                    state = STOP_S;
                    alarm(0);  // Disable the alarm
                    printf("Received FLAG, transitioning to STOP_S - Successful frame\n");

                    if (res == C_REJ0 || res == C_REJ1) {
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
                    printf("Expected FLAG in BCC_OK_S, got 0x%02X. Resetting to START_S\n", buf[0]);
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
    int res = 0;          // Holds the control field (C_I0 or C_I1) if found
    int deStuff = FALSE;  // Flag for handling byte-stuffing
    int size = 0;         // Tracks the size of the packet data

    printf("Starting readStateMachine...\n");

    //printf("\nDATA BYTES BEFORE DESTUFFING: ");

    while (state != STOP_S) {
        
        // Read a byte from the serial port
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;


        // If we are in byte-stuffing mode, handle the next byte accordingly
        if (deStuff) {
            if (buf[0] == FLAG_SEQ) {
                packet[size] = FLAG;
                //printf("0x%02X ", buf[0]);
                //printf("Byte-stuffed FLAG added to packet\n");
            } else if (buf[0] == ESC_SEQ) {
                packet[size] = ESC;
                //printf("0x%02X ", buf[0]);
                //printf("Byte-stuffed ESC added to packet\n");
            } else {
                //printf("Unexpected byte after ESC: 0x%02X\n", buf[0]);
                continue;
            }
            size++;
            deStuff = FALSE; // Reset deStuff mode
        } else {
            // Normal state handling without byte-stuffing
            switch (state) {
                case START_S:
                    //printf("State: START_S\n");
                    if (buf[0] == FLAG) {
                        state = FLAG_RCV_S;
                        //printf("Transitioned to FLAG_RCV_S\n");
                    }
                    break;

                case FLAG_RCV_S:
                    //printf("State: FLAG_RCV_S\n");
                    if (buf[0] == A_TX) {
                        state = A_RCV_S;
                        //printf("Transitioned to A_RCV_S\n");
                    } else if (buf[0] == FLAG) {
                        //printf("Received another FLAG, staying in FLAG_RCV_S\n");
                    } else {
                        state = START_S;
                        //printf("Invalid byte in FLAG_RCV_S, resetting to START_S\n");
                    }
                    break;

                case A_RCV_S:
                    //printf("State: A_RCV_S\n");
                    if (buf[0] == C_I0 || buf[0] == C_I1) {
                        state = C_RCV_S;
                        res = buf[0]; // Store the control field for later checks
                        //printf("Transitioned to C_RCV_S, ans set to: 0x%02X\n", ans);
                    } else if (buf[0] == FLAG) {
                        state = FLAG_RCV_S;
                        //printf("Received FLAG, transitioning back to FLAG_RCV_S\n");
                    } else {
                        state = START_S;
                        //printf("Invalid byte in A_RCV_S, resetting to START_S\n");
                    }
                    break;

                case C_RCV_S:
                    //printf("State: C_RCV_S\n");
                    if (buf[0] == (A_TX ^ res)) {
                        state = BCC_OK_S;
                        //printf("Transitioned to BCC_OK_S\n");
                    } else if (buf[0] == FLAG) {
                        state = FLAG_RCV_S;
                        //printf("Received FLAG in C_RCV_S, transitioning back to FLAG_RCV_S\n");
                    } else {
                        state = START_S;
                        //printf("Invalid BCC or unexpected byte in C_RCV_S, resetting to START_S\n");
                    }
                    break;

                case BCC_OK_S:
                    //printf("State: BCC_OK_S\n");
                    if (buf[0] == FLAG) {
                        state = STOP_S;  // End of frame detected
                        //printf("Transitioned to STOP_S - Frame successfully received\n");
                    } else if (buf[0] == ESC) {
                        //printf("0x%02X ", buf[0]);
                        deStuff = TRUE; // Start byte-stuffing mode
                        //printf("ESC detected, entering byte-stuffing mode\n");
                    } else {
                        packet[size] = buf[0]; // Add normal byte to packet
                        //printf("0x%02X ", buf[0]);
                        size++;
                        //printf("Added byte 0x%02X to packet, current size: %d\n", buf[0], size);
                    }
                    break;

                default:
                    printf("Unknown state encountered.\n");
                    break;
            }
        }
    }

    // Verify if iFrame matches the received control field (ans)
    if ((iFrame == 1 && res != 0x80) || (iFrame == 0 && res != 0x00)) {
        return -1;
    }

    //printf("\n");

    printf("Frame received successfully with size: %d\n", size);
    return size;
}


unsigned char discStateMachine() {
    alarmEnabled = TRUE;
    State state = START_S;
    unsigned char buf[5] = {0};
    printf("Starting discStateMachine...\n");

    while (state != STOP_S) {
        // Read a byte from the serial port
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;
        //printf("Received byte: 0x%02X, Current State: %d\n", buf[0], state);

        switch (state) {
            case START_S:
                if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                    printf("Transitioned to FLAG_RCV_S\n");
                }
                break;
            case FLAG_RCV_S:
                if (buf[0] == A_TX) {
                    state = A_RCV_S;
                    printf("Transitioned to A_RCV_S\n");
                } else if (buf[0] == FLAG) {
                    printf("Repeated FLAG, remaining in FLAG_RCV_S\n");
                } else {
                    state = START_S;
                    printf("Invalid byte, resetting to START_S\n");
                }
                break;
            case A_RCV_S:
                if (buf[0] == C_DISC) {
                    state = C_RCV_S;
                    printf("Transitioned to C_RCV_S\n");
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                    printf("FLAG received, returning to FLAG_RCV_S\n");
                } else {
                    state = START_S;
                    printf("Invalid byte, resetting to START_S\n");
                }
                break;
            case C_RCV_S:
                if (buf[0] == (A_TX ^ C_DISC)) {
                    state = BCC_OK_S;
                    printf("Transitioned to BCC_OK_S\n");
                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                    printf("FLAG received, returning to FLAG_RCV_S\n");
                } else {
                    state = START_S;
                    printf("Invalid BCC, resetting to START_S\n");
                }
                break;
            case BCC_OK_S:
                if (buf[0] == FLAG) {
                    alarm(0);
                    printf("DISC frame successfully received, stopping state machine.\n");
                    return 0;
                } else {
                    state = START_S;
                    printf("Expected FLAG at end, resetting to START_S\n");
                }
                break;
            default:
                printf("Unknown state encountered.\n");
                break;
        }
    }
    return 1;
}


#include <stdio.h>

unsigned char uaStateMachine() {
    State state = START_S;
    unsigned char buf[5] = {0};

    printf("Starting UA State Machine\n");

    while (state != STOP_S) {
        // Returns after 5 chars have been input
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;
        printf("Received byte: 0x%02X, Current state: %d\n", buf[0], state);

        switch (state) {
            case START_S:
                if (buf[0] == FLAG) {
                    printf("FLAG received, moving to FLAG_RCV_S\n");
                    state = FLAG_RCV_S;
                }
                break;

            case FLAG_RCV_S:
                if (buf[0] == A_TX) {
                    printf("A_TX received, moving to A_RCV_S\n");
                    state = A_RCV_S;
                } else if (buf[0] == FLAG) {
                    printf("FLAG received again in FLAG_RCV_S, staying in FLAG_RCV_S\n");
                } else {
                    printf("Unexpected byte in FLAG_RCV_S, resetting to START_S\n");
                    state = START_S;
                }
                break;

            case A_RCV_S:
                if (buf[0] == C_UA) {
                    printf("C_UA received, moving to C_RCV_S\n");
                    state = C_RCV_S;
                } else if (buf[0] == FLAG) {
                    printf("FLAG received in A_RCV_S, moving to FLAG_RCV_S\n");
                    state = FLAG_RCV_S;
                } else {
                    printf("Unexpected byte in A_RCV_S, resetting to START_S\n");
                    state = START_S;
                }
                break;

            case C_RCV_S:
                if (buf[0] == (A_TX ^ C_UA)) {
                    printf("BCC_OK condition met, moving to BCC_OK_S\n");
                    state = BCC_OK_S;
                } else if (buf[0] == FLAG) {
                    printf("FLAG received in C_RCV_S, moving to FLAG_RCV_S\n");
                    state = FLAG_RCV_S;
                } else {
                    printf("Unexpected byte in C_RCV_S, resetting to START_S\n");
                    state = START_S;
                }
                break;

            case BCC_OK_S:
                if (buf[0] == FLAG) {
                    printf("FLAG received in BCC_OK_S, returning 0\n");
                    return 0;
                } else {
                    printf("Unexpected byte in BCC_OK_S, resetting to START_S\n");
                    state = START_S;
                }
                break;

            default:
                printf("Unknown state encountered, exiting.\n");
                return 1;
        }
    }

    printf("State machine exited with STOP_S, returning 1\n");
    return 1;
}

