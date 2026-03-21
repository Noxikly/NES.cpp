#include <filesystem>

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QStringList>
#include <QUrl>

#include "gui/modules/audio.h"
#include "gui/state.h"
#include "gui/update.h"
#include "gui/w_logs.h"
#include "gui/w_main.h"
#include "gui/w_settings.h"
#include "ui_main.h"

WMain::WMain(const QString &romPath, QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);
    updater = std::make_unique<WUpdate>(this);

    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setFocus(Qt::OtherFocusReason);

    if (ui->frameView)
        ui->frameView->setFocusPolicy(Qt::NoFocus);

    resetDefaultBindings();
    rebuildKeyMaps();

    stpMenuActions();
    applyRegion(Core::PPU::Region::NTSC);
    updater->stpTimer();

    fpsTimer.start();
    updater->updWindowTitle();

    if (!romPath.isEmpty())
        loadRom(romPath);
}

WMain::~WMain() = default;

void WMain::dragEnterEvent(QDragEnterEvent *event) {
    const QMimeData *mime = event->mimeData();
    if (!mime || !mime->hasUrls()) {
        QMainWindow::dragEnterEvent(event);
        return;
    }

    for (const QUrl &url : mime->urls()) {
        if (!url.isLocalFile())
            continue;

        const QString localPath = url.toLocalFile();
        if (localPath.endsWith(".nes", Qt::CaseInsensitive)) {
            event->acceptProposedAction();
            return;
        }
    }

    QMainWindow::dragEnterEvent(event);
}

void WMain::dropEvent(QDropEvent *event) {
    const QMimeData *mime = event->mimeData();
    if (!mime || !mime->hasUrls()) {
        QMainWindow::dropEvent(event);
        return;
    }

    for (const QUrl &url : mime->urls()) {
        if (!url.isLocalFile())
            continue;

        const QString localPath = url.toLocalFile();
        if (!localPath.endsWith(".nes", Qt::CaseInsensitive))
            continue;

        loadRom(localPath);
        event->acceptProposedAction();
        return;
    }

    QMainWindow::dropEvent(event);
}

void WMain::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    const u8 m1 = joyMaskFromMap(p1KeyMap, event->key());
    const u8 m2 = joyMaskFromMap(p2KeyMap, event->key());

    if (m1 == 0 && m2 == 0) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    const auto applyNoOpposites = [](u8 &state, u8 mask) {
        if (mask == 0x80)
            state &= static_cast<u8>(~0x40);
        else if (mask == 0x40)
            state &= static_cast<u8>(~0x80);
        else if (mask == 0x20)
            state &= static_cast<u8>(~0x10);
        else if (mask == 0x10)
            state &= static_cast<u8>(~0x20);

        state |= mask;
    };

    if (m1 != 0)
        applyNoOpposites(joyState, m1);
    if (m2 != 0)
        applyNoOpposites(joyStateP2, m2);

    syncJoypad();
    event->accept();
}

void WMain::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    const u8 m1 = joyMaskFromMap(p1KeyMap, event->key());
    const u8 m2 = joyMaskFromMap(p2KeyMap, event->key());

    if (m1 == 0 && m2 == 0) {
        QMainWindow::keyReleaseEvent(event);
        return;
    }

    joyState &= static_cast<u8>(~m1);
    joyStateP2 &= static_cast<u8>(~m2);
    syncJoypad();
    event->accept();
}

u8 WMain::joyMaskFromMap(const QHash<int, u8> &map, int key) {
    const auto it = map.constFind(key);
    return (it == map.cend()) ? 0 : it.value();
}

