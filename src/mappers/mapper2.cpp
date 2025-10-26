#include "mapper2.hpp"


Mapper2::Mapper2(Cartridge &cartridge): Mapper(cartridge) {
    mirrorMode = cartridge.getMirrorMode();
    cntBank = cartridge.getPRG().size() / 0x4000;


    if (cartridge.getCHR().empty()) {
        cartridge.getCHR().resize(0x2000, 0);
    }
}


u8 Mapper2::readPRG(u16 addr) {
    addr -= 0x8000;

    if (addr < 0x4000) {
        u32 bankOffset = prgBank * 0x4000;
        return cartridge.getPRG()[(bankOffset + addr) % cartridge.getPRG().size()];
    } else {
        u32 lastBank = (cntBank - 1) * 0x4000;
        return cartridge.getPRG()[lastBank + (addr - 0x4000)];
    }
}


void Mapper2::writePRG(u16 /*addr*/, u8 value) {
    prgBank = value & 0x0F;
}


u8 Mapper2::readCHR(u16 addr) {
    addr &= 0x1FFF;
    return cartridge.getCHR()[addr];
}


void Mapper2::writeCHR(u16 addr, u8 value) {
    addr &= 0x1FFF;
    cartridge.getCHR()[addr] = value;
}
