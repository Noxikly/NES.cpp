#include "mem.hpp"
#include "common.hpp"
#include "apu.hpp"

u8 Memory::read(u16 addr) const {
    if (addr < 0x2000) {
        return state.ram[addr & MIRROR];
    }
    if (addr < 0x4000) {
        return (ppu) ? ppu->readReg(addr) : 0;
    }
    if (addr < 0x4020) {
        switch (addr) {
            case 0x4015:
                return apu ? apu->readStatus() : 0;
            case 0x4016: {
                const u8 value = ((state.joy1Shift & 0x01) | 0x40 );
                state.joy1Shift >>= 1;
                return value;
            }
            case 0x4017: {
                const u8 value = ((state.joy2Shift & 0x01) | 0x40 );
                state.joy2Shift >>= 1;
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
        state.ram[addr & MIRROR] = value;
        return;
    }
    if (addr < 0x4000) {
        if (ppu) ppu->writeReg(addr, value);
        return;
    }
    if (addr < 0x4020) {
        switch (addr) {
            case 0x4000: case 0x4001: case 0x4002: case 0x4003:
            case 0x4004: case 0x4005: case 0x4006: case 0x4007:
            case 0x4008: case 0x4009: case 0x400A: case 0x400B:
            case 0x400C: case 0x400D: case 0x400E: case 0x400F:
            case 0x4010: case 0x4011: case 0x4012: case 0x4013:
            case 0x4015: case 0x4017:
                if (apu) {
                    apu->writeReg(addr, value);
                }
                return;
            case 0x4014: {
                if (ppu) {
                    const u16 base = value << 8;
                    for (u16 i=0; i<256; ++i) {
                        const u8 data = read(base + i);
                        ppu->writeReg(0x2004, data);
                    }
                }
                addDma(state.dmaOdd ? 514 : 513);
                state.dmaOdd = !state.dmaOdd;
                return;
            }
            case 0x4016: {
                if (value & 0x01) {
                    state.joy1Shift = state.joy1;
                    state.joy2Shift = state.joy2;
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
