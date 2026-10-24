#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
 
uint8_t memory[4096];
uint8_t v[16];
uint8_t delay_timer;
uint8_t sound_timer;
uint16_t pc;
uint16_t I;
uint8_t display[64 * 32];

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
	printf("Rom successfully loaded. First two bytes %02x %02x\n",memory[0x200], memory[0x201]);
	return 0;
}

