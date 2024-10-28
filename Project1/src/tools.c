#include "tools.h"

int alarmEnabled = FALSE;
int alarmCount = 0;
int iFrame = 0;
long fileSize = 0;
time_t delta = 0;

void arrayInsert(unsigned char arr[], int *n, int value, int pos) {
    
    if (pos < 0 || pos > *n) {
        printf("Invalid position!\n");
        return;
    }
    
    for (int i = *n; i > pos; i--) {
        arr[i] = arr[i - 1];
    }
    
    arr[pos] = value;

    (*n)++;
}


void alarmHandler(int signal) {
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int writeResponse(int rr, int iFrame)
{
    unsigned char buf[5] = {0};

    // Build response frame
    buf[0] = FLAG;
    buf[1] = A_TX;

    // Determine control field based on rr and iFrame
    if (rr == TRUE) {
        if (iFrame == 0) {
            buf[2] = C_RR1;
            printf("Sending RR1\n");
        } else if (iFrame == 1) {
            buf[2] = C_RR0;
            printf("Sending RR0\n");
        }
    } else if (rr == FALSE) {
        if (iFrame == 0) {
            buf[2] = C_REJ0;
            printf("Sending REJ0\n");
        } else if (iFrame == 1) {
            buf[2] = C_REJ1;
            printf("Sending REJ1\n");
        }
    }

    // Calculate BCC and set final FLAG
    buf[3] = buf[1] ^ buf[2];
    buf[4] = FLAG;

    // Print the response buffer for debugging
    printf("Response Buffer: ");
    for (int i = 0; i < 5; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");

    // Send response
    return writeBytesSerialPort(buf, 5);
}


unsigned char* writeControl(long fileSize, const char *fileName, int *packetSize, int type){
    unsigned char l_size = 0;
    unsigned char l_name = strlen(fileName);
    long aux = fileSize; 

    while(aux > 0) {
        aux /= 256;
        l_size++;
    }

    *packetSize = 5 + l_size + l_name;
    unsigned char* buf = (unsigned char*)malloc(*packetSize);

    if (!buf) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    buf[0] = type;

    buf[1] = T_SIZE;
    buf[2] = l_size;

    int pos = 3;
    while (l_size > 0) {
        buf[pos++] = (unsigned char)(fileSize >> (8 * (l_size - 1)));
        l_size--;
    }

    buf[pos++] = T_NAME;    
    buf[pos++] = l_name;

    for (int i = 0; i < l_name; i++)
        buf[pos++] = fileName[i];

    return buf;
}

unsigned char* writeData(unsigned char* data, int dataSize, int seqNum) {

    unsigned char* buf = (unsigned char*)malloc(4 + dataSize);
    if (!buf) return NULL;  

    buf[0] = P_DATA;
    buf[1] = seqNum % 256;
    buf[2] = (dataSize >> 8) & 0xFF;
    buf[3] = dataSize & 0xFF;

    memcpy(buf + 4, data, dataSize);

    return buf;
}

long power(int x, int y) {
    long res = 1;

    for (int i = 0; i < y; i++)
        res *= x;
    
    return res;
}