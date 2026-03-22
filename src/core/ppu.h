#pragma once

#include <array>

#include "common/types.h"

namespace Core {
class Mapper;

class PPU {
public:
    static constexpr u16 WIDTH = 256;
    static constexpr u16 HEIGHT = 240;

    static constexpr u32 OPENBUS_DECAY_TICKS_NTSC = 5369318;
    static constexpr u32 OPENBUS_DECAY_TICKS_PAL = 5320342;

    enum class Region : u8 {
        NTSC = 0,
        PAL = 1,
        DENDY = 2,
    };

    static inline constexpr std::array<u32, 64> PALETTE = {
        0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0700,
        0x561D00, 0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000,
        0x000000, 0x000000, 0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC,
        0xB71E7B, 0xB53120, 0x994E00, 0x6B6D00, 0x388700, 0x0C9300, 0x008F32,
        0x007C8D, 0x000000, 0x000000, 0x000000, 0xFFFEFF, 0x64B0FF, 0x9290FF,
        0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22, 0xBCBE00, 0x88D800,
        0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000, 0xFFFEFF,
        0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
        0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000,
        0x000000};

public:
    explicit PPU(Mapper *m = nullptr) : mapper(m) {}
    ~PPU() = default;

public:
    bool debug{false};

    u8 readReg(u16 addr) { return r.readReg(addr); }
    void writeReg(u16 addr, u8 value) { r.writeReg(addr, value); }
    void step() { r.step(); }

public:
    struct State {
        /* Регистры PPU */
        u8 ppuctrl{0};      /* 0x2000: PPUCTRL   */
        u8 ppumask{0};      /* 0x2001: PPUMASK   */
        u8 ppustatus{0xA0}; /* 0x2002: PPUSTATUS */
        u8 oamaddr{0};      /* 0x2003: OAMADDR   */

        /* Регистры PPU для скроллинга */
        u8 w{0};          /* 0x2005/0x2006: latch записи (PPUSCROLL/PPUADDR) */
        u8 fineX{0};      /* 0x2005: fine X scroll */
        u16 v{0};         /* текущий VRAM адрес */
        u16 t{0};         /* временный VRAM адрес */
        u8 dataBuffer{0}; /* 0x2007: буфер чтения PPUDATA */

        u16 pixel{0};     /* PPU dot [0;340] */
        u16 scanline{0};  /* PPU scanline */
        bool nmi{0};      /* ожидание NMI */
        bool nmiLine{0};  /* текущее состояние NMI-линии */
        u8 nmiDelay{0};   /* задержка постановки NMI (в тактах PPU) */
        u8 mirrorMode{0}; /* mirroring */
        bool oddFrame{0}; /* пропуск цикла у нечётных кадров */
        u8 openBus{0};    /* open bus */
        std::array<u32, 8>
            openBusDecay{};     /* таймеры затухания битов open bus */
        bool nmiOutput{0};      /* выход NMI из PPUCTRL */
        bool suppressVblank{0}; /* подавление VBlank/NMI на edge-case */

        std::array<u8, 4096> vram{}; /* nametable/attribute RAM */
        std::array<u8, 32> pal{};    /* palette RAM */
        std::array<u8, 256> oam{};   /* OAM */

        struct BgFetch {
            bool valid{0}; /* валидность конвейера */
            u16 v{0};      /* VRAM адрес тайла */
            u16 table{0};  /* база pattern table */
            u8 p0{0};      /* палитра тайла 0 */
            u8 l0{0};      /* low byte тайла 0 */
            u8 h0{0};      /* high byte тайла 0 */
            u8 p1{0};      /* палитра тайла 1 */
            u8 l1{0};      /* low byte тайла 1 */
            u8 h1{0};      /* high byte тайла 1 */

            u8 nt{0};        /* latched nametable byte */
            u8 at{0};        /* latched attribute byte */
            u8 low{0};       /* latched pattern low byte */
            u8 high{0};      /* latched pattern high byte */
            u16 shLow{0};    /* сдвиговый регистр low plane */
            u16 shHigh{0};   /* сдвиговый регистр high plane */
            u16 shAttrLo{0}; /* сдвиговый регистр attr low */
            u16 shAttrHi{0}; /* сдвиговый регистр attr high */
        } bgFetch{};

        u8 spriteCount{0}; /* кол-во спрайтов в OAM */
        struct SpriteData {
            u8 x{0};    /* позиция X */
            u8 y{0};    /* позиция Y */
            u8 tile{0}; /* индекс тайла */
            u8 attr{0}; /* атрибуты (палитра, flip) */
            u8 id{0};   /* индекс спрайта в OAM */
            u8 low{0};  /* pattern low byte */
            u8 high{0}; /* pattern high byte */
        };
        std::array<SpriteData, 8> OAM{}; /* secondary OAM */

