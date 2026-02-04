#include "ppu.hpp"


auto Ppu::readReg(u16 addr) -> u8 {
    addr &= 7;

    switch (addr) {
        case 2: {  /* PPUSTATUS */
            const u8 ret = ppustatus;
            ppustatus &= ~0x80;
            w = 0;
            return ret;
        }

        case 4:  /* OAMDATA */
            return oam[oamaddr];

        case 7: {  /* PPUDATA */
            const u16 a = v & 0x3FFF;
            u8 val;
            
            if (a >= 0x3F00) {
                val = readVRAM(a);
                dataBuffer = readVRAM(a & 0x2FFF);
            } else {
                val = dataBuffer;
                dataBuffer = readVRAM(a);
            }

            v += (ppuctrl & 0x04) ? 32 : 1;
            return val;
        }

        default:
            return 0;
    }
}

void Ppu::writeReg(u16 addr, u8 data) {
    addr &= 7;

    switch (addr) {
        case 0:  /* PPUCTRL */
            ppuctrl = data;
            t = (t & ~0x0C00) | ((data & 0x03) << 10);
            break;

        case 1:  /* PPUMASK */
            ppumask = data;
            break;

        case 3:  /* OAMADDR */
            oamaddr = data;
            break;

        case 4:  /* OAMDATA */
            oam[oamaddr++] = data;
            break;

        case 5:  /* PPUSCROLL */
            if (w == 0) {
                t = (t & ~0x001F) | (data >> 3);
                fineX = data & 0x07;
                w = 1;
            } else {
                t = (t & 0x0C1F) | ((data & 0xF8) << 2) | ((data & 0x07) << 12);
                w = 0;
            }
            break;

        case 6:  /* PPUADDR */
            if (w == 0) {
                t = ((data & 0x3F) << 8) | (t & 0x00FF);
                w = 1;
            } else {
                t = (t & 0xFF00) | data;
                v = t;
                w = 0;
            }
            break;

        case 7:  /* PPUDATA */
            writeVRAM(v & 0x3FFF, data);
            v += (ppuctrl & 0x04) ? 32 : 1;
            break;
    }
}


void Ppu::step() {
    const bool rendering = (ppumask & 0x18) != 0;
    const bool preLine = (scanline == 261);
    const bool visible = (scanline < 240);
    const bool renderLine = (visible || preLine);


    if (visible && pixel >= 1 && pixel <= 256)
        renderPixel();


    /* Background fetches и scrolling */
    if (renderLine && rendering) {
        if (pixel >= 1 && pixel <= 256 && (pixel & 7) == 0) v = incrementX(v);
        if (pixel == 256) incrementY();
        if (pixel == 257) reloadX();
        if (preLine && pixel >= 280 && pixel <= 304) reloadY();
    }


    if (scanline == 241 && pixel == 1) {
        ppustatus |= 0x80;  /* флаг VBlank */
        if (ppuctrl & 0x80)
            nmi = true;
        frameReady = true;
    }


    if (preLine && pixel == 1) {
        ppustatus &= 0x1F;  /* Очистка VBlank, sprite 0 hit, sprite overflow */
        nmi = false;
        frameReady = false;
    }


    if (preLine && pixel == 339 && rendering && oddFrame) pixel = 340;


    /* Mapper */
    if (mapper && pixel == 260 && visible && rendering)
        mapper->step();

    if (pixel == 340 && renderLine)
        evalSprites();


    if (++pixel > 340) {
        pixel = 0;
        if (++scanline > 261) {
            scanline = 0;
            oddFrame = !oddFrame;
        }
    }
}


void Ppu::evalSprites() {
    spriteCount = 0;
    bool overflow = false;

    const u16 height = (ppuctrl & 0x20) ? 16 : 8;
    const u16 y = (scanline + 1) % 262;


    for (u8 i=0; i<64; ++i) {
        const u16 spriteLine = static_cast<u16>(oam[i * 4]) + 1;
        if (spriteLine >= 256) continue;
        if (y < spriteLine || y >= spriteLine + height) continue;


        if (spriteCount < 8) {
            auto& entry = OAM[spriteCount];
            entry.x = oam[i * 4 + 3];
            entry.y = static_cast<u8>(spriteLine);
            entry.tile = oam[i * 4 + 1];
            entry.attr = oam[i * 4 + 2];
            entry.id = i;

            u8 fineY = static_cast<u8>(y - spriteLine);
            const bool flipV = (entry.attr & 0x80) != 0;
            if (flipV) {
                fineY = static_cast<u8>(height - 1 - fineY);
            }

            u16 patAddr;
            if (height == 16) {
                const u16 bank = (entry.tile & 1) ? 0x1000 : 0x0000;
                const u16 tileIndex = (entry.tile & 0xFE) + ((fineY >= 8) ? 1 : 0);
                const u8 row = fineY & 7;
                patAddr = bank + tileIndex * 16 + row;
            } else {
                const u16 bank = (ppuctrl & 0x08) ? 0x1000 : 0x0000;
                patAddr = bank + entry.tile * 16 + fineY;
            }

            entry.low = readVRAM(patAddr);
            entry.high = readVRAM(patAddr + 8);
            spriteCount++;
        } else {
            overflow = true;
        }
    }

    if (overflow)
        ppustatus |= 0x20;
}


