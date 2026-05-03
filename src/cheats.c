/** @file cheats.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "SDL3/SDL.h"

#include "cheats.h"
#include "utils.h"

#define BUFFER_SIZE 15

static button_t sequence_ring_buffer[BUFFER_SIZE] = { 0 };
static int current_index = 0;

void add_to_ring_buffer(button_t button)
{
    sequence_ring_buffer[current_index] = button;
    current_index = (current_index + 1) % BUFFER_SIZE;
}

void clear_ring_buffer(void)
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        sequence_ring_buffer[i] = 0;
    }
    current_index = 0;
}

bool find_sequence(const button_t *sequence, int sequence_length)
{
    if (sequence_length > BUFFER_SIZE)
    {
        return false;
    }

    // The most recently added entry is at (current_index - 1).
    // Walk backwards through the ring buffer and compare against the
    // sequence in reverse, so the sequence must end exactly at the
    // last written position.
    for (int j = 0; j < sequence_length; j++)
    {
        int buf_pos = (current_index - 1 - j + BUFFER_SIZE) % BUFFER_SIZE;
        if (sequence_ring_buffer[buf_pos] != sequence[sequence_length - 1 - j])
        {
            return false;
        }
    }
    return true;
}
