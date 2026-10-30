#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>               
#include <SDL2/SDL.h>

#define SCALE 10

// look settings
#define RISE   0.6f
#define DECAY  0.85f
#define TINT_R 0.92f
#define TINT_G 0.95f
#define TINT_B 1.0f

SDL_Window   *window;
SDL_Renderer *renderer;

uint8_t  memory[4096];
uint8_t  V[16];
uint8_t  delay_timer;
uint8_t  sound_timer;
uint16_t pc;
uint16_t I;
uint8_t  display[64 * 32];
uint16_t stack[16];
uint8_t  sp = 0;
uint8_t  keys[16];

float             glow[64 * 32];    // per-pixel brightness
int use_glow = 1;
SDL_AudioDeviceID audio_dev = 0;    
int               audio_rate = 44100; 

uint8_t font[80] = {
    0xF0,0x90,0x90,0x90,0xF0, // 0
    0x20,0x60,0x20,0x20,0x70, // 1
    0xF0,0x10,0xF0,0x80,0xF0, // 2
    0xF0,0x10,0xF0,0x10,0xF0, // 3
    0x90,0x90,0xF0,0x10,0x10, // 4
    0xF0,0x80,0xF0,0x10,0xF0, // 5
    0xF0,0x80,0xF0,0x90,0xF0, // 6
    0xF0,0x10,0x20,0x40,0x40, // 7
    0xF0,0x90,0xF0,0x90,0xF0, // 8
    0xF0,0x90,0xF0,0x10,0xF0, // 9
    0xF0,0x90,0xF0,0x90,0x90, // A
    0xE0,0x90,0xE0,0x90,0xE0, // B
    0xF0,0x80,0x80,0x80,0xF0, // C
    0xE0,0x90,0x90,0x90,0xE0, // D
    0xF0,0x80,0xF0,0x80,0xF0, // E
    0xF0,0x80,0xF0,0x80,0x80  // F
};

int init_graphics(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;

    window = SDL_CreateWindow("Chip-8",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              64 * SCALE, 32 * SCALE, 0);
    if (!window) return 0;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 0;

    return 1;
}

// square wave while sound_timer > 0
void audio_callback(void *userdata, Uint8 *stream, int len) {
    static float phase = 0.0f;
    static float amp = 0.0f;
    float *out = (float *)stream;
    int count = len / sizeof(float);
    float target = (sound_timer > 0) ? 0.15f : 0.0f;
    for (int i = 0; i < count; i++) {
        amp += (target - amp) * 0.002f;     // ramp avoids clicks
        phase += 440.0f / audio_rate;
        if (phase >= 1.0f) phase -= 1.0f;
        out[i] = (phase < 0.5f) ? amp : -amp;
    }
}

// opens audio, continues silently if it fails
void init_audio(void) {
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_dev == 0) {
        printf("Audio unavailable: %s\n", SDL_GetError());
        return;
    }
    audio_rate = have.freq;
    SDL_PauseAudioDevice(audio_dev, 0);
}

int map_key(SDL_Keycode k) {
    switch (k) {
        case SDLK_1: return 0x1;  case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;  case SDLK_4: return 0xC;
        case SDLK_q: return 0x4;  case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;  case SDLK_r: return 0xD;
        case SDLK_a: return 0x7;  case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;  case SDLK_f: return 0xE;
        case SDLK_z: return 0xA;  case SDLK_x: return 0x0;
        case SDLK_c: return 0xB;  case SDLK_v: return 0xF;
    }
    return -1;
}

void render(void) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 64; col++) {
            if (display[row * 64 + col]) {
                SDL_Rect r = { col * SCALE, row * SCALE, SCALE, SCALE };
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
    SDL_RenderPresent(renderer);
}


// old-console look: solid pixels, slow fade
void render_phosphor(double dt_ms) {
    float frames = (float)(dt_ms * 60.0 / 1000.0);
    float rise  = 1.0f - powf(1.0f - RISE, frames);
    float decay = powf(DECAY, frames);

    SDL_SetRenderDrawColor(renderer, 8, 8, 12, 255);   // near-black background
    SDL_RenderClear(renderer);

    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 64; col++) {
            int i = row * 64 + col;

            // lit pixels brighten fast, unlit ones fade slowly
            if (display[i]) glow[i] += (1.0f - glow[i]) * rise;
            else            glow[i] *= decay;

            if (glow[i] < 0.03f) continue;
            float b = glow[i];

            SDL_SetRenderDrawColor(renderer,
                8 + (uint8_t)(b * 247 * TINT_R),
                8 + (uint8_t)(b * 247 * TINT_G),
                12 + (uint8_t)(b * 243 * TINT_B), 255);
            SDL_Rect r = { col * SCALE, row * SCALE, SCALE, SCALE };
            SDL_RenderFillRect(renderer, &r);
        }
    }
    SDL_RenderPresent(renderer);
}

