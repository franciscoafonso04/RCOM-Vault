#include "tools.h"

int alarmEnabled = FALSE;      // Indicates if the alarm is currently enabled (used for timeout control)
int alarmCount = 0;            // Tracks the number of alarms triggered
int iFrame = 0;                // Tracks the current information frame number (0 or 1)
int nRej = 0;                  // Counts the number of REJ frames received
long fileSize = 0;             // Represents the total file size in bytes
double delta = 0;              // Stores the total time taken for the transmission
extern int alarmTotalCount;    // Counts the total number of alarms, shared globally

// Function to insert a value into an array at a specified position
void arrayInsert(unsigned char arr[], int *n, int value, int pos) {

    if (pos < 0 || pos > *n) {
        printf("Invalid position!\n");
        return;
    }
    
    // Shift elements to the right to make space for the new element
    for (int i = *n; i > pos; i--) {
        arr[i] = arr[i - 1];
    }
    
    arr[pos] = value;   // Insert the new value at the specified position

    (*n)++;             // Increment the size of the array
}

// Alarm handler function to manage timeout events
void alarmHandler(int signal) {

    alarmEnabled = FALSE;               // Disable the alarm once triggered
    alarmCount++;                       // Increment the count for triggered alarms
    alarmTotalCount++;                  // Increment the count of total alarms triggered globally

    printf("Alarm #%d\n", alarmCount);  // Display the current alarm count
}

// Function to send an acknowledgment (RR) or reject (REJ) response
int writeResponse(int rr, int iFrame)
{
    unsigned char buf[5] = {0};         // Buffer to hold the response frame

    // Build response frame

    buf[0] = FLAG;                      // Start with FLAG byte
    buf[1] = A_TX;                      // Set the address field

    // Determine control field based on rr and iFrame
    if (rr == TRUE) {                   // If positive acknowledgment

        nRej = 0;                       // Reset REJ count as no error occurred

        if (iFrame == 0) {
            buf[2] = C_RR1;             // Send RR1 to indicate receiver is ready for frame 1
            printf("Sending RR1\n");
        } else if (iFrame == 1) {
            buf[2] = C_RR0;             // Send RR0 to indicate receiver is ready for frame 0
            printf("Sending RR0\n");
        }

    } 
    
    else if (rr == FALSE) {             // If negative acknowledgment

        nRej++;                         // Increment REJ count as an error occurred 
        if (iFrame == 0) {
            buf[2] = C_REJ0;            // Send REJ0 to reject frame 0
            printf("Sending REJ0\n");
        } else if (iFrame == 1) {
            buf[2] = C_REJ1;            // Send REJ1 to reject frame 1
            printf("Sending REJ1\n");
        }
    }

    buf[3] = buf[1] ^ buf[2];           // Calculate the BCC (error-checking byte)
    buf[4] = FLAG;                      // End with FLAG byte

    // Send response frame
    return writeBytesSerialPort(buf, 5);
}

// Function to create a control packet (start or end) containing file information
unsigned char* writeControl(long fileSize, const char *fileName, int *packetSize, int type){

    unsigned char l_size = 0;                   // Number of bytes needed to represent file size
    unsigned char l_name = strlen(fileName);    // Length of the file name
    long aux = fileSize;                        // Temporary variable to calculate file size length

    // Calculate the number of bytes needed to represent fileSize
    while(aux > 0) {
        aux /= 256;
        l_size++;
    }

    *packetSize = 5 + l_size + l_name;                              // Set the total packet size
    unsigned char* buf = (unsigned char*)malloc(*packetSize);       // Allocate memory for the packet

    if (!buf) return NULL;                                          // Check for successful memory allocation;                            

    buf[0] = type;                              // Set the packet type (start or end)
    buf[1] = T_SIZE;                            // Type identifier for file size
    buf[2] = l_size;                            // Set the length of the file size field

    int pos = 3;
    while (l_size > 0) {                        // Fill in file size bytes from high to low
        buf[pos++] = (unsigned char)(fileSize >> (8 * (l_size - 1)));
        l_size--;
    }

    buf[pos++] = T_NAME;                        // Type identifier for file name
    buf[pos++] = l_name;                        // Set the length of the file name

    // Copy the file name to the packet
    for (int i = 0; i < l_name; i++)
        buf[pos++] = fileName[i];

    return buf;                                 // Return the fixed-size buffer

}

// Function to create a data packet for transmission
unsigned char* writeData(unsigned char* data, int dataSize, int seqNum) {

    unsigned char* buf = (unsigned char*)malloc(4 + dataSize);      // Allocate memory for the data packet
    if (!buf) return NULL;                                          // Check for successful memory allocation

    buf[0] = P_DATA;                                                // Set packet type to data
    buf[1] = seqNum % 256;                                          // Set the sequence number (mod 256 for wrap-around)
    buf[2] = (dataSize >> 8) & 0xFF;                                // Store the high byte by shifting 8 bits to the right and masking.
    buf[3] = dataSize & 0xFF;                                       // Stpre the low byte by masking.

    memcpy(buf + 4, data, dataSize);                                // Copy the actual data into the packet

    return buf;                                                     // Return the assembled data packet
}

// Function to compute the power of x raised to y
long power(int x, int y) {   

    long res = 1;                           // Initialize result                   
    for (int i = 0; i < y; i++) res *= x;   // Multiply x by itself y times
    return res;                             // Return the computed power
}