#include <iostream>
#include <SDL3/SDL.h>

#include "cpu.hpp"
#include "memory.hpp"
#include "ppu.hpp"
#include "cartridge.hpp"
#include "mapperFactory.hpp"



struct Window {
    SDL_Window *w;
    SDL_Renderer *r;
    SDL_Texture *t;
};


int sdl_init(Window* win) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Не удалось инициализировать SDL3: " << SDL_GetError() << std::endl;
        return 0;
    }

    win->w = SDL_CreateWindow("NES.cpp", WIDTH * SCALE, HEIGHT * SCALE, 0);
    if (!win->w) {
        std::cerr << "SDL_CreateWindow: " << SDL_GetError() << std::endl;
        return 0;
    }

    win->r = SDL_CreateRenderer(win->w, NULL);
    if (!win->r) {
        std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << std::endl;
        return 0;
    }

    win->t = SDL_CreateTexture(win->r, 
                               SDL_PIXELFORMAT_ARGB8888, 
                               SDL_TEXTUREACCESS_STREAMING,
                               WIDTH, HEIGHT);
    if (!win->t) {
        std::cerr << "SDL_CreateTexture: " << SDL_GetError() << std::endl;
        return 0;
    }


    SDL_SetTextureScaleMode(win->t, SDL_SCALEMODE_NEAREST);

    return 1;
}


void sdl_free(Window *win) {
    if (win->t) SDL_DestroyTexture(win->t);
    if (win->r) SDL_DestroyRenderer(win->r);
    if (win->w) SDL_DestroyWindow(win->w);
    SDL_Quit();
}


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <rom.nes>" << std::endl;
        return 1;
    }


    Cartridge cart;
    try {
        cart.loadNES(argv[1]);
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка ROM: " << e.what() << std::endl;
        return 1;
    }


    std::unique_ptr<Mapper> mapper;
    try {
        mapper = MapperFactory::createMapper(cart);
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка маппера: " << e.what() << std::endl;
        return 1;
    }


    Ppu ppu;
    ppu.setMapper(mapper.get());
    ppu.setMirror(cart.getMirrorMode());
    
    Memory mem(mapper.get(), &ppu);
    Cpu cpu(&mem);
    cpu.reset();


    Window win{};
    if (!sdl_init(&win)) {
        return 1;
    }

    u32 pixels[WIDTH * HEIGHT];
    bool running = true;
    SDL_Event ev;
    u8 joy = 0;


    while (running) {

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT)
                running = false;
            
            if (ev.type == SDL_EVENT_KEY_DOWN) {
                switch (ev.key.scancode) {
                    case SDL_SCANCODE_ESCAPE: running = false; break;
                    case SDL_SCANCODE_D:      joy |= 0x80; break; /* Вправо  */
                    case SDL_SCANCODE_A:      joy |= 0x40; break; /* Влево   */
                    case SDL_SCANCODE_S:      joy |= 0x20; break; /* Вниз    */
                    case SDL_SCANCODE_W:      joy |= 0x10; break; /* Вверх   */
                    case SDL_SCANCODE_RETURN: joy |= 0x08; break; /* Старт   */
                    case SDL_SCANCODE_LSHIFT: joy |= 0x04; break; /* Выбрать */
                    case SDL_SCANCODE_X:      joy |= 0x02; break; /* B       */
                    case SDL_SCANCODE_Z:      joy |= 0x01; break; /* A       */
                    default: break;
                } 
            }
            
            if (ev.type == SDL_EVENT_KEY_UP) {
                switch (ev.key.scancode) {
                    case SDL_SCANCODE_D:      joy &= ~0x80; break; /* Вправо  */
                    case SDL_SCANCODE_A:      joy &= ~0x40; break; /* Влево   */
                    case SDL_SCANCODE_S:      joy &= ~0x20; break; /* Вниз    */
                    case SDL_SCANCODE_W:      joy &= ~0x10; break; /* Вверх   */
                    case SDL_SCANCODE_RETURN: joy &= ~0x08; break; /* Старт   */
                    case SDL_SCANCODE_LSHIFT: joy &= ~0x04; break; /* Выбрать */
                    case SDL_SCANCODE_X:      joy &= ~0x02; break; /* B       */
                    case SDL_SCANCODE_Z:      joy &= ~0x01; break; /* A       */
                    default: break;
                }
            }
        }


        mem.setJoy1(joy);


        bool frameCompleted = false;
        while (!frameCompleted) {
            cpu.exec();


            for (int i=0; i<3; ++i) {
                ppu.cycle();
                
                if (ppu.frameReady) {
                    ppu.frameReady = false;
                    frameCompleted = true;
                    break;
                }
            }


            if (ppu.nmiPending()) {
                cpu.do_nmi = true;
                ppu.clearNmi();
            }
        }


        ppu.copyFrame(pixels);
        SDL_UpdateTexture(win.t, nullptr, pixels, WIDTH * sizeof(u32));
        SDL_RenderClear(win.r);
        SDL_RenderTexture(win.r, win.t, nullptr, nullptr);
        SDL_RenderPresent(win.r);


        SDL_Delay(1000/100);
    }

    sdl_free(&win);
    return 0;
}
