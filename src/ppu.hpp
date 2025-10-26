#pragma once
#include "mapper.hpp"


class Ppu {
public:
    explicit Ppu() { 
        vram.fill(0); 
        pal.fill(0);
        oam.fill(0); 
        frame.fill(0); 
    }
    ~Ppu() = default;


    void setMapper(Mapper *m) { mapper = m; }
    void setMirror(u8 m) { mirrorMode = m; }

    auto readReg (u16 addr) -> u8;
    void writeReg(u16 addr, u8 value);
    void cycle();

    auto nmiPending() const -> bool { return nmi; }
    void clearNmi() { nmi = false; }
    void copyFrame(u32 *rgba) const;

public:
    enum : u8 {
        HORIZONTAL = 0, /* Горизонтальный миррор */
        VERTICAL   = 1, /* Вертикальный миррор   */
        FOUR       = 2, /* Четыре экрана         */
        SINGLE     = 3  /* Один экран            */
    };

    bool frameReady = false;

private:
    /* Регистры PPU */
    u8 ppuctrl{0};
    u8 ppumask{0};
    u8 ppustatus{0xA0};
    u8 oamaddr{0};

    /* Счетчики */
    u16  dot{0};
    i16  scanline{0};
    bool nmi{false};

    /* Регистры прокрутки */
    u8  w{0};          /* Переключатель записи (0/1) */
    u8  fineX{0};      /* Тонкая прокрутка X (0-7)   */
    u16 v{0};          /* Текущий адрес VRAM         */
    u16 t{0};          /* Временный адрес VRAM       */
    u8  dataBuffer{0}; /* Буфер для чтения PPUDATA   */
    
    u8 mirrorMode{HORIZONTAL};
    Mapper* mapper{nullptr};

    /* Память */
    std::array<u8, 2048> vram; /* Nametables: 0x2000-0x3EFF */
    std::array<u8, 32>   pal;  /* Палитра:    0x3F00-0x3FFF */
    std::array<u8, 256>  oam;  /* Спрайты                   */

    /* Кадр */
    std::array<u32, WIDTH * HEIGHT> frame{};

private:
    /* Чтение/запись VRAM */
    auto readVRAM (u16 addr) const -> u8;
    void writeVRAM(u16 addr, u8 data);
    auto mirrorAddress(u16 addr) const -> u16;
    auto rgb(u8 palIdx) const -> u32;

    /* Рендеринг */
    void renderPixel();
    void backgroundPixel(u8 x, u8 y, u8 &pixel, u8 &pal);
    void spritePixel(u8 x, u8 y, u8 &pixel, u8 &pal, u8 &prio, bool &sprite0);


    void incrementX() { v = ((v & 0x001F) == 31) ? (v&~0x001F) ^ 0x0400 : v+1; }
    
    void incrementY();
    void reloadX() { v = (v & ~0x041F) | (t & 0x041F); }
    void reloadY() { v = (v & ~0x7BE0) | (t & 0x7BE0); }
};
