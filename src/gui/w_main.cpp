#include "w_main.hpp"
#include "nes_state.hpp"
#include "ui_w_main.h"

#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDockWidget>
#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QFontDatabase>

#include <filesystem>

#include "core/cpu.hpp"
#include "core/mapper.hpp"
#include "core/memory.hpp"
#include "core/ppu.hpp"
#include "core/apu.hpp"


auto buildPttrn(const std::array<u8, 128 * 128>& pixels) -> QImage {
    QImage img(128, 128, QImage::Format_ARGB32);

    static const QRgb colors[4] = {
        qRgb(20, 20, 20),
        qRgb(100, 100, 100),
        qRgb(180, 180, 180),
        qRgb(245, 245, 245)
    };

    for (u8 y=0; y<128; ++y) {
        auto* row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (u8 x=0; x<128; ++x) {
            const u8 idx = pixels[y * 128 + x] & 0x03;
            row[x] = colors[idx];
        }
    }

    return img;
}


WMain::WMain(const QString& romPath, QWidget* parent)
    : QMainWindow(parent),
      ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);

    stpDockWidgets();
    stpMenuActions();
    stpTimer();

    fpsTimer.start();
    updWindowTitle();

    if (!romPath.isEmpty())
        loadRom(romPath);
}
WMain::~WMain() = default;

void WMain::clearCore() {
    cpu.reset();
    mem.reset();
    ppu.reset();
    mapper.reset();
    apu.reset();

    audio.reset();

    joyState = 0;
    romLoaded = false; 
    paused = false;
}


/* Контроллер NES */
u8 WMain::joyMaskForKey(u32 key) {
    switch (key) {
        case Qt::Key_D: return 0x80;     /* Right  */
        case Qt::Key_A: return 0x40;     /* Left   */
        case Qt::Key_S: return 0x20;     /* Down   */
        case Qt::Key_W: return 0x10;     /* Up     */
        case Qt::Key_Enter:
        case Qt::Key_C: return 0x08;     /* Start  */
        case Qt::Key_Shift: return 0x04; /* Select */
        case Qt::Key_X: return 0x02;     /* B      */
        case Qt::Key_Z: return 0x01;     /* A      */
        default: return 0;
    }
}


/* GUI */
void WMain::stpDockWidgets() {
    removeDockWidget(ui->dockCPU);
    removeDockWidget(ui->dockAPU);
    removeDockWidget(ui->dockPPU);

    addDockWidget(Qt::LeftDockWidgetArea, ui->dockAPU);
    addDockWidget(Qt::RightDockWidgetArea, ui->dockCPU);
    addDockWidget(Qt::RightDockWidgetArea, ui->dockPPU);
    splitDockWidget(ui->dockCPU, ui->dockPPU, Qt::Vertical);

    ui->dockAPU->hide();
    ui->dockCPU->hide();
    ui->dockPPU->hide();

    ui->actionAPU->setChecked(false);
    ui->actionCPU->setChecked(false);
    ui->actionPPU->setChecked(false);

    connect(ui->actionAPU, &QAction::toggled, ui->dockAPU, &QDockWidget::setVisible);
    connect(ui->actionCPU, &QAction::toggled, ui->dockCPU, &QDockWidget::setVisible);
    connect(ui->actionPPU, &QAction::toggled, ui->dockPPU, &QDockWidget::setVisible);

    connect(ui->dockAPU, &QDockWidget::visibilityChanged, ui->actionAPU, &QAction::setChecked);
    connect(ui->dockCPU, &QDockWidget::visibilityChanged, ui->actionCPU, &QAction::setChecked);
    connect(ui->dockPPU, &QDockWidget::visibilityChanged, ui->actionPPU, &QAction::setChecked);

    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->apuDebugText->setFont(mono);
    ui->cpuDebugText->setFont(mono);
    ui->ppuDebugText->setFont(mono);

    auto Preview = [](QLabel* label) {
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(150, 150);
        label->setStyleSheet("background-color: #111; border: 1px solid #444;");
    };

    pttrnRow = new QHBoxLayout();
    ppuPttrn0 = new QLabel(ui->dockPPUContents);
    ppuPttrn1 = new QLabel(ui->dockPPUContents);
    Preview(ppuPttrn0);
    Preview(ppuPttrn1);

    pttrnRow->addWidget(ppuPttrn0);
    pttrnRow->addWidget(ppuPttrn1);
    ui->verticalLayoutPPU->addLayout(pttrnRow);
}

