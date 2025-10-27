#pragma once
#include "../mapper.hpp"


class Mapper4 final : public Mapper {
public:
    explicit Mapper4(Cartridge &cartridge);

    u8 readPRG(u16 addr) override;
    void writePRG(u16 addr, u8 value) override;

    u8 readCHR(u16 addr) override;
    void writeCHR(u16 addr, u8 value) override;

    void step() override;

private:
    u8 regs[8]{};
    u8 bankSelect{0};
    bool modePRG{false};
    bool modeCHR{false};

    u32 cntPRG{0};
    u32 cntCHR{0};
    bool chrRAM{false};


    u8 cntIRQ{0};
    u8 irqLatch{0};
    bool irqReload{false};
    bool irqEnabled{false};

    u32 getPRG(u16 addr) const;
    u32 getCHR(u16 addr) const;
};
