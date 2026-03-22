#include "core/mem.h"
#include "core/apu.h"

/* Чтение из CPU memory map */
u8 Core::Memory::read(u16 addr) const {
    /* 0x0000-0x1FFF: RAM */
    if (addr < 0x2000) {
        return state.ram[addr & MIRROR];
    }

    /* 0x2000-0x3FFF: регистры PPU */
    if (addr < 0x4000) {
        if (!ppu) {
            return 0;
        }
        return ppu->readReg(addr);
    }

    /* 0x4000-0x401F: APU и I/O */
    if (addr < 0x4020) {
        switch (addr) {
        /* APU status */
        case 0x4015:
            if (!apu) {
                return 0;
            }
            return apu->readStatus();

        /* Контроллер 1 */
        case 0x4016: {
            u8 value;
            if (state.joy) {
                value = ((state.joy1 & 0x01) | 0x40);
            } else {
                value = ((state.joy1Shift & 0x01) | 0x40);
                state.joy1Shift =
                    static_cast<u8>((state.joy1Shift >> 1) | 0x80);
            }
            return value;
        }

        /* Контроллер 2 */
        case 0x4017: {
            u8 value;
            if (state.joy) {
                value = ((state.joy2 & 0x01) | 0x40);
            } else {
                value = ((state.joy2Shift & 0x01) | 0x40);
                state.joy2Shift =
                    static_cast<u8>((state.joy2Shift >> 1) | 0x80);
            }
            return value;
        }
        default:
            return 0;
        }
    }

    /* 0x6000-0x7FFF: PRG-RAM картриджа */
    if (addr >= 0x6000 && addr < 0x8000) {
        if (!mapper) {
            return 0;
        }
        return mapper->readRAM(addr);
    }

    /* 0x8000-0xFFFF: PRG-ROM/mapper */
    if (addr >= 0x8000) {
        if (!mapper) {
            return 0;
        }
        return mapper->readPRG(addr);
    }

    return 0;
}

/* Запись в CPU memory map */
void Core::Memory::write(u16 addr, u8 value) {
    /* 0x0000-0x1FFF: RAM */
    if (addr < 0x2000) {
        state.ram[addr & MIRROR] = value;
        return;
    }

    /* 0x2000-0x3FFF: регистры PPU */
    if (addr < 0x4000) {
        if (!ppu) {
            return;
        }
        ppu->writeReg(addr, value);
        return;
    }

    /* 0x4000-0x401F: APU / I/O / DMA */
    if (addr < 0x4020) {
        switch (addr) {
        /* APU регистры */
        case 0x4000:
        case 0x4001:
        case 0x4002:
        case 0x4003:
        case 0x4004:
        case 0x4005:
        case 0x4006:
        case 0x4007:
        case 0x4008:
        case 0x4009:
        case 0x400A:
        case 0x400B:
        case 0x400C:
        case 0x400D:
        case 0x400E:
        case 0x400F:
        case 0x4010:
        case 0x4011:
        case 0x4012:
        case 0x4013:
        case 0x4015:
        case 0x4017:
            if (!apu) {
                return;
            }
            apu->writeReg(addr, value);
            return;

        /* OAM DMA: копирование 256 байт на $2004 */
        case 0x4014: {
            if (!ppu) {
                addDma(state.dmaOdd ? 513 : 514);
                return;
            }

            const u16 base = value << 8;
            for (u16 i = 0; i < 256; ++i) {
                const u8 data = read(base + i);
                ppu->writeReg(0x2004, data);
            }
            addDma(state.dmaOdd ? 513 : 514);
            return;
        }

        /* JOY strobe */
        case 0x4016: {
            const bool newJoy = (value & 0x01) != 0;
            if (newJoy) {
                state.joy1Shift = state.joy1;
                state.joy2Shift = state.joy2;
            } else if (state.joy) {
                state.joy1Shift = state.joy1;
                state.joy2Shift = state.joy2;
            }
            state.joy = newJoy;
            return;
        }
        default:
            return;
        }
    }

    /* 0x6000-0x7FFF: PRG-RAM картриджа */
    if (addr >= 0x6000 && addr < 0x8000) {
        if (!mapper) {
            return;
        }
        mapper->writeRAM(addr, value);
        return;
    }

    /* 0x8000-0xFFFF: mapper write */
    if (addr >= 0x8000) {
        if (!mapper) {
            return;
        }
        mapper->writePRG(addr, value);
        return;
    }

    return;
}
