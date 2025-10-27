#pragma once
#include "cartridge.hpp"

class Mapper {
public:
    virtual ~Mapper() = default;

public:
    virtual u8 readPRG(u16 addr) = 0;
    virtual void writePRG(u16 addr, u8 value) = 0;

    virtual u8 readCHR(u16 addr) = 0;
    virtual void writeCHR(u16 addr, u8 value) = 0;

    virtual u8 readRAM(u16 addr) { return prgRAM[addr & 0x1FFF]; }
    virtual void writeRAM(u16 addr, u8 value) { prgRAM[addr & 0x1FFF] = value; }

    virtual u8 getMirrorMode() { return mirrorMode; }

    virtual void step() {}
    bool irqFlag{false};

protected:
    explicit Mapper(Cartridge& c): cartridge(c){}


    Cartridge &cartridge;
    u8 mirrorMode{0};
    u8 prgRAM[0x2000]{};
};
