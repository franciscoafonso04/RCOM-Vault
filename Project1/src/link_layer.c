// Link layer protocol implementation

#include "states.h"

extern int alarmEnabled, alarmCount;
extern int iFrame;
int timeout, nTries;
LinkLayerRole role;
extern long fileSize;
extern time_t delta;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 256
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // Initialize control flags and parameters
    alarmEnabled = FALSE;
    alarmCount = 0;

    timeout = connectionParameters.timeout;
    nTries = connectionParameters.nRetransmissions;
    role = connectionParameters.role;

    //printf("Starting llopen: timeout = %d, nTries = %d, role = %d\n", timeout, nTries, role);

    // Setup signal handler for alarm
    (void)signal(SIGALRM, alarmHandler);
    unsigned char buf[6] = {0};

    // Attempt to open the serial port
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0) {
        //printf("Failed to open serial port\n");
        return -1;
    }
    //printf("Serial port opened with file descriptor: %d\n", fd);

    State state = START_S;

    // Main loop to handle state transitions
    while (state != STOP_S) {

        // Transmission block for transmitter role
        if (alarmEnabled == FALSE && role == LlTx) {
            // Construct SET frame
            buf[0] = FLAG;
            buf[1] = A_TX;
            buf[2] = C_SET;
            buf[3] = buf[1] ^ buf[2];
            buf[4] = FLAG;

            writeBytesSerialPort(buf, 5);
            //printf("Transmitter sent SET: %d bytes written\n", bytes);

            // Enable alarm for timeout
            alarm(timeout);
            alarmEnabled = TRUE;
        }

        // Read bytes from serial port for both transmitter and receiver roles
        int byte = readByteSerialPort(buf);
        if (byte == 0) continue;

        //printf("Received byte: 0x%02X, Current State: %d\n", buf[0], state);

        // Call state machine and track state
        openStateMachine(&state, buf, role);
        //printf("Updated State after openStateMachine call: %d\n", state);

        // Check if the number of retries has exceeded the limit
        if (alarmCount >= nTries) {
            perror("Reached limit of retransmissions\n");
            return -1;
        }
    }

    // Respond with UA if role is LlRx (receiver)
    if (role == LlRx) {
        buf[0] = FLAG;
        buf[1] = A_RX;
        buf[2] = C_UA;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;

        int bytes = writeBytesSerialPort(buf, 5);
        printf("Receiver sent UA: %d bytes written\n", bytes);
    }

    printf("llopen completed successfully, returning file descriptor %d\n", fd);
    return fd;
}


////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    if (bufSize <= 0 || buf == NULL) return -1;
    
    int maxFrameSize = MAX_PAYLOAD_SIZE;
    int currentSize;
    int ans = 0;
    int size = 0;
    int bytes = 0;

    // Debug: initial settings
    printf("Starting llwrite: bufSize = %d, maxFrameSize = %d, nTries = %d\n", bufSize, maxFrameSize, nTries);
    
    alarmEnabled = FALSE;
    alarmCount = 0;
    
    while (alarmCount < nTries) {
        currentSize = 0;

        if (alarmEnabled == FALSE || ans < 0) {
            unsigned char frame[maxFrameSize];

            // Building frame header
            frame[currentSize++] = FLAG;
            frame[currentSize++] = A_TX;  
            frame[currentSize++] = (iFrame == 0) ? C_I0 : C_I1;; // Information frame number 0
            frame[currentSize++] = frame[1] ^ frame[2];
            
            unsigned char BCC2 = 0;
            
            // Copy data and calculate BCC2
            //printf("Calculating BCC2 and adding data...\n");
            while (currentSize < bufSize + 4) {
                BCC2 ^= *buf;
                frame[currentSize++] = *buf++;
                
            }
            frame[currentSize] = BCC2;
            //printf("Data and BCC2 added. BCC2 = 0x%02X, currentSize = %d\n", BCC2, currentSize);
            
            // Byte-Stuffing for FLAG and ESC characters
            size = 4;
            while (size < currentSize) {
                if (frame[size] == FLAG) {
                    frame[size] = ESC;
                    arrayInsert(frame, &maxFrameSize, FLAG_SEQ, size + 1);
                    currentSize++;
                    size++; // Move past the inserted byte to avoid re-checking it.
                    //printf("Byte-stuffing FLAG at index %d\n", size);
                } else if (frame[size] == ESC) {
                    frame[size] = ESC;
                    arrayInsert(frame, &maxFrameSize, ESC_SEQ, size + 1);
                    currentSize++;
                    size++; // Move past the inserted byte to avoid re-checking it.
                    //printf("Byte-stuffing ESC at index %d\n", size);
                }
                size++;
            }

            frame[currentSize++] = FLAG;
            //printf("Frame completed with end FLAG. Total frame size: %d\n", currentSize);

            // Send frame
            bytes = writeBytesSerialPort(frame, currentSize);
            //printf("%d bytes written to serial port\n", bytes);
            
            // Enable timeout
            alarm(timeout);
            alarmEnabled = TRUE;
        }
        
        // Wait for acknowledgment
        sleep(1);
        ans = writeStateMachine();
        printf("writeStateMachine response: %d\n", ans);

        // Success if acknowledgment received
        if (ans == 0) {
            //printf("Transmission successful. Total bytes written: %d\n", bytes);
            return bytes;
        }
    }

    printf("Transmission failed after %d attempts.\n", alarmCount);
    return -1;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // Attempt to read packet with state machine
    int size = readStateMachine(packet);
    printf("Read packet size: %d\n", size);

    // Check if no data was read
    if (size == 0) {
        printf("Didn't read anything, sending response to request retransmission.\n");
        writeResponse(TRUE, iFrame);
        return -1;
    }

    size -= 1; // Don't include BCC2 in the packet size
    unsigned char BCC2 = 0;

    // Calculate BCC2 over the packet data
    for (int i = 0; i < size; i++) {
        BCC2 ^= packet[i];
    }
    printf("Calculated BCC2: 0x%02X, Expected BCC2: 0x%02X\n", BCC2, packet[size]);

    // Check BCC2 validity
    if (BCC2 == packet[size]) {
        printf("BCC2 check passed. Sending RR.\n");
        writeResponse(TRUE, iFrame);

        // Toggle iFrame for the next expected frame
        iFrame = !iFrame;

        // Return the size of the valid packet data
        return size;
    } else {
        printf("BCC2 check failed. Sending REJ.\n");
        writeResponse(FALSE, iFrame);
        return -1;
    }
}