void WMain::stpMenuActions() {
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionOpen_ROM, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this,
            tr("Open NES ROM"),
            QString(),
            tr("NES ROM (*.nes);;All files (*.*)")
        );

        if (!path.isEmpty())
            loadRom(path);
    });

    connect(ui->actionLoad, &QAction::triggered, this, [this]() {
        if (!romLoaded) return;

        const QString path = QFileDialog::getOpenFileName(
            this,
            tr("Load State"),
            QString(),
            tr("NES State (*.nst);;All files (*.*)")
        );

        if (!path.isEmpty()) {
            Cpu::State cpuState;
            Ppu::State ppuState;
            Memory::State memState;
            Mapper::State mpState;
            Apu::State apuState;
            if (loadBinState(path, cpuState, ppuState, memState, mpState, apuState)) {
                if (!currRomPath.isEmpty())
                    mapper.get()->loadNES(currRomPath.toStdString());

                if(mapper->mapperNumber != mpState.mapperNumber) {
                    QMessageBox::warning(
                        this, 
                        tr("Load State"), 
                        tr("Mapper number mismatch. State file was created with a different mapper.")
                    );
                    return;
                }

                mapper->loadState(mpState);
                cpu->loadState(cpuState);
                ppu->loadState(ppuState);
                mem->loadState(memState);
                apu->loadState(apuState);

                doFrame();
                paused = true; 
                ui->actionPause->setChecked(true);
                updWindowTitle();
            } else
                QMessageBox::warning(
                    this, 
                    tr("Load State"), 
                    tr("Failed to load state from file.")
                );
        }
    });

    connect(ui->actionSave, &QAction::triggered, this, [this]() {
        if (!romLoaded) return;

        const QString path = QFileDialog::getSaveFileName(
            this,
            tr("Save State"),
            QString(),
            tr("NES State (*.nst);;All files (*.*)")
        );

        if (!path.isEmpty()) {
            if (!saveBinState(path, 
                              cpu->getState(), 
                              ppu->getState(), 
                              mem->getState(),
                              mapper->getState(),
                              apu->getState()))
            {
                QMessageBox::warning(
                    this, 
                    tr("Save State"), 
                    tr("Failed to save state to file.")
                );
            }
        }
    });

    connect(ui->actionPause, &QAction::toggled, this, [this](bool checked) {
        paused = checked;
        updWindowTitle();
    });

    connect(ui->actionReset, &QAction::triggered, this, [this]() {
        if (cpu) {
            cpu->reset();
            joyState = 0;
        }
    });

    connect(ui->actionReload, &QAction::triggered, this, [this]() {
        if (!currRomPath.isEmpty())
            loadRom(currRomPath);
    });

    connect(ui->actionShow_FPS, &QAction::toggled, this, [this](bool) {
        updWindowTitle();
    });
}


/* Эмуляция */
void WMain::stpTimer() {
    frameTimer.setTimerType(Qt::PreciseTimer);
    frameTimer.setInterval(16);

    connect(&frameTimer, &QTimer::timeout, this, [this]() {
        if (!romLoaded || paused) return;

        doFrame();
        updFpsCounter();

        if (ui->actionAPU->isChecked() || ui->actionCPU->isChecked() || ui->actionPPU->isChecked())
            updDebugPanels();
    });

    frameTimer.start();
}

void WMain::loadRom(const QString& romPath) {
    try {
        clearCore();
        mapper = std::make_unique<Mapper>();
        mapper->loadNES(romPath.toStdWString());

        std::filesystem::path mapperDir;
        const QStringList dirs = {
            QDir::cleanPath(QCoreApplication::applicationDirPath() + "/mappers"),
            QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../mappers"),
            QDir::cleanPath(QDir::currentPath() + "/mappers")
        };

        for (const QString& dir : dirs) {
            if (QDir(dir).exists()) {
                mapperDir = dir.toStdWString();
                break;
            }
        }

        if (mapperDir.empty()) mapper->load();
        else mapper->load(mapperDir);

        ppu = std::make_unique<Ppu>();
        ppu->setMapper(mapper.get());

        apu = std::make_unique<Apu>();

        mem = std::make_unique<Memory>(mapper.get(), ppu.get(), apu.get());
        cpu = std::make_unique<Cpu>(mem.get());
        cpu->powerUp();

        audio.reset();

        currRomPath = romPath;
        romLoaded = true;
        paused = false;
        joyState = 0;

        ui->actionPause->setChecked(false);
        ui->apuDebugText->clear();
        ui->cpuDebugText->clear();
        ui->ppuDebugText->clear();

        FpsUpdate = 0;
        currFps = 0.0;
        fpsTimer.restart();

        updWindowTitle();
    } catch (const std::exception& e) {
        clearCore();
        romLoaded = false;
        paused = false;

        ui->nesScreen->clear();
        ui->apuDebugText->clear();
        ui->cpuDebugText->clear();
        ui->ppuDebugText->clear();
        updWindowTitle();

        QMessageBox::critical(
            this,
            tr("ROM load error"),
            tr("Failed to load ROM:\n%1").arg(QString::fromStdString(e.what()))
        );
    }
}

