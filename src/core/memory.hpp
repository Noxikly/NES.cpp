#pragma once

#include "mapper.hpp"
#include "ppu.hpp"
#include "apu.hpp"

class Mapper;
class Ppu;
class Apu;


class Memory {
public:
    explicit Memory(Mapper* m = nullptr, Ppu* p = nullptr, Apu* a = nullptr)
                    : mapper(m), ppu(p), apu(a) { state.ram.fill(0); }
    ~Memory() = default;


    auto read(u16 addr) const -> u8;
    void write(u16 addr, u8 value);

public:
    struct State {
        std::array<u8, 2048> ram{}; /* 0x0000-0x07FF */
        u32 dma{0};
        bool dmaOdd{false};
        u8 joy1{0};
        u8 joy2{0};
        u8 joy1Shift{0};
        u8 joy2Shift{0};
    };

public:
    void setJoy1(u8 s) { state.joy1 = s; }
    void setJoy2(u8 s) { state.joy2 = s; }


    void addDma(u32 cycles) { state.dma += cycles; }
    auto getDma() -> u32 { const u32 d = state.dma; state.dma = 0; return d; }

    auto getState() const -> const State& { return state; }
    void loadState(const State& s) { state = s; }

    Mapper* mapper{nullptr};
    Ppu* ppu{nullptr};
    Apu* apu{nullptr};
    mutable State state;
};
