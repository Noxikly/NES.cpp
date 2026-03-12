#pragma once

#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QString>
#include <QTimer>

#include <memory>

#include "core/apu.hpp"
#include "core/common.hpp"
#include "core/cpu.hpp"
#include "core/mem.hpp"
#include "core/ppu.hpp"

class QKeyEvent;
class QLabel;
class NesAudio;

namespace Ui {
class MainWindow;
}

class WMain : public QMainWindow {
  public:
    explicit WMain(const QString &romPath = QString(),
                   QWidget *parent = nullptr);
    ~WMain() override;

  protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

  private:
    static u8 joyMaskKey(u32 key);
    static u8 joyMaskKeyP2(u32 key);

    void stpDockWidgets();
    void stpMenuActions();
    void stpTimer();
    void clearCore();

    void loadRom(const QString &romPath);
    void doFrame();
    void updDebugPanels();
    void updFpsCounter();
    void updWindowTitle();
    void applyRegion(Region region);
    double ppuPerCpu() const;

  private:
    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<Core::Mapper> mapper;
    std::unique_ptr<Core::PPU> ppu;
    std::unique_ptr<Core::APU> apu;
    std::unique_ptr<Core::Memory> mem;
    std::unique_ptr<Core::CPU> cpu;
    std::unique_ptr<NesAudio> audio;

    QTimer frameTimer;
    QElapsedTimer fpsTimer;

    qsizetype FpsUpdate{0};
    double currFps{0.0};

    QString currRomPath;
    u8 joyState{0};
    u8 joyStateP2{0};

    Region emuRegion{Region::NTSC};
    double ppuCyclesDebt{0.0};

    bool romLoaded{false};
    bool paused{false};
};
