#ifndef DISPLAY_H
#define DISPLAY_H

#include "fridge.h"

void display_init(void);
void display_update(const fridge_state_t *state);

#endif 
