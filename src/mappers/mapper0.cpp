#include "mapper0.hpp"


Mapper0::Mapper0(Cartridge &cartridge)
                 :Mapper(cartridge, MapperType::NROM)
{
    cntBank = (cartridge.getPRG().size() == 0x4000) ? 1 : 0;

    chrRAM = cartridge.getCHR().empty();
    if(chrRAM)
        cartridge.getCHR().resize(0x2000, 0);

}


u8 Mapper0::readPRG(u16 addr) {
    addr -= 0x8000;

    if (cntBank)
        addr &= 0x3FFF;

    return cartridge.getPRG()[addr];
}

void Mapper0::writePRG(u16 /*addr*/, u8 /*value*/) {}

u8 Mapper0::readCHR(u16 addr) {
    addr &= 0x1FFF;
    return cartridge.getCHR()[addr];
}

void Mapper0::writeCHR(u16 addr, u8 value) {
    if (!chrRAM) return;
    addr &= 0x1FFF;
    cartridge.getCHR()[addr] = value;
}
