// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include <time.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
     // time variables
    time_t start;
    time_t end;
    LinkLayer connectionParams;

    start = time(NULL);
    // Configuring link layer
    connectionParams.baudRate = baudRate;
    connectionParams.nRetransmissions = nTries;

    if (role[0] == 't')
        connectionParams.role = LlTx;
    else
        connectionParams.role = LlRx;

    strcpy(connectionParams.serialPort, serialPort);
    connectionParams.timeout = timeout;

    // Connecting to serial port
    if (llopen(connectionParams))
    {
        printf("Time exceeded!\n");
        printf("Failed to connect\n");
        return;
    }
    printf("Connected\n");
    start = time(NULL);
}
