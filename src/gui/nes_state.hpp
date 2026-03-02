#pragma once
 
#include <QFile>

#include "core/cpu.hpp"
#include "core/ppu.hpp"
#include "core/memory.hpp"
#include "core/mapper.hpp"
#include "core/apu.hpp"

 
inline auto saveBinState(const QString& path, 
                         const Cpu::State& cpu, 
                         const Ppu::State& ppu,
                         const Memory::State& mem,
                         const Mapper::State& mapper,
                         const Apu::State& apu) -> bool 
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);


    /* Mapper */
    out << static_cast<u8>(mapper.mapperNumber);
    out << static_cast<u8>(mapper.mirrorMode);
    out << static_cast<u8>(mapper.irqFlag);

    out << static_cast<u32>(mapper.prgRam.size());
    for (u8 b : mapper.prgRam) out << b;

    out << static_cast<u32>(mapper.chrRam.size());
    for (u8 b : mapper.chrRam) out << b;

    out << static_cast<u32>(mapper.mapperBlob.size());
    for (u8 b : mapper.mapperBlob) out << b;

    /* CPU */
    out << static_cast<u8>(cpu.regs.A); 
    out << static_cast<u8>(cpu.regs.X); 
    out << static_cast<u8>(cpu.regs.Y);
    out << static_cast<u8>(cpu.regs.P); 
    out << static_cast<u8>(cpu.regs.SP);
    out << static_cast<u16>(cpu.regs.PC);
    out << static_cast<u8>(cpu.do_nmi);
    out << static_cast<u8>(cpu.do_irq);
    out << static_cast<u32>(cpu.cycles);
    out << static_cast<quint64>(cpu.total_cycles);
    out << static_cast<u8>(cpu.page_crossed);

    /* PPU */
    out << static_cast<u8>(ppu.ppuctrl);
    out << static_cast<u8>(ppu.ppumask); 
    out << static_cast<u8>(ppu.ppustatus); 
    out << static_cast<u8>(ppu.oamaddr);
    out << static_cast<u8>(ppu.w);
    out << static_cast<u8>(ppu.fineX);
    out << static_cast<u16>(ppu.v);
    out << static_cast<u16>(ppu.t);
    out << static_cast<u8>(ppu.dataBuffer);
    out << static_cast<u16>(ppu.pixel);
    out << static_cast<u16>(ppu.scanline);
    out << static_cast<u8>(ppu.nmi);
    out << static_cast<u8>(ppu.mirrorMode);
    out << static_cast<u8>(ppu.oddFrame);
    out << static_cast<u8>(ppu.openBus);

    for (u8 b : ppu.vram) out << b;
    for (u8 b : ppu.pal)  out << b;
    for (u8 b : ppu.oam)  out << b;

    /* PPU */
    out << static_cast<u8>(ppu.bgFetch.valid);
    out << static_cast<u16>(ppu.bgFetch.v);
    out << static_cast<u16>(ppu.bgFetch.table);
    out << static_cast<u8>(ppu.bgFetch.p0);
    out << static_cast<u8>(ppu.bgFetch.l0);
    out << static_cast<u8>(ppu.bgFetch.h0);
    out << static_cast<u8>(ppu.bgFetch.p1);
    out << static_cast<u8>(ppu.bgFetch.l1);
    out << static_cast<u8>(ppu.bgFetch.h1);
    out << static_cast<u8>(ppu.spriteCount);
    for (const auto& s : ppu.OAM) {
        out << s.x;
        out << s.y;
        out << s.tile;
        out << s.attr;
        out << s.id;
        out << s.low;
        out << s.high;
    }

    /* Memory */
    for (u8 b : mem.ram)  out << b;
    out << static_cast<u32>(mem.dma);
    out << static_cast<u8>(mem.joy1);
    out << static_cast<u8>(mem.joy2);
    out << static_cast<u8>(mem.joy1Shift);
    out << static_cast<u8>(mem.joy2Shift);

    /* APU */
    out << static_cast<u8>(apu.pulse1.enabled);
    out << static_cast<u8>(apu.pulse1.duty);
    out << static_cast<u8>(apu.pulse1.volPeriod);
    out << static_cast<u8>(apu.pulse1.lengthHalt);
    out << static_cast<u8>(apu.pulse1.constVol);
    out << static_cast<u8>(apu.pulse1.envelDiv);
    out << static_cast<u8>(apu.pulse1.envelDecay);
    out << static_cast<u8>(apu.pulse1.envelStart);
    out << static_cast<u8>(apu.pulse1.swpPeriod);
    out << static_cast<u8>(apu.pulse1.swpShift);
    out << static_cast<u8>(apu.pulse1.swpDiv);
    out << static_cast<u8>(apu.pulse1.swpEnabled);
    out << static_cast<u8>(apu.pulse1.swpNegate);
    out << static_cast<u8>(apu.pulse1.swpReload);
    out << static_cast<u16>(apu.pulse1.timerPeriod);
    out << static_cast<u16>(apu.pulse1.timer);
    out << static_cast<u8>(apu.pulse1.seqPos);
    out << static_cast<u8>(apu.pulse1.lenCnt);

    out << static_cast<u8>(apu.pulse2.enabled);
    out << static_cast<u8>(apu.pulse2.duty);
    out << static_cast<u8>(apu.pulse2.volPeriod);
    out << static_cast<u8>(apu.pulse2.lengthHalt);
    out << static_cast<u8>(apu.pulse2.constVol);
    out << static_cast<u8>(apu.pulse2.envelDiv);
    out << static_cast<u8>(apu.pulse2.envelDecay);
    out << static_cast<u8>(apu.pulse2.envelStart);
    out << static_cast<u8>(apu.pulse2.swpPeriod);
    out << static_cast<u8>(apu.pulse2.swpShift);
    out << static_cast<u8>(apu.pulse2.swpDiv);
    out << static_cast<u8>(apu.pulse2.swpEnabled);
    out << static_cast<u8>(apu.pulse2.swpNegate);
    out << static_cast<u8>(apu.pulse2.swpReload);
    out << static_cast<u16>(apu.pulse2.timerPeriod);
    out << static_cast<u16>(apu.pulse2.timer);
    out << static_cast<u8>(apu.pulse2.seqPos);
    out << static_cast<u8>(apu.pulse2.lenCnt);

    out << static_cast<u8>(apu.triangle.enabled);
    out << static_cast<u8>(apu.triangle.linearReloadValue);
    out << static_cast<u8>(apu.triangle.linearCnt);
    out << static_cast<u8>(apu.triangle.ctrlFlag);
    out << static_cast<u8>(apu.triangle.linearReloadFlag);
    out << static_cast<u16>(apu.triangle.timerPeriod);
    out << static_cast<u16>(apu.triangle.timer);
    out << static_cast<u8>(apu.triangle.seqPos);
    out << static_cast<u8>(apu.triangle.lenCnt);

    out << static_cast<u8>(apu.noise.enabled);
    out << static_cast<u8>(apu.noise.volPeriod);
    out << static_cast<u8>(apu.noise.lenHalt);
    out << static_cast<u8>(apu.noise.constVol);
    out << static_cast<u8>(apu.noise.envelDiv);
    out << static_cast<u8>(apu.noise.envelDecay);
    out << static_cast<u8>(apu.noise.envelStart);
    out << static_cast<u8>(apu.noise.periodIndex);
    out << static_cast<u16>(apu.noise.timer);
    out << static_cast<u8>(apu.noise.lenCnt);
    out << static_cast<u8>(apu.noise.mode);
    out << static_cast<u16>(apu.noise.shiftReg);

    out << static_cast<u8>(apu.dmc.enabled);
    out << static_cast<u8>(apu.dmc.irqEnabled);
    out << static_cast<u8>(apu.dmc.rateIndex);
    out << static_cast<u8>(apu.dmc.outLevel);
    out << static_cast<u8>(apu.dmc.loop);
    out << static_cast<u8>(apu.dmc.irqFlag);
    out << static_cast<u8>(apu.dmc.sampleAddrReg);
    out << static_cast<u8>(apu.dmc.sampleLenReg);
    out << static_cast<u8>(apu.dmc.bitsRemain);
    out << static_cast<u8>(apu.dmc.shiftReg);
    out << static_cast<u8>(apu.dmc.sampleBuffer);
    out << static_cast<u8>(apu.dmc.bufferEmpty);
    out << static_cast<u16>(apu.dmc.timer);
    out << static_cast<u16>(apu.dmc.bytesRemain);
    out << static_cast<u8>(apu.dmc.active);

    out << static_cast<u32>(apu.frameCycle);
    out << static_cast<u8>(apu.frameCntMode5);
    out << static_cast<u8>(apu.irqInhibit);
    out << static_cast<u8>(apu.frameIrq);
    out << static_cast<u8>(apu.oddCycle);
    out << static_cast<u8>(apu.frameCntDelay);
    out << static_cast<u8>(apu.pendQuarterFrame);
    out << static_cast<u8>(apu.pendHalfFrame);
    out << static_cast<u8>(apu.delayHalfFrame);
    out << static_cast<u8>(apu.frameIrqRepeat);
    out << static_cast<double>(apu.sampleAcc);

    return file.error() == QFile::NoError;
}
 
