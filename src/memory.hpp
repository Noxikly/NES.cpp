#pragma once
#include "utils.hpp"
#include <array>


class Memory {
public:
    explicit Memory();
    ~Memory() = default;


    auto read(u16 addr) -> u8;
    void write(u16 addr, u8 value);

    std::array<u8, 2048> ram{}; // 0x0000-0x07FF
};
