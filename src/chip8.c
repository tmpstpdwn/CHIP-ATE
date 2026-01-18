/* [[INCLUDES]] */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "chip8.h"

/* [[VAR DCL - DEF]] */

// CHIP-8 internals
uint8_t video[VIDEO_HEIGHT * VIDEO_WIDTH];
uint8_t keypad[16];

static uint8_t registers[16];
static uint8_t memory[MEM_SIZE];
static uint16_t index_;
static uint16_t pc;
static uint16_t stack[16];
static uint8_t sp;
uint8_t delay_timer;
uint8_t sound_timer;
static uint16_t opcode;

// Fn tables
static chip8_fn table[T_MAX + 1];
static chip8_fn table0[T0_MAX + 1];
static chip8_fn table8[T8_MAX + 1];
static chip8_fn tableE[TE_MAX + 1];
static chip8_fn tableF[TF_MAX + 1];

// Fontset
static uint8_t fontset[FONTSET_SIZE] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F  
};

/* [[FN DCL]] */

// Load rom and fonts
void chip8_init(const char *file_name);
static void load_rom(const char *file_name);
static void load_fonts(void);

// Setup fn dispatch tables
static void setup_tables(void);
static void table0_fn(void);
static void table8_fn(void);
static void tableE_fn(void);
static void tableF_fn(void);

// Operations
static void OP_NULL(void);

static void OP_00E0(void);
static void OP_00EE(void);
static void OP_1NNN(void);
static void OP_2NNN(void);
static void OP_3XKK(void);
static void OP_4XKK(void);
static void OP_5XY0(void);
static void OP_6XKK(void);
static void OP_7XKK(void);

static void OP_8XY0(void);
static void OP_8XY1(void);
static void OP_8XY2(void);
static void OP_8XY3(void);
static void OP_8XY4(void);
static void OP_8XY5(void);
static void OP_8XY6(void);
static void OP_8XY7(void);
static void OP_8XYE(void);

static void OP_9XY0(void);

static void OP_ANNN(void);
static void OP_BNNN(void);
static void OP_CXKK(void);
static void OP_DXYN(void);
static void OP_EX9E(void);
static void OP_EXA1(void);

static void OP_FX07(void);
static void OP_FX0A(void);
static void OP_FX15(void);
static void OP_FX18(void);
static void OP_FX1E(void);
static void OP_FX29(void);
static void OP_FX33(void);
static void OP_FX55(void);
static void OP_FX65(void);

// Cycle : fetch, decode and execute
void chip8_cycle(void);

/* [[FN DEF]] */

void setup_tables(void) {

  table[0x0] = table0_fn;
  table[0x1] = OP_1NNN;
  table[0x2] = OP_2NNN;
  table[0x3] = OP_3XKK;
  table[0x4] = OP_4XKK;
  table[0x5] = OP_5XY0;
  table[0x6] = OP_6XKK;
  table[0x7] = OP_7XKK;
  table[0x8] = table8_fn;
  table[0x9] = OP_9XY0;
  table[0xA] = OP_ANNN;
  table[0xB] = OP_BNNN;
  table[0xC] = OP_CXKK;
  table[0xD] = OP_DXYN;
  table[0xE] = tableE_fn;
  table[0xF] = tableF_fn;

  for (size_t i = 0; i <= 0xE; i++) {
    table0[i] = OP_NULL;
    table8[i] = OP_NULL;
    tableE[i] = OP_NULL;
  }

  table0[0x0] = OP_00E0;
  table0[0xE] = OP_00EE;
  
  table8[0x0] = OP_8XY0;
  table8[0x1] = OP_8XY1;
  table8[0x2] = OP_8XY2;
  table8[0x3] = OP_8XY3;
  table8[0x4] = OP_8XY4;
  table8[0x5] = OP_8XY5;
  table8[0x6] = OP_8XY6;
  table8[0x7] = OP_8XY7;
  table8[0xE] = OP_8XYE;

  tableE[0x1] = OP_EXA1;
  tableE[0xE] = OP_EX9E;

  for (size_t i = 0; i <= 0x65; i++) {
    tableF[i] = OP_NULL;
  }

  tableF[0x07] = OP_FX07;
  tableF[0x0A] = OP_FX0A;
  tableF[0x15] = OP_FX15;
  tableF[0x18] = OP_FX18;
  tableF[0x1E] = OP_FX1E;
  tableF[0x29] = OP_FX29;
  tableF[0x33] = OP_FX33;
  tableF[0x55] = OP_FX55;
  tableF[0x65] = OP_FX65;
}

