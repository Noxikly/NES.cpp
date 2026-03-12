#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QSlider>
#include <QWidgetAction>

#include "nes_state.hpp"
#include "ui_w_main.h"
#include "w_main.hpp"

#include <filesystem>

#include "core/apu.hpp"
#include "core/common.hpp"
#include "core/cpu.hpp"
#include "core/mapper.hpp"
#include "core/mem.hpp"
#include "core/ppu.hpp"
#include "nes_audio.hpp"

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
} // namespace

WMain::WMain(const QString &romPath, QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);
    setFocus(Qt::OtherFocusReason);
    ui->nesScreen->setFocusPolicy(Qt::NoFocus);

    stpDockWidgets();
    stpMenuActions();
    applyRegion(Region::NTSC);
    stpTimer();

    fpsTimer.start();
    updWindowTitle();

    if (!romPath.isEmpty())
        loadRom(romPath);
}

WMain::~WMain() = default;

void WMain::clearCore() {
    cpu.reset();
    apu.reset();
    audio.reset();
    mem.reset();
    ppu.reset();
    mapper.reset();

    joyState = 0;
    joyStateP2 = 0;
    ppuCyclesDebt = 0.0;
    romLoaded = 0;
    paused = 0;
}

u8 WMain::joyMaskKey(u32 key) {
    switch (key) {
    case Qt::Key_D:
        return 0x80;
    case Qt::Key_A:
        return 0x40;
    case Qt::Key_S:
        return 0x20;
    case Qt::Key_W:
        return 0x10;
    case Qt::Key_Enter:
    case Qt::Key_C:
        return 0x08;
    case Qt::Key_Shift:
        return 0x04;
    case Qt::Key_X:
        return 0x02;
    case Qt::Key_Z:
        return 0x01;
    default:
        return 0;
    }
}

u8 WMain::joyMaskKeyP2(u32 key) {
    switch (key) {
    case Qt::Key_Right:
        return 0x80;
    case Qt::Key_Left:
        return 0x40;
    case Qt::Key_Down:
        return 0x20;
    case Qt::Key_Up:
        return 0x10;
    case Qt::Key_Enter:
        return 0x08;
    case Qt::Key_Shift:
        return 0x04;
    case Qt::Key_N:
        return 0x02;
    case Qt::Key_M:
        return 0x01;
    default:
        return 0;
    }
}

void WMain::stpDockWidgets() {
    removeDockWidget(ui->dockCPU);
    removeDockWidget(ui->dockPPU);

    addDockWidget(Qt::RightDockWidgetArea, ui->dockCPU);
    addDockWidget(Qt::RightDockWidgetArea, ui->dockPPU);
    splitDockWidget(ui->dockCPU, ui->dockPPU, Qt::Vertical);

    ui->dockCPU->hide();
    ui->dockPPU->hide();

    ui->actionCPU->setChecked(0);
    ui->actionPPU->setChecked(0);

    connect(ui->actionCPU, &QAction::toggled, ui->dockCPU,
            &QDockWidget::setVisible);
    connect(ui->actionPPU, &QAction::toggled, ui->dockPPU,
            &QDockWidget::setVisible);

    connect(ui->dockCPU, &QDockWidget::visibilityChanged, ui->actionCPU,
            &QAction::setChecked);
    connect(ui->dockPPU, &QDockWidget::visibilityChanged, ui->actionPPU,
            &QAction::setChecked);

    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->cpuDebugText->setFont(mono);
    ui->ppuDebugText->setFont(mono);

    auto preview = [](QLabel *label) {
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(150, 150);
        label->setStyleSheet("background-color: #111; border: 1px solid #444;");
    };

    auto *pttrnRow = new QHBoxLayout();
    auto *ppuPttrn0 = new QLabel(ui->dockPPUContents);
    auto *ppuPttrn1 = new QLabel(ui->dockPPUContents);
    ppuPttrn0->setObjectName("ppuPattern0Label");
    ppuPttrn1->setObjectName("ppuPattern1Label");
    preview(ppuPttrn0);
    preview(ppuPttrn1);

    pttrnRow->addWidget(ppuPttrn0);
    pttrnRow->addWidget(ppuPttrn1);
    ui->verticalLayoutPPU->addLayout(pttrnRow);

    ui->actionAudioEnable->setChecked(true);

    auto *volumeAction = new QWidgetAction(ui->menuAudio);
    auto *volumeWidget = new QWidget(ui->menuAudio);
    auto *volumeLayout = new QHBoxLayout(volumeWidget);
    volumeLayout->setContentsMargins(8, 4, 8, 4);
    volumeLayout->setSpacing(8);

    auto *volumeLabel = new QLabel(tr("Volume"), volumeWidget);
    auto *volumeSlider = new QSlider(Qt::Horizontal, volumeWidget);
    volumeSlider->setObjectName("audioVolumeSlider");
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(100);

    volumeLayout->addWidget(volumeLabel);
    volumeLayout->addWidget(volumeSlider);
    volumeAction->setDefaultWidget(volumeWidget);
    ui->menuAudio->addAction(volumeAction);
    volumeAction->setVisible(ui->actionAudioEnable->isChecked());
}

