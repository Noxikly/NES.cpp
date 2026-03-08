#pragma once

#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>
#include <QHBoxLayout>

#include <memory>
#include "core/common.hpp"

class CPU;
class APU;
class Mapper;
class Memory;
class PPU;
class QKeyEvent;
class QLabel;
class NesAudio;

namespace Ui {
    class MainWindow;
}

class WMain : public QMainWindow {
public:
    explicit WMain(const QString& romPath = QString(), QWidget* parent = nullptr);
    ~WMain() override;

    enum class Region : u8 {
        NTSC = 0,
        PAL = 1,
    };

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    static u8 joyMaskKey(u32 key);

    void stpDockWidgets();
    void stpMenuActions();
    void stpTimer();
    void clearCore();

    void loadRom(const QString& romPath);
    void doFrame();
    void updDebugPanels();
    void updFpsCounter();
    void updWindowTitle();
    void applyRegion(Region region);
    double ppuPerCpu() const;

private:
    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<Mapper> mapper;
    std::unique_ptr<PPU> ppu;
    std::unique_ptr<APU> apu;
    std::unique_ptr<Memory> mem;
    std::unique_ptr<CPU> cpu;
    std::unique_ptr<NesAudio> audio;

    QTimer frameTimer;
    QElapsedTimer fpsTimer;

    qsizetype FpsUpdate{0};
    double currFps{0.0};

    QString currRomPath;
    u8 joyState{0};

    Region emuRegion{Region::NTSC};
    double ppuCyclesDebt{0.0};

    bool romLoaded{false};
    bool paused{false};
};
