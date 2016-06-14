
// Chip8_EMU_SDL.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <chrono>
#include <thread>
#include "stdint.h"
#include "SDL.h"      /* SDL 2*/


#include "chip8.h"

using namespace std;

// Keypad keymap
uint8_t keymap[16] = {
	SDLK_x,
	SDLK_1,
	SDLK_2,
	SDLK_3,
	SDLK_q,
	SDLK_w,
	SDLK_e,
	SDLK_a,
	SDLK_s,
	SDLK_d,
	SDLK_z,
	SDLK_c,
	SDLK_4,
	SDLK_r,
	SDLK_f,
	SDLK_v,
};

int main(int argc, char **argv) {

	unsigned char debug_out[256];  /* Debug output filename */
	int debug;		/* Debug On > 0 or OFF = 0 */

	// Command usage
	if (argc < 2) {
		int i, j;

		/* Parse out filename */
		j = strlen(argv[0]);
		for (i = j; j > 0; j--) if (*(argv[0] + j) == '\\') break;

		printf("Usage:\r\n\r\n%s ROM_file Debug Output_File\r\n\r\n",argv[0]+j);
		cout << "ROM_file    -   Filename of Chip8 program to Execute\r\n" << endl;
		cout << "Debug       -   Initiate Debug Trace in Console Window or File\r\n" << endl;
		cout << "Output_File -   Filename to send Debug Trace Information to\r\n" << endl;
		cout << "Notes:\r\n" << endl;
		cout << "The Debug and Output_File parameters are optional.\r\n" ;
		cout << "To enter Debug Mode from the Console Window\r\n" ;
		cout << "Press any key. Backspace is recommended.\r\n" ;
		cout << "The Console Window must have the Focus." << endl;
		cout << "Console Debug Mode allows one to Trace Execution," << endl;
		cout << "Single Step, Set a Stop Address, and Dump Memory." << endl;
		cout << "Left and Right Mouse Clicks Pause/Start the Trace." << endl;

		return 1;
	}

	if (argc < 3) debug = 0;  /* Debug is off */
	else debug = 2; /* Debug is on, initially Single Step Mode */

	if (argc < 4) *debug_out = '\0';
	else strcpy((char *)debug_out, argv[3]);

	Chip8 chip8 = Chip8();          // Initialise Chip8


	int w = 1024;                   // Window width
	int h = 512;                   // Window height

									// The window we'll be rendering to
	SDL_Window* window = NULL;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		exit(1);
	}
	// Create window
	window = SDL_CreateWindow(
		"CHIP-8 Emulator",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		w, h, SDL_WINDOW_SHOWN
	);
	if (window == NULL) {
		printf("Window could not be created! SDL_Error: %s\n",
			SDL_GetError());
		exit(2);
	}

	// Create renderer
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_RenderSetLogicalSize(renderer, w, h);

	// Create texture that stores frame buffer
	SDL_Texture* sdlTexture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		64, 32);

	// Temporary pixel buffer
	uint32_t pixels[2048];


load:

	// Attempt to load ROM & Initialize Debugger
	if (!chip8.load(argv[1],debug_out,debug))
		return 2;

	// Emulation loop
	while (true) {
		chip8.emulate_cycle();

		// Process SDL events
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) exit(0);

			// Process keydown events
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE)
					exit(0);

				if (e.key.keysym.sym == SDLK_F1)
					goto load;      // *gasp*, a goto statement!
									// Used to reset/reload ROM

				for (int i = 0; i < 16; ++i) {
					if (e.key.keysym.sym == keymap[i]) {
						chip8.key[i] = 1;
					}
				}
			}
			// Process keyup events
			if (e.type == SDL_KEYUP) {
				for (int i = 0; i < 16; ++i) {
					if (e.key.keysym.sym == keymap[i]) {
						chip8.key[i] = 0;
					}
				}
			}
		}

		// If draw occurred, redraw SDL screen
		if (chip8.drawFlag) {
			chip8.drawFlag = false;

			// Store pixels in temporary buffer
			for (int i = 0; i < 2048; ++i) {
				uint8_t pixel = chip8.gfx[i];
				pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
			}
			// Update SDL texture
			SDL_UpdateTexture(sdlTexture, NULL, pixels, 64 * sizeof(Uint32));
			// Clear screen and render
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
			SDL_RenderPresent(renderer);
		}

		// Sleep to slow down emulation speed
		std::this_thread::sleep_for(std::chrono::microseconds(1200));

	}
}
