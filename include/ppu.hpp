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
    u8 ppuctrl{0};   /* 0x2000: PPUCTRL      */
    u8 ppumask{0};   /* 0x2001: PPUMASK      */
    u8 ppustatus{0xA0}; /* 0x2002: PPUSTATUS */
    u8 oamaddr{0};   /* 0x2003: OAMADDR      */

    /* Регистры PPU для скроллинга */
    u8  w{0};        /* 0x2005/0x2006: запись (PPUSCROLL/PPUADDR) */
    u8  fineX{0};    /* 0x2005: fine X scroll              */
    u16 v{0};        /* 0x2007: текущий VRAM адрес         */
    u16 t{0};        /* 0x2007: временный VRAM адрес       */
    u8  dataBuffer{0}; /* 0x2007: буфер чтения PPUDATA     */

private:
    u16  pixel{0};       /* PPU dot [0;340]      */
    u16  scanline{0};    /* PPU scanline [0;261] */
    bool nmi{false};     /* ожидание NMI         */
    u8   mirrorMode{HORIZONTAL}; /* mirroring    */
    bool oddFrame{false}; /* пропуск цикла у нечётных кадров */
    u8   openBus{0};     /* open bus             */ 


    Mapper* mapper{nullptr};
    
    std::array<u8, 4096> vram; /* nametable/attribute RAM  */
    std::array<u8, 32>   pal;  /* palette RAM              */
    std::array<u8, 256>  oam;  /* OAM                      */
    std::array<u32, WIDTH * HEIGHT> frame; /* готовый кадр */
    

    struct BgFetch {
        bool valid{false};
        u16 v{0};     /* VRAM адрес тайла   */
        u16 table{0}; /* база pattern table */
        u8 p0{0};     /* палитра тайла 0    */
        u8 l0{0};     /* low byte тайла 0   */
        u8 h0{0};     /* high byte тайла 0  */
        u8 p1{0};     /* палитра тайла 1    */
        u8 l1{0};     /* low byte тайла 1   */
        u8 h1{0};     /* high byte тайла 1  */
    };

    BgFetch bgFetch;

    u8 spriteCount{0}; /* кол-во спрайтов в OAM */
    struct SpriteData {
        u8 x;    /* позиция X                */
        u8 y;    /* позиция Y                */
        u8 tile; /* индекс тайла             */
        u8 attr; /* атрибуты (палитра, flip) */
        u8 id;   /* индекс спрайта в OAM     */
        u8 low;  /* pattern low byte         */
        u8 high; /* pattern high byte        */
    };
    std::array<SpriteData, 8> OAM; /* OAM */

private:
    /* Булевые функции для рендеринга */
    inline auto rendering() const -> bool  { return (ppumask & 0x18) != 0; }
    inline auto visible() const -> bool    { return (scanline < 240); }
    inline auto preLine() const -> bool    { return (scanline == 261); }
    inline auto renderLine() const -> bool { return visible() || preLine(); }
    inline auto renderDot() const -> bool  { return (pixel >= 1 && pixel <= 256); }   

private:
    auto readVRAM (u16 addr) const -> u8;
    void writeVRAM(u16 addr, u8 data);
    auto mirrorAddress(u16 addr) const -> u16;
    
    void renderPixel();
    void backgroundPixel(u8 x, u8 &pixel, u8 &pal);
    void spritePixel(u8 x, u8 &pixel, u8 &pal, u8 &prio, bool &sprite0);
    void evalSprites();


    inline u16 incrementX(u16 v) { return ((v & 0x001F) == 31) ? (v & ~0x001F) ^ 0x0400 : v+1; }
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