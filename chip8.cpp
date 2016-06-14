#include "stdafx.h"
#include "chip8.h"
#include <iostream>
#include <chrono>
#include "SDL.h"      /* SDL 2*/
#include <conio.h>


unsigned int  addr;          /* Instruction Address */
unsigned char msb, lsb;      /* Instruction High & Low order bytes */
unsigned char msbh, msbl;    /* Instruction MSB High & Low order Nibbles */
unsigned char lsbh, lsbl;    /* Instruction LSB High & Low order Nibbles */
char ins[128];               /* Debug Instruction Description */
FILE *fout;					 /* Debug output File descripter */
unsigned int stop;			 /* Debug Stop/Breakpoint Address */

unsigned char chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};


void Chip8::trace(char *ins)  /* Output Instructuction Trace */
{
  int i,j;
  char cmd[80];

  if (DEBUG > 0) { /* If Debug Trace is active */

		fprintf(fout, "\r\n%04X  %02X %02X    %s\r\n\r\n", addr, msb, lsb, ins);  /* Debug Trace Instruction Address & OpCode*/

		for (i = 0; i < 8; i++) { /* Display Register 0-7 Content */
			fprintf(fout, "V%X:%-02.2X  ", i, V[i]);
		} /* end, Display Register 0-7 Content */
		fprintf(fout, "\r\n");

		for (i = 8; i < 16; i++) { /* Display Register 8-15 Content */
			fprintf(fout, "V%X:%-02.2X  ", i, V[i]);
		} /* end, Display Register 8-15 Content */
		fprintf(fout, "\r\n");

		fprintf(fout, "I:%-04.4X  SP:%-2.2X  Delay:%-4.4X  Sound: %-4.4X\r\n", I, sp, delay_timer, sound_timer);

		fprintf(fout, "\r\n");

  } /* end, If Debug Trace is active */
	
  if (0 != _kbhit() || DEBUG==2 || stop==addr) { /* Debug Keyboard Input */
	  char *ptr;

	  stop = 0;  /* Clear the breakpoint */

	  if (DEBUG==0) printf("Debug Mode is OFF\r\n\r\n");
	  if (DEBUG==1) printf("Debug Mode is ON\r\n\r\n");
	  if (DEBUG == 2) printf("Debug Single Step Mode\r\n\r\n");

	  printf("Execution Paused\r\n\r\n");
menu:
	  printf("Command Menu\r\n");
	  printf("ON   - Turn Continuous Trace ON\r\n");
	  printf("OFF  - Turn Continuous Trace OFF\r\n");
	  printf("STEP - Single Step Mode\r\n");
	  printf("STOP - Stop Once at Address\r\n");
	  printf("GO   - Continue Execution\r\n");
	  printf("DUMP - Display Memory in Hex\r\n");
	  printf("Command: ");
	
	  scanf("%s", &cmd);	/* Check for Keyboard Input from the Debug Window */

	/* Convert command to uppercase */
	for (ptr = cmd; ptr < cmd + strlen(cmd); ptr++)
		if (islower(*ptr)) *ptr=_toupper(*ptr);

	if (0==strcmp("ON", cmd)) { DEBUG = 1; }
	if (0 == strcmp("OFF", cmd)) { DEBUG = 0; }
	if (0 == strcmp("STEP", cmd)) { DEBUG = 2; }
	if (0 == strcmp("DUMP",cmd)) { /* Dump */
		printf("Hex: Address Count? ");
		scanf("%X %X", &i, &j);
		printf("\r\n");
		j = j + i;  /* J is the stop address */
		if (j > 4096) j = 4096; /* Enforce Memory Limits */
		fprintf(fout,"Memory Dump\r\n%04.4X  ", i);
		for (int k=0; i < j; i++, k++) { /* 16 byte Rows */
			fprintf(fout,"%02.2X ", memory[i]);
			if (k >= 15) { /* New Row */
				k = -1; 
				if (i+1<j) fprintf(fout,"\r\n%04.4X  ", i+1); 
				} /* end, New Row */
		} /* end, 16 Byte Rows */
		fprintf(fout,"\r\n\r\n");
		goto menu;
	} /* end Dump */
	if (0 == strcmp("STOP", cmd)) { /* Stop */
		printf("Hex: Stop Address? ");
		scanf("%X", &stop);
		printf("\r\n");
		goto menu;
	} /* end Stop */

  } /* end Debug Keyboard Input */
}



Chip8::Chip8() {}
Chip8::~Chip8() {}

