#include "memory.hpp"


Memory::Memory() { ram.fill(0); }


auto Memory::read(u16 addr) -> u8 {
    if(addr < 0x2000) {
        return ram[addr & MIRROR];
    }

    return ram[addr];
}

void Memory::write(u16 addr, u8 value) {
    if(addr < 0x2000) {
        ram[addr & MIRROR] = value;
    }

    else { ram[addr] = value; }
}


