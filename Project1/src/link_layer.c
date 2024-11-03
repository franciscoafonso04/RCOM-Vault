// Link layer protocol implementation

#include "state_machines.h"

extern int alarmCount;        // Counts the number of triggered alarms
extern int alarmEnabled;      // Indicates if the alarm timer is active
extern int alarmTotalCount;   // Counts the total number of alarms during execution
extern int framesSent;        // Counts the total number of frames successfully sent
extern int iFrame;            // Indicates the current information frame number (0 or 1)
extern int rejCount;          // Counts the number of REJ frames received due to errors
extern long fileSize;         // Size of the file being transferred in bytes
extern double delta;          // Elapsed time for file transfer in seconds
int nTries;                   // Number of retry attempts allowed for each frame transmission
int timeout;                  // Timeout duration in seconds for retrying failed transmissions
LinkLayerRole role;           // Role in link layer (LlTx for transmitter, LlRx for receiver)


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 256

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // Initialize control flags and parameters
    alarmEnabled = FALSE;                                   // Disable alarm initially
    alarmCount = 0;                                         // Reset alarm counter to zero
    timeout = connectionParameters.timeout;                 // Set timeout duration
    nTries = connectionParameters.nRetransmissions + 1;     // Set retry limit (plus one for initial attempt)
    role = connectionParameters.role;                       // Define role (LlTx for transmitter, LlRx for receiver)

    printf("Starting llopen...\n");

    // Setup signal handler for alarm
    (void)signal(SIGALRM, alarmHandler);

    unsigned char buf[5] = {0};

    // Attempt to open the serial port
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0)
        return -1;
    
    // Initialize state for state machine
    State state = START_S;

    // Main loop to handle state transitions
    while (state != STOP_S && alarmCount < nTries) {

        // Transmission block for transmitter role
        if (alarmEnabled == FALSE && role == LlTx) {
    
            buf[0] = FLAG;                  // Start FLAG
            buf[1] = A_TX;                  // Address byte for transmitter
            buf[2] = C_SET;                 // Control byte for SET command
            buf[3] = buf[1] ^ buf[2];       // BCC1: XOR of address and control fields
            buf[4] = FLAG;                  // End FLAG

            writeBytesSerialPort(buf, 5);   // Send frame via serial port

            
            alarm(timeout);                 // Set timeout alarm
            alarmEnabled = TRUE;            // Enable alarm
        }

        // Read bytes from serial port for both transmitter and receiver roles
        int byte = readByteSerialPort(buf);
        if (byte == 0)
            continue;

        // Call state machine and track state
        openStateMachine(&state, buf, role);
    }

    // Check if the number of retries has exceeded the limit
    if (alarmCount >= nTries) {
        printf("Reached limit of retransmissions\n");
        return -1;
    }

    // Respond with UA if role is LlRx (receiver)
    if (role == LlRx) {
        buf[0] = FLAG;                  // Start FLAG
        buf[1] = A_RX;                  // Address byte for receiver
        buf[2] = C_UA;                  // Control byte for UA command
        buf[3] = buf[1] ^ buf[2];       // BCC1: XOR of address and control fields
        buf[4] = FLAG;                  // End FLAG

        int bytes = writeBytesSerialPort(buf, 5);   // Transmit the frame
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
    // Check for valid buffer and size
    if (bufSize <= 0 || buf == NULL)
        return -1;

    int maxFrameSize = MAX_PAYLOAD_SIZE;    // Maximum frame size
    int currentSize;                        // Tracks current frame size
    int ans = 0;                            // Response status
    int size = 0;                           // Position in frame during stuffing
    int bytes = 0;                          // Number of bytes written
    int bufPos = 0;                         // Position in buffer

    printf("\nStarting llwrite: frameSize = %d\n", bufSize);

    alarmEnabled = FALSE;                   // Reset alarm status
    alarmCount = 0;                         // Reset alarm counter to zero

    // Loop to retry transmission if necessary
    while (alarmCount < nTries) {
        currentSize = 0;
        bufPos = 0;
        maxFrameSize = MAX_PAYLOAD_SIZE;

        // Transmit frame if no alarm is set or response was negative
        if (alarmEnabled == FALSE || ans < 0) {
            unsigned char frame[maxFrameSize];

            // Building frame header
            frame[currentSize++] = FLAG;                        // Start FLAG
            frame[currentSize++] = A_TX;                        // Address byte 
            frame[currentSize++] = (iFrame == 0) ? C_I0 : C_I1; // Control byte based on frame number
            frame[currentSize++] = frame[1] ^ frame[2];         // BCC1: XOR of address and control fields

            unsigned char BCC2 = 0;         // Initialize BCC2 for data verification

            // Copy data and calculate BCC2
            while (currentSize < bufSize + 4) {
                BCC2 ^= buf[bufPos];                        // Calculate BCC2 over data
                frame[currentSize++] = buf[bufPos++];       // Add data to frame
            }
            frame[currentSize++] = BCC2;                    // Add BCC2 to frame

            // Byte-Stuffing for FLAG and ESC characters
            size = 4;
            while (size < currentSize) {
                if (frame[size] == FLAG) {                  // Replace FLAG with ESC + FLAG_SEQ
                    frame[size] = ESC;
                    arrayInsert(frame, &maxFrameSize, FLAG_SEQ, size + 1);
                    currentSize++;
                    size++; 
                }
                else if (frame[size] == ESC) {              // Replace ESC with ESC + ESC_SEQ
                    frame[size] = ESC;
                    arrayInsert(frame, &maxFrameSize, ESC_SEQ, size + 1);
                    currentSize++;
                    size++; 
                }
                size++;
            }

            frame[currentSize++] = FLAG;    // End FLAG                    

            // Send the complete frame over serial port
            bytes = writeBytesSerialPort(frame, currentSize);
            framesSent++;                   // Increment frames sent counter                                  

            alarm(timeout);                 // Set timeout alarm
            alarmEnabled = TRUE;            // Enable alarm
        }

        // Wait for acknowledgment using state machine
        ans = writeStateMachine();
        printf("writeStateMachine response: %d\n", ans);

        // Return bytes written on success
        if (ans == 0) {
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

    printf("\nStarting llread...\n");

    // Attempt to read packet with state machine
    int size = readStateMachine(packet);    // Obtain packet size from state machine

    // Check if no data was read
    if (size == 0) {
        printf("Didn't read anything, sending response to request retransmission.\n");
        writeResponse(TRUE, iFrame);        // Send RR to request retransmission
        return -1;                          // Indicate no data read 
    } 

    framesSent++;                           // Increment frame counter

    if (size == -1) {
        printf("iFrame mismatch with res\n");
        writeResponse(FALSE, iFrame);       // Send REJ due to incorrect iFrame
        rejCount++;                         // Increment rejection counter
        return -1;                          // Indicate mismatch
    }

    size -= 1;                              // Exclude BCC2 from packet size
    unsigned char BCC2 = 0;

    // Calculate BCC2 over the packet data
    for (int i = 0; i < size; i++) {
        BCC2 ^= packet[i];
    }

    printf("Calculated BCC2: 0x%02X, Expected BCC2: 0x%02X\n", BCC2, packet[size]);

    // Check BCC2 validity
    if (BCC2 == packet[size]) {
        writeResponse(TRUE, iFrame);        // Send RR to acknowledge valid frame

        iFrame = !iFrame;                   // Toggle iFrame for the next expected frame

        return size;                        // Return size of valid packet data
    } else {
        writeResponse(FALSE, iFrame);       // Send REJ for incorrect BCC2
        rejCount++;                         // Increment rejection counter
        return -1;                          // Indicate error
    }
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////

int llclose(int showStatistics)
{
    unsigned char buf[5] = {0};             // Buffer for DISC and UA frames

    // If role is transmitter (LlTx), initiate disconnection
    if (role == LlTx) {
        alarmEnabled = FALSE;
        alarmCount = 0;
        printf("\n");

        // Loop to send DISC frame and handle possible retransmissions
        while (alarmCount < nTries) {
            if (alarmEnabled == FALSE) {
                // Construct and send DISC frame
                buf[0] = FLAG;              // Start FLAG
                buf[1] = A_TX;              // Address byte for transmitter
                buf[2] = C_DISC;            // Control byte for DISC command
                buf[3] = buf[1] ^ buf[2];   // BCC1: XOR of address and control fields
                buf[4] = FLAG;              // End FLAG

                int bytes = writeBytesSerialPort(buf, 5);       // Send DISC frame
                printf("Transmitter sent DISC: %d bytes written\n", bytes);

                alarm(timeout);             // Set timeout alarm
                alarmEnabled = TRUE;        // Enable alarm
            }

            // Wait for DISC response, then send UA to acknowledge
            if (!discStateMachine()) {      // Exit loop if DISC received

                buf[0] = FLAG;              // Start FLAG
                buf[1] = A_TX;              // Address byte for transmitter
                buf[2] = C_UA;              // Control byte for UA command
                buf[3] = buf[1] ^ buf[2];   // BCC1: XOR of address and control fields
                buf[4] = FLAG;              // End FLAG

                int bytes = writeBytesSerialPort(buf, 5);   // Send UA frame
                printf("Transmitter sent UA: %d bytes written.\n", bytes);
                break;
            }
        }

        // Check if retransmissions exceeded limit
        if (alarmCount >= nTries) {
            printf("Time exceeded! Maximum retries reached.\n");
            return 1;
        }
    }
    
    // If role is receiver (LlRx), wait for DISC and respond
    else if (role == LlRx) {
        printf("\n");

        // Wait for DISC frame from transmitter
        while (discStateMachine()) {}

        // Send DISC frame to acknowledge disconnection
        do {
            
            buf[0] = FLAG;              // Start FLAG
            buf[1] = A_TX;              // Address byte for transmitter (besides being receiver acts as transmitter)
            buf[2] = C_DISC;            // Control byte for DISC command
            buf[3] = buf[1] ^ buf[2];   // BCC1: XOR of address and control fields
            buf[4] = FLAG;              // End FLAG

            int bytes = writeBytesSerialPort(buf, 5);
            printf("Receiver sent DISC: %d bytes written\n", bytes);

        } while (uaStateMachine());     // Wait for UA acknowledgment
    }

    // Close the serial port and log status
    int clstat = closeSerialPort();
    printf("Serial port closed with status: %d\n", clstat);

    // If requested, display connection statistics
    if (showStatistics) {
        if (role == LlTx) {
            printf("\nTransmitter Statistics:\n");
            printf("File size: %ld\n", fileSize);
            printf("Frames sent: %d\n", framesSent);
            printf("Total number of alarms: %d\n", alarmTotalCount);
        }
        else if (role == LlRx) {
            printf("\nReceiver Statistics:\n");
            printf("File size: %ld\n", fileSize);
            printf("Frames read: %d\n", framesSent);
            printf("Number of rejection/repetitions: %d\n", rejCount);
        }

        printf("Time elapsed: %.3f seconds\n", delta);
    }

    return clstat;
}
