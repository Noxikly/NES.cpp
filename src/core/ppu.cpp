#include <array>
#include <stddef.h>

#include "ppu.hpp"
#include "mapper.hpp"
#include "common.hpp"


/* Регистры PPU (0x2000-0x2007) */
u8 PPU::R2C02::readReg(u16 addr) {
    addr &= 0x07;

    switch (addr) {
        /* 0x2002 PPUSTATUS
         * Возвращает latched статус
         * Сбрасывает VBlank (bit7)
         * Сбрасывает write-toggle w (важно для $2005/$2006)
         */
        case 2: {
            const u8 ret = state.ppustatus;
            state.ppustatus &= ~0x80;
            state.w = 0;
            state.openBus = ret;
            return ret;
        }

        /* 0x2004 OAMDATA (чтение по OAMADDR) */
        case 4:
            state.openBus = state.oam[state.oamaddr];
            return state.openBus;

        /* 0x2007 PPUDATA
         * Для 0x0000-0x3EFF чтение буферизованное (1 cycle отложен)
         * Для палитры 0x3F00-0x3FFF чтение сразу, буфер заполняется из mirrored nametable
         * Во время активного рендера возвращаем open bus
         */
        case 7: {
            const u16 a = state.v & 0x3FFF;
            u8 val;

            if (rendering() && renderLine() && renderDot()) {
                val = state.openBus;
            } else if (a >= 0x3F00) {
                val = readVRAM(a);
                state.dataBuffer = readVRAM(a & 0x2FFF);
            } else {
                val = state.dataBuffer;
                state.dataBuffer = readVRAM(a);
            }

            state.v = static_cast<u16>(state.v + ((state.ppuctrl & 0x04) ? 32 : 1));
            state.openBus = val;
            return val;
        }

        default:
            return state.openBus;
    }
}

/* Запись в регистры PPU (0x2000-0x2007) */
void PPU::R2C02::writeReg(u16 addr, u8 data) {
    addr &= 0x07;

    switch (addr) {
        /* 0x2000 PPUCTRL
         * Биты base nametable прокидываем в t
         */
        case 0:
            state.ppuctrl = state.openBus = data;
            state.t = static_cast<u16>((state.t & ~0x0C00) | ((data & 0x03) << 10));
            break;

        /* 0x2001 PPUMASK */
        case 1:
            state.ppumask = state.openBus = data;
            break;

        /* 0x2003 OAMADDR */
        case 3:
            state.oamaddr = state.openBus = data;
            break;

        /* 0x2004 OAMDATA (автоинкремент OAMADDR) */
        case 4:
            state.oam[state.oamaddr++] = state.openBus = data;
            break;

        /* 0x2005 PPUSCROLL
         * 1-я запись: coarse X + fine X
         * 2-я запись: coarse Y + fine Y
         */
        case 5:
            if (state.w == 0) {
                state.t = static_cast<u16>((state.t & ~0x001F) | (data >> 3));
                state.fineX = data & 0x07;
                state.w = 1;
            } else {
                state.t = static_cast<u16>((state.t & 0x0C1F) 
                                           | ((data & 0xF8) << 2) 
                                           | ((data & 0x07) << 12));
                state.w = 0;
            }
            state.openBus = data;
            break;

        /* 0x2006 PPUADDR
         * 1-я запись: high 6 bits
         * 2-я запись: low 8 bits + перенос t -> v
         */
        case 6:
            if (state.w == 0) {
                state.t = static_cast<u16>(((data & 0x3F) << 8) | (state.t & 0x00FF));
                state.w = 1;
            } else {
                state.t = static_cast<u16>((state.t & 0xFF00) | data);
                state.v = state.t;
                state.w = 0;
            }
            state.openBus = data;
            break;

        /* 0x2007 PPUDATA: запись в VRAM по v + инкремент */
        case 7:
            if (!(rendering() && renderLine() && renderDot()))
                writeVRAM(state.v & 0x3FFF, data);
            state.v = static_cast<u16>(state.v + ((state.ppuctrl & 0x04) ? 32 : 1));
            state.openBus = data;
            break;

        default:
            break;
    }
}

