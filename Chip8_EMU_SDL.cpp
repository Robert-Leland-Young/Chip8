// Chip8_EMU_SDL.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <chrono>
#include <thread>
#include "stdint.h"
#include "SDL.h"      /* SDL 2*/
#include "chip8.h"

#include "windows.h"

using namespace std;


uint32_t keymap[16] = {
	SDLK_KP_PERIOD,		// 0
	SDLK_KP_1,			// 1
	SDLK_KP_2,			// 2
	SDLK_KP_3,			// 3
	SDLK_KP_4,			// 4
	SDLK_KP_5,			// 5
	SDLK_KP_6,			// 6
	SDLK_KP_7,			// 7
	SDLK_KP_8,			// 8
	SDLK_KP_9,			// 9
	SDLK_KP_0,			// A
	SDLK_KP_ENTER,		// B
	SDLK_KP_DIVIDE,		// C
	SDLK_KP_MULTIPLY,	// D
	SDLK_KP_MINUS,		// E
	SDLK_KP_PLUS		// F
};



void start_SDL(unsigned gfx_x, unsigned gfx_y, bool gfx_mode);

SDL_Window* window = NULL;
SDL_Renderer *renderer;
SDL_Texture* sdlTexture;


int main(int argc, char **argv) {

	unsigned char debug_out[256];  /* Debug output filename */
	int debug;		/* Debug On > 0 or OFF = 0 */


	// Command usage
	if (argc < 2) {
		int i, j;

		/* Parse out filename */
		j = strlen(argv[0]);
		for (i = j; j > 0; j--) if (*(argv[0] + j) == '\\') break;
		sprintf((char *)debug_out, argv[0] + j); 
		printf("Usage:\r\n\r\n%s ROM_file Debug Output_File\r\n\r\n",debug_out);
		cout << "ROM_file    -   Filename of Chip-8/48 program to Execute\r\n" << endl;
		cout << "Debug       -   Initiate Debug Trace in Console Window or File\r\n" << endl;
		cout << "Output_File -   Output Filename for Debug Trace Information\r\n" << endl;
		cout << "Notes:\r\n" << endl;
		cout << "The Debug and Output_File parameters are optional.\r\n" ;
		cout << "To enter Debug Mode from the Console Window\r\n" ;
		cout << "Press any key. Backspace is recommended.\r\n" ;
		cout << "The Console Window must have the Focus to Debug." << endl;
		cout << "Console Debug Mode allows one to Trace Execution," << endl;
		cout << "Single Step, Set a Stop Address, and Dump Memory." << endl;
		cout << "Left and Right Mouse Clicks Pause/Start the Trace." << endl;
		cout << "The Debugger is always started in Single Step Mode" << endl;
		cout << "when the Debug command line parameter is used." << endl;

		return 1;
	}

	if (argc < 3) debug = 0;  /* Debug is off */
	else debug = 2; /* Debug is on, initially Single Step Mode */

	if (argc < 4) *debug_out = '\0';
	else strcpy((char *)debug_out, argv[3]);

	Chip8 chip8 = Chip8();          // Initialise Chip8

	SetConsoleTitle(TEXT("Chip-8/48 Debugger"));

	// Assume Low Resolution Graphics

	chip8.gfx_mode = false;  // Assume Low resolution
	chip8.gfx_x = 64;		// 64 pixel & 64 byte buffer horixontal dimension
	chip8.gfx_y = 32;		// 32 pixel & 32 byte buffer vertical dimension


// High Resolution Graphics
/*
	chip8.gfx_mode = TRUE;  // Assume High resolution
	chip8.gfx_x = 128;		// 128 pixel & 128 byte buffer horixontal dimension
	chip8.gfx_y = 64;		// 64 pixel & 64 byte buffer vertical dimension
*/


	// Temporary pixel buffer, Assume High Resolution
	uint32_t pixels[64 * 128];


load:

	start_SDL(chip8.gfx_x, chip8.gfx_y, chip8.gfx_mode);		// Start SDL Graphics


	// Attempt to load ROM & Initialize Debugger
	if (!chip8.load(argv[1], debug_out, debug))
	{
		SDL_Quit();
		return 2;
	}
	if (argc < 3) printf("\r\nPress <Escape> or <BACKSPACE> to Debug\r\n");

	// Emulation loop
	while (true) {

		if (chip8.DEBUG!=4) chip8.emulate_cycle();  // Execute instructions
		else { // Pause instruction execution & Allow user to view Graphics screen
			char ins[] = "Pause Mode";
			chip8.trace(ins);  /* Debud Trace */
			chip8.drawFlag = true;
		} // end, Pause instruction execution & Allow user to view Graphics screen


		// Process SDL events
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) 
				exit(0);

			// Process keydown events
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					SDL_Quit();
					exit(0);
				}
				if (e.key.keysym.sym == SDLK_F1) {
					SDL_Quit();
					goto load;      // *gasp*, a goto statement!
									// Used to reset/reload ROM
				}

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
			for (unsigned i = 0; i < chip8.gfx_size; ++i) {
				uint8_t pixel = chip8.gfx[i];
				pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
			}
			// Update SDL texture
			SDL_UpdateTexture(sdlTexture, NULL, pixels, chip8.gfx_x * sizeof(Uint32));
			// Clear screen and render
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
			SDL_RenderPresent(renderer);
		}

		// Sleep to slow down emulation speed
		std::this_thread::sleep_for(std::chrono::microseconds(1200));

	}
}



void start_SDL(unsigned gfx_x, unsigned gfx_y, bool gfx_mode)
{
	int w, h;


	SDL_Quit();
	
	// The window we'll be rendering to

// Old Original Window Size
	if (gfx_mode == true) { /* High Resolution Graphics */
		w = 896;			      // Window width in pixels
		h = 644;		          // Window height in pixels

	} /* end, High Resolution Graphics */

	else { // Low Resolution Graphics Mode
			w = 768;                   // Window width in pixels
			h = 512;                   // Window height in pixels
	} // end, Low Resolution Graphics 

// Initialize SDL
if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
	printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	SDL_Quit();
	exit(1);
}
// Create window
window = SDL_CreateWindow(
	"CHIP-8/48 Emulator",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	w, h, SDL_WINDOW_SHOWN
);
if (window == NULL) {
	printf("Window could not be created! SDL_Error: %s\n",
		SDL_GetError());
	SDL_Quit();
	exit(2);
}

// Create renderer
renderer = SDL_CreateRenderer(window, -1, 0);
SDL_RenderSetLogicalSize(renderer, w, h);

// Create texture that stores frame buffer
sdlTexture = SDL_CreateTexture(renderer,
	SDL_PIXELFORMAT_ARGB8888,
	SDL_TEXTUREACCESS_STREAMING,
	gfx_x, gfx_y);

return;
}