void WMain::doFrame() {
    if (!mapper || !ppu || !mem || !cpu || !apu) return;

    mem->setJoy1(joyState);

    bool frameCompleted = false;
    while (!frameCompleted) {
        cpu->exec();

        const u32 cycles = cpu->getCycles();
        apu->step(cycles);

        const u32 ppuSteps = cycles * 3;
        for (u32 i=0; i<ppuSteps; ++i) {
            ppu->step();

            if (ppu->frameReady) {
                ppu->frameReady = false;
                frameCompleted = true;
            }
        }

        if (ppu->nmiPending()) {
            cpu->getState().do_nmi = true;
            ppu->clearNmi();
        }

        if (mapper->irqFlag)
           cpu->getState().do_irq = true;
    }

    ui->nesScreen->setFrameBuffer(ppu->getFrame());
    audio.pushSamples(apu->takeSamples());
}

void WMain::updDebugPanels() {
    if (!cpu || !ppu || !apu) return;

    if (ui->actionAPU->isChecked()) {
        const auto& s = apu->getState();
        ui->apuDebugText->setPlainText(QString(
            "FRAME\n"
            "cycle:  %1\n"
            "odd:    %2\n"
            "mode5:  %3\n"
            "irqInh: %4\n"
            "frameIrq:%5\n"
            "\n"
            "PULSE 1\n"
            "en:%6  len:%7\n"
            "tim:%8 seq:%9\n"
            "duty:%10 env:%11\n"
            "vol:%12\n"
            "\n"
            "PULSE 2\n"
            "en:%13 len:%14\n"
            "tim:%15 seq:%16\n"
            "duty:%17 env:%18\n"
            "vol:%19\n"
            "\n"
            "TRIANGLE\n"
            "en:%20 len:%21\n"
            "lin:%22 reload:%23\n"
            "tim:%24 seq:%25\n"
            "\n"
            "NOISE\n"
            "en:%26 len:%27\n"
            "per:%28 tim:%29\n"
            "mode:%30 sh:%31\n"
            "env:%32 vol:%33\n"
            "\n"
            "DMC\n"
            "en:%34 act:%35\n"
            "irqEn:%36 loop:%37 irq:%38\n"
            "rate:%39 timer:%40\n"
            "out:%41 bits:%42\n"
            "bytes:%43 empty:%44\n"
            "addr:0x%45 len:0x%46\n"
            "samp:0x%47 sh:0x%48"
        )
        .arg(s.frameCycle)
        .arg(s.oddCycle ? 1 : 0)
        .arg(s.frameCntMode5 ? 1 : 0)
        .arg(s.irqInhibit ? 1 : 0)
        .arg(s.frameIrq ? 1 : 0)
        .arg(s.pulse1.enabled ? 1 : 0)
        .arg(s.pulse1.lenCnt)
        .arg(s.pulse1.timerPeriod)
        .arg(s.pulse1.seqPos)
        .arg(s.pulse1.duty)
        .arg(s.pulse1.envelDecay)
        .arg(s.pulse1.constVol ? s.pulse1.volPeriod : s.pulse1.envelDecay)
        .arg(s.pulse2.enabled ? 1 : 0)
        .arg(s.pulse2.lenCnt)
        .arg(s.pulse2.timerPeriod)
        .arg(s.pulse2.seqPos)
        .arg(s.pulse2.duty)
        .arg(s.pulse2.envelDecay)
        .arg(s.pulse2.constVol ? s.pulse2.volPeriod : s.pulse2.envelDecay)
        .arg(s.triangle.enabled ? 1 : 0)
        .arg(s.triangle.lenCnt)
        .arg(s.triangle.linearCnt)
        .arg(s.triangle.linearReloadFlag ? 1 : 0)
        .arg(s.triangle.timerPeriod)
        .arg(s.triangle.seqPos)
        .arg(s.noise.enabled ? 1 : 0)
        .arg(s.noise.lenCnt)
        .arg(s.noise.periodIndex)
        .arg(s.noise.timer)
        .arg(s.noise.mode ? 1 : 0)
        .arg(s.noise.shiftReg, 4, 16, QChar('0'))
        .arg(s.noise.envelDecay)
        .arg(s.noise.constVol ? s.noise.volPeriod : s.noise.envelDecay)
        .arg(s.dmc.enabled ? 1 : 0)
        .arg(s.dmc.active ? 1 : 0)
        .arg(s.dmc.irqEnabled ? 1 : 0)
        .arg(s.dmc.loop ? 1 : 0)
        .arg(s.dmc.irqFlag ? 1 : 0)
        .arg(s.dmc.rateIndex)
        .arg(s.dmc.timer)
        .arg(s.dmc.outLevel)
        .arg(s.dmc.bitsRemain)
        .arg(s.dmc.bytesRemain)
        .arg(s.dmc.bufferEmpty ? 1 : 0)
        .arg(s.dmc.sampleAddrReg, 2, 16, QChar('0'))
        .arg(s.dmc.sampleLenReg, 2, 16, QChar('0'))
        .arg(s.dmc.sampleBuffer, 2, 16, QChar('0'))
        .arg(s.dmc.shiftReg, 2, 16, QChar('0'))
        );
    }

    if (ui->actionCPU->isChecked()) {
        const auto& state = cpu->getState();
        ui->cpuDebugText->setPlainText(QString(
            "OP: %1      AM:   %10\n"
            "A:  0x%2     Addr: 0x%11\n"
            "X:  0x%3\n"
            "Y:  0x%4         NV1BDIZC\n"
            "SP: 0x%6     P:  %5\n"
            "PC: 0x%7\n"
            "\n"
            "Cycles (last):  %8\n"
            "Cycles (total): %9"
        )
        .arg(QString::fromStdString(state.dbg.op), 3)
        .arg(state.regs.A, 2, 16, QChar('0'))
        .arg(state.regs.X, 2, 16, QChar('0'))
        .arg(state.regs.Y, 2, 16, QChar('0'))
        .arg(state.regs.P, 8, 2, QChar('0'))
        .arg(state.regs.SP, 2, 16, QChar('0'))
        .arg(state.regs.PC, 4, 16, QChar('0'))
        .arg(cpu->getCycles())
        .arg(cpu->getTotalCycles())
        .arg(QString::fromStdString(state.dbg.am), 3)
        .arg(state.dbg.addr, 4, 16, QChar('0'))
        );
    }

    if (ui->actionPPU->isChecked()) {
        const auto& state = ppu->getState();
        ui->ppuDebugText->setPlainText(QString(
            "PPUCTRL:   0x%1     V:     0x%5\n"
            "PPUMASK:   0x%2     T:     0x%6\n"
            "PPUSTATUS: 0x%3     fineX: %7\n"
            "OAMADDR:   0x%4     W:     %8\n"
        )
        .arg(state.ppuctrl, 2, 16, QChar('0'))
        .arg(state.ppumask, 2, 16, QChar('0'))
        .arg(state.ppustatus, 2, 16, QChar('0'))
        .arg(state.oamaddr, 2, 16, QChar('0'))
        .arg(state.v, 4, 16, QChar('0'))
        .arg(state.t, 4, 16, QChar('0'))
        .arg(state.fineX)
        .arg(state.w)
        );

        const QImage pt0 = buildPttrn(ppu->getPttrnTable(0));
        const QImage pt1 = buildPttrn(ppu->getPttrnTable(1));

        if (ppuPttrn0) {
            ppuPttrn0->setPixmap(QPixmap::fromImage(
                pt0.scaled(ppuPttrn0->size(), Qt::KeepAspectRatio, Qt::FastTransformation)
            ));
        }
        if (ppuPttrn1) {
            ppuPttrn1->setPixmap(QPixmap::fromImage(
                pt1.scaled(ppuPttrn1->size(), Qt::KeepAspectRatio, Qt::FastTransformation)
            ));
        }
    }
}

void WMain::updFpsCounter() {
    ++FpsUpdate;

    const qint64 ms = fpsTimer.elapsed();
    if (ms < 1000) return;

    currFps = static_cast<double>(FpsUpdate) * 1000.0 / static_cast<double>(ms);
    FpsUpdate = 0;
    fpsTimer.restart();

    if (ui->actionShow_FPS->isChecked())
        updWindowTitle();
}


/* Окно */
void WMain::updWindowTitle() {
    QString title = QStringLiteral("NES.cpp");

    if (!currRomPath.isEmpty())
        title += QStringLiteral(" - %1").arg(currRomPath.split('/').last());

    if (paused)
        title += QStringLiteral(" [PAUSED]");

    if (ui->actionShow_FPS->isChecked())
        title += QStringLiteral(" - FPS: %1").arg(currFps, 0, 'f', 1);

    setWindowTitle(title);
}


void WMain::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    if (const u8 mask = joyMaskForKey(event->key()); mask != 0) {
        joyState |= mask;
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void WMain::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    if (const u8 mask = joyMaskForKey(event->key()); mask != 0) {
        joyState &= static_cast<u8>(~mask);
        event->accept();
        return;
    }

    QMainWindow::keyReleaseEvent(event);
}
