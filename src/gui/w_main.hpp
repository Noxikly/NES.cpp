#pragma once

#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>
#include <QHBoxLayout>

#include <memory>

#include "core/constants.hpp"

class Cpu;
class Mapper;
class Memory;
class Ppu;
class QKeyEvent;
class QLabel;

namespace Ui {
    class MainWindow;
}

class WMain : public QMainWindow {
public:
    explicit WMain(const QString& romPath = QString(), QWidget* parent = nullptr);
    ~WMain() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    static u8 joyMaskForKey(u32 key);

    void stpDockWidgets();
    void stpMenuActions();
    void stpTimer();
    void clearCore();

    void loadRom(const QString& romPath);
    void doFrame();
    void updDebugPanels();
    void updFpsCounter();
    void updWindowTitle();

private:
    Ui::MainWindow* ui{nullptr};

    std::unique_ptr<Mapper> mapper;
    std::unique_ptr<Ppu> ppu;
    std::unique_ptr<Memory> mem;
    std::unique_ptr<Cpu> cpu;

    QTimer frameTimer;
    QElapsedTimer fpsTimer;
    QHBoxLayout* pttrnRow{nullptr};
    QLabel* ppuPttrn0{nullptr};
    QLabel* ppuPttrn1{nullptr};

    qsizetype FpsUpdate{0};
    double currFps{0.0};

    QString currRomPath;
    u8 joyState{0};

    bool romLoaded{false};
    bool paused{false};
};