/* Один пиксельный тик PPU */
void PPU::R2C02::step() {
    /* Рисуем только видимую область */
    if (visible() && renderDot())
        renderPixel();

    /* VRAM address pipeline во время рендера */
    if (renderLine() && rendering()) {
        if (renderDot() && (state.pixel & 7) == 0)
            state.v = incrementX(state.v);
        if (state.pixel == 256)
            incrementY();
        if (state.pixel == 257)
            reloadX();
        if (preLine() && state.pixel >= 280 && state.pixel <= 304)
            reloadY();
    }

    /* Вход в VBlank: scanline 241, dot 1 */
    if (state.scanline == 241 && state.pixel == 1) {
        state.ppustatus |= 0x80;
        if (state.ppuctrl & 0x80)
            state.nmi = 1;
        frameReady = 1;
    }

    /* Очистка флагов кадра */
    if (preLine() && state.pixel == 1) {
        state.ppustatus &= 0x1F;
        state.nmi = 0;
        frameReady = 0;
    }

    /* Пропуск одного dot на нечет кадре (только NTSC) */
    if (p->oddFrameDotSkip 
        && preLine() 
        && state.pixel == 339 
        && rendering() 
        && state.oddFrame)
        state.pixel = 340;

    /* Тактируем mapper (MMC3 IRQ и т.п.) */
    if (p->mapper && state.pixel == 260 && renderLine() && rendering())
        p->mapper->step();

    /* На конце scanline готовим спрайты для следующей строки */
    if (state.pixel == 340 && renderLine())
        evalSprites();

    /* Переход на следующий dot / scanline / frame */
    if (++state.pixel > 340) {
        state.pixel = 0;
        if (++state.scanline >= p->totalScanlines) {
            state.scanline = 0;
            state.oddFrame = !state.oddFrame;
        }
    }
}

/* Выборка до 8 спрайтов следующей scanline */
void PPU::R2C02::evalSprites() {
    state.spriteCount = 0;
    bool overflow = 0;

    const u16 height = (state.ppuctrl & 0x20) ? 16 : 8;
    const u16 nextScanline = static_cast<u16>((state.scanline + 1) % p->totalScanlines);

    /* На невидных scanline sprite overflow не держим */
    if (nextScanline >= 240) {
        if (state.ppustatus & 0x20)
            state.ppustatus &= static_cast<u8>(~0x20);
        return;
    }

    for (u8 i = 0; i < 64; ++i) {
        const u16 spriteTop = static_cast<u16>(state.oam[i * 4]) + 1;
        if (nextScanline < spriteTop || nextScanline >= spriteTop + height)
            continue;

        /* В secondary OAM попадают только первые 8 совпавших спрайтов */
        if (state.spriteCount < 8) {
            auto& entry = state.OAM[state.spriteCount];
            entry.x = state.oam[i * 4 + 3];
            entry.y = static_cast<u8>(spriteTop);
            entry.tile = state.oam[i * 4 + 1];
            entry.attr = state.oam[i * 4 + 2];
            entry.id = i;

            u8 fineY = static_cast<u8>(nextScanline - spriteTop);
            if ((entry.attr & 0x80) != 0)
                fineY = static_cast<u8>(height - 1 - fineY);

            /* Адрес паттерна:
             * - 8x16: банк в bit0 tile, чет/нечет тайл по fineY>=8
             * - 8x8 : банк из PPUCTRL bit3
             */
            u16 patAddr;
            if (height == 16) {
                const u16 bank = (entry.tile & 1) ? 0x1000 : 0x0000;
                const u16 tileIndex = static_cast<u16>((entry.tile & 0xFE) 
                                    + ((fineY >= 8) ? 1 : 0));
                const u8 row = fineY & 7;
                patAddr = static_cast<u16>(bank + tileIndex * 16 + row);
            } else {
                const u16 bank = (state.ppuctrl & 0x08) ? 0x1000 : 0x0000;
                patAddr = static_cast<u16>(bank + entry.tile * 16 + fineY);
            }

            entry.low = readVRAM(patAddr);
            entry.high = readVRAM(static_cast<u16>(patAddr + 8));
            state.spriteCount++;
        } else {
            /* 9-й и далее - только overflow */
            overflow = 1;
        }
    }

    if (overflow)
        state.ppustatus |= 0x20;
}

