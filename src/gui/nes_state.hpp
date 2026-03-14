#pragma once

#include <QFile>

#include "core/apu.hpp"
#include "core/cpu.hpp"
#include "core/mapper.hpp"
#include "core/mem.hpp"
#include "core/ppu.hpp"

inline constexpr u32 NES_STATE = 0x4E5354; /* NST */
inline constexpr u32 MIN_NES_STATE_VERSION = 2;
inline constexpr u32 NES_STATE_VERSION = 3;

inline constexpr u32 MAX_MAPPER_PRG_RAM = 16 * 1024 * 1024;
inline constexpr u32 MAX_MAPPER_CHR_RAM = 16 * 1024 * 1024;
inline constexpr u32 MAX_MAPPER_BLOB = 8 * 1024 * 1024;

inline auto saveBinState(const QString &path, const Core::CPU::State &cpu,
                         const Core::PPU::State &ppu,
                         const Core::APU::State &apu,
                         const Core::Memory::State &mem,
                         const Core::Mapper::State &mapper, u8 region) -> bool {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    out << static_cast<u32>(NES_STATE) << static_cast<u32>(NES_STATE_VERSION);

    /* Mapper */
    out << static_cast<u8>(mapper.mapperNumber)
        << static_cast<u8>(mapper.mirrorMode)
        << static_cast<u8>(mapper.irqFlag);

    out << static_cast<u32>(mapper.prgRam.size());
    for (u8 b : mapper.prgRam)
        out << b;

    out << static_cast<u32>(mapper.chrRam.size());
    for (u8 b : mapper.chrRam)
        out << b;

    out << static_cast<u32>(mapper.mapperBlob.size());
    for (u8 b : mapper.mapperBlob)
        out << b;

    /* CPU */
    out << static_cast<u8>(cpu.regs.A) << static_cast<u8>(cpu.regs.X)
        << static_cast<u8>(cpu.regs.Y) << static_cast<u8>(cpu.regs.P)
        << static_cast<u8>(cpu.regs.SP) << static_cast<u16>(cpu.regs.PC)
        << static_cast<u8>(cpu.do_nmi) << static_cast<u8>(cpu.do_irq)
        << static_cast<u32>(cpu.op_cycles) << static_cast<u8>(cpu.page_crossed);

    /* PPU */
    out << static_cast<u8>(ppu.ppuctrl) << static_cast<u8>(ppu.ppumask)
        << static_cast<u8>(ppu.ppustatus) << static_cast<u8>(ppu.oamaddr)
        << static_cast<u8>(ppu.w) << static_cast<u8>(ppu.fineX)
        << static_cast<u16>(ppu.v) << static_cast<u16>(ppu.t)
        << static_cast<u8>(ppu.dataBuffer) << static_cast<u16>(ppu.pixel)
        << static_cast<u16>(ppu.scanline) << static_cast<u8>(ppu.nmi)
        << static_cast<u8>(ppu.mirrorMode) << static_cast<u8>(ppu.oddFrame)
        << static_cast<u8>(ppu.openBus);

    out << static_cast<u8>(ppu.nmiLine) << static_cast<u8>(ppu.nmiDelay)
        << static_cast<u8>(ppu.nmiOutput) << static_cast<u8>(ppu.suppressVblank)
        << static_cast<u8>(ppu.secOAMAddr) << static_cast<u8>(ppu.primOAMIndex)
        << static_cast<u8>(ppu.spriteEvalDone);

    for (u32 d : ppu.openBusDecay)
        out << d;
    for (u8 b : ppu.secOAM)
        out << b;

    for (u8 b : ppu.vram)
        out << b;
    for (u8 b : ppu.pal)
        out << b;
    for (u8 b : ppu.oam)
        out << b;

    /* PPU bg/sprite */
    out << static_cast<u8>(ppu.bgFetch.valid) << static_cast<u16>(ppu.bgFetch.v)
        << static_cast<u16>(ppu.bgFetch.table)
        << static_cast<u8>(ppu.bgFetch.p0) << static_cast<u8>(ppu.bgFetch.l0)
        << static_cast<u8>(ppu.bgFetch.h0) << static_cast<u8>(ppu.bgFetch.p1)
        << static_cast<u8>(ppu.bgFetch.l1) << static_cast<u8>(ppu.bgFetch.h1)
        << static_cast<u8>(ppu.bgFetch.nt) << static_cast<u8>(ppu.bgFetch.at)
        << static_cast<u8>(ppu.bgFetch.low) << static_cast<u8>(ppu.bgFetch.high)
        << static_cast<u16>(ppu.bgFetch.shLow)
        << static_cast<u16>(ppu.bgFetch.shHigh)
        << static_cast<u16>(ppu.bgFetch.shAttrLo)
        << static_cast<u16>(ppu.bgFetch.shAttrHi)
        << static_cast<u8>(ppu.spriteCount);
    for (const auto &s : ppu.OAM) {
        out << s.x;
        out << s.y;
        out << s.tile;
        out << s.attr;
        out << s.id;
        out << s.low;
        out << s.high;
    }

    /* APU */
    out << apu.pulse1.enabled << apu.pulse1.duty << apu.pulse1.volPeriod
        << apu.pulse1.lengthHalt << apu.pulse1.constVol << apu.pulse1.envelDiv
        << apu.pulse1.envelDecay << apu.pulse1.envelStart
        << apu.pulse1.swpPeriod << apu.pulse1.swpShift << apu.pulse1.swpDiv
        << apu.pulse1.swpEnabled << apu.pulse1.swpNegate << apu.pulse1.swpReload
        << apu.pulse1.timerPeriod << apu.pulse1.timer << apu.pulse1.seqPos
        << apu.pulse1.lenCnt;

    out << apu.pulse2.enabled << apu.pulse2.duty << apu.pulse2.volPeriod
        << apu.pulse2.lengthHalt << apu.pulse2.constVol << apu.pulse2.envelDiv
        << apu.pulse2.envelDecay << apu.pulse2.envelStart
        << apu.pulse2.swpPeriod << apu.pulse2.swpShift << apu.pulse2.swpDiv
        << apu.pulse2.swpEnabled << apu.pulse2.swpNegate << apu.pulse2.swpReload
        << apu.pulse2.timerPeriod << apu.pulse2.timer << apu.pulse2.seqPos
        << apu.pulse2.lenCnt;

    out << apu.triangle.enabled << apu.triangle.linearReloadValue
        << apu.triangle.linearCnt << apu.triangle.ctrlFlag
        << apu.triangle.linearReloadFlag << apu.triangle.timerPeriod
        << apu.triangle.timer << apu.triangle.seqPos << apu.triangle.lenCnt;

    out << apu.noise.enabled << apu.noise.volPeriod << apu.noise.lenHalt
        << apu.noise.constVol << apu.noise.envelDiv << apu.noise.envelDecay
        << apu.noise.envelStart << apu.noise.periodIndex << apu.noise.timer
        << apu.noise.lenCnt << apu.noise.mode << apu.noise.shiftReg;

    out << apu.dmc.enabled << apu.dmc.irqEnabled << apu.dmc.rateIndex
        << apu.dmc.outLevel << apu.dmc.loop << apu.dmc.irqFlag
        << apu.dmc.sampleAddrReg << apu.dmc.sampleLenReg << apu.dmc.bitsRemain
        << apu.dmc.shiftReg << apu.dmc.sampleBuffer << apu.dmc.bufferEmpty
        << apu.dmc.timer << apu.dmc.bytesRemain << apu.dmc.active;

    out << apu.frameCycle << apu.frameCntMode5 << apu.irqInhibit << apu.frameIrq
        << apu.oddCycle << apu.frameCntDelay << apu.pendQuarterFrame
        << apu.pendHalfFrame << apu.delayHalfFrame << apu.sampleAcc;

    /* Memory */
    for (u8 b : mem.ram)
        out << b;
    out << static_cast<u32>(mem.dma) << static_cast<u8>(mem.dmaOdd)
        << static_cast<u8>(mem.joy1) << static_cast<u8>(mem.joy2)
        << static_cast<u8>(mem.joy1Shift) << static_cast<u8>(mem.joy2Shift)
        << static_cast<u8>(mem.joy);

    out << region;

    file.flush();
    return out.status() == QDataStream::Ok && file.error() == QFile::NoError;
}

