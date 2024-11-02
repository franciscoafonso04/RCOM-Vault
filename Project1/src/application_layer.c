// Application layer protocol implementation

#include "application_layer.h"
#include "tools.h"

int framesSent = 0;
int alarmTotalCount = 0;
int rejCount = 0;
extern long fileSize;
extern double delta;
extern int nRej;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connect;

    struct timespec start, end;

    connect.baudRate = baudRate;
    connect.nRetransmissions = nTries;

    if (strcmp("tx", role) == 0)
        connect.role = LlTx;
    else
        connect.role = LlRx;

    strcpy(connect.serialPort, serialPort);
    connect.timeout = timeout;

    if (llopen(connect) == -1) {
        printf("Failed to connect\n");
        return;
    }
    printf("Connected\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (connect.role == LlTx) {
        
        // Opening file to read.
        FILE *file = fopen(filename, "rb");
        if (file == NULL){
            printf("Error: Unable to open the file %s for reading.\n", filename);
            return;
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        size_t bytes_to_Read = 700;
        unsigned char buffer[MAX_PAYLOAD_SIZE];
        long fileLength = fileSize;

        int packetSize = 0;
        unsigned char* startPacket = writeControl(fileSize, filename, &packetSize, P_START);

        if (llwrite(startPacket, packetSize) == -1){
            printf("Time exceeded in startPacket\n");
            return;
        }
        free(startPacket);
        printf("startPacket funcionou!\n");

        int seqNum = 0;

        // Send data packets until all the bytes are sent
        while (fileLength > 0) {
            // Read the file to check if it is the last packet
            int bytesRead = fread(buffer, 1, (fileLength >= bytes_to_Read) ? bytes_to_Read : fileLength, file);
            if (bytesRead == 0) {
                printf("Error reading the file.\n");
                break;
            }

            unsigned char* dataPacket = writeData(buffer, bytesRead, seqNum++);
            if (llwrite(dataPacket, 4 + bytesRead) == -1){
                printf("Time exceeded in dataPacket\n");
                return;
            }
            free(dataPacket);

            fileLength -= bytesRead;
        }

        packetSize = 0;
        unsigned char* endPacket = writeControl(fileSize, filename, &packetSize, P_END);

        if (llwrite(endPacket, packetSize) == -1){
            printf("Time exceeded in endPacket\n");
            return;
        }
        free(endPacket);

        printf("All bytes were written\n");

        fclose(file);

    } else if (connect.role == LlRx) {
        long readBytes = 0;
        int pos = 0;
        unsigned char packet[MAX_PAYLOAD_SIZE];
        while (packet[0] != P_START){
            llread(packet);

            if (nRej > nTries) {
                printf("exceeded number of retries!\n");
                return;
            }
        }

        if (packet[1] == T_SIZE) {
            unsigned char n_bytes = packet[2];
            
            for (int i = n_bytes; i > 0; i--)
                readBytes += power(256, i - 1) * (int)packet[3 + pos++];
            
            fileSize = readBytes;
            printf("fileSize: %ld\n", fileSize);
        }
        else {
            printf("Error in the start control packet\n");
            return;
        }

        FILE *file = fopen(filename, "wb");
        if (file == NULL) {
            printf("Error: Unable to create or open the file %s for writing.\n", filename);
            return;
        }

        int size = -1;
        while (packet[0] != P_END) {
            // Read until a data packet is received
            while (size == -1) {
                size = llread(packet);

                if (nRej > nTries) {
                    printf("exceeded number of retries!\n");
                    return;
                }
            }

            // Check if the packet is the end control packet
            if (packet[0] == P_END)
                break;

            // Write the data to the file
            if (size > 0){
                size_t bytesWrittenNow = fwrite(packet + 4, 1, size - 4, file);
                if (bytesWrittenNow == (long)0)
                    break;
                readBytes -= bytesWrittenNow;

                double progress = 100.0 * ((double)(fileSize - readBytes) / (double)fileSize);
                printf("%.2f%% completed...\n", progress);
            }
            size = -1;
        }

        // Check if all the bytes were read and written
        if (readBytes > (long)0) {
            printf("Error: Data was lost! %ld bytes were lost\n", readBytes);
            return;
        }
        printf("All bytes were read and written\n");

        fclose(file);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    delta = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    llclose(TRUE); // SHOW STATISTICS
}