void table0_fn(void) {
  uint8_t index = opcode & 0x000Fu;
  if (index <= T0_MAX) {
    table0[index]();
  } else {
    fprintf(stderr, "Error: Unknown 0x0 opcode: 0x%X\n", opcode);
    exit(1);
  }
}

void table8_fn(void) {
  uint8_t index = opcode & 0x000Fu;
  if (index <= T8_MAX) {
    table8[index]();
  } else {
    fprintf(stderr, "Error: Unknown 0x8 opcode: 0x%X\n", opcode);
    exit(1);
  }
}

void tableE_fn(void) {
  uint8_t index = opcode & 0x000Fu;
  if (index <= TE_MAX) {
    tableE[index]();
  } else {
    fprintf(stderr, "Error: Unknown 0xE opcode: 0x%X\n", opcode);
    exit(1);
  }
}

void tableF_fn(void) {
  uint8_t index = opcode & 0x00FFu;
  if (index <= TF_MAX) {
    tableF[index]();
  } else {
    fprintf(stderr, "Error: Unknown 0xF opcode: 0x%X\n", opcode);
    exit(1);
  }
}

void load_rom(const char *file_name) {
  int byte;

  FILE *file = fopen(file_name, "rb");
  if (file == NULL) {
    perror("Error opening rom!");
    exit(1);
  }

  int i = 0;
  while (i < MEM_SIZE && (byte = fgetc(file)) != EOF) {
    memory[START_ADDRESS + i] = byte;
    i++;
  }

  if (i >= MEM_SIZE) {
    printf("Warning: ROM file is too large, only the first 4096 bytes will be loaded.\n");
  }

  fclose(file);
}

void load_fonts(void) {
  for (int i = 0; i < FONTSET_SIZE; i++) {
    memory[FONTSET_START_ADDRESS + i] = fontset[i];
  }
}

void OP_NULL(void) {
  
}

void OP_00E0(void) {
  memset(video, 0, sizeof(video));
}

void OP_00EE(void) {
  if (sp <= 0) {
    fprintf(stderr, "Error: Stack underflow!\n");
    exit(1);
  }
  sp--;
  pc = stack[sp];
}

void OP_1NNN(void) {
  pc = opcode & 0x0FFFu;
}

void OP_2NNN(void) {
  if (sp >= 16) {
    fprintf(stderr, "Error: Stack overflow!\n");
    exit(1);
  }
  stack[sp++] = pc;
  pc = opcode & 0x0FFFu;
}

void OP_3XKK(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t byte = opcode & 0x00FFu;
  if (registers[Vx] == byte) {
    pc += 2;
  }
}

void OP_4XKK(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t byte = opcode & 0x00FFu;
  if (registers[Vx] != byte) {
    pc += 2;
  }
}

void OP_5XY0(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  if (registers[Vx] == registers[Vy]) {
    pc += 2;
  }
}

void OP_6XKK(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t byte = opcode & 0x00FFu;
  registers[Vx] = byte;
}

void OP_7XKK(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t byte = opcode & 0x00FFu;
  registers[Vx] += byte;
}

void OP_8XY0(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  registers[Vx] = registers[Vy];
}

void OP_8XY1(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  registers[Vx] |= registers[Vy];
}

void OP_8XY2(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  registers[Vx] &= registers[Vy];
}

void OP_8XY3(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  registers[Vx] ^= registers[Vy];
}

void OP_8XY4(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  uint16_t sum = registers[Vx] + registers[Vy];

  if (sum > 0xFFu) {
    registers[0xF] = 1;
  } else {
    registers[0xF] = 0;
  }

  registers[Vx] = sum & 0xFFu;
}