void WMain::stpMenuActions() {
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    auto *region = new QActionGroup(this);
    region->setExclusive(true);
    region->addAction(ui->actionRegion_NTSC);
    region->addAction(ui->actionRegion_PAL);
    region->addAction(ui->actionRegion_Dendy);

    connect(ui->actionRegion_NTSC, &QAction::toggled, this,
            [this](bool checked) {
                if (checked)
                    applyRegion(Region::NTSC);
            });

    connect(ui->actionRegion_PAL, &QAction::toggled, this,
            [this](bool checked) {
                if (checked)
                    applyRegion(Region::PAL);
            });

    connect(ui->actionRegion_Dendy, &QAction::toggled, this,
            [this](bool checked) {
                if (checked)
                    applyRegion(Region::DENDY);
            });

    connect(ui->actionOpen_ROM, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Open NES ROM"), QString(),
            tr("NES ROM (*.nes);;All files (*.*)"));

        if (!path.isEmpty())
            loadRom(path);
    });

    connect(ui->actionLoad, &QAction::triggered, this, [this]() {
        if (!romLoaded || !cpu || !ppu || !apu || !mem || !mapper)
            return;

        const QString path = QFileDialog::getOpenFileName(
            this, tr("Load State"), QString(),
            tr("NES State (*.nst);;All files (*.*)"));

        if (path.isEmpty())
            return;

        Core::CPU::State cpuState{};
        Core::PPU::State ppuState{};
        Core::APU::State apuState{};
        Core::Memory::State memState{};
        Core::Mapper::State mapperState{};
        u8 regionState = 0;

        if (!loadBinState(path, cpuState, ppuState, apuState, memState,
                          mapperState, regionState)) {
            QMessageBox::warning(this, tr("Load State"),
                                 tr("Failed to load state from file."));
            return;
        }

        if (!currRomPath.isEmpty())
            mapper->loadNES(currRomPath.toStdWString());

        if (mapper->mapperNumber != mapperState.mapperNumber) {
            QMessageBox::warning(this, tr("Load State"),
                                 tr("Mapper number mismatch. State file was "
                                    "created with a different mapper."));
            return;
        }

        mapper->loadState(mapperState);
        cpu->loadState(cpuState);
        ppu->loadState(ppuState);
        apu->loadState(apuState);
        mem->loadState(memState);

        switch (regionState) {
        case 1:
            applyRegion(Region::PAL);
            break;
        case 2:
            applyRegion(Region::DENDY);
            break;
        default:
            applyRegion(Region::NTSC);
            break;
        }
        if (audio)
            audio->reset();

        doFrame();
        paused = true;
        ui->actionPause->setChecked(true);
        updWindowTitle();
    });

    connect(ui->actionSave, &QAction::triggered, this, [this]() {
        if (!romLoaded || !cpu || !ppu || !apu || !mem || !mapper)
            return;

        const QString path = QFileDialog::getSaveFileName(
            this, tr("Save State"), QString(),
            tr("NES State (*.nst);;All files (*.*)"));

        if (path.isEmpty())
            return;

        if (!saveBinState(path, cpu->getState(), ppu->getState(),
                          apu->getState(), mem->getState(), mapper->getState(),
                          static_cast<u8>(emuRegion == Region::PAL     ? 1
                                          : emuRegion == Region::DENDY ? 2
                                                                       : 0))) {
            QMessageBox::warning(this, tr("Save State"),
                                 tr("Failed to save state to file."));
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
            joyStateP2 = 0;
        }
    });

    connect(ui->actionReload, &QAction::triggered, this, [this]() {
        if (!currRomPath.isEmpty())
            loadRom(currRomPath);
    });

    connect(ui->actionShow_FPS, &QAction::toggled, this,
            [this](bool) { updWindowTitle(); });

    connect(ui->actionAudioEnable, &QAction::toggled, this,
            [this](bool enabled) {
                if (auto *act = ui->menuAudio->actions().isEmpty()
                                    ? nullptr
                                    : ui->menuAudio->actions().last())
                    act->setVisible(enabled);
                if (audio)
                    audio->setEnabled(enabled);
            });

    if (auto *volumeSlider = findChild<QSlider *>("audioVolumeSlider")) {
        connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {
            if (audio)
                audio->setVolume(static_cast<float>(value) / 100.0f);
        });
    }
}