void WMain::resetDefaultBindings() {
    bindings["P1_UP"] = Qt::Key_W;
    bindings["P1_DOWN"] = Qt::Key_S;
    bindings["P1_LEFT"] = Qt::Key_A;
    bindings["P1_RIGHT"] = Qt::Key_D;
    bindings["P1_SELECT"] = Qt::Key_Shift;
    bindings["P1_START"] = Qt::Key_Return;
    bindings["P1_B"] = Qt::Key_Z;
    bindings["P1_A"] = Qt::Key_X;

    bindings["P2_UP"] = Qt::Key_Up;
    bindings["P2_DOWN"] = Qt::Key_Down;
    bindings["P2_LEFT"] = Qt::Key_Left;
    bindings["P2_RIGHT"] = Qt::Key_Right;
    bindings["P2_SELECT"] = Qt::Key_Period;
    bindings["P2_START"] = Qt::Key_Slash;
    bindings["P2_B"] = Qt::Key_Semicolon;
    bindings["P2_A"] = Qt::Key_Apostrophe;
}

void WMain::rebuildKeyMaps() {
    p1KeyMap.clear();
    p2KeyMap.clear();

    p1KeyMap[bindings.value("P1_RIGHT")] = 0x80;
    p1KeyMap[bindings.value("P1_LEFT")] = 0x40;
    p1KeyMap[bindings.value("P1_DOWN")] = 0x20;
    p1KeyMap[bindings.value("P1_UP")] = 0x10;
    p1KeyMap[bindings.value("P1_START")] = 0x08;
    p1KeyMap[bindings.value("P1_SELECT")] = 0x04;
    p1KeyMap[bindings.value("P1_A")] = 0x01;
    p1KeyMap[bindings.value("P1_B")] = 0x02;

    p2KeyMap[bindings.value("P2_RIGHT")] = 0x80;
    p2KeyMap[bindings.value("P2_LEFT")] = 0x40;
    p2KeyMap[bindings.value("P2_DOWN")] = 0x20;
    p2KeyMap[bindings.value("P2_UP")] = 0x10;
    p2KeyMap[bindings.value("P2_START")] = 0x08;
    p2KeyMap[bindings.value("P2_SELECT")] = 0x04;
    p2KeyMap[bindings.value("P2_A")] = 0x01;
    p2KeyMap[bindings.value("P2_B")] = 0x02;
}

int WMain::bindingFor(const QString &bindId) const { return bindings.value(bindId, Qt::Key_unknown); }

void WMain::setBinding(const QString &bindId, int key) {
    bindings[bindId] = key;
    rebuildKeyMaps();
}

bool WMain::isAudioEnabled() const { return audioEnabled; }

int WMain::audioVolumePercent() const { return audioVolume; }

void WMain::setAudioEnabled(bool enabled) {
    audioEnabled = enabled;

    if (audio)
        audio->setEnabled(enabled);
}

void WMain::setAudioVolume(int volumePercent) {
    if (volumePercent < 0)
        volumePercent = 0;
    if (volumePercent > 100)
        volumePercent = 100;

    audioVolume = volumePercent;

    if (audio)
        audio->setVolume(static_cast<f32>(audioVolume) / 100.0f);
}

void WMain::syncJoypad() {
    if (!mem)
        return;

    mem->setJoy1(joyState);
    mem->setJoy2(joyStateP2);
}

