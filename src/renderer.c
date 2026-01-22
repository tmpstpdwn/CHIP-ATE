/* [[INCLUDES]] */

#include "chip8.h"
#include "renderer.h"
#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>

/* [[VAR DCL - DEF]] */

static const int screen_width = VIDEO_WIDTH * SCALE;
static const int screen_height = VIDEO_HEIGHT * SCALE;
static const char *title = "Chip-8";
static Sound beep_sound;

static const int key_map[16] = {
    KEY_X,     // 0
    KEY_ONE,   // 1
    KEY_TWO,   // 2
    KEY_THREE, // 3
    KEY_Q,     // 4
    KEY_W,     // 5
    KEY_E,     // 6
    KEY_A,     // 7
    KEY_S,     // 8
    KEY_D,     // 9
    KEY_Z,     // A
    KEY_C,     // B
    KEY_FOUR,  // C
    KEY_R,     // D
    KEY_F,     // E
    KEY_V      // F
};

/* [[FN DCL]] */

void renderer_init(void);
static void renderer_init_audio(void);
double renderer_draw(const uint8_t *video);
void renderer_audio_update(uint8_t sound_timer);
bool renderer_input(uint8_t *keypad);

/* [[FN DEF]] */

void renderer_init(void) {
  InitWindow(screen_width, screen_height, title);
  SetExitKey(KEY_ESCAPE);
  SetTargetFPS(FPS);
  renderer_init_audio();
}

static void renderer_init_audio(void) {
  InitAudioDevice();

  int samples = 44100 * 0.5f; // 0.5 seconds of the audio
  float *data = (float *)malloc(samples * sizeof(float));

  int period = 44100 / 440; // Samples per wave cycle (440Hz)
  for (int i = 0; i < samples; i++) {
    data[i] = ((i / (period / 2)) % 2 == 0) ? 0.2f : -0.2f; // 0.2f volume
  }

  Wave wave = {
    .frameCount = samples,
    .sampleRate = 44100,
    .sampleSize = 32,
    .channels = 1,
    .data = data
  };

  beep_sound = LoadSoundFromWave(wave);
  
  free(data); 
}

double renderer_draw(const uint8_t *video) {
  BeginDrawing();
  ClearBackground(BLACK);
  
  for (int y = 0; y < VIDEO_HEIGHT; y++) {
    for (int x = 0; x < VIDEO_WIDTH; x++) {
      int pixel = video[y * VIDEO_WIDTH + x];
      if (pixel) {
        DrawRectangle(x * SCALE, y * SCALE, SCALE, SCALE, WHITE);
      }
    }
  }

  EndDrawing();
  return GetFrameTime();
}

void renderer_audio_update(uint8_t sound_timer) {
  if (sound_timer > 0) {
    if (!IsSoundPlaying(beep_sound)) {
      PlaySound(beep_sound);
    }
  } else {
    if (IsSoundPlaying(beep_sound)) {
      StopSound(beep_sound);
    }
  }
}

bool renderer_input(uint8_t *keypad) {
  if (WindowShouldClose()) return false;
  for (int i = 0; i < 16; i++) {
    keypad[i] = IsKeyDown(key_map[i]);
  }
  return true;
}

/* [[END]] */
