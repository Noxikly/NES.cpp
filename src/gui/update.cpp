#include "gui/update.h"

#include <QElapsedTimer>
#include <QMetaObject>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "common/thread.h"

#include "gui/modules/audio.h"
#include "gui/w_main.h"
#include "ui_main.h"

#if defined(DEBUG)
#include "gui/w_logs.h"
#endif

namespace {
struct EmuFrameData {
    std::array<u32, Core::PPU::WIDTH * Core::PPU::HEIGHT> frame{};
    std::vector<f32> audio;
};

auto noAudio() -> const std::vector<f32> & {
    static const std::vector<f32> v;
    return v;
}

void pumpAudio(NesAudio *a) {
    if (a)
        a->pushSamples(noAudio());
}

#if defined(DEBUG)
struct DebugSnapshot {
    QString opName;
    QString amName;

    u8 a{0};
    u8 x{0};
    u8 y{0};
    u8 p{0};
    u8 sp{0};
    u16 pc{0};
    u32 cycles{0};

    bool nmiPending{false};
    bool irqPending{false};
    bool irqMasked{false};

    u8 ppuctrl{0};
    u8 ppumask{0};
    u8 ppustatus{0};
    u8 oamaddr{0};
    u16 v{0};
    u16 t{0};
    u8 fineX{0};
    u8 w{0};

    bool withPatterns{false};
    std::array<u8, 128 * 128> pt0{};
    std::array<u8, 128 * 128> pt1{};
};

struct DebugRenderData {
    QString cpuText;
    QString ppuText;
    bool hasPatterns{false};
    QImage pt0;
    QImage pt1;
};

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

    static const QRgb colors[4] = {qRgb(20, 20, 20), qRgb(100, 100, 100),
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

auto buildCpuDebugText(const DebugSnapshot &s) -> QString {
    return QString("OP: %1   AM: %2\n"
                   "A:  0x%3\n"
                   "X:  0x%4\n"
                   "Y:  0x%5\n"
                   "P:  %6\n"
                   "SP: 0x%7\n"
                   "PC: 0x%8\n"
                   "Cycles (last): %9\n"
                   "NMI pending: %10\n"
                   "IRQ pending: %11   IRQ masked (I): %12")
        .arg(s.opName, s.amName)
        .arg(s.a, 2, 16, QChar('0'))
        .arg(s.x, 2, 16, QChar('0'))
        .arg(s.y, 2, 16, QChar('0'))
        .arg(s.p, 8, 2, QChar('0'))
        .arg(s.sp, 2, 16, QChar('0'))
        .arg(s.pc, 4, 16, QChar('0'))
        .arg(s.cycles)
        .arg(s.nmiPending ? "yes" : "no")
        .arg(s.irqPending ? "yes" : "no")
        .arg(s.irqMasked ? "yes" : "no");
}

auto buildPpuDebugText(const DebugSnapshot &s) -> QString {
    return QString("PPUCTRL:   0x%1     V:     0x%5\n"
                   "PPUMASK:   0x%2     T:     0x%6\n"
                   "PPUSTATUS: 0x%3     fineX: %7\n"
                   "OAMADDR:   0x%4     W:     %8\n")
        .arg(s.ppuctrl, 2, 16, QChar('0'))
        .arg(s.ppumask, 2, 16, QChar('0'))
        .arg(s.ppustatus, 2, 16, QChar('0'))
        .arg(s.oamaddr, 2, 16, QChar('0'))
        .arg(s.v, 4, 16, QChar('0'))
        .arg(s.t, 4, 16, QChar('0'))
        .arg(s.fineX)
        .arg(s.w);
}
#endif

} /* namespace */

#if defined(DEBUG)
struct WUpdate::DebugWorker {
    using Worker =
        Common::Thread::LatestTaskWorker<DebugSnapshot, DebugRenderData>;

    Worker worker;