void WMain::stpMenuActions() {
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionDebug, &QAction::triggered, this, &WMain::openLogsWindow);

    connect(ui->actionSettings, &QAction::triggered, this, [this]() {
        if (!settingsWindow)
            settingsWindow = std::make_unique<WSettings>(this, this);

        settingsWindow->show();
        settingsWindow->raise();
        settingsWindow->activateWindow();
    });

    auto *region = new QActionGroup(this);
    region->setExclusive(true);
    region->addAction(ui->actionRegionNTSC);
    region->addAction(ui->actionRegionPAL);
    region->addAction(ui->actionRegionDendy);

    connect(ui->actionRegionNTSC, &QAction::toggled, this, [this](bool checked) {
        if (checked)
            applyRegion(Core::PPU::Region::NTSC);
    });

    connect(ui->actionRegionPAL, &QAction::toggled, this, [this](bool checked) {
        if (checked)
            applyRegion(Core::PPU::Region::PAL);
    });

    connect(ui->actionRegionDendy, &QAction::toggled, this, [this](bool checked) {
        if (checked)
            applyRegion(Core::PPU::Region::DENDY);
    });

    connect(ui->actionOpen_ROM, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Open NES ROM"), QString(), tr("NES ROM (*.nes);;All files (*.*)"));

        if (!path.isEmpty())
            loadRom(path);
    });

    connect(ui->actionClose_Game, &QAction::triggered, this, [this]() {
        clearCore();
        currRomPath.clear();

        if (logsWindow) {
            logsWindow->setCpuDebugText(QString());
            logsWindow->setPpuDebugText(QString());
            logsWindow->setStopped(false);
        }
    });

    connect(ui->actionReload_ROM, &QAction::triggered, this, [this]() {
        if (!currRomPath.isEmpty())
            loadRom(currRomPath);
    });

    connect(ui->actionPause, &QAction::toggled, this, [this](bool checked) {
        paused = checked;

        if (logsWindow)
            logsWindow->setStopped(checked);

        if (updater)
            updater->updWindowTitle();
    });

    connect(ui->actionReset, &QAction::triggered, this, [this]() {
        if (cpu)
            cpu->reset();
        if (apu)
            apu->reset();
        joyState = 0;
        joyStateP2 = 0;
        syncJoypad();
    });

    connect(ui->actionSave, &QAction::triggered, this, [this]() {
        if (!romLoaded || !cpu || !ppu || !apu || !mem || !mapper)
            return;

        const QString path = QFileDialog::getSaveFileName(
            this, tr("Save State"), QString(), tr("NES State (*.nst);;All files (*.*)"));

        if (path.isEmpty())
            return;

        const u8 regionCode = static_cast<u8>(
            emuRegion == Core::PPU::Region::PAL
                ? 1
                : emuRegion == Core::PPU::Region::DENDY ? 2 : 0);

        if (!saveBinState(path, cpu->getState(), ppu->getState(), apu->getState(),
                          mem->getState(), mapper->getState(), regionCode)) {
            QMessageBox::warning(this, tr("Save State"),
                                 tr("Failed to save state to file."));
        }
    });

    connect(ui->actionLoad, &QAction::triggered, this, [this]() {
        if (!romLoaded || !cpu || !ppu || !apu || !mem || !mapper)
            return;

        const QString path = QFileDialog::getOpenFileName(
            this, tr("Load State"), QString(), tr("NES State (*.nst);;All files (*.*)"));

        if (path.isEmpty())
            return;

        Core::CPU::State cpuState{};
        Core::PPU::State ppuState{};
        Core::APU::State apuState{};
        Core::Memory::State memState{};
        Core::Mapper::State mapperState{};
        u8 regionState = 0;

        if (!loadBinState(path, cpuState, ppuState, apuState, memState, mapperState,
                          regionState)) {
            QMessageBox::warning(this, tr("Load State"),
                                 tr("Failed to load state from file."));
            return;
        }

        if (!currRomPath.isEmpty())
            mapper->loadNES(currRomPath.toStdWString());

        if (mapper->mapperNumber != mapperState.mapperNumber) {
            QMessageBox::warning(this, tr("Load State"),
                                 tr("Mapper number mismatch. State file was created "
                                    "with a different mapper."));
            return;
        }

        mapper->loadState(mapperState);
        cpu->loadState(cpuState);
        ppu->loadState(ppuState);
        apu->loadState(apuState);
        mem->loadState(memState);

        switch (regionState) {
        case 1:
            applyRegion(Core::PPU::Region::PAL);
            break;
        case 2:
            applyRegion(Core::PPU::Region::DENDY);
            break;
        default:
            applyRegion(Core::PPU::Region::NTSC);
            break;
        }

        if (audio)
            audio->reset();

        if (updater) {
            updater->doFrame();
            updater->updDebugPanels();
            updater->updWindowTitle();
        }

        paused = true;
        if (ui && ui->actionPause)
            ui->actionPause->setChecked(true);
        if (logsWindow)
            logsWindow->setStopped(true);
    });

    ui->actionRegionNTSC->setChecked(true);
}

