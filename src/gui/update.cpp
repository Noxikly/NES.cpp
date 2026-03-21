#include "gui/update.h"

#include "gui/modules/audio.h"
#include "gui/w_main.h"
#include "gui/w_logs.h"
#include "ui_main.h"

namespace {
auto toAddrModeString(Core::CPU::C6502::AddrMode am) -> const char * {
    switch (am) {
    case Core::CPU::C6502::IMM:
        return "IMM";
    case Core::CPU::C6502::IMP:
        return "IMP";
    case Core::CPU::C6502::ZPG:
        return "ZPG";
    case Core::CPU::C6502::ZPGX:
        return "ZPX";
    case Core::CPU::C6502::ZPGY:
        return "ZPY";
    case Core::CPU::C6502::REL:
        return "REL";
    case Core::CPU::C6502::ABS:
        return "ABS";
    case Core::CPU::C6502::ABSX:
        return "ABX";
    case Core::CPU::C6502::ABSY:
        return "ABY";
    case Core::CPU::C6502::IND:
        return "IND";
    case Core::CPU::C6502::INDX:
        return "INX";
    case Core::CPU::C6502::INDY:
        return "INY";
    }

    return "???";
}

auto buildPttrn(const std::array<u8, 128 * 128> &pixels) -> QImage {
    QImage img(128, 128, QImage::Format_ARGB32);

    static const QRgb colors[4] = {
        qRgb(20, 20, 20), qRgb(100, 100, 100),
        qRgb(180, 180, 180), qRgb(245, 245, 245)};

    for (u8 y = 0; y < 128; ++y) {
        auto *row = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (u8 x = 0; x < 128; ++x) {
            const u8 idx = pixels[y * 128 + x] & 0x03;
            row[x] = colors[idx];
        }
    }

    return img;
}
} // namespace

WUpdate::WUpdate(WMain *owner) : main(owner) {}

void WUpdate::stpTimer() {
    if (!main)
        return;

    main->frameTimer.setTimerType(Qt::PreciseTimer);
    updFrameCap();

    QObject::connect(&main->frameTimer, &QTimer::timeout, main, [this]() {
        if (!main->romLoaded || main->paused)
            return;

        syncDebugFlagsFromLogWindow();

        doFrame();
        updFpsCounter();

        if (main->logsWindow)
            updDebugPanels();
    });

    main->frameTimer.start();
}

void WUpdate::updFrameCap() {
    if (!main)
        return;

    const bool palLike = (main->emuRegion == Core::PPU::Region::PAL) ||
                         (main->emuRegion == Core::PPU::Region::DENDY);

    main->frameTimer.setInterval(palLike ? 20 : 16);
}

void WUpdate::doFrame() {
    if (!main || !main->mapper || !main->ppu || !main->apu || !main->mem ||
        !main->cpu)
        return;

    syncInputToMemory();

    main->ppu->r.frameReady = false;
    while (!main->ppu->r.frameReady)
        runCpuInstruction();

    presentAudioAndVideo();
}

void WUpdate::updDebugPanels() {
    if (!main || !main->cpu || !main->ppu || !main->logsWindow)
        return;

    const auto &regs = main->cpu->c.regs;
    const bool nmiPending = main->cpu->c.do_nmi;
    const bool irqPending = main->cpu->c.do_irq;
    const bool irqMasked = (regs.P & Core::CPU::C6502::I) != 0;
    const QString opName = QString::fromStdString(main->cpu->getLastOpName());
    const QString amName =
        QString::fromLatin1(toAddrModeString(main->cpu->getLastAddrMode()));
    const QString cpuText = QString("OP: %1   AM: %2\n"
                                    "A:  0x%3\n"
                                    "X:  0x%4\n"
                                    "Y:  0x%5\n"
                                    "P:  %6\n"
                                    "SP: 0x%7\n"
                                    "PC: 0x%8\n"
                                    "Cycles (last): %9\n"
                                    "NMI pending: %10\n"
                                    "IRQ pending: %11   IRQ masked (I): %12")
                                .arg(opName, amName)
                                .arg(regs.A, 2, 16, QChar('0'))
                                .arg(regs.X, 2, 16, QChar('0'))
                                .arg(regs.Y, 2, 16, QChar('0'))
                                .arg(regs.P, 8, 2, QChar('0'))
                                .arg(regs.SP, 2, 16, QChar('0'))
                                .arg(regs.PC, 4, 16, QChar('0'))
                                .arg(main->cpu->c.op_cycles)
                                .arg(nmiPending ? "yes" : "no")
                                .arg(irqPending ? "yes" : "no")
                                .arg(irqMasked ? "yes" : "no");

    const auto &state = main->ppu->getState();
    const QString ppuText =
        QString("PPUCTRL:   0x%1     V:     0x%5\n"
                "PPUMASK:   0x%2     T:     0x%6\n"
                "PPUSTATUS: 0x%3     fineX: %7\n"
                "OAMADDR:   0x%4     W:     %8\n")
            .arg(state.ppuctrl, 2, 16, QChar('0'))
            .arg(state.ppumask, 2, 16, QChar('0'))
            .arg(state.ppustatus, 2, 16, QChar('0'))
            .arg(state.oamaddr, 2, 16, QChar('0'))
            .arg(state.v, 4, 16, QChar('0'))
            .arg(state.t, 4, 16, QChar('0'))
            .arg(state.fineX)
            .arg(state.w);

    const QImage pt0 = buildPttrn(main->ppu->r.getPttrnTable(0));
    const QImage pt1 = buildPttrn(main->ppu->r.getPttrnTable(1));

    main->logsWindow->setCpuDebugText(cpuText);
    main->logsWindow->setPpuDebugText(ppuText);
    main->logsWindow->setPpuPatternTables(pt0, pt1);
}

