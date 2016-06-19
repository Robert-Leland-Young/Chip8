#ifndef CHIP_8_H
#define CHIP_8_H

#include <stdint.h>

class Chip8 {
private:
    uint16_t stack[16];                 // Stack
    uint16_t sp;                        // Stack pointer

    uint8_t memory[4096];               // Memory (4k)
    uint8_t V[16];                      // V registers (V0-VF)
	uint8_t RPL[8];						// Chip-48 Register V0-V7 Store Bank

    uint16_t pc;                        // Program counter
    uint16_t opcode;                    // Current op code
    uint16_t I;                         // Index register

    uint8_t delay_timer;                // Delay timer
    uint8_t sound_timer;                // Sound timer

    void init();

public:
	// Graphics buffer Low Resolution 64x32 High Resolution 128x64
  //uint8_t  gfx[64 * 32];              
	uint8_t  gfx[128 * 64];				// Always reserve for High Resolution
	bool	 gfx_mode;					// True High Resolution, False Low Resolution
	unsigned  gfx_size = 128 * 64;		// Graphics Buffer Size
	unsigned  gfx_x;					// Graphics X coordinate size
	unsigned  gfx_y;					// Graphics Y coordinate size

	uint8_t  key[16];                   // Keypad
    bool drawFlag;                      // Indicates a draw has occurred

    Chip8();
    ~Chip8();

    void emulate_cycle();               // Emulate one cycle
										// Load application & Debugger
	bool load(const char *file_path, unsigned char *debug_out, int debug); 


	void trace(char *ins);				// Debug Instruction Trace
	int DEBUG;							// Debug Flag

};

#endif // CHIP_8_H
