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
                fineX = data & 7;
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
                t = (t & 0x00FF) | ((data & 0x3F) << 8);
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
    if (++dot >= DOTS) {
        dot = 0;
        if (++scanline >= LINES) {
            scanline = 0;
        }
    }

    bool isRender = (ppumask & 0x18) != 0;
    bool visibleScanline = (scanline < HEIGHT || scanline == 261);
    bool preLine = (scanline == 261);


    if (scanline < HEIGHT && dot >= 1 && dot <= 256)
        renderPixel();


    if (visibleScanline && isRender) {
        if (dot == 256)
            incrementY();

        if (dot == 257)
            reloadX();

        if (preLine && dot >= 280 && dot <= 304)
            reloadY();
    }

    /* VBlank */
    if (scanline == 241 && dot == 1) {
        ppustatus |= 0x80;
        if (ppuctrl & 0x80)
            nmi = true;

        frameReady = true;
    }

    if (scanline == 261 && dot == 1) {
        ppustatus &= 0x1F;
        nmi = false;
    }
}


void Ppu::renderPixel() {
    u8 x = dot - 1;
    u8 y = scanline;

    /* Рендеринг фона */
    u8 bgPixel = 0, bgPal = 0;
    if (ppumask & 0x08)
        backgroundPixel(x, y, bgPixel, bgPal);

    /* Рендеринг спрайтов */
    u8 fgPixel = 0, fgPal = 0, fgPrio = 0;
    bool sprite0 = false;
    if (ppumask & 0x10)
        spritePixel(x, y, fgPixel, fgPal, fgPrio, sprite0);

    /* Обрезка левых 8 пикселей */
    bool clipLeftBg  = !(ppumask & 0x02);
    bool clipLeftSpr = !(ppumask & 0x04);
    if (x < 8) {
        if (clipLeftBg)  bgPixel = 0;
        if (clipLeftSpr) fgPixel = 0;
    }


    if (sprite0 && bgPixel != 0 && fgPixel != 0 && x != 255)
        ppustatus |= 0x40;


    u8 pixel = bgPixel;
    u8 palIdxGroup = bgPal;

    if (fgPixel != 0) {
        if (bgPixel == 0 || fgPrio == 0) {
            pixel = fgPixel;
            palIdxGroup = fgPal + 4;
        }
    }


    u16 paletteAddr = (pixel == 0) ? 0x3F00 : (0x3F00 + (palIdxGroup << 2) + pixel);
    frame[y * WIDTH + x] = rgb(readVRAM(paletteAddr));
}


void Ppu::backgroundPixel(u8 x, u8 /*y*/, u8 &pixel, u8 &pal) {

    u16 coarseX = v & 0x001F;
    u16 coarseY = (v & 0x03E0) >> 5;
    u16 fineY   = (v >> 12) & 0x07;

    /* Вычисление абсолютной X */
    u16 scrollX = coarseX * 8 + fineX + x;

    /* Определение nametable */
    u16 ntX = (v >> 10) & 1;
    u16 ntY = (v >> 11) & 1;


    if (scrollX >= 256) {
        ntX ^= 1;      /* Переключение на следующий nametable */
        scrollX -= 256;
    }

    /* Вычисление позиции тайла и пикселя внутри него */
    u16 tileX = scrollX >> 3;
    u8 pixelX = scrollX & 7;

    /* Построение адреса nametable */
    u16 ntBase = 0x2000 | (ntX << 10) | (ntY << 11);

    /* Чтение ID тайла из nametable */
    u16 nameAddr = ntBase + coarseY * 32 + tileX;
    u8 tileId = readVRAM(nameAddr);

    /* Чтение байта атрибутов */
    u16 attrAddr = ntBase + 0x3C0 + (coarseY >> 2) * 8 + (tileX >> 2);
    u8 attr = readVRAM(attrAddr);
    u8 shift = ((tileX & 2) ? 2 : 0) | ((coarseY & 2) ? 4 : 0);
    pal = (attr >> shift) & 0x03;

    /* Чтение данных паттерна */
    u16 patternBase = (ppuctrl & 0x10) ? 0x1000 : 0x0000;
    u16 patternAddr = patternBase + tileId * 16 + fineY;
    u8 lo = readVRAM(patternAddr);
    u8 hi = readVRAM(patternAddr + 8);

    /* Извлечение пикселя */
    u8 bit = 7 - pixelX;
    pixel = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
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


        u8 lo = readVRAM(patAddr);
        u8 hi = readVRAM(patAddr + 8);
        u8 xInSprite = x - xPos;
        u8 bit = flipH ? xInSprite : (7 - xInSprite);
        u8 sprPixel = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);


        if (sprPixel == 0) continue;


        if (pixel == 0) {
            pixel = sprPixel;
            pal = attr & 3;
            prio = (attr >> 5) & 1;
        }


        if (i == 0 && sprPixel != 0) {
            sprite0 = true;
        }


        if (pixel != 0 && i > 0) {
            return;
        }
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
        if ((idx & 0x03) == 0 && idx >= 0x10) {
            idx &= 0x0F;
        }
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

    switch (mirrorMode) {
        case HORIZONTAL:
            table = (table == 0 || table == 1) ? 0 : 1;
            break;  /* A,A,B,B */

        case VERTICAL:
            table = (table == 0 || table == 2) ? 0 : 1;
            break;  /* A,B,A,B */

        case SINGLE:
            table = 0;
            break;  /* A,A,A,A */

        case FOUR:
            table &= 1;
            break;  /* A,B,A,B (2KB вместо 4KB) */
    }

    return (table * 0x400) + offset;
}


auto Ppu::rgb(u8 palIdx) const -> u32 {
    palIdx &= 0x3F;
    u8 r = paletteRGB[palIdx * 3 + 0];
    u8 g = paletteRGB[palIdx * 3 + 1];
    u8 b = paletteRGB[palIdx * 3 + 2];
    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}


void Ppu::copyFrame(u32 *rgba) const {
    for (u16 i = 0; i < WIDTH * HEIGHT; ++i) {
        rgba[i] = frame[i];
    }
}