void Ppu::renderPixel() {
    const u8 x = pixel - 1;

    /* Background */
    u8 bgPixel = 0, bgPal = 0;
    if (ppumask & 0x08) {
        backgroundPixel(x, bgPixel, bgPal);
    }

    /* Sprites */
    u8 fgPixel = 0, fgPal = 0, fgPrio = 0;
    bool sprite0 = false;
    if (ppumask & 0x10)
        spritePixel(x, fgPixel, fgPal, fgPrio, sprite0);


    if (x < 8) {
        if (!(ppumask & 0x02)) bgPixel = 0;
        if (!(ppumask & 0x04)) fgPixel = 0;
    }


    if (sprite0 && bgPixel != 0)
        ppustatus |= 0x40;


    u8 px = bgPixel;
    u8 palGroup = bgPal;

    if (fgPixel != 0) {
        if (bgPixel == 0 || fgPrio == 0) {
            px = fgPixel;
            palGroup = fgPal + 4;
        }
    }


    u16 paletteAddr = 0x3F00;
    if (px != 0)
        paletteAddr = 0x3F00 + (palGroup << 2) + px;


    u8 colorIdx = readVRAM(paletteAddr) & 0x3F;
    if (ppumask & 0x01)
        colorIdx &= 0x30;

    frame[scanline*WIDTH + x] = palette[colorIdx];
}


void Ppu::backgroundPixel(u8 x, u8 &pixel, u8 &pal) {
    const u16 fineY = (v >> 12) & 0x07;
    const u16 pttrn = (ppuctrl & 0x10) ? 0x1000 : 0x0000;


    /* загрузка данных о тайле */
    auto fetchTile = [&](u16 vv, u8 &palOut, u8 &low, u8 &high) {
        const u16 nameAddr = 0x2000 | (vv & 0x0FFF);
        const u8 tileId = readVRAM(nameAddr);

        const u16 attrAddr = 0x23C0 | (vv & 0x0C00) | ((vv >> 4) & 0x38) | ((vv >> 2) & 0x07);
        const u8 attr = readVRAM(attrAddr);
        const u8 shift = ((vv >> 4) & 4) | (vv & 2); /* {0,2,4,6} */
        palOut = (attr >> shift) & 3;

        const u16 pat = pttrn + tileId * 16 + fineY;
        low  = readVRAM(pat);
        high = readVRAM(pat + 8);
    };


    if (!bgFetch.valid || bgFetch.v != v || bgFetch.pattern != pttrn) {
        bgFetch.v = v;
        bgFetch.pattern = pttrn;

        fetchTile(v, bgFetch.p0, bgFetch.l0, bgFetch.h0);
        fetchTile(incrementX(v), bgFetch.p1, bgFetch.l1, bgFetch.h1);
        bgFetch.valid = true;
    }

    const u8 idx = fineX + (x & 7); /* 0..14 */

    u8 low = bgFetch.l0, high = bgFetch.h0;
    pal = bgFetch.p0;

    if (idx >= 8) {
        low = bgFetch.l1;
        high = bgFetch.h1;
        pal = bgFetch.p1;
    }

    const u8 subX = idx & 7;
    const u8 bit  = 7 - subX;
    pixel = ((low >> bit) & 1) | (((high >> bit) & 1) << 1);
}


void Ppu::spritePixel(u8 x, u8 &pixel, u8 &pal, u8 &prio, bool &sprite0) {
    pixel = 0;
    sprite0 = false;


    for (u8 i=0; i<spriteCount; ++i) {
        const auto& spr = OAM[i];

        if (x < spr.x || x >= spr.x + 8) continue;
        i16 dx = (i16)x - (i16)spr.x;
        if (dx < 0 || dx >= 8) continue;

        const bool flipH = (spr.attr & 0x40) != 0;
        const u8 bit = flipH ? u8(dx) : u8(7 - dx);
        const u8 sprPixel = ((spr.low >> bit) & 1) | (((spr.high >> bit) & 1) << 1);

        if (sprPixel == 0) continue;

        if (pixel == 0) {
            pixel = sprPixel;
            pal = spr.attr & 3;
            prio = (spr.attr >> 5) & 1;
        }

        if (spr.id == 0 && sprPixel != 0)
            sprite0 = true;
    }
}


auto Ppu::readVRAM(u16 addr) const -> u8 {
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        return mapper ? mapper->readCHR(addr) : 0;
    }
    
    if (addr < 0x3F00) {
        return vram[mirrorAddress(addr)];
    }

    if (addr < 0x4000) {
        u8 idx = addr & 0x1F;
        /* Зеркалирование 0x3F10/14/18/1C -> 0x3F00/04/08/0C */
        if ((idx & 0x13) == 0x10)
            idx &= 0x0F;
        return pal[idx] & 0x3F;
    }

    return 0;
}

void Ppu::writeVRAM(u16 addr, u8 data) {
    addr &= 0x3FFF;
    bgFetch.valid = false;

    if (addr < 0x2000) {
        if (mapper) mapper->writeCHR(addr, data);
    }
    else if (addr < 0x3F00) {
        vram[mirrorAddress(addr)] = data;
    }
    else if (addr < 0x4000) {
        u8 idx = addr & 0x1F;
        /* Зеркалирование записи 0x3F10/14/18/1C -> 0x3F00/04/08/0C  */
        if ((idx & 0x13) == 0x10)
            idx &= 0x0F;
        pal[idx] = data & 0x3F;
    }
}


auto Ppu::mirrorAddress(u16 addr) const -> u16 {
    addr = 0x2000 + (addr & 0x0FFF);
    const u16 offset = addr & 0x03FF;
    u16 table = (addr >> 10) & 0x03;

    const u8 mode = mapper ? mapper->mirrorMode : mirrorMode;
    switch (mode) {
        case HORIZONTAL:  table = (table < 2) ? 0 : 1; break; /* A,A,B,B */
        case VERTICAL:    table &= 1; break;                  /* A,B,A,B */
        case SINGLE_DOWN: table = 0; break;                   /* A,A,A,A */
        case SINGLE_UP:   table = 1; break;                   /* B,B,B,B */
        case FOUR:        break;                              /* A,B,C,D */
    }

    return (table * 0x400) + offset;
}