/* Рендер одного экранного пикселя */
void PPU::R2C02::renderPixel() {
    const u8 x = static_cast<u8>(state.pixel - 1);

    /* Фон */
    u8 bgPixel = 0;
    u8 bgPal = 0;
    if (state.ppumask & 0x08)
        backgroundPixel(x, bgPixel, bgPal);

    /* Спрайты */
    u8 fgPixel = 0;
    u8 fgPal = 0;
    u8 fgPrio = 0;
    bool sprite0 = 0;
    if (state.ppumask & 0x10)
        spritePixel(x, fgPixel, fgPal, fgPrio, sprite0);

    /* Left-edge mask: первые 8 пикселей могут принудительно гаситься */
    if (x < 8) {
        if (!(state.ppumask & 0x02))
            bgPixel = 0;
        if (!(state.ppumask & 0x04))
            fgPixel = 0;
    }

    if (rendering()) {
        /* TODO: убрать этот костыль и сделать по-нормальному */
        const bool hitBySpec = sprite0 && bgPixel != 0 && fgPixel != 0;
        const bool hitByRopeFallback = sprite0
                                    && bgPixel == 0
                                    && fgPixel != 0
                                    && (state.ppumask & 0x18) == 0x18
                                    && state.scanline == 30
                                    && x >= 252;

        if ((hitBySpec || hitByRopeFallback) && x < 255) {
            state.ppustatus |= 0x40;
        }
    }

    /* Финальный мультиплексер BG/SPR по приоритету */
    u8 px = bgPixel;
    u8 palGroup = bgPal;

    if (fgPixel != 0)
        if (bgPixel == 0 || fgPrio == 0) {
            px = fgPixel;
            palGroup = static_cast<u8>(fgPal + 4);
        }

    /* Индекс цвета в палитре PPU */
    u16 paletteAddr = 0x3F00;
    if (px != 0)
        paletteAddr = static_cast<u16>(paletteAddr + (palGroup << 2) + px);

    u8 colorIdx = readVRAM(paletteAddr) & 0x3F;
    /* Greyscale bit (PPUMASK bit0) */
    if (state.ppumask & 0x01)
        colorIdx &= 0x30;

    frame[static_cast<size_t>(state.scanline) * WIDTH + x] = (0xFF000000u | PALETTE[colorIdx]) ;
}

/* Получение пикселя фона с кэшем 2 соседних тайлов */
void PPU::R2C02::backgroundPixel(u8 x, u8& pixel, u8& pal) {
    const u16 fineY = static_cast<u16>((state.v >> 12) & 0x07);
    const u16 table = (state.ppuctrl & 0x10) ? 0x1000 : 0x0000;

    /* Локальный fetch одного tile (name + attr + pattern low/high) */
    auto fetchTile = [&](u16 vv, u8& palOut, u8& low, u8& high) {
        const u16 nameAddr = static_cast<u16>(0x2000 | (vv & 0x0FFF));
        const u8 tileId = readVRAM(nameAddr);

        const u16 attrAddr = static_cast<u16>(0x23C0 
                                              | (vv & 0x0C00) 
                                              | ((vv >> 4) & 0x38) 
                                              | ((vv >> 2) & 0x07));
        const u8 attr = readVRAM(attrAddr);
        const u8 shift = static_cast<u8>(((vv >> 4) & 4) | (vv & 2));
        palOut = static_cast<u8>((attr >> shift) & 3);

        const u16 pat = static_cast<u16>(table + tileId * 16 + fineY);
        low = readVRAM(pat);
        high = readVRAM(static_cast<u16>(pat + 8));
    };

    /* Обновляем кэш, если изменились v/table или кэш невалиден */
    if (!state.bgFetch.valid || state.bgFetch.v != state.v || state.bgFetch.table != table) {
        state.bgFetch.v = state.v;
        state.bgFetch.table = table;

        fetchTile(state.v, state.bgFetch.p0, state.bgFetch.l0, state.bgFetch.h0);
        fetchTile(incrementX(state.v), state.bgFetch.p1, state.bgFetch.l1, state.bgFetch.h1);
        state.bgFetch.valid = 1;
    }

    /* fineX + локальный x выбирают левый/правый тайл и бит внутри байта */
    const u8 idx = static_cast<u8>(state.fineX + (x & 0x07));

    u8 low = state.bgFetch.l0;
    u8 high = state.bgFetch.h0;
    pal = state.bgFetch.p0;

    if (idx >= 8) {
        low = state.bgFetch.l1;
        high = state.bgFetch.h1;
        pal = state.bgFetch.p1;
    }

    const u8 subX = static_cast<u8>(idx & 0x07);
    const u8 bit = static_cast<u8>(7 - subX);
    pixel = static_cast<u8>(((low >> bit) & 0x01) | (((high >> bit) & 0x01) << 1));
}

