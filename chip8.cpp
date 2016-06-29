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
char ins[1024];              /* Debug Instruction Description */
FILE *fout;					 /* Debug output File descripter */
unsigned int stop=0x1000;	 /* Debug Stop/Breakpoint Address */

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


unsigned char chip48_BCD_Digit[100] =
{
	0xF0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x20, 0x60, 0x60, 0x20, 0x20, 0x20, 0x20, 0x70, 0x70, //1
	0xF0, 0xF0, 0x10, 0x10, 0xF0, 0xF0, 0x80, 0x80, 0xF0, 0xF0, //2
	0xF0, 0xF0, 0x10, 0x10, 0xF0, 0xF0, 0x10, 0x10, 0xF0, 0xF0, //3
	0x90, 0x90, 0x90, 0x90, 0xF0, 0xF0, 0x10, 0x10, 0x10, 0x10, //4
	0xF0, 0xF0, 0x80, 0x80, 0xF0, 0xF0, 0x10, 0x10, 0xF0, 0xF0, //5
	0xF0, 0xF0, 0x80, 0x80, 0xF0, 0xF0, 0x90, 0x90, 0xF0, 0xF0, //6
	0xF0, 0xF0, 0x10, 0x10, 0x20, 0x40, 0x40, 0x40, 0x40, 0x40, //7
	0xF0, 0xF0, 0x90, 0x90, 0xF0, 0xF0, 0x90, 0x90, 0xF0, 0xF0, //8
	0xF0, 0xF0, 0x90, 0x90, 0xF0, 0xF0, 0x10, 0x10, 0xF0, 0xF0  //9
};


extern 	SDL_Window* window;
extern void start_SDL(unsigned gfx_x, unsigned gfx_y, bool gfx_mode);

static int last_debug = 0;

