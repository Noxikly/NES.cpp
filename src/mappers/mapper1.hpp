#pragma once
#include "../mapper.hpp"


class Mapper1 final : public Mapper {
public:
    explicit Mapper1(Cartridge &cartridge);

    u8 readPRG(u16 addr) override;
    void writePRG(u16 addr, u8 value) override;

    u8 readCHR(u16 addr) override;
    void writeCHR(u16 addr, u8 value) override;

private:
    u8 shiftReg{0x10};
    u8 control{0x0C};
    u8 chrBank0{0};
    u8 chrBank1{0};
    u8 prgBank{0};


    u8 prgBankCount{0};
    u8 chrBankCount{0};
    bool chrRAM{false};
};

