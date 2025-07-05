/** @file cheats.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef CHEATS_H
#define CHEATS_H

#include <SDL3/SDL.h>

#include "utils.h"

void add_to_ring_buffer(button_t button);
void clear_ring_buffer(void);
bool find_sequence(const button_t *sequence, int sequence_length);

#endif // CHEATS_H
