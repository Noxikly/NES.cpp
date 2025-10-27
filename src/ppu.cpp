#include "ppu.hpp"


auto Ppu::readReg(u16 addr) -> u8 {
    addr &= 7;

    switch (addr) {
        case 2: {  /* PPUSTATUS */
            u8 ret = ppustatus;
            ppustatus &= ~0x80;
            w = 0;
            return ret;
        }

        case 4:  /* OAMDATA */
            return oam[oamaddr];

        case 7: {  /* PPUDATA */
            u16 a = v & 0x3FFF;
            u8 val = (a>=0x3F00) ? readVRAM(a) : dataBuffer;
            dataBuffer = readVRAM((a>=0x3F00) ? a&0x2FFF : a);

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
                /* X scroll */
                t = (t & ~0x001F) | (data >> 3);
                fineX = data & 0x07;
                w = 1;
            } else {
                /* Y scroll */
                t = (t & ~0x73E0) | ((data & 0xF8) << 2) | ((data & 0x07) << 12);
                w = 0;
            }
            break;

        case 6:  /* PPUADDR */
            if (w == 0) {
                /* Старший байт */
                t = ((data & 0x3F) << 8) | (t & 0x00FF);
                w = 1;
            } else {
                /* Младший байт */
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



void Ppu::cycle() {
    if (++pixel >= PIXELS) {
        pixel = 0;
        if (++scanline >= LINES)
            scanline = 0;
    }

    bool rendering = (ppumask & 0x18) != 0;
    bool preLine = (scanline == 261);
    bool visible = (scanline < HEIGHT || preLine);

    if (scanline < HEIGHT && pixel > 0 && pixel < 257)
        renderPixel();

    if (visible && rendering) {
        if (pixel == 256) incrementY();
        if (pixel == 257) reloadX();
        if (preLine && pixel >= 280 && pixel <= 304) reloadY();
    }


    if (scanline == 241 && pixel == 0) {
        ppustatus |= 0x80;
        if (ppuctrl & 0x80) nmi = true;
        frameReady = true;
    }

    if (preLine && pixel == 1) {
        ppustatus &= 0x1F;
        nmi = false;
    }

    if (mapper && pixel == 260 && scanline < 240)
        mapper->step();
}


void Ppu::renderPixel() {
    u8 x = pixel - 1;
    u8 y = scanline;


    u8 bgPixel = 0, bgPal = 0;
    if (ppumask & 0x08)
        backgroundPixel(x, y, bgPixel, bgPal);


    u8 fgPixel = 0, fgPal = 0, fgPrio = 0;
    bool sprite0 = false;
    if (ppumask & 0x10)
        spritePixel(x, y, fgPixel, fgPal, fgPrio, sprite0);


    if (x < 8) {
        if (!(ppumask & 0x02)) bgPixel = 0;
        if (!(ppumask & 0x04)) fgPixel = 0;
    }


    if (sprite0 && bgPixel != 0 && fgPixel != 0 && x != 255)
        ppustatus |= 0x40;


    u8 px = bgPixel;
    u8 palIdxGroup = bgPal;

    if (fgPixel != 0) {
        if (bgPixel == 0 || fgPrio == 0) {
            px = fgPixel;
            palIdxGroup = fgPal + 4;
        }
    }


    u16 paletteAddr = (px == 0) ? 0x3F00 : (0x3F00 + (palIdxGroup << 2) + px);
    frame[y * WIDTH + x] = rgb(readVRAM(paletteAddr));
}


void Ppu::backgroundPixel(u8 x, u8 /*y*/, u8 &pixel, u8 &pal) {
    u16 coarseX = (v >> 0) & 0x1F;
    u16 coarseY = (v >> 5) & 0x1F;
    u16 fineY = (v >> 12) & 0x07;


    u16 pixelX = (coarseX * 8 + fineX + x) & 0x1FF;
    u16 pixelY = (coarseY * 8 + fineY) & 0x1FF;


    u16 ntX = (v >> 10) & 0x01;
    u16 ntY = (v >> 11) & 0x01;
    if (pixelX >= 256) { ntX ^= 1; pixelX -= 256; }
    if (pixelY >= 240) { ntY ^= 1; pixelY -= 240; }

    u16 tileX = pixelX >> 3;
    u8 fX = pixelX & 7;
    u16 tileY = pixelY >> 3;


    u16 ntBase = 0x2000 | (ntX << 10) | (ntY << 11);
    u16 nameAddr = ntBase + tileY * 32 + tileX;
    u8  tileId = readVRAM(nameAddr);


    u16 attrAddr = ntBase + 0x3C0 + (tileY >> 2) * 8 + (tileX >> 2);
    u8 attr = readVRAM(attrAddr);
    u8 shift = ((tileX & 2) ? 2 : 0) | ((tileY & 2) ? 4 : 0);
    pal = (attr >> shift) & 3;


    u16 patternBase = (ppuctrl & 0x10) ? 0x1000 : 0x0000;
    u16 patternAddr = patternBase + tileId * 16 + fineY;
    u8 low = readVRAM(patternAddr);
    u8 high = readVRAM(patternAddr + 8);

    u8 bit = 7 - fX;
    pixel = ((low >> bit) & 1) | (((high >> bit) & 1) << 1);
}


void Ppu::spritePixel(u8 x, u8 y, u8 &pixel, u8 &pal, u8 &prio, bool &sprite0) {
    pixel = 0;
    sprite0 = false;

    u8 height = (ppuctrl & 0x20) ? 16 : 8;  /* 8x8 или 8x16 режим */


    for (u8 i = 0; i < 64; ++i) {

        u8 yPos = oam[i * 4];
        if (y < yPos || y >= yPos + height) continue;

        u8 tile = oam[i * 4 + 1];
        u8 attr = oam[i * 4 + 2];
        u8 xPos = oam[i * 4 + 3];


        if (x < xPos || x >= xPos + 8) continue;

        /* Расчет позиции внутри спрайта */
        u8 fineY = y - yPos;
        bool flipV = (attr & 0x80) != 0;
        bool flipH = (attr & 0x40) != 0;

        if (flipV) {
            fineY = height - 1 - fineY;
        }


        u16 patAddr;
        if (height == 16) {
            u16 bank = (tile & 1) ? 0x1000 : 0x0000;
            u16 tileIndex = (tile & 0xFE) + ((fineY >= 8) ? 1 : 0);
            u8 row = fineY & 7;
            patAddr = bank + tileIndex * 16 + row;
        } else {
            u16 bank = (ppuctrl & 0x08) ? 0x1000 : 0x0000;
            patAddr = bank + tile * 16 + fineY;
        }


        u8 low = readVRAM(patAddr);
        u8 high = readVRAM(patAddr + 8);
        u8 bit = flipH ? (x - xPos) : (7 - (x - xPos));
        u8 sprPixel = ((low >> bit) & 1) | (((high >> bit) & 1) << 1);


        if (sprPixel == 0) continue;

        if (pixel == 0) {
            pixel = sprPixel;
            pal = attr & 3;
            prio = (attr >> 5) & 1;
        }

        if (i == 0 && sprPixel != 0)
            sprite0 = true;
    }
}


void Ppu::incrementY() {
    if ((v & 0x7000) != 0x7000) {
        v += 0x1000;
        return;
    }


    v &= ~0x7000;
    u16 y = (v & 0x03E0) >> 5;

    if (y == 29) {
        y = 0;
        v ^= 0x0800;
    } else if (y == 31) {
        y = 0;
    } else {
        y++;
    }

    v = (v & ~0x03E0) | (y << 5);
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
        /* Зеркалирование $10/$14/$18/$1C в $00/$04/$08/$0C */
        if ((idx & 0x03) == 0 && idx >= 0x10)
            idx &= 0x0F;
        return pal[idx] & 0x3F;
    }

    return 0;
}


void Ppu::writeVRAM(u16 addr, u8 data) {
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        if (mapper) mapper->writeCHR(addr, data);
    }
    else if (addr < 0x3F00) {
        vram[mirrorAddress(addr)] = data;
    }
    else if (addr < 0x4000) {
        u8 idx = addr & 0x1F;
        /* Зеркалирование записи в $10/$14/$18/$1C */
        if ((idx & 0x03) == 0 && idx >= 0x10)
            pal[idx & 0x0F] = data & 0x3F;
        pal[idx] = data & 0x3F;
    }
}


auto Ppu::mirrorAddress(u16 addr) const -> u16 {
    addr = 0x2000 + (addr & 0x0FFF);
    u16 offset = addr & 0x03FF;
    u16 table = (addr >> 10) & 0x03;


    u8 mode = mapper ? mapper->getMirrorMode() : mirrorMode;
    switch (mode) {
        case HORIZONTAL:
            table = (table < 2) ? 0 : 1;
            break;  /* A,A,B,B */

        case VERTICAL:
            table &= 1;
            break;  /* A,B,A,B */

        case SINGLE_DOWN:
            table = 0;
            break;  /* A,A,A,A */

        case SINGLE_UP:
            table = 1;
            break;  /* B,B,B,B */

        case FOUR:
            break;  /* A,B,A,B */
    }

    return (table * 0x400) + offset;
}


auto Ppu::rgb(u8 palIdx) const -> u32 {
    palIdx &= 0x3F;
    u8 r = paletteRGB[palIdx * 3];
    u8 g = paletteRGB[palIdx * 3 + 1];
    u8 b = paletteRGB[palIdx * 3 + 2];
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}


void Ppu::copyFrame(u32 *rgba) const {
    for (u16 i=0; i<WIDTH * HEIGHT; ++i)
        rgba[i] = frame[i];
}
