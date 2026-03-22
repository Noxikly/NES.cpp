#include <algorithm>
#include <filesystem>

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QStringList>
#include <QTimer>
#include <QUrl>

#include "gui/modules/audio.h"
#include "gui/state.h"
#include "gui/update.h"
#include "gui/w_main.h"
#include "gui/w_settings.h"
#include "ui_main.h"

#if defined(DEBUG)
#include "gui/w_logs.h"
#endif

namespace {
auto toFsPath(const QString &path) -> std::filesystem::path {
#if defined(_WIN32)
    return std::filesystem::path(path.toStdWString());
#else
    const QByteArray encoded = QFile::encodeName(path);
    return std::filesystem::path(encoded.constData());
#endif
}

auto isNesFile(const QString &path) -> bool {
    return path.endsWith(".nes", Qt::CaseInsensitive);
}

auto firstNesPath(const QMimeData *mime) -> QString {
    if (!mime || !mime->hasUrls())
        return {};

    for (const QUrl &url : mime->urls()) {
        if (!url.isLocalFile())
            continue;

        const QString path = url.toLocalFile();
        if (isNesFile(path))
            return path;
    }

    return {};
}

void setDirMask(u8 &state, u8 mask) {
    if (mask == 0x80)
        state &= static_cast<u8>(~0x40);
    else if (mask == 0x40)
        state &= static_cast<u8>(~0x80);
    else if (mask == 0x20)
        state &= static_cast<u8>(~0x10);
    else if (mask == 0x10)
        state &= static_cast<u8>(~0x20);

    state |= mask;
}

class UpdateCriticalGuard {
public:
    explicit UpdateCriticalGuard(WUpdate *updater) : updater(updater) {
        if (this->updater)
            wasRunning = this->updater->suspendForCriticalSection();
    }

    ~UpdateCriticalGuard() {
        if (updater)
            updater->resumeAfterCriticalSection(wasRunning);
    }

private:
    WUpdate *updater{nullptr};
    bool wasRunning{false};
};

} /* namespace */

WMain::WMain(const QString &romPath, QWidget *p)
    : QMainWindow(p), ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);
    updater = std::make_unique<WUpdate>(this);

#if !defined(DEBUG)
    if (ui->actionDebug) {
        ui->actionDebug->setVisible(false);
        ui->actionDebug->setEnabled(false);
    }
#endif

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
    if (firstNesPath(event->mimeData()).isEmpty()) {
        QMainWindow::dragEnterEvent(event);
        return;
    }

    event->acceptProposedAction();
}

void WMain::dropEvent(QDropEvent *event) {
    const QString path = firstNesPath(event->mimeData());
    if (path.isEmpty()) {
        QMainWindow::dropEvent(event);
        return;
    }

    event->acceptProposedAction();
    QTimer::singleShot(0, this, [this, path]() { loadRom(path); });
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

    if (m1 != 0)
        setDirMask(joyState, m1);
    if (m2 != 0)
        setDirMask(joyStateP2, m2);

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

int WMain::bindingFor(const QString &bindId) const {
    return bindings.value(bindId, Qt::Key_unknown);
}

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
    volumePercent = std::clamp(volumePercent, 0, 100);

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

#if defined(DEBUG)
    connect(ui->actionDebug, &QAction::triggered, this, &WMain::openLogsWindow);
#endif

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

    const auto bindRegion = [this](QAction *a, Core::PPU::Region r) {
        connect(a, &QAction::toggled, this, [this, r](bool checked) {
            if (checked)
                applyRegion(r);
        });
    };

    bindRegion(ui->actionRegionNTSC, Core::PPU::Region::NTSC);
    bindRegion(ui->actionRegionPAL, Core::PPU::Region::PAL);
    bindRegion(ui->actionRegionDendy, Core::PPU::Region::DENDY);

    connect(ui->actionOpen_ROM, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Open NES ROM"), QString(),
            tr("NES ROM (*.nes);;All files (*.*)"));

        if (!path.isEmpty())
            loadRom(path);
    });

    connect(ui->actionClose_Game, &QAction::triggered, this, [this]() {
        UpdateCriticalGuard guard(updater.get());

        clearCore();
        currRomPath.clear();

#if defined(DEBUG)
        resetLogsUi();
#endif
    });

    connect(ui->actionReload_ROM, &QAction::triggered, this, [this]() {
        if (!currRomPath.isEmpty())
            loadRom(currRomPath);
    });

    connect(ui->actionPause, &QAction::toggled, this, [this](bool checked) {
        if (updater && checked)
            updater->suspendForCriticalSection();

        paused = checked;

#if defined(DEBUG)
        if (logsWindow)
            logsWindow->setStopped(checked);
#endif

        if (updater && !checked)
            updater->resumeAfterCriticalSection(true);

        if (updater)
            updater->updWindowTitle();
    });

    connect(ui->actionReset, &QAction::triggered, this, [this]() {
        UpdateCriticalGuard guard(updater.get());

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

        UpdateCriticalGuard guard(updater.get());

        const QString path = QFileDialog::getSaveFileName(
            this, tr("Save State"), QString(),
            tr("NES State (*.nst);;All files (*.*)"));

        if (path.isEmpty())
            return;

        const u8 regionCode =
            static_cast<u8>(emuRegion == Core::PPU::Region::PAL     ? 1
                            : emuRegion == Core::PPU::Region::DENDY ? 2
                                                                    : 0);

        if (!saveBinState(path, cpu->getState(), ppu->getState(),
                          apu->getState(), mem->getState(), mapper->getState(),
                          regionCode)) {
            QMessageBox::warning(this, tr("Save State"),
                                 tr("Failed to save state to file."));
        }
    });

    connect(ui->actionLoad, &QAction::triggered, this, [this]() {
        if (!romLoaded || !cpu || !ppu || !apu || !mem || !mapper)
            return;

        UpdateCriticalGuard guard(updater.get());

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

        try {
            if (!currRomPath.isEmpty())
                mapper->loadNES(toFsPath(currRomPath));

            if (mapper->mapperNumber != mapperState.mapperNumber) {
                QMessageBox::warning(
                    this, tr("Load State"),
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

#if defined(DEBUG)
                updater->updDebugPanels();
#endif

                updater->updWindowTitle();
            }

            paused = true;
            if (ui && ui->actionPause)
                ui->actionPause->setChecked(true);

#if defined(DEBUG)
            if (logsWindow)
                logsWindow->setStopped(true);
#endif

        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Load State"),
                                 tr("Failed to apply loaded state:\n%1")
                                     .arg(QString::fromLocal8Bit(e.what())));
        }
    });

    ui->actionRegionNTSC->setChecked(true);
}

