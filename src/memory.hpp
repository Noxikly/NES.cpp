#pragma once
#include <array>
#include "mapper.hpp"

class Mapper;

class Memory {
public:
    explicit Memory(Mapper* m = nullptr)
                    : mapper(m) { ram.fill(0); }
    ~Memory() = default;


    auto read(u16 addr) const -> u8;
    void write(u16 addr, u8 value);

    std::array<u8, 2048> ram{}; // 0x0000-0x07FF
    Mapper* mapper{nullptr};
};
