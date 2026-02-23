#include "memory.hpp"


auto Memory::read(u16 addr) const -> u8 {
    if (addr < 0x2000) {
        return ram[addr & MIRROR];
    }
    if (addr < 0x4000) {
        return (ppu) ? ppu->readReg(addr) : 0;
    }
    if (addr < 0x4020) {
        switch (addr) {
            case 0x4016: {
                const u8 value = ((joy1Shift & 1) | 0x40 );
                joy1Shift >>= 1;
                return value;
            }
            case 0x4017: {
                const u8 value = ((joy2Shift & 1) | 0x40 );
                joy2Shift >>= 1;
                return value;
            }
            default:
                return 0;
        }
    }
    if (addr >= 0x6000 && addr < 0x8000) {
        return mapper ? mapper->readRAM(addr) : 0;
    }
    if (addr >= 0x8000) {
        return mapper ? mapper->readPRG(addr) : 0;
    }
    return 0;
}

void Memory::write(u16 addr, u8 value) {
    if (addr < 0x2000) {
        ram[addr & MIRROR] = value;
        return;
    }
    if (addr < 0x4000) {
        if (ppu) ppu->writeReg(addr, value);
        return;
    }
    if (addr < 0x4020) {
        switch (addr) {
            case 0x4014: {
                if (ppu) {
                    const u16 base = value << 8;
                    for (u16 i=0; i<256; ++i) {
                        const u8 data = read(base + i);
                        ppu->writeReg(0x2004, data);
                    }
                }
                addDma(513);
                return;
            }
            case 0x4016: {
                if (value & 1) {
                    joy1Shift = joy1;
                    joy2Shift = joy2;
                }
                return;
            }
            default:
                return;
        }
    }
    if (addr >= 0x6000 && addr < 0x8000) {
        if (mapper) mapper->writeRAM(addr, value);
        return;
    }
    if (addr >= 0x8000) {
        if (mapper) mapper->writePRG(addr, value);
        return;
    }
}
