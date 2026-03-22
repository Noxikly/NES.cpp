#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QMainWindow>
#include <QString>
#include <QTimer>

#include "common/types.h"

#include "core/apu.h"
#include "core/cpu.h"
#include "core/mapper.h"
#include "core/mem.h"
#include "core/ppu.h"

class QDragEnterEvent;
class QDropEvent;
class QKeyEvent;
class WUpdate;
class WSettings;

#if defined(DEBUG)
class WLogs;
#endif

class NesAudio;

namespace Ui {
class MainWindow;
}

class WMain : public QMainWindow {
    friend class WUpdate;

public:
    explicit WMain(const QString &romPath = QString(), QWidget *p = nullptr);
    ~WMain() override;

    int bindingFor(const QString &bindId) const;
    void setBinding(const QString &bindId, int key);

    bool isAudioEnabled() const;
    int audioVolumePercent() const;
    void setAudioEnabled(bool enabled);
    void setAudioVolume(int volumePercent);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    static u8 joyMaskFromMap(const QHash<int, u8> &map, int key);

    void stpMenuActions();
#if defined(DEBUG)
    void openLogsWindow();
    void resetLogsUi();
#endif
    void clearCore();
    void loadRom(const QString &romPath);
    void applyRegion(Core::PPU::Region region);
    void syncJoypad();
    void resetDefaultBindings();
    void rebuildKeyMaps();

private:
    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<Core::Mapper> mapper;
    std::unique_ptr<Core::PPU> ppu;
    std::unique_ptr<Core::APU> apu;
    std::unique_ptr<Core::Memory> mem;
    std::unique_ptr<Core::CPU> cpu;
    std::unique_ptr<NesAudio> audio;
    std::unique_ptr<WUpdate> updater;
    std::unique_ptr<WSettings> settingsWindow;
#if defined(DEBUG)
    std::unique_ptr<WLogs> logsWindow;
#endif

    QTimer frameTimer;
    QElapsedTimer fpsTimer;

    qsizetype fpsUpdate{0};
    f64 currFps{0.0};
    u32 ppuPhaseAcc{0};

    QString currRomPath;
    Core::PPU::Region emuRegion{Core::PPU::Region::NTSC};

    u8 joyState{0};
    u8 joyStateP2{0};

    QHash<QString, int> bindings;
    QHash<int, u8> p1KeyMap;
    QHash<int, u8> p2KeyMap;

    bool audioEnabled{true};
    int audioVolume{100};

    bool romLoaded{false};
    bool paused{false};
};
