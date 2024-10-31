// States header file.

#ifndef _STATE_MACHINES_H_
#define _STATE_MACHINES_H_

#include "tools.h"

int openStateMachine(State *state, unsigned char *buf, LinkLayerRole role);
int writeStateMachine();
int readStateMachine(unsigned char *packet);
unsigned char discStateMachine();
unsigned char uaStateMachine();

#endif // _STATE_MACHINES_H_
