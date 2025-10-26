#include "mapper1.hpp"


Mapper1::Mapper1(Cartridge &cartridge)
    : Mapper(cartridge, MapperType::NROM)
{
    mirrorMode = cartridge.getMirrorMode();
    prgBankCount = cartridge.getPRG().size() / 0x4000;
    chrBankCount = cartridge.getCHR().size() / 0x1000;


    chrRAM = cartridge.getCHR().empty();
    if (chrRAM) {
        cartridge.getCHR().resize(0x2000, 0);
        chrBankCount = 2;
    }
}


u8 Mapper1::readPRG(u16 addr) {
    addr -= 0x8000;
    
    u8 prgMode = (control >> 2) & 0x03;
    u32 bankOffset;
    
    switch (prgMode) {
        case 0:
        case 1:
            bankOffset = (prgBank & 0xFE) * 0x4000;
            return cartridge.getPRG()[(bankOffset + addr) % cartridge.getPRG().size()];
        case 2:
            if (addr < 0x4000) {
                return cartridge.getPRG()[addr];
            } else {
                bankOffset = prgBank * 0x4000;
                return cartridge.getPRG()[(bankOffset + (addr - 0x4000)) % cartridge.getPRG().size()];
            }
        case 3:
            if (addr < 0x4000) {
                bankOffset = prgBank * 0x4000;
                return cartridge.getPRG()[(bankOffset + addr) % cartridge.getPRG().size()];
            } else {
                u32 lastBank = (prgBankCount - 1) * 0x4000;
                return cartridge.getPRG()[lastBank + (addr - 0x4000)];
            }
    }
    
    return 0;
}


void Mapper1::writePRG(u16 addr, u8 value) {
    if (value & 0x80) {
        shiftReg = 0x10;
        control |= 0x0C;
        return;
    }

    bool fill = shiftReg & 1;
    shiftReg >>= 1;
    shiftReg |= (value & 1) << 4;


    if (fill) {
        u8 reg = (addr >> 13) & 0x03;
        
        switch (reg) {
            case 0:  /* 0x8000-0x9FFF */
                control = shiftReg;
                switch (control & 0x03) {
                    case 0: mirrorMode = 3; break;  /* Single lower */
                    case 1: mirrorMode = 3; break;  /* Single upper */
                    case 2: mirrorMode = 1; break;  /* Vertical */
                    case 3: mirrorMode = 0; break;  /* Horizontal */
                }
                break;
            
            case 1:  /* 0xA000-0xBFFF */
                chrBank0 = shiftReg;
                break;
            
            case 2:  /* 0xC000-0xDFFF */
                chrBank1 = shiftReg;
                break;
            
            case 3:  /* 0xE000-0xFFFF */
                prgBank = shiftReg & 0x0F;
                break;
        }
        
        shiftReg = 0x10;
    }
}


u8 Mapper1::readCHR(u16 addr) {
    addr &= 0x1FFF;
    
    u8 chrMode = (control >> 4) & 0x01;
    u32 bankOffset;
    
    if (chrMode == 0) {
        /* 8KB mode */
        bankOffset = (chrBank0 & 0xFE) * 0x1000;
    } else {
        /* 4KB mode */
        if (addr < 0x1000) {
            bankOffset = chrBank0 * 0x1000;
        } else {
            bankOffset = chrBank1 * 0x1000;
            addr -= 0x1000;
        }
    }
    
    return cartridge.getCHR()[(bankOffset + addr) % cartridge.getCHR().size()];
}


void Mapper1::writeCHR(u16 addr, u8 value) {
    if (!chrRAM) return;
    addr &= 0x1FFF;
    
    u8 chrMode = (control >> 4) & 0x01;
    u32 bankOffset;
    
    if (chrMode == 0) {
        /* 8KB mode */
        bankOffset = (chrBank0 & 0xFE) * 0x1000;
    } else {
        /* 4KB mode */
        if (addr < 0x1000) {
            bankOffset = chrBank0 * 0x1000;
        } else {
            bankOffset = chrBank1 * 0x1000;
            addr -= 0x1000;
        }
    }
    
    cartridge.getCHR()[(bankOffset + addr) % cartridge.getCHR().size()] = value;
}