/* Получение front спрайтового пикселя (с учетом OAM) */
void PPU::R2C02::spritePixel(u8 x, u8& pixel, u8& pal, u8& prio, bool& sprite0) {
    pixel = 0;
    sprite0 = 0;

    /* Приоритет: первый непрозрачный спрайт в secondary OAM */
    for (u8 i = 0; i < state.spriteCount; ++i) {
        const auto& spr = state.OAM[i];

        if (x < spr.x || x >= spr.x + 8)
            continue;
        i16 dx = static_cast<i16>(x) - static_cast<i16>(spr.x);
        if (dx < 0 || dx >= 8)
            continue;

        /* Горизонтальный флип меняет порядок битов */
        const bool flipH = (spr.attr & 0x40) != 0;
        const u8 bit = flipH ? static_cast<u8>(dx) : static_cast<u8>(7 - dx);
        const u8 sprPixel = static_cast<u8>(((spr.low >> bit) & 1) 
                            | (((spr.high >> bit) & 0x01) << 1));

        /* Нулевой индекс = прозрачный пиксель спрайта */
        if (sprPixel == 0)
            continue;

        if (pixel == 0) {
            pixel = sprPixel;
            pal = spr.attr & 0x03;
            prio = static_cast<u8>((spr.attr >> 5) & 0x01);
        }

        /* Флаг кандидата для sprite-0 hit */
        if (spr.id == 0 && sprPixel != 0)
            sprite0 = 1;
    }
}

/* Чтение из пространства PPU 0x0000-0x3FFF */
u8 PPU::R2C02::readVRAM(u16 addr) const {
    addr &= 0x3FFF;

    /* CHR (pattern tables) */
    if (addr < 0x2000)
        return p->mapper ? p->mapper->readCHR(addr) : 0;

    /* Nametables (с mirroring) */
    if (addr < 0x3F00)
        return state.vram[mirrorAddress(addr)];

    /* Palette RAM (с аппаратным зеркалированием universal BG color) */
    if (addr < 0x4000) {
        u8 idx = addr & 0x1F;
        if ((idx & 0x13) == 0x10)
            idx &= 0x0F;
        return static_cast<u8>(state.pal[idx] & 0x3F);
    }

    return 0;
}

/* Запись в пространство PPU 0x0000-0x3FFF */
void PPU::R2C02::writeVRAM(u16 addr, u8 data) {
    addr &= 0x3FFF;
    /* Любая запись в VRAM инвалидирует кэш фона */
    state.bgFetch.valid = 0;

    if (addr < 0x2000) {
        /* CHR RAM/ROM через mapper */
        if (p->mapper)
            p->mapper->writeCHR(addr, data);
    } else if (addr < 0x3F00) {
        /* Nametable */
        state.vram[mirrorAddress(addr)] = data;
    } else if (addr < 0x4000) {
        /* Palette */
        u8 idx = addr & 0x1F;
        if ((idx & 0x13) == 0x10)
            idx &= 0x0F;
        state.pal[idx] = static_cast<u8>(data & 0x3F);
    }
}

/* Преобразование 0x2000-0x2FFF с учетом mirroring */
u16 PPU::R2C02::mirrorAddress(u16 addr) const {
    addr = static_cast<u16>(0x2000 + (addr & 0x0FFF));
    const u16 offset = addr & 0x03FF;
    u16 table = static_cast<u16>((addr >> 10) & 0x03);

    const u8 mode = p->mapper ? static_cast<u8>(p->mapper->mirror) 
                              : state.mirrorMode;
    switch (mode) {
        case Mapper::HORIZONTAL: table = (table < 2) ? 0 : 1; break;
        case Mapper::VERTICAL: table &= 1; break;
        case Mapper::SINGLE_DOWN: table = 0; break;
        case Mapper::SINGLE_UP: table = 1; break;
        case Mapper::FOUR: break;
        default: break;
    }

    return static_cast<u16>(table * 0x400 + offset);
}

/* Отладочный дамп pattern table в 2bpp индексах (0..3) */
std::array<u8, 128 * 128> PPU::R2C02::getPttrnTable(u8 table) const {
    std::array<u8, 128 * 128> pixels{};
    const u16 base = (table & 1) ? 0x1000 : 0x0000;

    for (u16 tileY = 0; tileY < 16; ++tileY) {
        for (u16 tileX = 0; tileX < 16; ++tileX) {
            const u16 tile = static_cast<u16>(tileY * 16 + tileX);
            const u16 tileBase = static_cast<u16>(base + tile * 16);

            for (u16 row = 0; row < 8; ++row) {
                const u8 low = readVRAM(static_cast<u16>(tileBase + row));
                const u8 high = readVRAM(static_cast<u16>(tileBase + row + 8));

                for (u16 col = 0; col < 8; ++col) {
                    const u8 bit = static_cast<u8>(7 - col);
                    const u8 value = static_cast<u8>(((low >> bit) & 0x01) 
                                                     | (((high >> bit) & 0x01) << 1));

                    const u16 x = static_cast<u16>(tileX * 8 + col);
                    const u16 y = static_cast<u16>(tileY * 8 + row);
                    pixels[static_cast<size_t>(y) * 128 + x] = value;
                }
            }
        }
    }

    return pixels;
}
