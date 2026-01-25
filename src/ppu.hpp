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
    void step();
    
    auto nmiPending() const -> bool { return nmi; }
    void clearNmi() { nmi = false; }
    auto getFrame() const -> const u32* { return frame.data(); }
    
    bool frameReady{false};

public:
    enum : u8 {
        HORIZONTAL  = 0,  /* [A,A,B,B] */
        VERTICAL    = 1,  /* [A,B,A,B] */
        FOUR        = 2,  /* [A,B,C,D] */
        SINGLE_DOWN = 3,  /* [A,A,A,A] */
        SINGLE_UP   = 4,  /* [B,B,B,B] */
    };

    /* Регистры PPU */
    u8 ppuctrl{0};
    u8 ppumask{0};
    u8 ppustatus{0xA0};
    u8 oamaddr{0};

    /* Регистры PPU для скроллинга */
    u8  w{0};
    u8  fineX{0};
    u16 v{0};
    u16 t{0};
    u8  dataBuffer{0};

private:
    u16  pixel{0};
    u16  scanline{0};
    bool nmi{false};
    u8   mirrorMode{HORIZONTAL};
    bool oddFrame{false};
    
    Mapper* mapper{nullptr};
    
    std::array<u8, 4096> vram;
    std::array<u8, 32>   pal;
    std::array<u8, 256>  oam;
    std::array<u32, WIDTH * HEIGHT> frame;
    

    u8 spriteCount{0};
    struct SpriteData 
    {
        u8 y, tile, attr, x;
        u8 id;
    };
    std::array<SpriteData, 8> OAM;

private:
    auto readVRAM (u16 addr) const -> u8;
    void writeVRAM(u16 addr, u8 data);
    auto mirrorAddress(u16 addr) const -> u16;
    
    void renderPixel();
    void backgroundPixel(u8 x, u8 &pixel, u8 &pal);
    void spritePixel(u8 x, u8 y, u8 &pixel, u8 &pal, u8 &prio, bool &sprite0);
    void evalSprites();
    
    inline void incrementX() { 
        if ((v & 0x001F) == 31) {
            v = (v & ~0x001F) ^ 0x0400;
        } else {
            v++;
        }
    }
    
    void incrementY() { 
        if ((v & 0x7000) != 0x7000) {
            v += 0x1000;
        } else {
            v &= ~0x7000;
            u16 y = (v & 0x03E0) >> 5;
            if (y == 29) {
                y = 0;
                v ^= 0x0800;
            } else if (y == 31) {
                y = 0;
            } else {
                y++;
            }
            v = (v & ~0x03E0) | (y << 5);
        }
    }
    
    inline void reloadX() { v = (v & ~0x041F) | (t & 0x041F); }
    inline void reloadY() { v = (v & ~0x7BE0) | (t & 0x7BE0); }
};