    DebugWorker()
        : worker([](DebugSnapshot &&snapshot) {
              DebugRenderData render{};
              render.cpuText = buildCpuDebugText(snapshot);
              render.ppuText = buildPpuDebugText(snapshot);
              render.hasPatterns = snapshot.withPatterns;

              if (snapshot.withPatterns) {
                  render.pt0 = buildPttrn(snapshot.pt0);
                  render.pt1 = buildPttrn(snapshot.pt1);
              }

              return render;
          }) {}
};
#endif

struct WUpdate::EmuWorker {
    std::mutex coreMutex;
    Common::Thread::LatestValue<EmuFrameData> output;
    std::unique_ptr<Common::Thread::PausableLoopWorker> loop;
    std::chrono::steady_clock::time_point nextTick{};
    bool nextTickInit{false};
};

void WUpdate::handleEmuWorkerFailure() {
    if (!main)
        return;

    QMetaObject::invokeMethod(
        main,
        [this]() {
            if (!main)
                return;

            main->paused = true;
            if (main->ui && main->ui->actionPause)
                main->ui->actionPause->setChecked(true);

#if defined(DEBUG)
            if (main->logsWindow)
                main->logsWindow->setStopped(true);
#endif

            updWindowTitle();
        },
        Qt::QueuedConnection);
}

void WUpdate::emuWorkerTick() {
    if (!emuWorker)
        return;

    using clock = std::chrono::steady_clock;

    std::unique_ptr<EmuFrameData> out;
    bool canRun = false;
    bool palLike = false;

    {
        std::lock_guard<std::mutex> coreLock(emuWorker->coreMutex);

        canRun = main && main->romLoaded && !main->paused && main->ppu &&
                 main->apu && main->mem && main->mapper && main->cpu;
        palLike = main && ((main->emuRegion == Core::PPU::Region::PAL) ||
                           (main->emuRegion == Core::PPU::Region::DENDY));

        if (canRun) {
            out = std::make_unique<EmuFrameData>();

            emulateFrameCore();

            out->frame = main->ppu->frame;
            out->audio = std::move(main->apu->samples);
            main->apu->samples.clear();
        }
    }

    if (!canRun) {
        emuWorker->nextTickInit = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }

    if (!emuWorker->nextTickInit) {
        emuWorker->nextTick = clock::now();
        emuWorker->nextTickInit = true;
    }

    const auto frameDuration = palLike ? std::chrono::milliseconds(20)
                                       : std::chrono::microseconds(16667);
    emuWorker->nextTick += frameDuration;

    if (out)
        emuWorker->output.publish(std::move(*out));

    const auto now = clock::now();
    if (now < emuWorker->nextTick)
        std::this_thread::sleep_until(emuWorker->nextTick);
    else
        emuWorker->nextTick = now;
}

WUpdate::WUpdate(WMain *p) : main(p) {
    startEmuWorker();

#if defined(DEBUG)
    startDebugWorker();
#endif
}

WUpdate::~WUpdate() {
    stopEmuWorker();

#if defined(DEBUG)
    stopDebugWorker();
#endif
}

void WUpdate::startEmuWorker() {
    if (emuWorker)
        return;

    emuWorker = std::make_unique<EmuWorker>();
    emuWorker->loop = std::make_unique<Common::Thread::PausableLoopWorker>(
        [this]() { emuWorkerTick(); },
        [this](std::exception_ptr) { handleEmuWorkerFailure(); });
}

void WUpdate::stopEmuWorker() {
    if (!emuWorker)
        return;

    if (emuWorker->loop)
        emuWorker->loop->stop();

    emuWorker.reset();
}

auto WUpdate::suspendForCriticalSection() -> bool {
    if (!emuWorker)
        return false;

    bool wasRunning = false;
    if (emuWorker->loop) {
        wasRunning = !emuWorker->loop->isPaused();
        emuWorker->loop->pause();
    }

    std::lock_guard<std::mutex> coreLock(emuWorker->coreMutex);
    return wasRunning;
}

void WUpdate::resumeAfterCriticalSection(bool wasRunning) {
    if (!emuWorker || !wasRunning)
        return;

    if (emuWorker->loop)
        emuWorker->loop->resume();
}

auto WUpdate::applyReadyEmuFrame() -> bool {
    if (!main || !emuWorker)
        return false;

    auto ready = emuWorker->output.tryTake();
    if (!ready.has_value())
        return false;

    if (main->audio)
        main->audio->pushSamples(ready->audio);

    if (main->ui && main->ui->frameView)
        main->ui->frameView->setFrameBuffer(ready->frame);

    return true;
}

void WUpdate::syncDbgSafe() {
    if (!emuWorker) {
        syncDebugFlagsFromLogWindow();
        return;
    }

    std::lock_guard<std::mutex> lk(emuWorker->coreMutex);
    syncDebugFlagsFromLogWindow();
}

#if defined(DEBUG)
auto WUpdate::shouldSyncDebugFlags() const -> bool {
    if (!main)
        return false;

    const bool logsVisible =
        (main->logsWindow != nullptr) && main->logsWindow->isVisible();

    static bool lastLogsVisible = false;
    static QElapsedTimer debugSyncTimer;
    static bool debugSyncTimerStarted = false;

    if (!debugSyncTimerStarted) {
        debugSyncTimer.start();
        debugSyncTimerStarted = true;
        lastLogsVisible = logsVisible;
        return true;
    }

    if (logsVisible != lastLogsVisible) {
        lastLogsVisible = logsVisible;
        debugSyncTimer.restart();
        return true;
    }

    if (logsVisible && debugSyncTimer.elapsed() >= 100) {
        debugSyncTimer.restart();
        return true;
    }

    return false;
}

void WUpdate::startDebugWorker() {
    if (debugWorker)
        return;

    debugWorker = std::make_unique<DebugWorker>();
}

void WUpdate::stopDebugWorker() {
    if (!debugWorker)
        return;

    debugWorker->worker.stop();

    debugWorker.reset();
}

void WUpdate::submitDebugSnapshot() {
    if (!main || !main->cpu || !main->ppu || !main->logsWindow || !debugWorker)
        return;

    std::unique_lock<std::mutex> coreLock;
    if (emuWorker)
        coreLock = std::unique_lock<std::mutex>(emuWorker->coreMutex);

    DebugSnapshot snapshot{};

    const auto &regs = main->cpu->c.regs;
    snapshot.opName = QString::fromStdString(main->cpu->getLastOpName());
    snapshot.amName =
        QString::fromLatin1(toAddrModeString(main->cpu->getLastAddrMode()));

    snapshot.a = regs.A;
    snapshot.x = regs.X;
    snapshot.y = regs.Y;
    snapshot.p = regs.P;
    snapshot.sp = regs.SP;
    snapshot.pc = regs.PC;
    snapshot.cycles = main->cpu->c.op_cycles;

    snapshot.nmiPending = main->cpu->c.do_nmi;
    snapshot.irqPending = main->cpu->c.do_irq;
    snapshot.irqMasked = (regs.P & Core::CPU::C6502::I) != 0;

    const auto &state = main->ppu->getState();
    snapshot.ppuctrl = state.ppuctrl;
    snapshot.ppumask = state.ppumask;
    snapshot.ppustatus = state.ppustatus;
    snapshot.oamaddr = state.oamaddr;
    snapshot.v = state.v;
    snapshot.t = state.t;
    snapshot.fineX = state.fineX;
    snapshot.w = state.w;

    const bool ppuTabActive = main->logsWindow->isPpuTabActive();
    bool updatePatternTables = false;

    if (ppuTabActive) {
        static QElapsedTimer patternTimer;
        static bool patternTimerStarted = false;

        if (!patternTimerStarted) {
            patternTimer.start();
            patternTimerStarted = true;
            updatePatternTables = true;
        } else if (patternTimer.elapsed() >= 33) {
            patternTimer.restart();
            updatePatternTables = true;
        }
    }

    if (updatePatternTables) {
        snapshot.withPatterns = true;
        snapshot.pt0 = main->ppu->r.getPttrnTable(0);
        snapshot.pt1 = main->ppu->r.getPttrnTable(1);
    }

    debugWorker->worker.submit(std::move(snapshot));
}

void WUpdate::applyReadyDebugResult() {
    if (!main || !main->logsWindow || !debugWorker)
        return;

    auto ready = debugWorker->worker.tryTake();
    if (!ready.has_value())
        return;

    static QString lastCpuText;
    static QString lastPpuText;

    if (main->logsWindow->isCpuTabActive() && ready->cpuText != lastCpuText) {
        main->logsWindow->setCpuDebugText(ready->cpuText);
        lastCpuText = ready->cpuText;
    }

    if (main->logsWindow->isPpuTabActive() && ready->ppuText != lastPpuText) {
        main->logsWindow->setPpuDebugText(ready->ppuText);
        lastPpuText = ready->ppuText;
    }

    if (main->logsWindow->isPpuTabActive() && ready->hasPatterns)
        main->logsWindow->setPpuPatternTables(ready->pt0, ready->pt1);
}
#endif

void WUpdate::stpTimer() {
    if (!main)
        return;

    main->frameTimer.setTimerType(Qt::PreciseTimer);
    updFrameCap();

    QObject::connect(&main->frameTimer, &QTimer::timeout, main, [this]() {
        if (!main->romLoaded)
            return;

        bool syncDbg = true;

#if defined(DEBUG)
        syncDbg = shouldSyncDebugFlags();
#endif

        if (syncDbg)
            syncDbgSafe();

        if (!applyReadyEmuFrame()) {
            pumpAudio(main->audio.get());
            return;
        }

        updFpsCounter();

#if defined(DEBUG)
        if (main->logsWindow && main->logsWindow->isVisible()) {
            updDebugPanels();
        }
#endif
    });

    main->frameTimer.start();
}

void WUpdate::updFrameCap() {
    if (!main)
        return;

    main->frameTimer.setInterval(2);
}

void WUpdate::doFrame() {
    if (!emuWorker) {
        emulateFrameCore();
        presentAudioAndVideo();
        return;
    }

    std::lock_guard<std::mutex> coreLock(emuWorker->coreMutex);
    emulateFrameCore();
    presentAudioAndVideo();
}

void WUpdate::emulateFrameCore() {
    if (!main || !main->mapper || !main->ppu || !main->apu || !main->mem ||
        !main->cpu)
        return;

    syncInputToMemory();

    static constexpr u32 kMaxCpuInstructionsPerFrame = 2000000;
    u32 safetyCounter = 0;

    main->ppu->r.frameReady = false;
    while (!main->ppu->r.frameReady) {
        runCpuInstruction();

        if (++safetyCounter >= kMaxCpuInstructionsPerFrame) {
            if (main)
                main->paused = true;
            break;
        }
    }
}

#if defined(DEBUG)
void WUpdate::updDebugPanels() {
    if (!main || !main->cpu || !main->ppu || !main->logsWindow)
        return;

    applyReadyDebugResult();
    submitDebugSnapshot();
}
#endif

void WUpdate::updFpsCounter() {
    if (!main)
        return;

    ++main->fpsUpdate;

    const qint64 ms = main->fpsTimer.elapsed();
    if (ms < 1000)
        return;

    main->currFps =
        static_cast<f64>(main->fpsUpdate) * 1000.0 / static_cast<f64>(ms);
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

#if !defined(DEBUG)
    if (main->cpu)
        main->cpu->debug = false;
    if (main->ppu)
        main->ppu->debug = false;
    if (main->apu)
        main->apu->debug = false;
    if (main->mem)
        main->mem->debug = false;
    if (main->mapper)
        main->mapper->debug = false;
#else
    const bool hasLogs =
        static_cast<bool>(main->logsWindow) && main->logsWindow->isVisible();

    if (main->cpu)
        main->cpu->debug = hasLogs && main->logsWindow->allowCpu();
    if (main->ppu)
        main->ppu->debug = hasLogs && main->logsWindow->allowPpu();
    if (main->apu)
        main->apu->debug = false;
    if (main->mem)
        main->mem->debug = false;
    if (main->mapper)
        main->mapper->debug = false;
#endif
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
    const u32 totalPhase = main->ppuPhaseAcc + (cycles * ppuNum);
    const u32 ppuSteps = totalPhase / ppuDen;
    main->ppuPhaseAcc = totalPhase % ppuDen;

    for (u32 ppuStep = 0; ppuStep < ppuSteps; ++ppuStep) {
        main->ppu->r.step();

        if (main->ppu->r.nmiPending()) {
            main->cpu->c.do_nmi = true;
            main->ppu->r.clearNmi();
        }
    }

    main->apu->step(cycles);

    const auto &apuState = main->apu->getState();
    if (main->mapper->irqFlag || apuState.frameIrq || apuState.dmc.irqFlag)
        main->cpu->c.do_irq = true;
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