////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////

int llclose(int showStatistics)
{
    unsigned char buf[5] = {0};

    if (role == LlTx) {
        alarmEnabled = FALSE;
        alarmCount = 0;
        printf("Role: LlTx (Transmitter)\n");

        while (alarmCount < nTries) {
            if (alarmEnabled == FALSE) {
                // Construct and send DISC frame
                buf[0] = FLAG;
                buf[1] = A_TX;
                buf[2] = C_DISC;
                buf[3] = buf[1] ^ buf[2];
                buf[4] = FLAG;

                int bytes = writeBytesSerialPort(buf, 5);
                printf("Transmitter sent DISC: %d bytes written\n", bytes);

                alarmEnabled = TRUE;
                alarm(timeout);
            }

            // Wait for DISC frame to send UA frame
            if (!discStateMachine()) {
                buf[0] = FLAG;
                buf[1] = A_TX;
                buf[2] = C_UA;
                buf[3] = buf[1] ^ buf[2];
                buf[4] = FLAG;

                int bytes = writeBytesSerialPort(buf, 5);
                printf("Transmitter sent UA: %d bytes written\n", bytes);
                break;
            }
        }

        if (alarmCount >= nTries) {
            printf("Time exceeded! Maximum retries reached.\n");
            return 1;
        }

    } else { // role == LlRx
        printf("Role: LlRx (Receiver)\n");

        while (discStateMachine()) {
            printf("Waiting for DISC frame from Transmitter...\n");
        }

        do {
            // Send DISC in response
            buf[0] = FLAG;
            buf[1] = A_TX;
            buf[2] = C_DISC;
            buf[3] = buf[1] ^ buf[2];
            buf[4] = FLAG;

            int bytes = writeBytesSerialPort(buf, 5);
            printf("Receiver sent DISC: %d bytes written\n", bytes);

        } while (uaStateMachine());
    }

    // Close serial port and optionally display statistics
    int clstat = closeSerialPort();
    printf("Serial port closed with status: %d\n", clstat);

    if (showStatistics) {
        printf("\nStatistics:\n");

        if (role == LlTx)
            printf("User: Transmitter\n");
        else
            printf("User: Receiver\n");

        printf("File size: %ld\n", fileSize);

        if (role == LlTx) {
            printf("Frames sent: %d\n", 3);  // Placeholder values
            printf("Total number of alarms: %d\n", 3); // Placeholder
        } else {
            printf("Frames read: %d\n", 3); // Placeholder
            printf("Number of rejection/repetitions: %d\n", 3); // Placeholder
        }

        printf("Time spent: %g seconds\n", 3.2); // Placeholder
        printf("Total time: %ld seconds\n", delta);
    }

    return clstat;
}