        std::array<u8, 32>
            secOAM{};           /* буфер secondary OAM (8 спрайтов x 4 байта) */
        u8 secOAMAddr{0};       /* индекс записи в secondary OAM */
        u8 primOAMIndex{0};     /* индекс чтения из primary OAM */
        bool spriteEvalDone{0}; /* флаг завершения sprite evaluation */
    } state{};

    State &getState() { return state; }
    void loadState(const State &s) { state = s; }

    void setRegion(Region region) {
        videoMode = region;
        if (videoMode == Region::PAL) {
            preRenderScanline = 311;
            totalScanlines = 312;
            oddFrameDotSkip = false;
            vblankScanline = 241;
        } else if (videoMode == Region::DENDY) {
            preRenderScanline = 311;
            totalScanlines = 312;
            oddFrameDotSkip = false;
            vblankScanline = 291;
        } else {
            preRenderScanline = 261;
            totalScanlines = 262;
            oddFrameDotSkip = true;
            vblankScanline = 241;
        }
    }

public:
    std::array<u32, WIDTH * HEIGHT> frame{};

private:
    Mapper *mapper{nullptr};
    bool frameReady{0};
    Region videoMode{Region::NTSC};
    u16 preRenderScanline{261};
    u16 totalScanlines{262};
    u16 vblankScanline{241};
    bool oddFrameDotSkip{true};

public:
    class R2C02 {
    public:
        explicit R2C02(PPU *p)
            : p(p), state(p->state), frame(p->frame),
              frameReady(p->frameReady) {
            state.vram.fill(0);
            state.pal.fill(0);
            state.oam.fill(0);
            frame.fill(0);
        }
        ~R2C02() = default;

    private:
        PPU *p;

    public:
        State &state;
        std::array<u32, WIDTH * HEIGHT> &frame;
        bool &frameReady;

        void setMirror(u8 m) { state.mirrorMode = m; }
        bool nmiPending() const { return state.nmi; }
        void clearNmi() { state.nmi = 0; }

        u8 readReg(u16 addr);
        void writeReg(u16 addr, u8 data);
        void step();
        std::array<u8, 128 * 128> getPttrnTable(u8 table) const;

    private:
        inline bool rendering() const { return (state.ppumask & 0x18) != 0; }
        inline bool visible() const { return (state.scanline < 240); }
        inline bool preLine() const {
            return (state.scanline == p->preRenderScanline);
        }
        inline bool renderLine() const { return visible() || preLine(); }
        inline bool renderDot() const {
            return (state.pixel >= 1 && state.pixel <= 256);
        }
        inline bool fetchDot() const {
            return renderDot() || (state.pixel >= 321 && state.pixel <= 336);
        }

        u8 readVRAM(u16 addr) const;
        void writeVRAM(u16 addr, u8 data);
        u16 mirrorAddress(u16 addr) const;

        void renderPixel();
        void backgroundPixel(u8 &pixel, u8 &pal);
        void spritePixel(u8 x, u8 &pixel, u8 &pal, u8 &prio, bool &sprite0);
        void evalSprites();
        void bgFetchTick();
        void spriteTimingTick();
        void refreshOpenBus(u8 value, u8 mask = 0xFF);
        void tickOpenBusDecay();
        void updateNmiState(bool delayVblank = false);
        void incrementVRAMAddr();

        inline u16 incrementX(u16 v) {
            return ((v & 0x001F) == 31)
                       ? static_cast<u16>((v & ~0x001F) ^ 0x0400)
                       : static_cast<u16>(v + 1);
        }

        void incrementY() {
            if ((state.v & 0x7000) != 0x7000) {
                state.v += 0x1000;
            } else {
                state.v &= ~0x7000;
                u16 y = static_cast<u16>((state.v & 0x03E0) >> 5);
                if (y == 29) {
                    y = 0;
                    state.v ^= 0x0800;
                } else if (y == 31) {
                    y = 0;
                } else {
                    y++;
                }
                state.v = static_cast<u16>((state.v & ~0x03E0) | (y << 5));
            }
        }

        inline void reloadX() {
            state.v =
                static_cast<u16>((state.v & ~0x041F) | (state.t & 0x041F));
        }
        inline void reloadY() {
            state.v =
                static_cast<u16>((state.v & ~0x7BE0) | (state.t & 0x7BE0));
        }
    };

    R2C02 r{this};
};

} /* namespace Core */
