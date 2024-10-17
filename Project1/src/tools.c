#include "tools.h"

void insert(int arr[], int *n, int value, int pos) {
    // First, make sure pos is a valid index within the array
    if (pos < 0 || pos > *n) {
        printf("Invalid position!\n");
        return;
    }
    
    // Shift elements to the right from the end to the position
    for (int i = *n; i > pos; i--) {
        arr[i] = arr[i - 1];
    }

    // Insert the new value at the position
    arr[pos] = value;

    // Increase the size of the array (if using dynamic allocation)
    (*n)++;
}