// Initialise
void Chip8::init() {
    pc      = 0x200;    // Set program counter to 0x200
    opcode  = 0;        // Reset op code
    I     = 0;          // Reset I
    sp      = 0;        // Reset stack pointer

    // Clear the display
    for (int i = 0; i < 2048; ++i) {
        gfx[i] = 0;
    }

    // Clear the stack, keypad, and V registers
    for (int i = 0; i < 16; ++i) {
        stack[i]    = 0;
        key[i]      = 0;
        V[i]        = 0;
    }

    // Clear memory
    for (int i = 0; i < 4096; ++i) {
        memory[i] = 0;
    }

    // Load font set into memory
    for (int i = 0; i < 80; ++i) {
        memory[i] = chip8_fontset[i];
    }

    // Reset timers
    delay_timer = 0;
    sound_timer = 0;

    // Seed rng
    srand ((unsigned)time(NULL));
}

// Initialise and load ROM into memory
bool Chip8::load(const char *file_path, unsigned char *debug_out, int debug) {
    // Initialise
    init();

    printf("Loading ROM: %s\r\n\r\n", file_path);

    // Open ROM file
    FILE* rom = fopen(file_path, "rb");
    if (rom == NULL) {
        std::cerr << "Failed to open ROM" << std::endl;
        return false;
    }

    // Get file size
    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    // Allocate memory to store rom
    char* rom_buffer = (char*) malloc(sizeof(char) * rom_size);
    if (rom_buffer == NULL) {
        std::cerr << "Failed to allocate memory for ROM" << std::endl;
        return false;
    }

    // Copy ROM into buffer
    size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, rom);
    if (result != rom_size) {
        std::cerr << "Failed to read ROM" << std::endl;
        return false;
    }

    // Copy buffer to memory
    if ((4096-512) > rom_size){
        for (int i = 0; i < rom_size; ++i) {
            memory[i + 512] = (uint8_t)rom_buffer[i];   // Load into memory starting
                                                        // at 0x200 (=512)
        }
    }
    else {
        std::cerr << "ROM too large to fit in memory" << std::endl;
        return false;
    }

    // Clean up
    fclose(rom);
    free(rom_buffer);

	/* Initialize the Debugger */
	
	/* Set output file pointer if no output file is specified */
	if (debug < 1 || *debug_out==NULL) { /* Default stdout */
		fout = stdout;
	}
	else {

		/* Try to open the output file */
		fout = fopen((char *)debug_out, "wb");
		if (NULL == fout) { /* failed to open Output file */
			printf("Failed to open Output: %s\n", debug_out);
			return(false);
		}
	}
	Chip8::DEBUG = debug;    // Inmitialize the Debugger
    return true;
}

