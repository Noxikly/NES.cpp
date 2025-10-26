#pragma once
#include "mapper.hpp"
#include "ppu.hpp"

class Mapper;
class Ppu;


class Memory {
public:
    explicit Memory(Mapper* m = nullptr, Ppu* p = nullptr)
                    : mapper(m), ppu(p) { ram.fill(0); }
    ~Memory() = default;


    auto read(u16 addr) const -> u8;
    void write(u16 addr, u8 value);


    void setJoy1(u8 state) { joy1 = state; }
    void setJoy2(u8 state) { joy2 = state; }


    std::array<u8, 2048> ram{}; /* 0x0000-0x07FF */
    Mapper* mapper{nullptr};
    Ppu* ppu{nullptr};

private:
    u8 joy1{0};
    u8 joy2{0};
    mutable u8 joy1Shift{0};
    mutable u8 joy2Shift{0};
};