void WUpdate::updFpsCounter() {
    if (!main)
        return;

    ++main->fpsUpdate;

    const qint64 ms = main->fpsTimer.elapsed();
    if (ms < 1000)
        return;

    main->currFps = static_cast<f64>(main->fpsUpdate) * 1000.0 /
                    static_cast<f64>(ms);
    main->fpsUpdate = 0;
    main->fpsTimer.restart();

    updWindowTitle();
}

void WUpdate::updWindowTitle() {
    if (!main)
        return;

    QString title = QStringLiteral("NES.cpp");

    if (!main->currRomPath.isEmpty())
        title +=
            QStringLiteral(" - %1").arg(main->currRomPath.split('/').last());

    if (main->paused)
        title += QStringLiteral(" [PAUSED]");

    title += QStringLiteral(" - FPS: %1").arg(main->currFps, 0, 'f', 1);

    main->setWindowTitle(title);
}

void WUpdate::syncDebugFlagsFromLogWindow() {
    if (!main)
        return;

    const bool hasLogs = static_cast<bool>(main->logsWindow);

    if (main->cpu)
        main->cpu->debug = hasLogs && main->logsWindow->allowCpu();
    if (main->ppu)
        main->ppu->debug = hasLogs && main->logsWindow->allowPpu();
    if (main->apu)
        main->apu->debug = hasLogs && main->logsWindow->allowApu();
    if (main->mem)
        main->mem->debug = hasLogs && main->logsWindow->allowMemory();
    if (main->mapper)
        main->mapper->debug = hasLogs && main->logsWindow->allowMapper();
}

void WUpdate::syncInputToMemory() {
    if (!main || !main->mem)
        return;

    main->mem->setJoy1(main->joyState);
    main->mem->setJoy2(main->joyStateP2);
}

void WUpdate::runCpuInstruction() {
    if (!main || !main->mapper || !main->ppu || !main->apu || !main->mem ||
        !main->cpu)
        return;

    main->cpu->exec();

    u32 cycles = main->cpu->c.op_cycles;
    if (cycles == 0)
        cycles = 1;

    const u32 ppuNum = (main->emuRegion == Core::PPU::Region::PAL) ? 16u : 3u;
    const u32 ppuDen = (main->emuRegion == Core::PPU::Region::PAL) ? 5u : 1u;

    for (u32 c = 0; c < cycles; ++c) {
        main->ppuPhaseAcc += ppuNum;
        for (; main->ppuPhaseAcc >= ppuDen; main->ppuPhaseAcc -= ppuDen) {
            main->ppu->r.step();

            if (main->ppu->r.nmiPending()) {
                main->cpu->c.do_nmi = true;
                main->ppu->r.clearNmi();
            }
        }

        main->apu->step(1);

        if (main->mapper->irqFlag || main->apu->getState().frameIrq ||
            main->apu->getState().dmc.irqFlag) {
            main->cpu->c.do_irq = true;
        }
    }
}

void WUpdate::presentAudioAndVideo() {
    if (!main)
        return;

    if (main->audio && main->apu && !main->apu->samples.empty()) {
        main->audio->pushSamples(main->apu->samples);
        main->apu->samples.clear();
    }

    if (main->ui && main->ppu && main->ui->frameView)
        main->ui->frameView->setFrameBuffer(main->ppu->frame);
}

auto WUpdate::ppuPerCpu() const -> f64 {
    if (main && main->emuRegion == Core::PPU::Region::PAL)
        return 3.2;

    return 3.0;
}