void WMain::stpTimer() {
    frameTimer.setTimerType(Qt::PreciseTimer);
    frameTimer.setInterval(16);

    connect(&frameTimer, &QTimer::timeout, this, [this]() {
        if (!romLoaded || paused)
            return;

        doFrame();
        updFpsCounter();

        if (ui->actionCPU->isChecked() || ui->actionPPU->isChecked())
            updDebugPanels();
    });

    frameTimer.start();
}

void WMain::loadRom(const QString &romPath) {
    try {
        clearCore();

        mapper = std::make_unique<Core::Mapper>();
        mapper->loadNES(romPath.toStdWString());

        std::filesystem::path mapperDir;
        const QStringList dirs = {
            QDir::cleanPath(QCoreApplication::applicationDirPath() +
                            "/mappers"),
            QDir::cleanPath(QCoreApplication::applicationDirPath() +
                            "/../mappers"),
            QDir::cleanPath(QDir::currentPath() + "/mappers")};

        for (const QString &dir : dirs) {
            if (QDir(dir).exists()) {
                mapperDir = dir.toStdWString();
                break;
            }
        }

        if (mapperDir.empty())
            mapper->load();
        else
            mapper->load(mapperDir);

        ppu = std::make_unique<Core::PPU>(mapper.get());
        ppu->setRegion(emuRegion);
        apu = std::make_unique<Core::APU>();
        apu->powerUp();
        apu->cyclesPerSample = (emuRegion == Region::PAL)     ? PAL_CYCLES
                               : (emuRegion == Region::DENDY) ? DENDY_CYCLES
                                                              : NTSC_CYCLES;
        audio = std::make_unique<NesAudio>();
        audio->setEnabled(ui->actionAudioEnable->isChecked());
        if (auto *slider = findChild<QSlider *>("audioVolumeSlider"))
            audio->setVolume(static_cast<float>(slider->value()) / 100.0f);
        audio->reset();

        mem =
            std::make_unique<Core::Memory>(mapper.get(), ppu.get(), apu.get());
        cpu = std::make_unique<Core::CPU>(mem.get());
        cpu->reset();
        ppuCyclesDebt = 0.0;

        const u32 warmupScanlines =
            (emuRegion == Region::PAL || emuRegion == Region::DENDY) ? 312
                                                                     : 262;
        for (u32 i = 0; i < 3 * warmupScanlines * 341; ++i)
            ppu->r.step();

        currRomPath = romPath;
        romLoaded = 1;
        paused = 0;
        joyState = 0;
        joyStateP2 = 0;

        ui->actionPause->setChecked(0);
        ui->cpuDebugText->clear();
        ui->ppuDebugText->clear();

        FpsUpdate = 0;
        currFps = 0.0;
        fpsTimer.restart();

        updWindowTitle();
    } catch (const std::exception &e) {
        clearCore();
        romLoaded = 0;
        paused = 0;

        ui->nesScreen->clear();
        ui->cpuDebugText->clear();
        ui->ppuDebugText->clear();
        updWindowTitle();

        QMessageBox::critical(this, tr("ROM load error"),
                              tr("Failed to load ROM:\n%1")
                                  .arg(QString::fromStdString(e.what())));
    }
}

void WMain::doFrame() {
    if (!mapper || !ppu || !apu || !mem || !cpu)
        return;

    mem->setJoy1(joyState);
    mem->setJoy2(joyStateP2);

    ppu->r.frameReady = 0;
    const double ppuPerCpuRatio = ppuPerCpu();
    while (!ppu->r.frameReady) {
        cpu->exec();

        u32 cycles = cpu->c.op_cycles;
        if (cycles == 0) {
            cycles = 1;
        }

        ppuCyclesDebt += static_cast<double>(cycles) * ppuPerCpuRatio;
        while (ppuCyclesDebt >= 1.0) {
            ppu->r.step();
            ppuCyclesDebt -= 1.0;
        }

        if (mapper->irqFlag)
            cpu->c.do_irq = 1;

        if (apu->getState().frameIrq || apu->getState().dmc.irqFlag)
            cpu->c.do_irq = 1;

        if (ppu->r.nmiPending()) {
            cpu->c.do_nmi = 1;
            ppu->r.clearNmi();
        }

        apu->step(cycles);
    }

    if (audio && !apu->samples.empty()) {
        audio->pushSamples(apu->samples);
        apu->samples.clear();
    }

    ui->nesScreen->setFrameBuffer(ppu->frame);
}

