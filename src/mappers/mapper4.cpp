#include "mapper4.hpp"


Mapper4::Mapper4(Cartridge &cartridge) : Mapper(cartridge) {
    mirrorMode = cartridge.getMirrorMode();
    cntPRG = cartridge.getPRG().size() / 0x2000;
    cntCHR = cartridge.getCHR().size() / 0x0400;


    chrRAM = cartridge.getCHR().empty();
    if (chrRAM) {
        cartridge.getCHR().resize(0x2000, 0);
        cntCHR = 8;
    }


    regs[0] = 0;
    regs[1] = 2;
    regs[2] = 4;
    regs[3] = 5;
    regs[4] = 6;
    regs[5] = 7;
    regs[6] = 0;
    regs[7] = 1;
}


u32 Mapper4::getPRG(u16 addr) const {
    addr -= 0x8000;
    u8 bankIndex = (addr >> 13) & 0x03;

    if (!modePRG)
        switch (bankIndex) {
            case 0: return regs[6] % cntPRG;
            case 1: return regs[7] % cntPRG;
            case 2: return (cntPRG - 2) % cntPRG;
            case 3: return (cntPRG - 1) % cntPRG;
        }
    else
        switch (bankIndex) {
            case 0: return (cntPRG - 2) % cntPRG;
            case 1: return regs[7] % cntPRG;
            case 2: return regs[6] % cntPRG;
            case 3: return (cntPRG - 1) % cntPRG;
        }

    return 0;
}

u32 Mapper4::getCHR(u16 addr) const {
    addr &= 0x1FFF;
    u8 bankIndex = (addr >> 10) & 0x07;

    if (!modeCHR)
        switch (bankIndex) {
            case 0: case 1: return ((regs[0] & 0xFE) + (bankIndex & 1)) % cntCHR;
            case 2: case 3: return ((regs[1] & 0xFE) + (bankIndex & 1)) % cntCHR;
            case 4: return regs[2] % cntCHR;
            case 5: return regs[3] % cntCHR;
            case 6: return regs[4] % cntCHR;
            case 7: return regs[5] % cntCHR;
        }
    else
        switch (bankIndex) {
            case 0: return regs[2] % cntCHR;
            case 1: return regs[3] % cntCHR;
            case 2: return regs[4] % cntCHR;
            case 3: return regs[5] % cntCHR;
            case 4: case 5: return ((regs[0] & 0xFE) + (bankIndex & 1)) % cntCHR;
            case 6: case 7: return ((regs[1] & 0xFE) + (bankIndex & 1)) % cntCHR;
        }

    return 0;
}

u8 Mapper4::readPRG(u16 addr) {
    u32 bank = getPRG(addr);
    u16 offset = (addr - 0x8000) & 0x1FFF;
    return cartridge.getPRG()[bank * 0x2000 + offset];
}

void Mapper4::writePRG(u16 addr, u8 value) {
    if (addr < 0x8000) return;

    bool evenAddr = (addr & 0x01) == 0;

    if (addr >= 0x8000 && addr <= 0x9FFF) {
        if (evenAddr) {
            bankSelect = value & 0x07;
            modePRG = (value & 0x40) != 0;
            modeCHR = (value & 0x80) != 0;
        } else
            regs[bankSelect] = value;
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF && evenAddr) {
        mirrorMode = (value & 0x01) ? 0 : 1;
    }
    else if (addr >= 0xC000 && addr <= 0xDFFF) {
        if (evenAddr)
            irqLatch = value;
        else
            irqReload = true;
    }
    else if (addr >= 0xE000 && addr <= 0xFFFF) {
        irqEnabled = (evenAddr) ? false : true;
    }
}

u8 Mapper4::readCHR(u16 addr) {
    addr &= 0x1FFF;
    u32 bank = getCHR(addr);
    u16 offset = addr & 0x03FF;
    return cartridge.getCHR()[bank * 0x0400 + offset];
}

void Mapper4::writeCHR(u16 addr, u8 value) {
    if (!chrRAM) return;

    addr &= 0x1FFF;
    u32 bank = getCHR(addr);
    u16 offset = addr & 0x03FF;
    cartridge.getCHR()[bank * 0x0400 + offset] = value;
}

void Mapper4::step() {
    if (cntIRQ == 0 || irqReload) {
        cntIRQ = irqLatch;
        irqReload = false;
    } else {
        cntIRQ--;
    }

    if (cntIRQ == 0 && irqEnabled) {
        irqFlag = true;
    }
}