void WMain::openLogsWindow() {
    if (!logsWindow) {
        logsWindow = std::make_unique<WLogs>(this);

        connect(logsWindow.get(), &WLogs::stopToggled, this, [this](bool stopped) {
            paused = stopped;
            if (ui && ui->actionPause)
                ui->actionPause->setChecked(stopped);
            if (updater)
                updater->updWindowTitle();
        });

        connect(logsWindow.get(), &WLogs::stepForwardRequested, this, [this]() {
            if (!paused || !romLoaded || !updater)
                return;

            updater->runCpuInstruction();
            updater->presentAudioAndVideo();
            updater->updDebugPanels();
            updater->updWindowTitle();
        });

        connect(logsWindow.get(), &WLogs::stepBackRequested, this, [this]() {
            if (logsWindow) {
                logsWindow->appendLine(
                    QStringLiteral("[INFO] Step back is not implemented in the src runtime yet"));
            }
        });
    }

    logsWindow->show();
    logsWindow->raise();
    logsWindow->activateWindow();

    if (updater) {
        updater->syncDebugFlagsFromLogWindow();
        updater->updDebugPanels();
    }
}

void WMain::clearCore() {
    cpu.reset();
    apu.reset();
    audio.reset();
    mem.reset();
    ppu.reset();
    mapper.reset();

    joyState = 0;
    joyStateP2 = 0;
    ppuPhaseAcc = 0;
    romLoaded = false;
    paused = false;

    if (ui && ui->frameView)
        ui->frameView->clear();

    if (logsWindow) {
        logsWindow->setCpuDebugText(QString());
        logsWindow->setPpuDebugText(QString());
        logsWindow->setStopped(false);
    }

    if (updater)
        updater->updWindowTitle();
}

void WMain::loadRom(const QString &romPath) {
    try {
        clearCore();

        mapper = std::make_unique<Core::Mapper>();
        mapper->loadNES(romPath.toStdWString());

        std::filesystem::path mapperDir;
        const QStringList dirs = {
            QDir::cleanPath(QCoreApplication::applicationDirPath() + "/mappers"),
            QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../mappers"),
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

        apu->cyclesPerSample = (emuRegion == Core::PPU::Region::PAL)
                                   ? Core::APU::PAL_CYCLES
                               : (emuRegion == Core::PPU::Region::DENDY)
                                   ? Core::APU::DENDY_CYCLES
                                   : Core::APU::NTSC_CYCLES;

        audio = std::make_unique<NesAudio>();
        audio->setEnabled(audioEnabled);
        audio->setVolume(static_cast<f32>(audioVolume) / 100.0f);
        audio->reset();

        mem = std::make_unique<Core::Memory>(mapper.get(), ppu.get(), apu.get());
        cpu = std::make_unique<Core::CPU>(mem.get());
        cpu->reset();

        currRomPath = romPath;
        romLoaded = true;
        paused = false;

        joyState = 0;
        joyStateP2 = 0;
        syncJoypad();

        if (ui->actionPause)
            ui->actionPause->setChecked(false);

        if (ui->frameView)
            ui->frameView->setFrameBuffer(ppu->frame);

        if (logsWindow) {
            logsWindow->setCpuDebugText(QString());
            logsWindow->setPpuDebugText(QString());
            logsWindow->setStopped(false);
        }

        fpsUpdate = 0;
        currFps = 0.0;
        fpsTimer.restart();
        if (updater)
            updater->updWindowTitle();
    } catch (const std::exception &e) {
        clearCore();
        QMessageBox::critical(this, tr("Open ROM"), tr("Failed to load ROM:\n%1").arg(e.what()));
    }
}

void WMain::applyRegion(Core::PPU::Region region) {
    emuRegion = region;

    if (ppu)
        ppu->setRegion(region);

    if (apu) {
        apu->cyclesPerSample = (region == Core::PPU::Region::PAL)
                                   ? Core::APU::PAL_CYCLES
                               : (region == Core::PPU::Region::DENDY)
                                   ? Core::APU::DENDY_CYCLES
                                   : Core::APU::NTSC_CYCLES;
    }

    if (updater)
        updater->updFrameCap();

    if (updater)
        updater->updWindowTitle();
}
