// States header file.

#ifndef _STATES_H_
#define _STATES_H_

#include "tools.h"

int openStateMachine(State state, unsigned char *buf, LinkLayerRole role);
int writeStateMachine();
unsigned char readStateMachine(unsigned char *packet);
unsigned char discStateMachine();
unsigned char uaStateMachine();

#endif // _STATES_H_