inline auto loadBinState(const QString &path, Core::CPU::State &cpu,
                         Core::PPU::State &ppu, Core::APU::State &apu,
                         Core::Memory::State &mem, Core::Mapper::State &mapper,
                         u8 &region) -> bool {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    u32 magic = 0;
    u32 version = 0;
    in >> magic >> version;
    if (in.status() != QDataStream::Ok || magic != NES_STATE ||
        version < MIN_NES_STATE_VERSION || version > NES_STATE_VERSION)
        return false;

    /* Буферные переменные */
    u8 u8v;
    u32 u32v;

    /* Mapper */
    in >> mapper.mapperNumber >> mapper.mirrorMode >> mapper.irqFlag;
    if (in.status() != QDataStream::Ok)
        return false;

    in >> u32v;
    if (in.status() != QDataStream::Ok || u32v > MAX_MAPPER_PRG_RAM)
        return false;
    mapper.prgRam.resize(u32v);
    for (u32 i = 0; i < u32v; ++i) {
        in >> u8v;
        mapper.prgRam[i] = u8v;
    }
    if (in.status() != QDataStream::Ok)
        return false;

    in >> u32v;
    if (in.status() != QDataStream::Ok || u32v > MAX_MAPPER_CHR_RAM)
        return false;
    mapper.chrRam.resize(u32v);
    for (u32 i = 0; i < u32v; ++i) {
        in >> u8v;
        mapper.chrRam[i] = u8v;
    }
    if (in.status() != QDataStream::Ok)
        return false;

    in >> u32v;
    if (in.status() != QDataStream::Ok || u32v > MAX_MAPPER_BLOB)
        return false;
    mapper.mapperBlob.resize(u32v);
    for (u32 i = 0; i < u32v; ++i) {
        in >> u8v;
        mapper.mapperBlob[i] = u8v;
    }
    if (in.status() != QDataStream::Ok)
        return false;

    /* CPU */
    in >> cpu.regs.A >> cpu.regs.X >> cpu.regs.Y >> cpu.regs.P >> cpu.regs.SP >>
        cpu.regs.PC >> cpu.do_nmi >> cpu.do_irq >> cpu.op_cycles >>
        cpu.page_crossed;
    if (in.status() != QDataStream::Ok)
        return false;

    /* PPU */
    in >> ppu.ppuctrl >> ppu.ppumask >> ppu.ppustatus >> ppu.oamaddr >> ppu.w >>
        ppu.fineX >> ppu.v >> ppu.t >> ppu.dataBuffer >> ppu.pixel >>
        ppu.scanline >> ppu.nmi >> ppu.mirrorMode >> ppu.oddFrame >>
        ppu.openBus;
    if (in.status() != QDataStream::Ok)
        return false;

    if (version >= 3) {
        in >> ppu.nmiLine >> ppu.nmiDelay >> ppu.nmiOutput >>
            ppu.suppressVblank >> ppu.secOAMAddr >> ppu.primOAMIndex >>
            ppu.spriteEvalDone;
        if (in.status() != QDataStream::Ok)
            return false;

        for (u32 &d : ppu.openBusDecay)
            in >> d;
        if (in.status() != QDataStream::Ok)
            return false;

        for (u8 &b : ppu.secOAM)
            in >> b;
        if (in.status() != QDataStream::Ok)
            return false;
    } else {
        ppu.nmiOutput = (ppu.ppuctrl & 0x80) != 0;
        ppu.nmiLine = ppu.nmiOutput && ((ppu.ppustatus & 0x80) != 0);
        ppu.nmiDelay = 0;
        ppu.suppressVblank = 0;
        ppu.openBusDecay.fill(0);
        ppu.secOAM.fill(0xFF);
        ppu.secOAMAddr = 0;
        ppu.primOAMIndex = 0;
        ppu.spriteEvalDone = 0;
    }

    for (u16 i = 0; i < 4096; ++i) {
        in >> u8v;
        ppu.vram[i] = u8v;
    }
    if (in.status() != QDataStream::Ok)
        return false;
    for (u8 i = 0; i < 32; ++i) {
        in >> u8v;
        ppu.pal[i] = u8v;
    }
    for (u16 i = 0; i < 256; ++i) {
        in >> u8v;
        ppu.oam[i] = u8v;
    }

    /* PPU bg/sprite */
    in >> ppu.bgFetch.valid >> ppu.bgFetch.v >> ppu.bgFetch.table >>
        ppu.bgFetch.p0 >> ppu.bgFetch.l0 >> ppu.bgFetch.h0 >> ppu.bgFetch.p1 >>
        ppu.bgFetch.l1 >> ppu.bgFetch.h1 >> ppu.bgFetch.nt >> ppu.bgFetch.at >>
        ppu.bgFetch.low >> ppu.bgFetch.high >> ppu.bgFetch.shLow >>
        ppu.bgFetch.shHigh >> ppu.bgFetch.shAttrLo >> ppu.bgFetch.shAttrHi >>
        ppu.spriteCount;
    for (auto &s : ppu.OAM) {
        in >> s.x;
        in >> s.y;
        in >> s.tile;
        in >> s.attr;
        in >> s.id;
        in >> s.low;
        in >> s.high;
    }
    if (in.status() != QDataStream::Ok)
        return false;

    /* APU */
    in >> apu.pulse1.enabled >> apu.pulse1.duty >> apu.pulse1.volPeriod >>
        apu.pulse1.lengthHalt >> apu.pulse1.constVol >> apu.pulse1.envelDiv >>
        apu.pulse1.envelDecay >> apu.pulse1.envelStart >>
        apu.pulse1.swpPeriod >> apu.pulse1.swpShift >> apu.pulse1.swpDiv >>
        apu.pulse1.swpEnabled >> apu.pulse1.swpNegate >> apu.pulse1.swpReload >>
        apu.pulse1.timerPeriod >> apu.pulse1.timer >> apu.pulse1.seqPos >>
        apu.pulse1.lenCnt;

    in >> apu.pulse2.enabled >> apu.pulse2.duty >> apu.pulse2.volPeriod >>
        apu.pulse2.lengthHalt >> apu.pulse2.constVol >> apu.pulse2.envelDiv >>
        apu.pulse2.envelDecay >> apu.pulse2.envelStart >>
        apu.pulse2.swpPeriod >> apu.pulse2.swpShift >> apu.pulse2.swpDiv >>
        apu.pulse2.swpEnabled >> apu.pulse2.swpNegate >> apu.pulse2.swpReload >>
        apu.pulse2.timerPeriod >> apu.pulse2.timer >> apu.pulse2.seqPos >>
        apu.pulse2.lenCnt;

    in >> apu.triangle.enabled >> apu.triangle.linearReloadValue >>
        apu.triangle.linearCnt >> apu.triangle.ctrlFlag >>
        apu.triangle.linearReloadFlag >> apu.triangle.timerPeriod >>
        apu.triangle.timer >> apu.triangle.seqPos >> apu.triangle.lenCnt;

    in >> apu.noise.enabled >> apu.noise.volPeriod >> apu.noise.lenHalt >>
        apu.noise.constVol >> apu.noise.envelDiv >> apu.noise.envelDecay >>
        apu.noise.envelStart >> apu.noise.periodIndex >> apu.noise.timer >>
        apu.noise.lenCnt >> apu.noise.mode >> apu.noise.shiftReg;

    in >> apu.dmc.enabled >> apu.dmc.irqEnabled >> apu.dmc.rateIndex >>
        apu.dmc.outLevel >> apu.dmc.loop >> apu.dmc.irqFlag >>
        apu.dmc.sampleAddrReg >> apu.dmc.sampleLenReg >> apu.dmc.bitsRemain >>
        apu.dmc.shiftReg >> apu.dmc.sampleBuffer >> apu.dmc.bufferEmpty >>
        apu.dmc.timer >> apu.dmc.bytesRemain >> apu.dmc.active;

    in >> apu.frameCycle >> apu.frameCntMode5 >> apu.irqInhibit >>
        apu.frameIrq >> apu.oddCycle >> apu.frameCntDelay >>
        apu.pendQuarterFrame >> apu.pendHalfFrame >> apu.delayHalfFrame >>
        apu.sampleAcc;
    if (in.status() != QDataStream::Ok)
        return false;

    /* Memory */
    for (u16 i = 0; i < 2048; ++i) {
        in >> u8v;
        mem.ram[i] = u8v;
    }
    in >> mem.dma >> mem.dmaOdd >> mem.joy1 >> mem.joy2 >> mem.joy1Shift >>
        mem.joy2Shift;
    if (in.status() != QDataStream::Ok)
        return false;

    if (version >= 3) {
        in >> mem.joy;
        if (in.status() != QDataStream::Ok)
            return false;
    } else {
        mem.joy = false;
    }

    in >> region;
    if (in.status() != QDataStream::Ok)
        return false;

    return file.error() == QFile::NoError;
}
