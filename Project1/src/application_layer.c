// Application layer protocol implementation

#include "application_layer.h"
#include "tools.h"

extern int nRej;              // Tracks the total number of REJ frames
extern long fileSize;         // Size of the file being transferred in bytes
extern double delta;          // Elapsed time for file transfer in seconds
int alarmTotalCount = 0;      // Counts the total number of alarms during execution 
int framesSent = 0;           // Counts the total number of frames successfully sent
int rejCount = 0;             // Counts the number of REJ frames received due to errors


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connect;                  // Structure to hold link layer connection parameters

    struct timespec start, end;         // Variables for measuring elapsed time

    // Initialize link layer parameters
    connect.baudRate = baudRate;        // Set baud rate for the connection
    connect.nRetransmissions = nTries;  // Set number of allowed retransmissions
 
    // Determine the role (transmitter or receiver) based on input
    if (strcmp("tx", role) == 0) connect.role = LlTx;
    else connect.role = LlRx;

    strcpy(connect.serialPort, serialPort);     // Copy serial port name to connect structure     
    connect.timeout = timeout;                  // Set timeout for connection

    // Attempt to open the link layer connection
    if (llopen(connect) == -1) {        
        printf("Failed to connect\n");
        return;
    }
    printf("Connected\n");                  // Log successful connection
    
    clock_gettime(CLOCK_MONOTONIC, &start); // Start time measurement

    if (connect.role == LlTx) {             // Check if the role is transmitter
        
        // Opening file to read.
        FILE *file = fopen(filename, "rb");
        if (file == NULL){
            printf("Error: Unable to open the file %s for reading.\n", filename);
            return;                         // Exit if file cannot be opened
        }

        // Get file size
        fseek(file, 0, SEEK_END);               // Move to the end of the file
        fileSize = ftell(file);                 // Get the size of the file
        fseek(file, 0, SEEK_SET);               // Reset file pointer to the beginning

        size_t bytes_to_Read = 700;             // Define maximum bytes to read in one operation
        unsigned char buffer[MAX_PAYLOAD_SIZE]; // Buffer for reading data from file
        long fileLength = fileSize;             // Copy the file length for processing

        int packetSize = 0;                     // Initialize packet size to create start packet
        unsigned char* startPacket = writeControl(fileSize, filename, &packetSize, P_START);

        // Send the start packet
        if (llwrite(startPacket, packetSize) == -1){
            printf("Time exceeded in startPacket\n");
            return;                             // Exit on failure to send start packet
        }

        free(startPacket);                      // Free allocated memory for start packet
        printf("startPacket funcionou!\n");     // Log successful start packet transmission

        int seqNum = 0;                         // Initialize sequence number for data packets

        // Send data packets until all the bytes are sent
        while (fileLength > 0) {

            // Read the file to check if it is the last packet
            int bytesRead = fread(buffer, 1, (fileLength >= bytes_to_Read) ? bytes_to_Read : fileLength, file);
            if (bytesRead == 0) {
                printf("Error reading the file.\n");
                break;                          // Exit on read erro
            }

            // Create a data packet and send it
            unsigned char* dataPacket = writeData(buffer, bytesRead, seqNum++);
            if (llwrite(dataPacket, 4 + bytesRead) == -1){
                printf("Time exceeded in dataPacket\n");
                return;                         // Exit if sending the data packet fails
            }
            free(dataPacket);                   // Free memory allocated for data packet                   

            fileLength -= bytesRead;            // Decrease remaining file length
        }

        // Create and send end packet
        packetSize = 0;
        unsigned char* endPacket = writeControl(fileSize, filename, &packetSize, P_END);

        if (llwrite(endPacket, packetSize) == -1){
            printf("Time exceeded in endPacket\n");
            return;                             // Exit if sending the end packet fails
        }

        free(endPacket);                        // Free memory allocated for end packet

        printf("All bytes were written\n");     // Log successful transmission of all bytes

        fclose(file);                           // Close the file after reading

    } else if (connect.role == LlRx) {          // Check if the role is receiver

        long readBytes = 0;                     // Initialize bytes read counter
        int pos = 0;                            // Position index for accessing packet
        unsigned char packet[MAX_PAYLOAD_SIZE]; // Buffer for received packets

        // Read until the start control packet is received
        while (packet[0] != P_START){
            llread(packet);                     // Read packet from link layer

            // Check for exceeded number of retries
            if (nRej > nTries) {
                printf("exceeded number of retries!\n");
                return;                         // Exit if retries exceeded
            }
        }

        // Check if the received packet is the start control packet
        if (packet[1] == T_SIZE) {
            unsigned char n_bytes = packet[2];
            
            // Calculate total number of bytes indicated in the control packet
            for (int i = n_bytes; i > 0; i--)
                readBytes += power(256, i - 1) * (int)packet[3 + pos++];
            
            fileSize = readBytes;                   // Store the calculated file size
            printf("fileSize: %ld\n", fileSize);    // Log the file size
        }

        else {
            printf("Error in the start control packet\n");  // Exit if the control packet is invalid
            return;
        }

        // Open the file for writing received data
        FILE *file = fopen(filename, "wb");
        if (file == NULL) {
            printf("Error: Unable to create or open the file %s for writing.\n", filename);
            return;                                 // Exit if file cannot be opened
        }

        int size = -1;                              // Initialize size of the received packet
        while (packet[0] != P_END) {                // Read until the end control packet is received

            // Read until a data packet is received
            while (size == -1) {
                size = llread(packet);              // Read a packet from the link layer

                // Check for exceeded number of retries
                if (nRej > nTries) {
                    printf("exceeded number of retries!\n");
                    return;                         // Exit if retries exceeded
                }
            }

            // Check if the packet is the end control packet
            if (packet[0] == P_END) break;

            // Write the data to the file
            if (size > 0){

                size_t bytesWrittenNow = fwrite(packet + 4, 1, size - 4, file); // Write data from the packet to the file
                if (bytesWrittenNow == (long)0) break;                          // Exit if writing to file fails
                readBytes -= bytesWrittenNow;                                   // Decrease the count of remaining bytes to read

                // Calculate and log progress
                double progress = 100.0 * ((double)(fileSize - readBytes) / (double)fileSize);
                printf("%.2f%% completed...\n", progress);

            }
            size = -1;  // Reset size for next packet
        }

        // Check if all the bytes were read and written successfully
        if (readBytes > (long)0) {
            printf("Error: Data was lost! %ld bytes were lost\n", readBytes);
            return;                                     // Exit if data loss is detected
        }
        printf("All bytes were read and written\n");    // Log successful read and write completion

        fclose(file);                                   // Close the file after writing
    }

    clock_gettime(CLOCK_MONOTONIC, &end);               // End time measurement

    // Calculate and store the elapsed time
    delta = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Close the link layer connection and show statistics
    llclose(TRUE); 
}