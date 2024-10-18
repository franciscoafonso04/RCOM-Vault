#include "tools.h"

extern int alarmEnabled, alarmCount;

void insert(int arr[], int *n, int value, int pos) {
    
    if (pos < 0 || pos >= *n) {
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
    alarmEnabled = 0;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

void remove(int arr[], int *n, int pos) {
    
    if (pos < 0 || pos >= *n) {
        printf("Invalid position!\n");
        return;
    }

    for (int i = pos; i < *n-1; i++) {
        arr[i] = arr[i+1];
    }

    (*n)--;
}

int writeResponse(int rr, int iFrame){

    unsigned char buf[6] = {0};

    buf[0] = FLAG;
    buf[1] = A_RX;

    if (rr == 1){ // TRUE
        if (iFrame == 0){
            buf[2] = C_RR0;
        }    
        else if (iFrame == 1){
            buf[2] = C_RR1;
        }
    }
    else if (rr == 0){ // FALSE
        if (iFrame == 0){
            buf[2] = C_REJ0;
        }    
        else if (iFrame == 1){
            buf[2] = C_REJ1;
        }
    }

    buf[3] = buf[1] ^ buf[2];
    buf[4] = FLAG;

    return writeBytesSerialPort(*buf, 6);
}