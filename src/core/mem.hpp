#pragma once

#include <array>

#include "mapper.hpp"
#include "ppu.hpp"
#include "common.hpp"

class APU;

namespace Core {
class Memory;
}

class Memory {
public:
    explicit Memory(Mapper* m = nullptr, PPU* p = nullptr, APU* a = nullptr)
        : mapper(m), ppu(p), apu(a) {
        state.ram.fill(0);
    }
    ~Memory() = default;

    u8 read(u16 addr) const;
    void write(u16 addr, u8 value);

public:
    struct State {
        std::array<u8, 2048> ram{}; /* 0x0000-0x07FF */
        u32 dma{0};
        bool dmaOdd{0};
        u8 joy1{0};
        u8 joy2{0};
        u8 joy1Shift{0};
        u8 joy2Shift{0};
    };

    const State& getState() const { return state; }
    void loadState(const State& s) { state = s; }

public:
    void setJoy1(u8 s) { state.joy1 = s; }
    void setJoy2(u8 s) { state.joy2 = s; }

    void addDma(u32 cycles) { state.dma += cycles; }
    u32 getDma() { const u32 d = state.dma; state.dma = 0; return d; }

public:
    Mapper *mapper{nullptr};
    PPU *ppu{nullptr};
    APU *apu{nullptr};
    mutable State state;
};
