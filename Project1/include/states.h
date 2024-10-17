// States header file.

#ifndef _STATES_H_
#define _STATES_H_

#include <stdio.h>
#include "tools.h"
#include "link_layer.h"

int openStateMachine(State state, unsigned char *buf, LinkLayer connectionParameters);
int writeStateMachine(int Ns);
#endif // _STATES_H_
