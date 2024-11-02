#include "state_machines.h"

extern int alarmEnabled;    // Indicates if the alarm timer is active.
extern int iFrame;          // Tracks the current information frame number (0 or 1)

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

    State state = START_S;          // Initialize state for state machine
    unsigned char buf[1] = {0};     // Buffer to hold received bytes
    int res = 0;                    // Variable to store the control field

    // Start of the main loop, runs until state is STOP_S or alarmEnabled is false
    while (state != STOP_S && alarmEnabled == TRUE) {

        // Attempt to read a byte from the serial port
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;  // No byte received, loop continues


        // Handle each state and transition
        switch (state) {
            case START_S:
                // Wait for the starting FLAG to initiate frame reception
                if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;
                }
                break;

            case FLAG_RCV_S:
                // Expect the address byte (A_TX) after FLAG
                if (buf[0] == A_TX) {
                    state = A_RCV_S;

                } else if (buf[0] == FLAG) {
                    // Remain in FLAG_RCV_S if another FLAG is received
                    state = FLAG_RCV_S;

                } else {
                    // Reset to START_S if an unexpected byte is received
                    state = START_S;
                }
                break;

            case A_RCV_S:
                // Expect one of the control fields (RR0, RR1, REJ0, REJ1) from the receiver
                if (buf[0] == C_RR0 || buf[0] == C_RR1 || buf[0] == C_REJ0 || buf[0] == C_REJ1) {
                    res = buf[0];        // Store the control field to determine the response type
                    state = C_RCV_S;

                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S;  // Return to FLAG_RCV_S if FLAG is received again
                
                } else {
                    state = START_S;    // Reset to START_S if any other byte is received
                }
                break;

            case C_RCV_S:
                // Check for BCC1 (XOR of address and control) to confirm frame validity
                if (buf[0] == (A_TX ^ res)) {
                    state = BCC_OK_S;   // Move to BCC_OK_S if BCC1 is valid

                } else if (buf[0] == FLAG) {
                    state = FLAG_RCV_S; // Return to FLAG_RCV_S if FLAG is received

                } else {
                    state = START_S;    // Reset to START_S if BCC1 check fails
                }
                break;

            case BCC_OK_S:
                // Expect the closing FLAG to confirm the end of the frame
                if (buf[0] == FLAG) {

                    state = STOP_S;    // Transition to STOP_S to indicate end of frame
                    alarm(0);          // Disable the alarm

                    // Handle responses based on control field content
                    if (res == C_REJ0 || res == C_REJ1) {
                        alarmCount = 0;     // Reset alarm count on REJ (Negative Acknowledgment)
                        printf("Received REJ, returning -2\n");
                        return -2;          // Return -2 for REJ, indicating retransmission needed
                    }

                    if ((res == C_RR0 && iFrame == 0) || (res == C_RR1 && iFrame == 1)) {
                        // Check if RR is duplicate (same iFrame), no retransmission needed
                        printf("Received duplicate RR, returning -3\n");
                        return -3;          // Return -3 to indicate duplicate RR
                    }

                    // Handle RR for valid acknowledgment
                    if (res == C_RR0) {
                        iFrame = 0;     // Set iFrame to 0 if RR0 is received
                        printf("Received RR0, iFrame set to 0\n");
                    }
                    if (res == C_RR1) {
                        iFrame = 1;     // Set iFrame to 1 if RR1 is received
                        printf("Received RR1, iFrame set to 1\n");
                    }
                    return 0;           // Return 0 for successful acknowledgment

                } else {
                    state = START_S;    // Reset to START_S if end FLAG is not received
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

    State state = START_S;  // Initialize state for state machine

    unsigned char buf[1] = {0}; // Buffer to hold received bytes
    int res = -1;               // Holds the control field (C_I0 or C_I1) if found
    int deStuff = FALSE;        // Flag for handling byte-stuffing
    int size = 0;               // Tracks the size of the packet data


    while (state != STOP_S) {
        
        int byte = readByteSerialPort(buf);     // Read a byte from the serial port
        if (byte == 0) continue;                // No byte received, loop continues


        // If we are in byte-stuffing mode, handle the next byte accordingly
        if (deStuff) {
            if (buf[0] == FLAG_SEQ) {
                packet[size] = FLAG;        // Replace the FLAG_SEQ with FLAG in the packet

            } else if (buf[0] == ESC_SEQ) {
                packet[size] = ESC;         // Replace the ESC_SEQ with ESC in the packet

            } else {            
                continue;                   // If the byte is unexpected, skip the current loop iteration
            }
            size++;
            deStuff = FALSE; // Reset deStuff mode
        } 
        
        else {
            // Normal state handling without byte-stuffing
            switch (state) {
                case START_S:
                    // Wait for the starting FLAG to initiate frame reception
                    if (buf[0] == FLAG) {
                        state = FLAG_RCV_S; 
                    }

                    break;

                case FLAG_RCV_S:
                    // Expect the address byte (A_TX) after FLAG
                    if (buf[0] == A_TX) {
                        state = A_RCV_S;

                    } else if (buf[0] == FLAG) {
                        state = FLAG_RCV_S; // Remain in FLAG_RCV_S if another FLAG is received

                    }else {
                        state = START_S; // Reset to START_S if an unexpected byte is received
                    }
                    break;

                case A_RCV_S:
                    // Check if the control byte matches either C_I0 or C_I1
                    if (buf[0] == C_I0 || buf[0] == C_I1) {
                        state = C_RCV_S; // Transition to C_RCV_S state
                        res = buf[0]; // Store the control field for later checks

                    } else if (buf[0] == FLAG) {
                        state = FLAG_RCV_S;   // Transition back to FLAG_RCV_S if a FLAG is received

                    }else {
                        state = START_S;    // Reset to START_S if an unexpected byte is received
                    }
                    
                    break;

                case C_RCV_S:
                    // Validate the BCC field by checking against the expected value
                    if (buf[0] == (A_TX ^ res)) {
                        state = BCC_OK_S;           // If valid, transition to BCC_OK_S

                    } else if (buf[0] == FLAG) {
                        state = FLAG_RCV_S;         // Transition back to FLAG_RCV_S if a FLAG is received         

                    } else { 
                        state = START_S;    // Reset to START_S if an unexpected byte is received
                    }
                    
                    break;

                case BCC_OK_S:
                    // Expect the closing FLAG to confirm the end of the frame
                    if (buf[0] == FLAG) {
                        state = STOP_S;  // End of frame detected
                    } else if (buf[0] == ESC) {
                        deStuff = TRUE; // Start byte-stuffing mode
                    } else {
                        packet[size] = buf[0];  // Add normal byte to packet
                        size++;                 // Increment the size of the packet
                    }
                    break;

                default:
                    printf("Unknown state encountered.\n");
                    break;
            }
        }
    }


    // Verify if iFrame matches the received control field (res)
    // This ensures the correct frame has been received
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

