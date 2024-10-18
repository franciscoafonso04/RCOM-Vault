// States header file.

#ifndef _STATES_H_
#define _STATES_H_

#include <stdio.h>
#include "tools.h"
#include "link_layer.h"

int openStateMachine(State state, unsigned char *buf, LinkLayer connectionParameters);
int writeStateMachine();
unsigned char readStateMachine(unsigned char *packet);

#endif // _STATES_H_
