/* [[INCLUDES]] */

#include "chip8.h"
#include "renderer.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define CLOCK 400

/* [[VAR DCL - DEF]] */

int run = 1;
double cpu_accumulator = 0.0;
double timer_accumulator = 0.0;
double time_per_cpu_cycle; 
const double time_per_timer_tick = 1.0 / 60.0;

/* [[MAIN]] */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path to .ch8 rom> [clock]\n", argv[0]);
        return 1;
    }

    char *rom_path = NULL;
    int clock_speed;

    // Args check
    if (argc >= 3) {
        clock_speed = atoi(argv[2]);
        rom_path = argv[1];
        if (!clock_speed) clock_speed = CLOCK;
    } else {
        rom_path = argv[1];
        clock_speed = CLOCK;
    }

    time_per_cpu_cycle = 1.0 / clock_speed;

    chip8_init(rom_path);
    renderer_init();

    double dt = 0;

    while (run) {
        cpu_accumulator += dt;
        timer_accumulator += dt;

        run = renderer_input(keypad);

        while (cpu_accumulator >= time_per_cpu_cycle) {
            chip8_cycle();
            cpu_accumulator -= time_per_cpu_cycle;
        }

        if (timer_accumulator >= time_per_timer_tick) {
            if (delay_timer > 0) delay_timer--;
            if (sound_timer > 0) sound_timer--;
        
            renderer_audio_update(sound_timer);
            timer_accumulator -= time_per_timer_tick;
        }

        dt = renderer_draw(video);
    }

    return 0;
}

/* [[END]] */
