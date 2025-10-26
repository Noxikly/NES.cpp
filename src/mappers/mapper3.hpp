#pragma once
#include "../mapper.hpp"


class Mapper3 final : public Mapper {
public:
    explicit Mapper3(Cartridge &cartridge);

    u8 readPRG(u16 addr) override;
    void writePRG(u16 addr, u8 value) override;

    u8 readCHR(u16 addr) override;
    void writeCHR(u16 addr, u8 value) override;

private:
    u8 chrBank{0};
};