inline auto loadBinState(const QString& path, 
                         Cpu::State& cpu, 
                         Ppu::State& ppu,
                         Memory::State& mem,
                         Mapper::State& mapper,
                         Apu::State& apu) -> bool 
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);


    /* Буферные переменные */
    u8 u8v; u16 u16v; u32 u32v; quint64 u64v;

    /* Mapper */
    in >> u8v; mapper.mapperNumber = u8v;
    in >> u8v; mapper.mirrorMode = u8v;
    in >> u8v; mapper.irqFlag = u8v;

    in >> u32v;
    mapper.prgRam.resize(u32v);
    for (u32 i=0; i<u32v; ++i) { in >> u8v; mapper.prgRam[i] = u8v; }

    in >> u32v;
    mapper.chrRam.resize(u32v);
    for (u32 i=0; i<u32v; ++i) { in >> u8v; mapper.chrRam[i] = u8v; }

    in >> u32v;
    mapper.mapperBlob.resize(u32v);
    for (u32 i=0; i<u32v; ++i) { in >> u8v; mapper.mapperBlob[i] = u8v; }

    /* CPU */
    in >> u8v; cpu.regs.A = u8v;
    in >> u8v; cpu.regs.X = u8v;
    in >> u8v; cpu.regs.Y = u8v;
    in >> u8v; cpu.regs.P = u8v;
    in >> u8v; cpu.regs.SP = u8v;
    in >> u16v; cpu.regs.PC = u16v;
    in >> u8v; cpu.do_nmi = u8v;
    in >> u8v; cpu.do_irq = u8v;
    in >> u32v; cpu.cycles = u32v;
    in >> u64v; cpu.total_cycles = u64v;
    in >> u8v; cpu.page_crossed = u8v;

    /* PPU */
    in >> u8v; ppu.ppuctrl = u8v;
    in >> u8v; ppu.ppumask = u8v;
    in >> u8v; ppu.ppustatus = u8v;
    in >> u8v; ppu.oamaddr = u8v;
    in >> u8v; ppu.w = u8v;
    in >> u8v; ppu.fineX = u8v;
    in >> u16v; ppu.v = u16v;
    in >> u16v; ppu.t = u16v;
    in >> u8v; ppu.dataBuffer = u8v;
    in >> u16v; ppu.pixel = u16v;
    in >> u16v; ppu.scanline = u16v;
    in >> u8v; ppu.nmi = u8v;
    in >> u8v; ppu.mirrorMode = u8v;
    in >> u8v; ppu.oddFrame = u8v;
    in >> u8v; ppu.openBus = u8v;

    for (u16 i=0; i<4096; ++i) { in >> u8v; ppu.vram[i] = u8v; }
    for (u8 i=0; i<32; ++i)   { in >> u8v; ppu.pal[i] = u8v; }
    for (u16 i=0; i<256; ++i)  { in >> u8v; ppu.oam[i] = u8v; }

    /* PPU */
    in >> u8v; ppu.bgFetch.valid = u8v;
    in >> u16v; ppu.bgFetch.v = u16v;
    in >> u16v; ppu.bgFetch.table = u16v;
    in >> u8v; ppu.bgFetch.p0 = u8v;
    in >> u8v; ppu.bgFetch.l0 = u8v;
    in >> u8v; ppu.bgFetch.h0 = u8v;
    in >> u8v; ppu.bgFetch.p1 = u8v;
    in >> u8v; ppu.bgFetch.l1 = u8v;
    in >> u8v; ppu.bgFetch.h1 = u8v;
    in >> u8v; ppu.spriteCount = u8v;
    for (auto& s : ppu.OAM) {
        in >> s.x;
        in >> s.y;
        in >> s.tile;
        in >> s.attr;
        in >> s.id;
        in >> s.low;
        in >> s.high;
    }

        /* Memory */
    for (u16 i=0; i<2048; ++i) { in >> u8v; mem.ram[i] = u8v; }
    in >> u32v; mem.dma = u32v;
    in >> u8v; mem.joy1 = u8v;
    in >> u8v; mem.joy2 = u8v;
    in >> u8v; mem.joy1Shift = u8v;
    in >> u8v; mem.joy2Shift = u8v;

    /* APU */
    in >> u8v; apu.pulse1.enabled = u8v;
    in >> u8v; apu.pulse1.duty = u8v;
    in >> u8v; apu.pulse1.volPeriod = u8v;
    in >> u8v; apu.pulse1.lengthHalt = u8v;
    in >> u8v; apu.pulse1.constVol = u8v;
    in >> u8v; apu.pulse1.envelDiv = u8v;
    in >> u8v; apu.pulse1.envelDecay = u8v;
    in >> u8v; apu.pulse1.envelStart = u8v;
    in >> u8v; apu.pulse1.swpPeriod = u8v;
    in >> u8v; apu.pulse1.swpShift = u8v;
    in >> u8v; apu.pulse1.swpDiv = u8v;
    in >> u8v; apu.pulse1.swpEnabled = u8v;
    in >> u8v; apu.pulse1.swpNegate = u8v;
    in >> u8v; apu.pulse1.swpReload = u8v;
    in >> u16v; apu.pulse1.timerPeriod = u16v;
    in >> u16v; apu.pulse1.timer = u16v;
    in >> u8v; apu.pulse1.seqPos = u8v;
    in >> u8v; apu.pulse1.lenCnt = u8v;

    in >> u8v; apu.pulse2.enabled = u8v;
    in >> u8v; apu.pulse2.duty = u8v;
    in >> u8v; apu.pulse2.volPeriod = u8v;
    in >> u8v; apu.pulse2.lengthHalt = u8v;
    in >> u8v; apu.pulse2.constVol = u8v;
    in >> u8v; apu.pulse2.envelDiv = u8v;
    in >> u8v; apu.pulse2.envelDecay = u8v;
    in >> u8v; apu.pulse2.envelStart = u8v;
    in >> u8v; apu.pulse2.swpPeriod = u8v;
    in >> u8v; apu.pulse2.swpShift = u8v;
    in >> u8v; apu.pulse2.swpDiv = u8v;
    in >> u8v; apu.pulse2.swpEnabled = u8v;
    in >> u8v; apu.pulse2.swpNegate = u8v;
    in >> u8v; apu.pulse2.swpReload = u8v;
    in >> u16v; apu.pulse2.timerPeriod = u16v;
    in >> u16v; apu.pulse2.timer = u16v;
    in >> u8v; apu.pulse2.seqPos = u8v;
    in >> u8v; apu.pulse2.lenCnt = u8v;

    in >> u8v; apu.triangle.enabled = u8v;
    in >> u8v; apu.triangle.linearReloadValue = u8v;
    in >> u8v; apu.triangle.linearCnt = u8v;
    in >> u8v; apu.triangle.ctrlFlag = u8v;
    in >> u8v; apu.triangle.linearReloadFlag = u8v;
    in >> u16v; apu.triangle.timerPeriod = u16v;
    in >> u16v; apu.triangle.timer = u16v;
    in >> u8v; apu.triangle.seqPos = u8v;
    in >> u8v; apu.triangle.lenCnt = u8v;

    in >> u8v; apu.noise.enabled = u8v;
    in >> u8v; apu.noise.volPeriod = u8v;
    in >> u8v; apu.noise.lenHalt = u8v;
    in >> u8v; apu.noise.constVol = u8v;
    in >> u8v; apu.noise.envelDiv = u8v;
    in >> u8v; apu.noise.envelDecay = u8v;
    in >> u8v; apu.noise.envelStart = u8v;
    in >> u8v; apu.noise.periodIndex = u8v;
    in >> u16v; apu.noise.timer = u16v;
    in >> u8v; apu.noise.lenCnt = u8v;
    in >> u8v; apu.noise.mode = u8v;
    in >> u16v; apu.noise.shiftReg = u16v;

    in >> u8v; apu.dmc.enabled = u8v;
    in >> u8v; apu.dmc.irqEnabled = u8v;
    in >> u8v; apu.dmc.rateIndex = u8v;
    in >> u8v; apu.dmc.outLevel = u8v;
    in >> u8v; apu.dmc.loop = u8v;
    in >> u8v; apu.dmc.irqFlag = u8v;
    in >> u8v; apu.dmc.sampleAddrReg = u8v;
    in >> u8v; apu.dmc.sampleLenReg = u8v;
    in >> u8v; apu.dmc.bitsRemain = u8v;
    in >> u8v; apu.dmc.shiftReg = u8v;
    in >> u8v; apu.dmc.sampleBuffer = u8v;
    in >> u8v; apu.dmc.bufferEmpty = u8v;
    in >> u16v; apu.dmc.timer = u16v;
    in >> u16v; apu.dmc.bytesRemain = u16v;
    in >> u8v; apu.dmc.active = u8v;

    in >> u32v; apu.frameCycle = u32v;
    in >> u8v; apu.frameCntMode5 = u8v;
    in >> u8v; apu.irqInhibit = u8v;
    in >> u8v; apu.frameIrq = u8v;
    in >> u8v; apu.oddCycle = u8v;
    in >> u8v; apu.frameCntDelay = u8v;
    in >> u8v; apu.pendQuarterFrame = u8v;
    in >> u8v; apu.pendHalfFrame = u8v;
    in >> u8v; apu.delayHalfFrame = u8v;
    in >> u8v; apu.frameIrqRepeat = u8v;
    double sampleAcc = 0.0;
    in >> sampleAcc;
    apu.sampleAcc = sampleAcc;

    return file.error() == QFile::NoError;
}
