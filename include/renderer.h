// Renderer.h
#ifndef RENDERER_H
#define RENDERER_H

// Includes
#include "stdint.h"

// Defines
#define SCALE 10
#define FPS 60

// Fns
void renderer_init(void);
double renderer_draw(const uint8_t *video);
void renderer_audio_update(uint8_t sound_timer);
int renderer_input(uint8_t *keypad);

#endif
