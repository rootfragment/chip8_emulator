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

void cycle(void){
	uint16_t opcode = (memory[pc] << 8) | memory[pc+1];
	pc += 2;
	uint8_t x = (opcode & 0x0F00) >> 8;
	uint8_t y = (opcode & 0x0F00) >> 4;
	uint8_t n = opcode & 0x000F;
	uint8_t nn = opcode & 0x00FF;
	uint16_t nnn = opcode & 0x0FFF;

	printf("pc = %03x opcode= %04x\n", pc -2,opcode);

	switch (opcode & 0xF000){
		case 0x1000:
			pc = nnn;
			break;
		case 0x6000:
			v[x] = nn;
			break;
		case 0xA000:
			I = nnn;
			break;
		default:
			printf("Not defined yet \n");
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
	for(int i = 0; i<10; i++){
		cycle();
	}
	printf("v0 = %d,v1 = %d I=%03x\n",v[0],v[1],I);
	return 0;
}