void OP_8XY5(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  if (registers[Vx] > registers[Vy]) {
    registers[0xF] = 1;
  } else {
    registers[0xF] = 0;
  }

  registers[Vx] -= registers[Vy];
}

void OP_8XY6(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  registers[0xF] = registers[Vy] & 0x1u;
  registers[Vx] = registers[Vy] >> 1;
}

void OP_8XY7(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  if (registers[Vy] > registers[Vx]) {
    registers[0xF] = 1;
  } else {
    registers[0xF] = 0;
  }

  registers[Vx] = registers[Vy] - registers[Vx];
}

void OP_8XYE(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  registers[0xF] = (registers[Vy] & 0x80u) >> 7u;
  registers[Vx] = registers[Vy] << 1;
}

void OP_9XY0(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  if (registers[Vx] != registers[Vy]) {
    pc += 2;
  }
}

void OP_ANNN(void) {
  index_ = opcode & 0x0FFFu;
}

void OP_BNNN(void) {
  uint16_t address = opcode & 0x0FFFu;
  pc = address + registers[0x0];
}

void OP_CXKK(void) {
  uint8_t Vx = (opcode & 0x0F00) >> 8u;
  uint8_t byte = opcode & 0x00FFu;
  registers[Vx] = rand() & byte;
}

void OP_DXYN(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  uint8_t height = (opcode & 0x000Fu);

  uint8_t x_pos = registers[Vx] % VIDEO_WIDTH;
  uint8_t y_pos = registers[Vy] % VIDEO_HEIGHT;

  registers[0xF] = 0;
  for (unsigned int row = 0; row < height; row++) {
    uint8_t sprite_byte = memory[index_ + row];
  
    for (unsigned int col = 0; col < 8; col++) {
      if ((sprite_byte & (0x80u >> col))) {
        uint16_t x_coord = x_pos + col;
        uint16_t y_coord = y_pos + row;

        if (x_coord < VIDEO_WIDTH && y_coord < VIDEO_HEIGHT) {
          uint8_t *screen_pixel = &video[y_coord * VIDEO_WIDTH + x_coord];

          if (*screen_pixel) {
            registers[0xF] = 1; // Collision
          }
          *screen_pixel ^= 0xFF;
        }
      }
    }

  }

}

void OP_EX9E(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t key = registers[Vx];
  if (keypad[key]) {
    pc += 2;
  }
}

void OP_EXA1(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t key = registers[Vx];
  if (!keypad[key]) {
    pc += 2;
  }
}

void OP_FX07(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  registers[Vx] = delay_timer; 
}

void OP_FX0A(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  for (uint8_t i = 0; i < 16; i++) {
    if (keypad[i]) {
      registers[Vx] = i;
      return;
    }
  }

  pc -= 2;
}

void OP_FX15(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  delay_timer = registers[Vx];
}

void OP_FX18(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  sound_timer = registers[Vx];
}

void OP_FX1E(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  index_ += registers[Vx];
}

void OP_FX29(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t digit = registers[Vx];
  index_ = FONTSET_START_ADDRESS + (5 * digit);
}

void OP_FX33(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t value = registers[Vx];

  memory[index_ + 2] = value % 10;
  value /= 10;

  memory[index_ + 1] = value % 10;
  value /= 10;

  memory[index_] = value % 10;
  value /= 10;
}

void OP_FX55(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  for (uint8_t i = 0; i <= Vx; i++) {
    memory[index_ + i] = registers[i];
  }
}

void OP_FX65(void) {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  for (uint8_t i = 0; i <= Vx; i++) {
    registers[i] = memory[index_ + i];
  }
}

void chip8_init(const char *file_name) {
  load_rom(file_name);
  load_fonts();
  setup_tables();
  pc = START_ADDRESS;
}

void chip8_cycle(void) {
  opcode = (memory[pc] << 8u) | memory[pc + 1];

  printf("PC: %X OP %X\n", pc, opcode);

  pc += 2;

  table[(opcode & 0xF000u) >> 12u]();
}

/* [[END]] */