// Emulate one cycle
void Chip8::emulate_cycle() {

	
	addr = pc;					/* Get Instruction Address */
	msb = memory[pc];			/* High order Byte of Instruction */
	lsb = memory[pc+1];			/* Low order Byte of Instruction */
	msbh = msb >> 4;			/* Shift out lower Nibble of MSB */
	msbl = 0x0F & msb;			/* Mask out upper Nibble of MSB */
								/*  Get LSB Nibbles */
	lsbh = lsb >> 4;			/* Shift out lower Nibble of LSB */
	lsbl = 0x0F & lsb;			/* Mask out upper Nibble of LSB */


	// Fetch op code
    opcode = memory[pc] << 8 | memory[pc + 1];   // Op code is two bytes

    switch(opcode & 0xF000){

        // 0 Upper Nibble Opcode
        case 0x0000:

			if (lsbh == 0xC) { /* 00CN OpCode*/
				/* Chip-48 OpCode */
				sprintf(ins, "SCD   #%1X             ' Scroll Down #%1X Lines", lsbl, lsbl);
				trace(ins);  /* Debug Trace */
				
				
				break;
			} /* end, 00CN OpCode */

            switch (opcode & 0x00FF) { /* 0 Upper byte switch */
				
				// 00E0 - Clear screen
                case 0x00E0:

					strcpy(ins,"CLS                  ' Clear Screen"); /* Clear Screen */
					trace(ins);  /* Debud Trace */

					for (int i = 0; i < 2048; ++i) {
                        gfx[i] = 0;
                    }
                    drawFlag = true;
                    pc+=2;
                    break;

                // 00EE - Return from subroutine
                case 0x00EE:

					strcpy(ins,"RET                  ' Return"); /* Return from Subroutine */
					trace(ins);   /* Debug Trace */

					--sp;
                    pc = stack[sp];
                    pc += 2;
                    break;

				case 0x00FB:
					/* Chip-48 OpCode */

					sprintf(ins, "SCR                  ' Scroll 4 Pixels Right"); 
					trace(ins);   /* Debug Trace */


					break;

				case 0x00FC:
					/* Chip-48 OpCode */

					sprintf(ins, "SCL                  ' Scroll 4 Pixels Left");
					trace(ins);   /* Debug Trace */


					break;

				case 0x00FD:
					/* Chip-48 OpCode */
					
					sprintf(ins, "EXIT                 ' Exit Interpreter");
					trace(ins);   /* Debug Trace */


					break;

				case 0x00FE:
					/* Chip-48 OpCode */

					sprintf(ins, "LOW                  ' Disable Extended Graphics");
					trace(ins);   /* Debug Trace */


					break;

				case 0x00FF:
					/* Chip-48 OpCode */

					sprintf(ins, "HIGH                 ' Enable Extended Graphics");
					trace(ins);   /* Debug Trace */


					break;



                default:
                    printf("\nUnknown op code: %.4X\r\n", opcode);
					pc += 2;
					//   exit(3);
            } /* end, 0 Upper byte switch */
            break;

        // 1NNN - Jumps to address NNN
        case 0x1000:

			sprintf(ins, "JMP   #%1X%02X           ' JUMP to #%1X%02X", msbl, lsb, msbl, lsb); /* Jump to ADDRESS */
			trace(ins);
			
			pc = opcode & 0x0FFF;
            break;

        // 2NNN - Calls subroutine at NNN
        case 0x2000:

			sprintf(ins, "CALL  #%1X%02X           ' Call Subroutine", msbl, lsb); /* Call Subroutine */
			trace(ins);
			
			stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;

        // 3XNN - Skips the next instruction if VX equals NN.
        case 0x3000:

			sprintf(ins, "SE    V%1X,#%02X         ' Skip Next OP if V%1X=#%02X", msbl, lsb, msbl, lsb);
			trace(ins);
			
			if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
            break;

        // 4XNN - Skips the next instruction if VX does not equal NN.
        case 0x4000:

			sprintf(ins, "SNE   V%1X,#%02X         ' Skip Next OP if V%1X!=#%02X", msbl, lsb, msbl, lsb);
			trace(ins);
			
			if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
            break;

        // 5XY0 - Skips the next instruction if VX equals VY.
        case 0x5000:

			sprintf(ins, "SE    V%1X,V%1X          ' Skip Next OP if V%1X=V%1X", msbl, lsbh, msbl, lsbh);
			trace(ins);
			
			if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;
            break;

        // 6XNN - Sets VX to NN.
        case 0x6000:

			sprintf(ins, "LD    V%1X,#%02X         ' Load V%1X with #%1X", msbl, lsb, msbl, lsb);
			trace(ins);
			
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;

        // 7XNN - Adds NN to VX.
        case 0x7000:

			sprintf(ins, "ADD   V%1X,#%02X         ' Set V%1X = V%1X + #%02X", msbl, lsb, msbl, msbl, lsb);
			trace(ins);
			
			V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;

        // 8XY_
        case 0x8000:
            switch (opcode & 0x000F) {

                // 8XY0 - Sets VX to the value of VY.
                case 0x0000:
 
					sprintf(ins, "LD    V%1X,V%1X          ' Load V%1X with V%1X", msbl, lsbh, msbl, lsbh);
					trace(ins);
					
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                // 8XY1 - Sets VX to (VX OR VY).
                case 0x0001:
                    
					sprintf(ins, "OR    V%1X,V%1X          ' V%1X = V%1X OR V%1X", msbl, lsbh, msbl, msbl, lsbh);
					trace(ins);

					V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                // 8XY2 - Sets VX to (VX AND VY).
                case 0x0002:
                    
					sprintf(ins, "AND   V%1X,V%1X          ' V%1X = V%1X AND V%1X", msbl, lsbh, msbl, msbl, lsbh);
					trace(ins);

					V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                // 8XY3 - Sets VX to (VX XOR VY).
                case 0x0003:
                    
					sprintf(ins, "XOR   V%1X,V%1X          ' V%1X = V%1X XOR V%1X", msbl, lsbh, msbl, msbl, lsbh);
					trace(ins);
					
					V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                // 8XY4 - Adds VY to VX. VF is set to 1 when there's a carry,
                // and to 0 when there isn't.
                case 0x0004:
                    
					sprintf(ins, "ADD   V%1X,V%1X          ' V%1X = V%1X + V%1X, VF=1 for Carry", msbl, lsbh, msbl, msbl, lsbh);
					trace(ins);
					
					V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
                        V[0xF] = 1; //carry
                    else
                        V[0xF] = 0;
                    pc += 2;
                    break;

                // 8XY5 - VY is subtracted from VX. VF is set to 0 when
                // there's a borrow, and 1 when there isn't.
                case 0x0005:
                    
					sprintf(ins, "SUB   V%1X,V%1X          ' V%1X = V%1X - V%1X, VF=1 for V%1X>V%1X", msbl, lsbh, msbl, msbl, lsbh, msbl, lsbh);
					trace(ins);

					if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
                        V[0xF] = 0; // there is a borrow
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                // 0x8XY6 - Shifts VX right by one. VF is set to the value of
                // the least significant bit of VX before the shift.
                case 0x0006:
                    
					sprintf(ins, "SHR   V%1X,V%1X          ' V%1X = V%1X/2, VF=1 if V%1X bit0=1", msbl, lsbh, msbl, msbl, msbl);
					trace(ins);

					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;
                    break;

                // 0x8XY7: Sets VX to VY minus VX. VF is set to 0 when there's
                // a borrow, and 1 when there isn't.
                case 0x0007:
                    
					sprintf(ins, "SUBN  V%1X,V%1X          ' V%1X = V%1X - V%1X, VF=1 for V%1X>V%1X", msbl, lsbh, msbl, lsbh, msbl, lsbh, msbl);
					trace(ins);
					
					if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])	// VY-VX
                        V[0xF] = 0; // there is a borrow
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                // 0x8XYE: Shifts VX left by one. VF is set to the value of
                // the most significant bit of VX before the shift.
                case 0x000E:
                    
					sprintf(ins, "SHL   V%1X,V%1X          ' V%1X = V%1X*2, VF=1 if V%1X bit7=1", msbl, lsbh, msbl, msbl, msbl);
					trace(ins);
					
					V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;
                    break;

                default:
                    printf("\nUnknown op code: %.4X\n", opcode);
					pc += 2;
				//  exit(3);
            }
            break;

        // 9XY0 - Skips the next instruction if VX doesn't equal VY.
        case 0x9000:

			sprintf(ins, "SNE   V%1X,V%1X          ' Skip Next OP if V%1X!=V%1X", msbl, lsbh, msbl, lsbh);
			trace(ins);
			
			if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;
            break;

        // ANNN - Sets I to the address NNN.
        case 0xA000:

			sprintf(ins, "LD    I,#%03X         ' Load I with #%3X", lsb + (msbl << 8), lsb + (msbl << 8));
			trace(ins);
			
			I = opcode & 0x0FFF;
            pc += 2;
            break;

        // BNNN - Jumps to the address NNN plus V0.
        case 0xB000:
            
			sprintf(ins, "JP    V0,#%03X        ' Jump to Address V0+#%03X", lsb + (msbl << 8), lsb + (msbl << 8)); /* Jump to ADDRESS */
			trace(ins);
			
			pc = (opcode & 0x0FFF) + V[0];
            break;

        // CXNN - Sets VX to a random number, masked by NN.
        case 0xC000:

			sprintf(ins, "RND   V%1X,#%02X         ' Set V%1X = (RND) AND #%02X", msbl, lsb, msbl, lsb);
			trace(ins);
			
			V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            pc += 2;
            break;

        // DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8
        // pixels and a height of N pixels.
        // Each row of 8 pixels is read as bit-coded starting from memory
        // location I;
        // I value doesn't change after the execution of this instruction.
        // VF is set to 1 if any screen pixels are flipped from set to unset
        // when the sprite is drawn, and to 0 if that doesn't happen.
        case 0xD000:
        {
            unsigned short x = V[(opcode & 0x0F00) >> 8];
            unsigned short y = V[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;

			sprintf(ins, "DRW   V%1X,V%1X,#%1x       ' Display #%1X Sprite(s) from [I] at V%1X,V%1X", msbl, lsbh, lsbl, lsbl, msbl, lsbh);
			trace(ins);
			
			
			V[0xF] = 0;
            for (int yline = 0; yline < height; yline++)
            {
                pixel = memory[I + yline];
                for(int xline = 0; xline < 8; xline++)
                {
                    if((pixel & (0x80 >> xline)) != 0)
                    {
                        if(gfx[(x + xline + ((y + yline) * 64))] == 1)
                        {
                            V[0xF] = 1;
                        }
                        gfx[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }

            drawFlag = true;
            pc += 2;
        }
            break;

        // EX__
        case 0xE000:

            switch (opcode & 0x00FF) {
                // EX9E - Skips the next instruction if the key stored
                // in VX is pressed.
                case 0x009E:
                    
					sprintf(ins, "SKP   V%1X             ' Skip Next OP if (V%1X) = (KEY) Down", msbl, msbl);
					trace(ins);
					
					if (key[V[(opcode & 0x0F00) >> 8]] != 0)
                        pc +=  4;
                    else
                        pc += 2;
                    break;

                // EXA1 - Skips the next instruction if the key stored
                // in VX isn't pressed.
                case 0x00A1:

					sprintf(ins, "SKNP  V%1X             ' Skip Next OP if (V%1X) = (KEY) UP", msbl, msbl);
					trace(ins);
					
					if (key[V[(opcode & 0x0F00) >> 8]] == 0)
                        pc +=  4;
                    else
                        pc += 2;
                    break;

                default:
                    printf("\nUnknown op code: %.4X\n", opcode);
					pc += 2;
				//   exit(3);
            }
            break;

        // FX__
        case 0xF000:
            switch(opcode & 0x00FF)
            {
                // FX07 - Sets VX to the value of the delay timer
                case 0x0007:
                    
					sprintf(ins, "LD    V%1X,DT          ' V%1X = (DT) Get Delay Timer", msbl, msbl);
					trace(ins);
					
					V[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                    break;

                // FX0A - A key press is awaited, and then stored in VX
                case 0x000A:
                {
                    bool key_pressed = false;

					sprintf(ins, "LD    V%1X,K           ' V%1X = (KEY) Get Key Input", msbl, msbl);
					trace(ins);
						
					
					for(int i = 0; i < 16; ++i)
                    {
                        if(key[i] != 0)
                        {
                            V[(opcode & 0x0F00) >> 8] = i;
                            key_pressed = true;
                        }
                    }

                    // If no key is pressed, return and try again.
                    if(!key_pressed)
                        return;

                    pc += 2;
                }
                    break;

                // FX15 - Sets the delay timer to VX
                case 0x0015:

					sprintf(ins, "LD    DT,V%1X          ' DT = V%1X  Set Delay Timer", msbl, msbl);
					trace(ins);
					
					delay_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                // FX18 - Sets the sound timer to VX
                case 0x0018:

					sprintf(ins, "LD    ST,V%1X          ' ST = V%1X  Set Sound Timer", msbl, msbl);
					trace(ins);
					
					
					sound_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                // FX1E - Adds VX to I
                case 0x001E:
                    // VF is set to 1 when range overflow (I+VX>0xFFF), and 0
                    // when there isn't.
                    
					sprintf(ins, "ADD   I,V%1X           ' Set I = I + V%1X", msbl, msbl);
					trace(ins);
										
					if(I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    I += V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                // FX29 - Sets I to the location of the sprite for the
                // character in VX. Characters 0-F (in hexadecimal) are
                // represented by a 4x5 font
                case 0x0029:
                    
					sprintf(ins, "LD    F,V%1X           ' Set I = Address of Sprite in V%1X", msbl, msbl);
					trace(ins);
										
					I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc += 2;
                    break;

                // FX33 - Stores the Binary-coded decimal representation of VX
                // at the addresses I, I plus 1, and I plus 2
                case 0x0033:
                    
					sprintf(ins, "LD    B,V%1X           ' Store BCD of V%1X at [I] to [I+2]", msbl, msbl);
					trace(ins);
					
					
					memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
                    pc += 2;
                    break;

                // FX55 - Stores V0 to VX in memory starting at address I
                case 0x0055:
                    
					sprintf(ins, "LD    [I],V%1X         ' Store V0 thru V%1X at [I]", msbl, msbl);
					trace(ins);
										
					for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
                        memory[I + i] = V[i];

                    // On the original interpreter, when the
                    // operation is done, I = I + X + 1.
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                    break;

                case 0x0065:
                    
					sprintf(ins, "LD    V%1X,[I]         ' Read V0 thru V%1X From [I]", msbl, msbl);
					trace(ins);
					
					
					for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
                        V[i] = memory[I + i];

                    // On the original interpreter,
                    // when the operation is done, I = I + X + 1.
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                    break;

                default:
                    printf ("Unknown opcode [0xF000]: 0x%X\n", opcode);
            }
            break;

        default:
            printf("\nUnimplemented op code: %.4X\n", opcode);
        //  exit(3);
    }


    // Update timers
	if (delay_timer > 0) {
		--delay_timer;
		}

	if (sound_timer > 0) {
		--sound_timer;
		printf("\7");  // Sound Tone is a Beep
		}
	
}