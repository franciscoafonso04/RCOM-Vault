# Data Link Protocol
- First Lab Report
- FEUP - Computer Networks
- Alexandre Ramos - up202208028
- Francisco Afonso - up202208115
  
# Summary

This report documents the work developed for the first laboratory of the computer networks course (course?). 
The objective of the project is to transfer data between 2 machines by a layered application approach and the Stop & Wait protocol.

We achieved our goals, as the application successfully sends the file while having the robustness to withstand noise and temporary termination of the connection betwwen the two machines

# Introduction

In this report, we aim to provide insight into the structure, implementation and testing of the project, as well as performance and efficiency statistics.
The objective of the project is to develop an application using the C programming language that transfers files between 2 linux machines connected through a RS-232 serial cable. 
This is achieved by a layered application approach and the Stop & Wait protocol.

The report is stuctured as follows:

1. Architecture
   - Functional blocks and interfaces.
2. Code Structure
   - APIs.
   - Main data structures.
   - Main functions and their relationship with the architecture.
3. Main Use Cases
   - Identification.
   - Function call sequences.
4. Logical Link Protocol
   - Identification of the main functional aspects.
   - Description of the implementation strategy of these aspects with code excerpts.
5. Application Protocol
   - Identification of the main functional aspects.
   - Description of the implementation strategy of these aspects with code excerpts.
6. Validation
   - Description of the tests performed with quantified presentation of the results.
7. Data Link Protocol Efficiency
   - Statistical characterization of the protocol's efficiency. 
   - Theoretical characterization of a Stop & Wait protocol.
8. Conclusions
   - Synthesis of the information presented in the previous sections.
   - Reflection on the learning objectives achieved.

# Architecture

The application is divided in the Application Layer, Link Layer and the Serial Port Layer. We were tasked with implementing the first two.

## Application Layer

The application layer is the highest level of abstraction, performing the tasks in the most superficial way. 
It is responsible for connecting and disconnecting the two machines, as well as opening, sending, receiving and closing the file to be transfered. 

## Link Layer

The Link Layer receives orders from the application layer and executes them.
It is responsible for applying the Stop & Wait protocol and assuring data is correcly trasnfered between the 2 machines.

# Code Structure

The code we implemented is structured as four files and their respective headers.

## application_layer.c

### Function

```c
void applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout const char *filename);

// Arguments:
// serialPort: Serial port name (e.g., /dev/ttyS0).
// role: Application role {"tx", "rx"}.
// baudrate: Baudrate of the serial port.
// nTries: Maximum number of frame retries.
// timeout: Frame timeout.
// filename: Name of the file to send / receive.
```

### Global Variables

```c
int framesSent = 0; // total number of frames sent, used for statistics
int alarmTotalCount = 0; // total number of alarms set off, used for statistics
int rejCount = 0; // total number of rejection commands sent, used for statistics
extern long fileSize; // total size of the file in bytes, used for statistics
extern time_t delta; // total time passed between the connection and disconnection phases, used for statistics
extern int nRej; // number of consecutive rejection commands sent, used for terminating the program if necessary
```

## link_layer.c

### Functions

```c
// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);
```

### Global Variables

```c
extern int alarmEnabled, alarmCount, alarmTotalCount, rejCount;
extern int iFrame;
extern int framesSent;
extern long fileSize;
extern time_t delta;
int timeout, nTries;
LinkLayerRole role;
```

### Macros

```c
// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1
```

### Structures

```c
typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;
```

## state_machines.c

### Functions

```c
int openStateMachine(State *state, unsigned char *buf, LinkLayerRole role);
int writeStateMachine();
int readStateMachine(unsigned char *packet);
unsigned char discStateMachine();
unsigned char uaStateMachine();
```

### Global Variables

```c
extern int iFrame;
extern int alarmEnabled;
```

### Macros

```c

```

### Structures

```c

```

## tools.c

### Functions

```c
 
```

### Global Variables

```c
 
```

### Macros

```c

```

### Structures

```c

```

# Main use Cases



# Logical Link Protocol



# Application Protocol



# Validation



# Data Link Protocol Eefficiency



# Conclusions



# Appendix - Source Code