#if defined(DEBUG)
void WMain::openLogsWindow() {
    if (!logsWindow) {
        logsWindow = std::make_unique<WLogs>(this);

        connect(logsWindow.get(), &WLogs::stopToggled, this,
                [this](bool stopped) {
                    paused = stopped;
                    if (ui && ui->actionPause)
                        ui->actionPause->setChecked(stopped);
                    if (updater)
                        updater->updWindowTitle();
                });

        connect(logsWindow.get(), &WLogs::stepForwardRequested, this, [this]() {
            if (!paused || !romLoaded || !updater)
                return;

            UpdateCriticalGuard guard(updater.get());

            updater->runCpuInstruction();
            updater->presentAudioAndVideo();
            updater->updDebugPanels();
            updater->updWindowTitle();
        });

        connect(logsWindow.get(), &WLogs::stepBackRequested, this, []() {});
    }

    logsWindow->show();
    logsWindow->raise();
    logsWindow->activateWindow();

    if (updater) {
        updater->syncDebugFlagsFromLogWindow();
        QTimer::singleShot(0, this, [this]() {
            if (updater && logsWindow && logsWindow->isVisible())
                updater->updDebugPanels();
        });
    }
}

void WMain::resetLogsUi() {
    if (!logsWindow)
        return;

    logsWindow->setCpuDebugText(QString());
    logsWindow->setPpuDebugText(QString());
    logsWindow->setStopped(false);
}
#endif

void WMain::clearCore() {
    if (ui && ui->frameView)
        ui->frameView->clear();

    cpu.reset();
    apu.reset();
    mem.reset();
    ppu.reset();

    joyState = 0;
    joyStateP2 = 0;
    ppuPhaseAcc = 0;
    romLoaded = false;
    paused = false;

#if defined(DEBUG)
    resetLogsUi();
#endif

    if (updater)
        updater->updWindowTitle();
}

void WMain::loadRom(const QString &romPath) {
    if (romLoaded) {
        const QString exePath = QCoreApplication::applicationFilePath();
        const bool relaunched =
            QProcess::startDetached(exePath, QStringList{romPath});

        if (relaunched) {
            QTimer::singleShot(0, this, &QWidget::close);
            return;
        }
    }

    const bool timerWasActive = frameTimer.isActive();
    if (timerWasActive)
        frameTimer.stop();

    UpdateCriticalGuard guard(updater.get());

    try {
        clearCore();

        if (!mapper)
            mapper = std::make_unique<Core::Mapper>();

        mapper->loadNES(toFsPath(romPath));

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

        apu->cyclesPerSample =
            (emuRegion == Core::PPU::Region::PAL)     ? Core::APU::PAL_CYCLES
            : (emuRegion == Core::PPU::Region::DENDY) ? Core::APU::DENDY_CYCLES
                                                      : Core::APU::NTSC_CYCLES;

        if (!audio)
            audio = std::make_unique<NesAudio>();

        audio->setEnabled(audioEnabled);
        audio->setVolume(static_cast<f32>(audioVolume) / 100.0f);
        audio->reset();

        mem =
            std::make_unique<Core::Memory>(mapper.get(), ppu.get(), apu.get());
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

#if defined(DEBUG)
        resetLogsUi();
#endif

        fpsUpdate = 0;
        currFps = 0.0;
        fpsTimer.restart();
        if (updater)
            updater->updWindowTitle();

        if (timerWasActive && !frameTimer.isActive())
            frameTimer.start();
    } catch (const std::exception &e) {
        clearCore();
        QMessageBox::critical(this, tr("Open ROM"),
                              tr("Failed to load ROM:\n%1").arg(e.what()));

        if (timerWasActive && !frameTimer.isActive())
            frameTimer.start();
    }
}

void WMain::applyRegion(Core::PPU::Region region) {
    UpdateCriticalGuard guard(updater.get());

    emuRegion = region;

    if (ppu)
        ppu->setRegion(region);

    if (apu) {
        apu->cyclesPerSample =
            (region == Core::PPU::Region::PAL)     ? Core::APU::PAL_CYCLES
            : (region == Core::PPU::Region::DENDY) ? Core::APU::DENDY_CYCLES
                                                   : Core::APU::NTSC_CYCLES;
    }

    if (updater)
        updater->updFrameCap();

    if (updater)
        updater->updWindowTitle();
}