void init(void) {
    memset(memory, 0, sizeof(memory));
    memset(V, 0, sizeof(V));
    memset(display, 0, sizeof(display));
    memset(glow, 0, sizeof(glow));      // NEW
    memset(keys, 0, sizeof(keys));
    memset(stack, 0, sizeof(stack));
    memcpy(&memory[0x50], font, sizeof(font));
    I = 0;
    pc = 0x200;
    sp = 0;
    delay_timer = 0;
    sound_timer = 0;
}

int load_rom(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    
    //find file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    
    if (size <= 0){
        printf("Rom is empty : %s\n Rechecck rom\n", filename);
        fclose(f);
        return 0;
    }
    
    if (size > 3584){   // if size greater than 4096 - 0x200 flag rom too large
        printf("Rom too large: %ld bytes. Max available is 3584 bytes", size);
        fclose(f);
        return 0;
    }
    size_t got = fread(&memory[0x200], 1, size, f);
    fclose(f);
    
    if (got != (size_t) size){
        printf("Failed to read Rom fully: %s\n", filename);
        return 0;
    }
    return 1;
}

void cycle(void) {
    // FETCH
    pc &= 0xFFF;
    uint16_t opcode = (memory[pc] << 8) | memory[(pc + 1) & 0xFFF];
    pc += 2;

    // DECODE
    uint8_t  x   = (opcode & 0x0F00) >> 8;
    uint8_t  y   = (opcode & 0x00F0) >> 4;
    uint8_t  n   =  opcode & 0x000F;
    uint8_t  nn  =  opcode & 0x00FF;
    uint16_t nnn =  opcode & 0x0FFF;

    // EXECUTE
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (nn) {
                case 0xE0:              // 00E0: clear screen
                    memset(display, 0, sizeof(display));
                    break;
                case 0xEE:              // 00EE: return from subroutine
                    if (sp > 0) {
                        sp--;
                        pc = stack[sp];
                    }
                    break;
            }
            break;

        case 0x1000:                    // 1NNN: jump
            pc = nnn;
            break;

        case 0x2000:                    // 2NNN: call subroutine
            if (sp < 16) {
                stack[sp] = pc;
                sp++;
            }
            pc = nnn;
            break;

        case 0x3000:                    // 3XNN: skip if V[x] == nn
            if (V[x] == nn) pc += 2;
            break;

        case 0x4000:                    // 4XNN: skip if V[x] != nn
            if (V[x] != nn) pc += 2;
            break;

        case 0x5000:                    // 5XY0: skip if V[x] == V[y]
            if (V[x] == V[y]) pc += 2;
            break;

        case 0x6000:                    // 6XNN: V[x] = nn
            V[x] = nn;
            break;

        case 0x7000:                    // 7XNN: V[x] += nn
            V[x] += nn;
            break;

        case 0x8000:                    // 8XY_: math and logic
            switch (n) {
                case 0x0: V[x] = V[y]; break;
                case 0x1: V[x] |= V[y]; break;
                case 0x2: V[x] &= V[y]; break;
                case 0x3: V[x] ^= V[y]; break;
                case 0x4: {             // add with carry
                    uint16_t sum = V[x] + V[y];
                    V[x] = sum & 0xFF;
                    V[0xF] = sum > 255;
                    break;
                }
                case 0x5: {             // V[x] = V[x] - V[y]
                    uint8_t flag = V[x] >= V[y];
                    V[x] -= V[y];
                    V[0xF] = flag;
                    break;
                }
                case 0x6: {             // shift right
                    uint8_t flag = V[x] & 1;
                    V[x] >>= 1;
                    V[0xF] = flag;
                    break;
                }
                case 0x7: {             // V[x] = V[y] - V[x]
                    uint8_t flag = V[y] >= V[x];
                    V[x] = V[y] - V[x];
                    V[0xF] = flag;
                    break;
                }
                case 0xE: {             // shift left
                    uint8_t flag = (V[x] >> 7) & 1;
                    V[x] <<= 1;
                    V[0xF] = flag;
                    break;
                }
            }
            break;

        case 0x9000:                    // 9XY0: skip if V[x] != V[y]
            if (V[x] != V[y]) pc += 2;
            break;

        case 0xA000:                    // ANNN: I = nnn
            I = nnn;
            break;

        case 0xB000:                    // BNNN: jump to nnn + V0
            pc = nnn + V[0];
            break;

        case 0xC000:                    // CXNN: V[x] = random & nn
            V[x] = (rand() % 256) & nn;
            break;

        case 0xD000: {                  // DXYN: draw sprite
            uint8_t px = V[x] % 64;
            uint8_t py = V[y] % 32;
            V[0xF] = 0;

            for (int row = 0; row < n; row++) {
                if (py + row >= 32) break;
                uint8_t sprite_byte = memory[(I + row) & 0xFFF];

                for (int col = 0; col < 8; col++) {
                    if (px + col >= 64) break;
                    if (sprite_byte & (0x80 >> col)) {
                        int idx = (py + row) * 64 + (px + col);
                        if (display[idx]) V[0xF] = 1;
                        display[idx] ^= 1;
                    }
                }
            }
            break;
        }

        case 0xE000:
            if (nn == 0x9E) {           // EX9E: skip if key pressed
                if (keys[V[x] & 0xF]) pc += 2;
            } else if (nn == 0xA1) {    // EXA1: skip if key NOT pressed
                if (!keys[V[x] & 0xF]) pc += 2;
            }
            break;

        case 0xF000:
            switch (nn) {
                case 0x07: V[x] = delay_timer; break;
                case 0x0A: {            // wait for a keypress
                    int found = 0;
                    for (int i = 0; i < 16; i++) {
                        if (keys[i]) { V[x] = i; found = 1; break; }
                    }
                    if (!found) pc -= 2;    // repeat until a key is pressed
                    break;
                }
                case 0x15: delay_timer = V[x]; break;
                case 0x18: sound_timer = V[x]; break;
                case 0x1E: I += V[x]; break;
                case 0x29: I = 0x50 + (V[x] & 0xF) * 5; break;
                case 0x33:              // binary-coded decimal
                    memory[I & 0xFFF]       = V[x] / 100;
                    memory[(I + 1) & 0xFFF] = (V[x] / 10) % 10;
                    memory[(I + 2) & 0xFFF] = V[x] % 10;
                    break;
                case 0x55:              // store V0..Vx into memory
                    for (int i = 0; i <= x; i++) memory[(I + i) & 0xFFF] = V[i];
                    break;
                case 0x65:              // load V0..Vx from memory
                    for (int i = 0; i <= x; i++) V[i] = memory[(I + i) & 0xFFF];
                    break;
            }
            break;

        default:
            printf("opcode %04X not implemented yet\n", opcode);
    }
}