void Chip8::trace(char *ins)  /* Output Instructuction Trace */
{
  int i,j;
  char cmd[80];
  unsigned int byte;
  int disp_menu = 0;

  
  if (DEBUG > 0 && DEBUG !=4) { /* If Debug Trace is active */

status:
		fprintf(fout, "\r\n%04X  %02X %02X    %s\r\n\r\n", addr, msb, lsb, ins);  /* Debug Trace Instruction Address & OpCode*/

		for (i = 0; i < 8; i++) { /* Display Register 0-7 Content */
			fprintf(fout, "V%X:%-02.2X  ", i, V[i]);
		} /* end, Display Register 0-7 Content */
		fprintf(fout, "\r\n");

		for (i = 8; i < 16; i++) { /* Display Register 8-15 Content */
			fprintf(fout, "V%X:%-02.2X  ", i, V[i]);
		} /* end, Display Register 8-15 Content */

		fprintf(fout, "\r\n\r\n");

		fprintf(fout, "I:%-04.4X  SP:%-2.2X  PC:%-4.4X  Delay:%-4.4X  Sound:%-4.4X\r\n\r\n", I, sp, pc, delay_timer, sound_timer);

		/* Display Call Stack */
		fprintf(fout, "Stack: ");
		for (i = 0; i < 8; i++) fprintf(fout, "%-04.4X  ", stack[i]);
		fprintf(fout,"\r\n       ");
		for (i = 8; i < 16; i++) fprintf(fout, "%-04.4X  ", stack[i]);

		fprintf(fout, "\r\n\r\n");

  } /* end, If Debug Trace is active */
	
  if (0 != _kbhit() || DEBUG==2 || stop==addr || disp_menu==1) { /* Debug Keyboard Input */
	  char *ptr;

	  disp_menu = 0;

	  if (DEBUG==0) printf("Debug: Trace is OFF\r\n\r\n");
	  if (DEBUG==1) printf("Debug: Trace is ON\r\n\r\n");
	  if (DEBUG == 2) printf("Debug: Single Step Mode\r\n\r\n");

	  if (stop<0x1000) printf("Stop at: %4.4X\r\n\r\n",stop);
menu:
	  printf("   Debug Command Menu\r\n\r\n");
	  printf("ON   - Turn Continuous Trace ON\r\n");
	  printf("OFF  - Turn Continuous Trace OFF\r\n");
	  printf("STEP - Single Step Opcode Mode\r\n");
	  printf("STOP - Stop at Address, 0x1000>=Off\r\n");
	  printf("SET  - Set Memory Bytes\r\n");
	  printf("REG  - Set Register Value\r\n");
	  printf("DUMP - Display Memory in Hex\r\n");
	  printf("STAT - Show Registers/Status\r\n");
	  printf("GFX  - Toggle Graphics Pixel\r\n");
	  printf("CLS  - Clear Graphics Screen\r\n");
	  printf("LOOK - Enable Graphics Focus\r\n");
	  printf("GO   - Continue Execution\r\n");


	  printf("Command: ");
	
	scanf("%s", &cmd); 	/* Check for Keyboard Input from the Debug Window */

	/* Convert command to uppercase */
	for (ptr = cmd; ptr < cmd + strlen(cmd); ptr++)
		if (islower(*ptr)) *ptr=_toupper(*ptr);

	if (0==strcmp("ON", cmd)) { DEBUG = 1; }
	if (0 == strcmp("OFF", cmd)) { DEBUG = 0; }
	if (0 == strcmp("STEP", cmd)) { DEBUG = 2; goto status; }
	if (0 == strcmp("STAT", cmd)) { disp_menu = 1; goto status; }
	if (0 == strcmp("LOOK", cmd)) { last_debug = DEBUG; DEBUG = 4; }
	if (0 == strcmp("GO", cmd)) { if (DEBUG == 4) DEBUG=last_debug; }
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
	if (0 == strcmp("SET", cmd)) { /* Set */
		int k;
		unsigned int addr;

		printf("Hex: Address Count? ");
		scanf("%X %X", &addr,&k);
		printf("\r\n");
		for (i = 0; i < k; i++) { /* set mem */
			if (addr >= 0x1000) break;
			printf("%4.4X: 0x", addr);
			scanf("%X", &byte);
			if (addr >= 0x1000) break;
			memory[addr] = byte;
			addr++;
		} /* end, set mem */
		goto menu;
	} /* end Set */
	if (0 == strcmp("REG", cmd)) { /* Set Reg */

		printf("Set Register Value\r\n");
		printf("Register Names: V0-F, I, SP, PC\r\n");
		printf("Enter: Name Hex_Value? ");
		scanf("%2s %X", cmd, &j);
		/* Convert register name to uppercase */
		for (ptr = cmd; ptr < cmd + strlen(cmd); ptr++)
			if (islower(*ptr)) *ptr = _toupper(*ptr);

		if (*cmd == 'V') { // V0-F
			sscanf(cmd+1,"%x",&i);
			if (i < 0x10) V[i] = j;
			else printf("*** ERROR ***\r\n*** Bad Register Name: %s", cmd);
		} /* end, V0-F */
		else if (*cmd == 'I') { /* I */
		   if (j<0x1000) I = j;
		   else printf("*** ERROR ***\r\n*** Bad Index Value: 0x%X > 0xFFF", j);
		} /* end, I */
		else if (*cmd == 'S' && cmd[1] == 'P') { /* SP */
			if (j<16) sp = j;
			else printf("*** ERROR ***\r\n*** Bad SP Value: 0x%X > 16", j);
		} /* end, SP */
		else if (*cmd == 'P' && cmd[1] == 'C') { /* PC */
			if (j % 2 == 1) printf("*** ERROR ***\r\n*** Bad PC Value: 0x%X is odd", j);
			else if (j>0xFFE) printf("*** ERROR ***\r\n*** Bad PC Value: 0x%X > 0xFFE", j);
			else pc = j;
		} /* end, PC */
		else printf("*** ERROR ***\r\n*** Bad Register Name: %s",cmd);
		printf("\r\n");
		disp_menu = 1; /* Signal menu display */
		goto status;
	} /* end, Set Reg */

	if (0 == strcmp("GFX", cmd)) { /* GFX */
		unsigned int X, Y;

		printf("Decimal Screen Coordinates: X Y? ");
		scanf("%u %u", &X, &Y);

		if (X >= gfx_x) printf("*** ERROR X:%d Coordinate > %d", X, gfx_x-1); // Bad Horizontal 
		else if (Y>=gfx_y) printf("*** ERROR Y:%d Coordinate > %d", Y, gfx_y-1); // Bad Vertical 
		else if (gfx[X + gfx_x*Y] == 1) gfx[X + gfx_x*Y]=0; // Toggle Pixel OFF
		else gfx[X + gfx_x*Y] = 1; // Toggle Pixel ON
		printf("\r\n");
		drawFlag = true;
	} /* end GFX */

	if (0 == strcmp("CLS", cmd)) { /* CLS */
		unsigned int i;

		for (i=0; i<gfx_size;i++) gfx[i] = 0;
		printf("\r\n");
		// Debug Test Full screen Mode Graphics mapping
//		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

		drawFlag = true;
	} /* end CLS */


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
    for (unsigned i = 0; i < gfx_size; ++i) {
        gfx[i] = 0;
    }

    // Clear the stack, keypad, and V registers
    for (int i = 0; i < 16; ++i) {
        stack[i]    = 0;
        key[i]      = 0;
        V[i]        = 0;
    }

	// Clear RPL Register Store
	for (int i = 0; i < 8; i++) RPL[i] = 0;
	
	// Clear memory
    for (int i = 0; i < 4096; ++i) {
        memory[i] = 0;
    }

    // Load Chip-8 font set into memory
    for (int i = 0; i < 80; ++i) {
        memory[i] = chip8_fontset[i];
    }

	// Load Chip-48 font set into memory
	unsigned char c;
	for (int i = 0; i < 100; i++) {
		c= chip48_BCD_Digit[i];
		memory[i + 80] = c;
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
				int i;
				int N;

				N = lsbl * gfx_x;

				for (i = (gfx_x*gfx_y-1)-N; i >= 0; i--) gfx[i + N] = gfx[i]; /* shift video down N lines */
				for (i = 0; i < N ; i++) gfx[i] = 0;  /* Zero out the 1st N rows (lines) */

				drawFlag = true; /* Signal a video redraw */

				pc+=2;  /* Next Instruction */

				sprintf(ins, "SCD   #%1X             ' Scroll Down #%1X Lines", lsbl, lsbl);
				trace(ins);  /* Debug Trace */

				break;
			} /* end, 00CN OpCode */

            switch (opcode & 0x00FF) { /* 0 Upper byte switch */
				
				// 00E0 - Clear screen
                case 0x00E0:

					for (unsigned i = 0; i < gfx_size; ++i) {
                        gfx[i] = 0;
                    }
                    drawFlag = true;
                    pc+=2;

					strcpy(ins, "CLS                  ' Clear Screen"); /* Clear Screen */
					trace(ins);  /* Debud Trace */

					break;

                // 00EE - Return from subroutine
                case 0x00EE:
					if (sp > 0) { // If there is a Subroutine Active
						--sp;
						pc = stack[sp];
						pc += 2;
						strcpy(ins, "RET                  ' Return"); /* Return from Subroutine */
					} // end, If there is a Subroutine Active
					else { // Stack Underflow, bad Return Opcode
						strcpy(ins, "RET                  ' Return"); /* Return from Subroutine */
						strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
						strcat(ins, "*** STACK UNDERFLOW ***\r\n");
						strcat(ins, "*** Too many 'RET' Operations ***\r\n");
						DEBUG = 2; // Put Debugger in Single Step Mode
					} // end, Stack Underflow, bad Return Opcode

					trace(ins);   /* Debug Trace */

					break;

				case 0x00FB:
					/* Chip-48 OpCode */
				{ /* handle 4 pixel shift Right */
					unsigned int i, j;

					/* Shift the Rows Right 4 bytes or pixels*/
					for (j = 0; j < gfx_y; j++) { /* Each Row */
						for (i = (gfx_x-1); i >=4; i--) {/* Each Column */
							gfx[i+j*gfx_x]= gfx[(i-4)+j*gfx_x];
						} /* end, Each Column */
						/* Blank the left most columns */
						for (i = 0; i < 4; i++) gfx[i+j*gfx_x] = 0;
					} /* end, each Row */
					
				} /* end, handle 4 pixel shift Right */

				drawFlag = true;  /* signal a screen rewrite */
				pc += 2;   /* Next Instruction */

				sprintf(ins, "SCR                  ' Scroll 4 Pixels Right");
				trace(ins);   /* Debug Trace */

				break;

				case 0x00FC:
					/* Chip-48 OpCode */
				{ /* handle 4 pixel shift Left */
					unsigned int i, j;

					/* Shift the Rows Left 4 bytes or pixels*/
					for (j = 0; j < gfx_y; j++) { /* Each Row */
						for (i = 0; i<(gfx_x-4) ; i++) {/* Each Column */
							gfx[i + (gfx_x * j)] = gfx[i + (gfx_x * j) + 4];
						} /* end, Each Column */
						  /* Blank the Right most columns */
						for (i = gfx_x-4; i < gfx_x; i++) gfx[i+(gfx_x*j)] = 0;
					} /* end, each Row */
					
				} /* end, handle 4 pixel shift Left */

				drawFlag = true;  /* signal a screen rewrite */
				pc += 2;   /* Next Instruction */

				sprintf(ins, "SCL                  ' Scroll 4 Pixels Left");
				trace(ins);   /* Debug Trace */

				break;

				case 0x00FD:
					/* Chip-48 OpCode */
					
					sprintf(ins, "EXIT                 ' Exit Interpreter");
					trace(ins);   /* Debug Trace */

					exit(0);  /* time to leave */
					break;

				case 0x00FE:
					/* Chip-48 OpCode */

					/* Not implemented yet */
//					SDL_SetWindowFullscreen(window, 0);  // Set Low Resolution Graphics

					gfx_mode = false;  // Assume Low resolution
					gfx_x = 64;		// 64 pixel & 64 byte buffer horixontal dimension
					gfx_y = 32;		// 32 pixel & 32 byte buffer vertical dimension

					start_SDL(gfx_x, gfx_y, gfx_mode);		// Start SDL Graphics

					// Clear the display
					for (unsigned i = 0; i < gfx_size; ++i) {
						gfx[i] = 0;
					}

					pc += 2; /* Next Instruction */

					sprintf(ins, "LOW                  ' Disable Extended Graphics");
					trace(ins);   /* Debug Trace */

					break;

				case 0x00FF:
					/* Chip-48 OpCode */

 				    /* Not implemented yet */

//					SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);  // Set High Resolution Graphics

					// High Resolution Graphics
					gfx_mode = true;  // Assume High resolution
					gfx_x = 128;		// 128 pixel & 128 byte buffer horixontal dimension
					gfx_y = 64;		// 64 pixel & 64 byte buffer vertical dimension

					start_SDL(gfx_x, gfx_y, gfx_mode);		// Start SDL Graphics
					// Clear the display
					for (unsigned i = 0; i < gfx_size; ++i) {
						gfx[i] = 0;
					}

					pc += 2; /* Next Instruction */

					sprintf(ins, "HIGH                 ' Enable Extended Graphics");
					trace(ins);   /* Debug Trace */

					break;



                default:

					printf("\nUnknown op code: %.4X\r\n", opcode);
//					pc += 2;

					sprintf(ins, "Unknown OpCode      ' Code Error");
					strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
					DEBUG = 2; // Put Debugger in Single Step Mode

					trace(ins);   /* Debug Trace */

								  //   exit(3);
            } /* end, 0 Upper byte switch */
            break;

        // 1NNN - Jumps to address NNN
        case 0x1000:


			pc = opcode & 0x0FFF;

			sprintf(ins, "JMP   #%1X%02X           ' JUMP to #%1X%02X", msbl, lsb, msbl, lsb); /* Jump to ADDRESS */
			trace(ins);

			break;

        // 2NNN - Calls subroutine at NNN
        case 0x2000:
			if (sp < 16) { // Enforce 16 nested subroutine call limit
				stack[sp] = pc;
				++sp;
				pc = opcode & 0x0FFF;
				sprintf(ins, "CALL  #%1X%02X           ' Call Subroutine", msbl, lsb); /* Call Subroutine */
			} // end, Enforce 16 nested subroutine call limit
			else { // Stack Overflow, bad Call Opcode
				sprintf(ins, "CALL  #%1X%02X           ' Call Subroutine", msbl, lsb); /* Call Subroutine */
				strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
				strcat(ins, "*** STACK OVERFLOW ***\r\n");
				strcat(ins, "*** Too many 'CALL' Operations ***\r\n");
				DEBUG = 2; // Put Debugger in Single Step Mode
			} // end, Stack Overflow, bad Call Opcode

			trace(ins);

			break;

        // 3XNN - Skips the next instruction if VX equals NN.
        case 0x3000:

			if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;

			sprintf(ins, "SE    V%1X,#%02X         ' Skip Next OP if V%1X=#%02X", msbl, lsb, msbl, lsb);
			trace(ins);

			break;

        // 4XNN - Skips the next instruction if VX does not equal NN.
        case 0x4000:

			if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
			
			sprintf(ins, "SNE   V%1X,#%02X         ' Skip Next OP if V%1X!=#%02X", msbl, lsb, msbl, lsb);
			trace(ins);

			break;

        // 5XY0 - Skips the next instruction if VX equals VY.
        case 0x5000:

			if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;

			sprintf(ins, "SE    V%1X,V%1X          ' Skip Next OP if V%1X=V%1X", msbl, lsbh, msbl, lsbh);
			trace(ins);

			break;

        // 6XNN - Sets VX to NN.
        case 0x6000:

			
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;

			sprintf(ins, "LD    V%1X,#%02X         ' Load V%1X with #%1X", msbl, lsb, msbl, lsb);
			trace(ins);

			break;

        // 7XNN - Adds NN to VX.
        case 0x7000:

			V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;

			sprintf(ins, "ADD   V%1X,#%02X         ' Set V%1X = V%1X + #%02X", msbl, lsb, msbl, msbl, lsb);
			trace(ins);

			break;

        // 8XY_
        case 0x8000:
            switch (opcode & 0x000F) {

                // 8XY0 - Sets VX to the value of VY.
                case 0x0000:
 
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;

					sprintf(ins, "LD    V%1X,V%1X          ' Load V%1X with V%1X", msbl, lsbh, msbl, lsbh);
					trace(ins);

					break;

                // 8XY1 - Sets VX to (VX OR VY).
                case 0x0001:
                    
					V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    pc += 2;

					sprintf(ins, "OR    V%1X,V%1X          ' V%1X = V%1X OR V%1X", msbl, lsbh, msbl, msbl, lsbh);
					trace(ins);

					break;

                // 8XY2 - Sets VX to (VX AND VY).
                case 0x0002:
                    
					V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;

					sprintf(ins, "AND   V%1X,V%1X          ' V%1X = V%1X AND V%1X", msbl, lsbh, msbl, msbl, lsbh);
					trace(ins);
					
					break;

                // 8XY3 - Sets VX to (VX XOR VY).
                case 0x0003:
                    
					V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;

					sprintf(ins, "XOR   V%1X,V%1X          ' V%1X = V%1X XOR V%1X", msbl, lsbh, msbl, msbl, lsbh);
					trace(ins);

					break;

                // 8XY4 - Adds VY to VX. VF is set to 1 when there's a carry,
                // and to 0 when there isn't.
                case 0x0004:
                    
					V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
                        V[0xF] = 1; //carry
                    else
                        V[0xF] = 0;
                    pc += 2;

					sprintf(ins, "ADD   V%1X,V%1X          ' V%1X = V%1X + V%1X, VF=1 for Carry", msbl, lsbh, msbl, msbl, lsbh);
					trace(ins);

					break;

                // 8XY5 - VY is subtracted from VX. VF is set to 0 when
                // there's a borrow, and 1 when there isn't.
                case 0x0005:
                    
					if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
                        V[0xF] = 0; // there is a borrow
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;

					sprintf(ins, "SUB   V%1X,V%1X          ' V%1X = V%1X - V%1X, VF=1 for V%1X>V%1X", msbl, lsbh, msbl, msbl, lsbh, msbl, lsbh);
					trace(ins);

					break;

                // 0x8XY6 - Shifts VX right by one. VF is set to the value of
                // the least significant bit of VX before the shift.
                case 0x0006:
                    
					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;

					sprintf(ins, "SHR   V%1X,V%1X          ' V%1X = V%1X/2, VF=1 if V%1X bit0=1", msbl, lsbh, msbl, msbl, msbl);
					trace(ins);
					
					break;

                // 0x8XY7: Sets VX to VY minus VX. VF is set to 0 when there's
                // a borrow, and 1 when there isn't.
                case 0x0007:
                    
					if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])	// VY-VX
                        V[0xF] = 0; // there is a borrow
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    pc += 2;

					sprintf(ins, "SUBN  V%1X,V%1X          ' V%1X = V%1X - V%1X, VF=1 for V%1X>V%1X", msbl, lsbh, msbl, lsbh, msbl, lsbh, msbl);
					trace(ins);

					break;

                // 0x8XYE: Shifts VX left by one. VF is set to the value of
                // the most significant bit of VX before the shift.
                case 0x000E:
                    
					V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;

					sprintf(ins, "SHL   V%1X,V%1X          ' V%1X = V%1X*2, VF=1 if V%1X bit7=1", msbl, lsbh, msbl, msbl, msbl);
					trace(ins);

					break;

                default:

					
					printf("\nUnknown op code: %.4X\n", opcode);
//					pc += 2;

					sprintf(ins, "Unknown OpCode      ' Code Error");
					strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
					DEBUG = 2; // Put Debugger in Single Step Mode
					trace(ins);   /* Debug Trace */

								  //  exit(3);
            }
            break;

        // 9XY0 - Skips the next instruction if VX doesn't equal VY.
        case 0x9000:

			if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;

			sprintf(ins, "SNE   V%1X,V%1X          ' Skip Next OP if V%1X!=V%1X", msbl, lsbh, msbl, lsbh);
			trace(ins);

			
			break;

        // ANNN - Sets I to the address NNN.
        case 0xA000:

			I = opcode & 0x0FFF;
            pc += 2;

			sprintf(ins, "LD    I,#%03X         ' Load I with #%3X", lsb + (msbl << 8), lsb + (msbl << 8));
			trace(ins);

			break;

        // BNNN - Jumps to the address NNN plus V0.
        case 0xB000:
            
			// Check User's Jump Target Address 

			if (((opcode & 0x0FFF) + V[0]) >= 0x0FFF) { // Out of Address Space
			// User's Target address is out of memory limits
				uint16_t addr = (opcode & 0x0FFF) + V[0]; // Get Target address
				char temp[128];

				sprintf(ins, "JP    V0,#%03X        ' Jump to Address V0+#%03X=0x%4.4X", lsb + (msbl << 8), lsb + (msbl << 8),addr); /* Jump to ADDRESS */
				strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
				strcat(ins, "*** Memory Range Limit***\r\n");
				sprintf(temp, "*** Invalid Jump Address: 0x%04.4X\r\n", addr);
				strcat(ins, temp);
				DEBUG = 2; // Put Debugger in Single Step Mode
				trace(ins);
				break; // Report Error

			} // end, Out of Address Space
			
			pc = (opcode & 0x0FFF) + V[0];

			sprintf(ins, "JP    V0,#%03X        ' Jump to Address V0+#%03X", lsb + (msbl << 8), lsb + (msbl << 8)); /* Jump to ADDRESS */
			trace(ins);

			break;

        // CXNN - Sets VX to a random number, masked by NN.
        case 0xC000:

			V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            pc += 2;

			sprintf(ins, "RND   V%1X,#%02X         ' Set V%1X = (RND) AND #%02X", msbl, lsb, msbl, lsb);
			trace(ins);

			break;

        // DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8
        // pixels and a height of N pixels.
        // Each row of 8 pixels is read as bit-coded starting from memory
        // location I;
        // I value doesn't change after the execution of this instruction.
        // VF is set to 1 if any screen pixels are flipped from set to unset
        // when the sprite is drawn (XORed), and to 0 if that doesn't happen.
        case 0xD000:
		{
			unsigned short x = V[(opcode & 0x0F00) >> 8]; // Horizontal Screen Coordinate
			unsigned short y = V[(opcode & 0x00F0) >> 4]; // Vertical Screen Coordinate

			unsigned short height = opcode & 0x000F;
			unsigned short pixel;

			if (height == 0 && gfx_mode == true) height = 16; /* Check for a Chip-48 16x16 sprite draw opcode*/

			// Check for Video Range Index Exceptions for High Resolution 16x16 Sprite
			if (((x + 15 + ((y + height - 1) * gfx_x)) >= gfx_x*gfx_y) && height==16) { // Video Index Error
				sprintf(ins, "DRW   V%1X,V%1X,#%1x       ' Display #%1X Sprite(s) from [I] at V%1X,V%1X", msbl, lsbh, lsbl, lsbl, msbl, lsbh);
				strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
				strcat(ins, "*** Video Index Range ***\r\n");
				DEBUG = 2; // Put Debugger in Single Step Mode
				trace(ins);
				break; // Report Error
			} // end, Video Index Error
			  
			  // Check for Video Range Index Exceptions for Low Resolution
			if ((x + 7 + ((y + height-1) * gfx_x)) >= gfx_x*gfx_y)  { // Video Index Error
				sprintf(ins, "DRW   V%1X,V%1X,#%1x       ' Display #%1X Sprite(s) from [I] at V%1X,V%1X", msbl, lsbh, lsbl, lsbl, msbl, lsbh);
				strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
				strcat(ins, "*** Video Index Range ***\r\n");
				DEBUG = 2; // Put Debugger in Single Step Mode
				trace(ins);
				break; // Report Error
			} // end, Video Index Error

			if (height == 16 && gfx_mode == true) { /* Check for a Chip-48 16x16 sprite draw opcode*/
				/* draw 16 lines two bytes wide */

				V[0xF] = 0;  /* Assume no flipped pixels */
				for (int yline = 0; yline < height; yline++) { /* for N Pixels high */

					// First Horizontal Byte				
					pixel = memory[I + 2*yline];  /* Get a Sprite byte from Memory*/
					for (int xline = 0; xline < 8; xline++) /* for 8 pixels wide */
					{
						if ((pixel & (0x80 >> xline)) != 0) /* if memory pixel is on */
						{
							if (gfx[(x + xline + ((y + yline) * gfx_x))] == 1)
							{ /* if screen pixel is on */
								V[0xF] = 1;
							} /* end, if screen pixel is on */
							gfx[x + xline + ((y + yline) * gfx_x)] ^= 1; /* XOR Graphics Bits */
						} /* end, if memory pixel is on */
					} /* end for 8 pixels wide */

					  // Second Horizontal Byte				
					pixel = memory[I + 1 + 2*yline];  /* Get a Sprite byte from Memory*/
					for (int xline = 0; xline < 8; xline++) /* for 8 pixels wide */
					{
						if ((pixel & (0x80 >> xline)) != 0) /* if memory pixel is on */
						{
							if (gfx[(x + 8 + xline + ((y + yline) * gfx_x))] == 1)
							{ /* if screen pixel is on */
								V[0xF] = 1;
							} /* end, if screen pixel is on */
						gfx[x + 8 + xline + ((y + yline) * gfx_x)] ^= 1; /* XOR Graphics Bits */
						} /* end, if memory pixel is on */
					} /* end for 8 pixels wide */


				} /* end, for N pixels high */

			} /* end, Check for a Chip-48 16x16 sprite draw opcode*/

			else { // Low Graphics Resolution Sprite
			V[0xF] = 0;  /* Assume no flipped pixels */
			for (int yline = 0; yline < height; yline++) /* for N Pixels high */
			{
				pixel = memory[I + yline];  /* Get a Sprite byte from Memory*/
				for (int xline = 0; xline < 8; xline++) /* for 8 pixels wide */
				{
					if ((pixel & (0x80 >> xline)) != 0) /* if memory pixel is on */
					{
						if (gfx[(x + xline + ((y + yline) * gfx_x))] == 1)
						{ /* if screen pixel is on */
							V[0xF] = 1;
						} /* end, if screen pixel is on */
						gfx[x + xline + ((y + yline) * gfx_x)] ^= 1; /* XOR Graphics Bits */
					} /* end, if memory pixel is on */
				} /* end for 8 pixels wide */
			} /* end, for N pixels high */

		} // end,  Low Graphics Resolution Sprite
			drawFlag = true;
            pc += 2;
        }
		sprintf(ins, "DRW   V%1X,V%1X,#%1x       ' Display #%1X Sprite(s) from [I] at V%1X,V%1X", msbl, lsbh, lsbl, lsbl, msbl, lsbh);
		trace(ins);

		break;

        // EX__
        case 0xE000:

            switch (opcode & 0x00FF) {
                // EX9E - Skips the next instruction if the key stored
                // in VX is pressed.
                case 0x009E:
                    
					if (key[V[(opcode & 0x0F00) >> 8]] != 0)
                        pc +=  4;
                    else
                        pc += 2;

					sprintf(ins, "SKP   V%1X             ' Skip Next OP if (V%1X) = (KEY) Down", msbl, msbl);
					trace(ins);

					break;

                // EXA1 - Skips the next instruction if the key stored
                // in VX isn't pressed.
                case 0x00A1:

					if (key[V[(opcode & 0x0F00) >> 8]] == 0)
                        pc +=  4;
                    else
                        pc += 2;

					sprintf(ins, "SKNP  V%1X             ' Skip Next OP if (V%1X) = (KEY) UP", msbl, msbl);
					trace(ins);

					break;

                default:

					
					printf("\nUnknown op code: %.4X\n", opcode);
//					pc += 2;

					sprintf(ins, "Unknown OpCode      ' Code Error");
					strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
					DEBUG = 2; // Put Debugger in Single Step Mode
					trace(ins);   /* Debug Trace */

					//   exit(3);
            }
            break;

        // FX__
        case 0xF000:
            switch(opcode & 0x00FF)
            {
                // FX07 - Sets VX to the value of the delay timer
                case 0x0007:
                    
					V[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;

					sprintf(ins, "LD    V%1X,DT          ' V%1X = (DT) Get Delay Timer", msbl, msbl);
					trace(ins);

					break;

                // FX0A - A key press is awaited, and then stored in VX
                case 0x000A:
                {
                    bool key_pressed = false;

					drawFlag = true; // Refresh video if waiting on keyinput

					for(int i = 0; i < 16; ++i)
                    {
                        if(key[i] != 0)
                        {
                            V[(opcode & 0x0F00) >> 8] = i;
                            key_pressed = true;
                        }
                    }

					// If a key was pressed increment PC,
					// otherwise don't increment PC
					// To allow the Debugger to show a keyboard wait state
					if (key_pressed) pc += 2; // Key pressed

					sprintf(ins, "LD    V%1X,K           ' V%1X = (KEY) Get Key Input", msbl, msbl);
					trace(ins);


				}
                    break;

                // FX15 - Sets the delay timer to VX
                case 0x0015:

					delay_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;

					sprintf(ins, "LD    DT,V%1X          ' DT = V%1X  Set Delay Timer", msbl, msbl);
					trace(ins);

					break;

                // FX18 - Sets the sound timer to VX
                case 0x0018:

					
					sound_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;

					sprintf(ins, "LD    ST,V%1X          ' ST = V%1X  Set Sound Timer", msbl, msbl);
					trace(ins);

					break;

                // FX1E - Adds VX to I
                case 0x001E:
                    // VF is set to 1 when range overflow (I+VX>0xFFF), and 0
                    // when there isn't.
                    
					if(I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;

					if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF) { // Out of Address Space
						// User's Target address is out of memory limits
						uint16_t addr = I + V[(opcode & 0x0F00) >> 8]; // Get Target address
						char temp[128];
						int last_debug = DEBUG;

						sprintf(ins, "ADD   I,V%1X           ' Set I = I + V%1X, I=0x%4.4X", msbl, msbl,I);
						strcat(ins, "\r\n*** EXECUTION WARNING ***\r\n");
						strcat(ins, "*** Memory Range Limit ***\r\n");
						sprintf(temp, "*** Invalid Index Address: 0x%04.4X\r\n", addr);
						strcat(ins, temp);
						DEBUG = 1; // Temporarily Put Debugger in Trace Mode
						trace(ins);
						DEBUG = last_debug; // Restore Debugger State

					} // end, Out of Address Space

					
					I += V[(opcode & 0x0F00) >> 8];
                    pc += 2;

					sprintf(ins, "ADD   I,V%1X           ' Set I = I + V%1X", msbl, msbl);
					trace(ins);

					break;

                // FX29 - Sets I to the location of the sprite for the
                // character in VX. Characters 0-F (in hexadecimal) are
                // represented by a 4x5 font
                case 0x0029:
                    
					I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc += 2;

					sprintf(ins, "LD    F,V%1X           ' Set I = Address of Sprite in V%1X", msbl, msbl);
					trace(ins);

					break;

					// FX30 - Point I at the previously stored 10 byte Sprite for
					//        the BCD digit in memory. Decimal Digits 0-9 at 80 (0x50)
				case 0x0030:
					/* Chip-48 Extended Instruction */

					I = 0x50 + 10 * V[msbl]; /* Table Base + 10*(Register Contents)   */

					pc += 2;

					sprintf(ins, "LD    I,V%1X           'Load I with address of BCD Sprite in V%1X", msbl, msbl);
					trace(ins);

					break;


                // FX33 - Stores the Binary-coded decimal representation of VX
                // at the addresses I, I plus 1, and I plus 2
                case 0x0033:
                    
					if (I > 0xFFD) { // Out of Address Space
						// User's Target address is out of memory limits
						uint16_t addr = I+2; // Get Target address
						char temp[128];

						sprintf(ins, "LD    B,V%1X           ' Store BCD of V%1X at [I] to [I+2]", msbl, msbl);
						strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
						strcat(ins, "*** Memory Range Limit ***\r\n");
						sprintf(temp, "*** Invalid Index Address: 0x%04.4X-0x%04.4X\r\n",I, addr);
						strcat(ins, temp);
						DEBUG = 2; // Put Debugger in Step Mode
						trace(ins);
						break;
					} // end, Out of Address Space

					memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
                    pc += 2;

					sprintf(ins, "LD    B,V%1X           ' Store BCD of V%1X at [I] to [I+2]", msbl, msbl);
					trace(ins);

					break;

                // FX55 - Stores V0 to VX in memory starting at address I
                case 0x0055:
                    
					if (I > (0x1000-(msbl+1))) { // Out of Address Space
						// User's Target address is out of memory limits
						uint16_t addr = I + msbl; // Get Target address
						char temp[128];

						sprintf(ins, "LD    [I],V%1X         ' Store V0 thru V%1X at [I]", msbl, msbl);
						strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
						strcat(ins, "*** Memory Range Limit ***\r\n");
						sprintf(temp, "*** Invalid Index Address: 0x%04.4X-0x%04.4X\r\n", I, addr);
						strcat(ins, temp);
						DEBUG = 2; // Put Debugger in Step Mode
						trace(ins);
						break;
					} // end, Out of Address Space


					for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
                        memory[I + i] = V[i];

                    // On the original interpreter, when the
                    // operation is done, I = I + X + 1.
					// I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;

					sprintf(ins, "LD    [I],V%1X         ' Store V0 thru V%1X at [I]", msbl, msbl);
					trace(ins);

					break;

                case 0x0065:
                    
					if (I > (0x1000 - (msbl + 1))) { // Out of Address Space
						// User's Target address is out of memory limits
						uint16_t addr = I + msbl; // Get Target address
						char temp[128];

						sprintf(ins, "LD    V%1X,[I]         ' Read V0 thru V%1X From [I]", msbl, msbl);
						strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
						strcat(ins, "*** Memory Range Limit ***\r\n");
						sprintf(temp, "*** Invalid Index Address: 0x%04.4X-0x%04.4X\r\n", I, addr);
						strcat(ins, temp);
						DEBUG = 2; // Put Debugger in Step Mode
						trace(ins);
						break;
					} // end, Out of Address Space


					for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
                        V[i] = memory[I + i];

                    // On the original interpreter,
                    // when the operation is done, I = I + X + 1.
					// I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;

					sprintf(ins, "LD    V%1X,[I]         ' Read V0 thru V%1X From [I]", msbl, msbl);
					trace(ins);

					break;

				case 0x0075:
					/* Chip-48 Instruction */
					/* Store Registers V0-V[msbl] in RPL Register Store */

					if (msbl>7) { // Too many Registers
						// Opcode specifies too many registers
						char temp[128];
						int last_debug = DEBUG;

						sprintf(ins, "LD    RPL,V%1X         ' Store V0 thru V%1X in RPL", msbl, msbl);
						strcat(ins, "\r\n*** EXECUTION WARNING ***\r\n");
						strcat(ins, "*** Too many Registers - Bad OpCode ***\r\n");
						sprintf(temp, "*** Valid Registers: 0-7 ***\r\n");
						strcat(ins, temp);
						DEBUG = 1; // Temporarily Put Debugger in Trace Mode
						trace(ins);
						DEBUG = last_debug; // Restore Debugger State

					} // end, Too many Registers


					/* Ignore the high order bit of msbl */
					if (msbl > 7) msbl = 7;  /* Enforce Store limits to V0-V7 */


					for (int i = 0; i <= msbl;  ++i) RPL[i] = V[i];

					pc += 2;

					sprintf(ins, "LD    RPL,V%1X         ' Store V0 thru V%1X in RPL", msbl, msbl);
					trace(ins);

					break;

				case 0x0085:
					/* Chip-48 Instruction */
					/* Load Registers V0-V[msbl] from RPL Register Store */

					if (msbl>7) { // Too many Registers
						// Opcode specifies too many registers
						char temp[128];
						int last_debug = DEBUG;

						sprintf(ins, "LD    RPL,V%1X         ' Store V0 thru V%1X in RPL", msbl, msbl);
						strcat(ins, "\r\n*** EXECUTION WARNING ***\r\n");
						strcat(ins, "*** Too many Registers - Bad OpCode ***\r\n");
						sprintf(temp, "*** Valid Registers: 0-7 ***\r\n");
						strcat(ins, temp);
						DEBUG = 1; // Temporarily Put Debugger in Trace Mode
						trace(ins);
						DEBUG = last_debug; // Restore Debugger State

					} // end, Too many Registers

					/* Ignore the high order bit of msbl */
					if (msbl > 7) msbl = 7;  /* Enforce Store limits to V0-V7 */
					for (int i = 0; i <= msbl; ++i) V[i]=RPL[i];

					pc += 2;

					sprintf(ins, "LD    V%1X,RPL         ' Load V0 thru V%1X from RPL", msbl, msbl);
					trace(ins);

					break;


				
				default:

					printf ("Unknown opcode [0xF000]: 0x%X\n", opcode);
					sprintf(ins,"Unknown opcode [0xF000]: 0x%X\n", opcode);
					strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
					DEBUG = 2; // Put Debugger in Single Step Mode
					trace(ins);   /* Debug Trace */


			}
            break;

        default:

			printf("\nUnimplemented op code: %.4X\n", opcode);
			sprintf(ins,"\nUnimplemented op code: %.4X\n", opcode);
			strcat(ins, "\r\n*** EXECUTION ERROR ***\r\n");
			DEBUG = 2; // Put Debugger in Single Step Mode
			trace(ins);   /* Debug Trace */

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