#include "memory.hpp"


auto Memory::read(u16 addr) const -> u8 {
    if (addr < 0x2000) {
        return ram[addr & MIRROR];
    }
    if (addr < 0x4000) {
        return 0;
    }
    if (addr < 0x4018) {
        return 0;
    }
    if (addr >= 0x8000 && mapper) {
        return mapper->readPRG(addr);
    }

    return 0;
}

void Memory::write(u16 addr, u8 value) {
    if (addr < 0x2000) {
        ram[addr & MIRROR] = value;
        return;
    }
    if (addr >= 0x8000 && mapper) {
        mapper->writePRG(addr, value);
        return;
    }
}
