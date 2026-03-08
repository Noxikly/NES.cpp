#pragma once

#include <array>

#include "common.hpp"

class Mapper;


class PPU {
public:
    explicit PPU(Mapper* m = nullptr) : mapper(m) {}
    ~PPU() = default;

    u8 readReg(u16 addr)              { return r.readReg(addr); }
    void writeReg(u16 addr, u8 value) { r.writeReg(addr, value); }
    void step()                       { r.step(); }

public:
    struct State {
        u8 ppuctrl{0};
        u8 ppumask{0};
        u8 ppustatus{0xA0};
        u8 oamaddr{0};

        u8  w{0};
        u8  fineX{0};
        u16 v{0};
        u16 t{0};
        u8  dataBuffer{0};

        u16 pixel{0};
        u16 scanline{0};
        bool nmi{0};
        u8 mirrorMode{0};
        bool oddFrame{0};
        u8 openBus{0};

        std::array<u8, 4096> vram{};
        std::array<u8, 32>   pal{};
        std::array<u8, 256>  oam{};

        struct BgFetch {
            bool valid{0};
            u16 v{0};
            u16 table{0};
            u8 p0{0};
            u8 l0{0};
            u8 h0{0};
            u8 p1{0};
            u8 l1{0};
            u8 h1{0};
        } bgFetch{};

        u8 spriteCount{0};
        struct SpriteData {
            u8 x{0};
            u8 y{0};
            u8 tile{0};
            u8 attr{0};
            u8 id{0};
            u8 low{0};
            u8 high{0};
        };
        std::array<SpriteData, 8> OAM{};
    } state{};

    enum class VideoMode : u8 {
        NTSC = 0,
        PAL = 1,
    };

    State& getState()              { return state; }
    void loadState(const State& s) { state = s;    }

    void setVideoMode(VideoMode mode) {
        videoMode = mode;
        if (videoMode == VideoMode::PAL) {
            preRenderScanline = 311;
            totalScanlines = 312;
            oddFrameDotSkip = false;
        } else {
            preRenderScanline = 261;
            totalScanlines = 262;
            oddFrameDotSkip = true;
        }
    }

public:
    std::array<u32, WIDTH * HEIGHT> frame{};

private:
    Mapper* mapper{nullptr};
    bool frameReady{0};
    VideoMode videoMode{VideoMode::NTSC};
    u16 preRenderScanline{261};
    u16 totalScanlines{262};
    bool oddFrameDotSkip{true};

public:
    class R2C02 {
    public:
        explicit R2C02(PPU* p)
            : p(p)
            , state(p->state)
            , frame(p->frame)
            , frameReady(p->frameReady) {
            state.vram.fill(0);
            state.pal.fill(0);
            state.oam.fill(0);
            frame.fill(0);
        }
        ~R2C02() = default;

    private:
        PPU* p;

    public:
        State& state;
        std::array<u32, WIDTH * HEIGHT>& frame;
        bool& frameReady;

        void setMirror(u8 m)            { state.mirrorMode = m; }
        bool nmiPending() const         { return state.nmi; }
        void clearNmi()                 { state.nmi = 0; }

        u8 readReg(u16 addr);
        void writeReg(u16 addr, u8 data);
        void step();
        std::array<u8, 128 * 128> getPttrnTable(u8 table) const;

    private:
        inline bool rendering() const  { return (state.ppumask & 0x18) != 0; }
        inline bool visible() const    { return (state.scanline < 240); }
        inline bool preLine() const    { return (state.scanline == p->preRenderScanline); }
        inline bool renderLine() const { return visible() || preLine(); }
        inline bool renderDot() const  { return (state.pixel >= 1 && state.pixel <= 256); }

        u8 readVRAM(u16 addr) const;
        void writeVRAM(u16 addr, u8 data);
        u16 mirrorAddress(u16 addr) const;

        void renderPixel();
        void backgroundPixel(u8 x, u8& pixel, u8& pal);
        void spritePixel(u8 x, u8& pixel, u8& pal, u8& prio, bool& sprite0);
        void evalSprites();

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

        inline void reloadX() { state.v = static_cast<u16>((state.v & ~0x041F) | (state.t & 0x041F)); }
        inline void reloadY() { state.v = static_cast<u16>((state.v & ~0x7BE0) | (state.t & 0x7BE0)); }
    };

    R2C02 r{this};
};
