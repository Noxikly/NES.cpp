#include "mapper3.hpp"


Mapper3::Mapper3(Cartridge &cartridge): Mapper(cartridge) {
    mirrorMode = cartridge.getMirrorMode();
}


u8 Mapper3::readPRG(u16 addr) {
    addr -= 0x8000;

    if (cartridge.getPRG().size() == 0x4000)
        addr &= 0x3FFF;
    
    return cartridge.getPRG()[addr];
}

void Mapper3::writePRG(u16 /*addr*/, u8 value) { chrBank = value & 0x03; }

u8 Mapper3::readCHR(u16 addr) {
    u32 bankOffset = chrBank * 0x2000;
    return cartridge.getCHR()[(bankOffset + addr) % cartridge.getCHR().size()];
}

void Mapper3::writeCHR(u16 /*addr*/, u8 /*value*/) {}
