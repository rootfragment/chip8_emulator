#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<SDL2/SDL.h>


#define SCALE 10

SDL_Window *window;
SDL_Renderer *renderer;


uint8_t memory[4096];
uint8_t v[16];
uint8_t delay_timer;
uint8_t sound_timer;
uint16_t pc;
uint16_t I;
uint8_t display[64 * 32];
uint16_t stack[16];
uint8_t  sp = 0;

int init_graphics(void){
	if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;

	window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 64 * SCALE, 32 * SCALE, 0);
	if (!window) return 0;

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) return 0;

	return 1;
}

void render(void){
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for(int row = 0; row <32; row++){
		for (int col =0; col < 64; col++){
			if (display[row*64 + col]){
				SDL_Rect r = { col * SCALE, row * SCALE, SCALE, SCALE};
				SDL_RenderFillRect(renderer, &r);
			}
		}
	}
	SDL_RenderPresent(renderer);
}



void init(void){
	memset(memory, 0, sizeof(memory));
	memset(v, 0, sizeof(v));
	memset(display, 0, sizeof(I));
	I = 0;
	pc = 0x200;
	delay_timer = 0;
	sound_timer = 0;
}

int load_rom(const char *filename){
	FILE *f = fopen(filename, "rb");
	if (!f) return 0;
	fread(&memory[0x200], 1, 4096 - 0x200, f);
	fclose(f);
	return 1;
}

void cycle(void) {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;


    uint8_t  x   = (opcode & 0x0F00) >> 8;
    uint8_t  y   = (opcode & 0x00F0) >> 4;
    uint8_t  n   =  opcode & 0x000F;
    uint8_t  nn  =  opcode & 0x00FF;
    uint16_t nnn =  opcode & 0x0FFF;


    switch (opcode & 0xF000) {
        case 0x0000:
            switch (nn) {
                case 0xE0:              // 00E0: clear screen
                    memset(display, 0, sizeof(display));
                    break;
                case 0xEE:              // 00EE: return from subroutine
                    sp--;
                    pc = stack[sp];
                    break;
            }
            break;

        case 0x1000:                    // 1NNN: jump
            pc = nnn;
            break;

        case 0x2000:                    // 2NNN: call subroutine
            stack[sp] = pc;
            sp++;
            pc = nnn;
            break;

        case 0x3000:                    // 3XNN: skip if v[x] == nn
            if (v[x] == nn) pc += 2;
            break;

        case 0x4000:                    // 4XNN: skip if v[x] != nn
            if (v[x] != nn) pc += 2;
            break;

        case 0x5000:                    // 5XY0: skip if v[x] == v[y]
            if (v[x] == v[y]) pc += 2;
            break;

        case 0x6000:                    // 6XNN: v[x] = nn
            v[x] = nn;
            break;

        case 0x7000:                    // 7XNN: v[x] += nn
            v[x] += nn;
            break;

        case 0x9000:                    // 9XY0: skip if v[x] != v[y]
            if (v[x] != v[y]) pc += 2;
            break;

        case 0xA000:                    // ANNN: I = nnn
            I = nnn;
            break;

        case 0xD000: {                  // DXYN: draw sprite
            uint8_t px = v[x] % 64;
            uint8_t py = v[y] % 32;
            v[0xF] = 0;

            for (int row = 0; row < n; row++) {
                if (py + row >= 32) break;
                uint8_t sprite_byte = memory[I + row];

                for (int col = 0; col < 8; col++) {
                    if (px + col >= 64) break;
                    if (sprite_byte & (0x80 >> col)) {
                        int idx = (py + row) * 64 + (px + col);
                        if (display[idx]) v[0xF] = 1;
                        display[idx] ^= 1;
                    }
                }
            }
            break;
        }

        default:
            printf("opcode %04X not implemented yet\n", opcode);
    }
}

int main(int argc, char **argv){
	if (argc < 2) {
		printf("Usage: chip romfile\n");
	return 1;
	}
	init();
	if (!load_rom(argv[1])){
		printf("Could not open romfile %s", argv[1]);
		return 1;
	}
	if (!init_graphics()){
		printf("SDL failed %s\n", SDL_GetError());
		return 1;
	}
	int running = 1;
	while (running){
		SDL_Event e;
		while(SDL_PollEvent(&e)){
			if(e.type == SDL_QUIT) running =0;
		}
		for(int i = 0; i< 10; i++) cycle();
		if (delay_timer > 0) delay_timer--;
		if (sound_timer > 0) sound_timer--;
		render();
		SDL_Delay(16);
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