void WMain::applyRegion(Region region) {
    emuRegion = region;
    ppuCyclesDebt = 0.0;

    if (ui) {
        ui->actionRegion_PAL->setChecked(emuRegion == Region::PAL);
        ui->actionRegion_NTSC->setChecked(emuRegion == Region::NTSC);
        ui->actionRegion_Dendy->setChecked(emuRegion == Region::DENDY);
    }

    if (ppu)
        ppu->setRegion(emuRegion);

    if (apu)
        apu->cyclesPerSample = (emuRegion == Region::PAL)     ? PAL_CYCLES
                               : (emuRegion == Region::DENDY) ? DENDY_CYCLES
                                                              : NTSC_CYCLES;

    if (emuRegion == Region::PAL)
        frameTimer.setInterval(20);
    else if (emuRegion == Region::DENDY)
        frameTimer.setInterval(20);
    else
        frameTimer.setInterval(16);
}

double WMain::ppuPerCpu() const {
    if (emuRegion == Region::PAL)
        return 3.2;

    return 3.0;
}

void WMain::updDebugPanels() {
    if (!cpu || !ppu)
        return;

    if (ui->actionCPU->isChecked()) {
        const auto &regs = cpu->c.regs;
        const QString opName = QString::fromStdString(cpu->getLastOpName());
        const QString amName =
            QString::fromLatin1(toAddrModeString(cpu->getLastAddrMode()));
        ui->cpuDebugText->setPlainText(QString("OP: %1   AM: %2\n"
                                               "A:  0x%3\n"
                                               "X:  0x%4\n"
                                               "Y:  0x%5\n"
                                               "P:  %6\n"
                                               "SP: 0x%7\n"
                                               "PC: 0x%8\n"
                                               "Cycles (last): %9")
                                           .arg(opName, amName)
                                           .arg(regs.A, 2, 16, QChar('0'))
                                           .arg(regs.X, 2, 16, QChar('0'))
                                           .arg(regs.Y, 2, 16, QChar('0'))
                                           .arg(regs.P, 8, 2, QChar('0'))
                                           .arg(regs.SP, 2, 16, QChar('0'))
                                           .arg(regs.PC, 4, 16, QChar('0'))
                                           .arg(cpu->c.op_cycles));
    }

    if (ui->actionPPU->isChecked()) {
        const auto &state = ppu->getState();
        ui->ppuDebugText->setPlainText(
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
                .arg(state.w));

        const QImage pt0 = buildPttrn(ppu->r.getPttrnTable(0));
        const QImage pt1 = buildPttrn(ppu->r.getPttrnTable(1));

        if (auto *ppuPttrn0 =
                ui->dockPPUContents->findChild<QLabel *>("ppuPattern0Label")) {
            ppuPttrn0->setPixmap(QPixmap::fromImage(
                pt0.scaled(ppuPttrn0->size(), Qt::KeepAspectRatio,
                           Qt::FastTransformation)));
        }

        if (auto *ppuPttrn1 =
                ui->dockPPUContents->findChild<QLabel *>("ppuPattern1Label")) {
            ppuPttrn1->setPixmap(QPixmap::fromImage(
                pt1.scaled(ppuPttrn1->size(), Qt::KeepAspectRatio,
                           Qt::FastTransformation)));
        }
    }
}

void WMain::updFpsCounter() {
    ++FpsUpdate;

    const qint64 ms = fpsTimer.elapsed();
    if (ms < 1000)
        return;

    currFps = static_cast<double>(FpsUpdate) * 1000.0 / static_cast<double>(ms);
    FpsUpdate = 0;
    fpsTimer.restart();

    if (ui->actionShow_FPS->isChecked())
        updWindowTitle();
}

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

void WMain::keyPressEvent(QKeyEvent *event) {
    if (const u8 mask = joyMaskKey(event->key()); mask != 0) {
        joyState |= mask;
        event->accept();
        return;
    }

    if (const u8 mask = joyMaskKeyP2(event->key()); mask != 0) {
        joyStateP2 |= mask;
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void WMain::keyReleaseEvent(QKeyEvent *event) {
    if (const u8 mask = joyMaskKey(event->key()); mask != 0) {
        joyState &= static_cast<u8>(~mask);
        event->accept();
        return;
    }

    if (const u8 mask = joyMaskKeyP2(event->key()); mask != 0) {
        joyStateP2 &= static_cast<u8>(~mask);
        event->accept();
        return;
    }

    QMainWindow::keyReleaseEvent(event);
}