int main(int argc, char **argv) {
    const char *rom = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-glow") == 0) {
            use_glow = 0;
        } else if (strncmp(argv[i], "--", 2) == 0) {
            printf("Unknown option: %s\n", argv[i]);
            return 1;
        } else {
            if (rom) {
                printf("Only one ROM file allowed\n");
                return 1;
            }
            rom = argv[i];
        }
    }

    if (!rom) {
        printf("Usage: chip [--no-glow] romfile\n");
        return 1;
    }

    init();
    if (!load_rom(rom)) {
        printf("Could not load romfile %s\n", rom);
        return 1;
    }

    if (!init_graphics()) {
        printf("SDL failed %s\n", SDL_GetError());
        return 1;
    }
    init_audio();                     

    int    ips = 700;                   // instructions per second (+ / -)
    int    paused = 0;
    double cpu_acc = 0.0;
    double timer_acc = 0.0;
    Uint32 last = SDL_GetTicks();

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode s = e.key.keysym.sym;       // NEW: hotkeys
                if (s == SDLK_ESCAPE) running = 0;
                if (s == SDLK_p) paused = !paused;
                if (s == SDLK_BACKSPACE) {              // reset
                    init();
                    load_rom(argv[1]);
                    cpu_acc = timer_acc = 0.0;
                }
                if ((s == SDLK_EQUALS || s == SDLK_KP_PLUS) && ips < 5000) ips += 100;
                if ((s == SDLK_MINUS || s == SDLK_KP_MINUS) && ips > 100) ips -= 100;

                int k = map_key(e.key.keysym.sym);
                if (k >= 0) keys[k] = 1;
            }
            if (e.type == SDL_KEYUP) {
                int k = map_key(e.key.keysym.sym);
                if (k >= 0) keys[k] = 0;
            }
        }

        Uint32 now = SDL_GetTicks();
        double dt = now - last;
        last = now;
        if (dt > 100.0) dt = 100.0;

        if (!paused) {
            // CPU runs at ips instead of a fixed 10 per frame
            cpu_acc += dt * ips / 1000.0;
            while (cpu_acc >= 1.0) { cycle(); cpu_acc -= 1.0; }

            // timers tick at exactly 60 Hz
            timer_acc += dt;
            while (timer_acc >= 1000.0 / 60.0) {
                if (delay_timer > 0) delay_timer--;
                if (sound_timer > 0) sound_timer--;
                timer_acc -= 1000.0 / 60.0;
            }
        }

        if (use_glow) render_phosphor(dt);
        else          render();          
        SDL_Delay(4);                   // was SDL_Delay(16)
    }

    if (audio_dev) SDL_CloseAudioDevice(audio_dev);     
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
