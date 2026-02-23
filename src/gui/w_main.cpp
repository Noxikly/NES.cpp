#include "w_main.hpp"
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
#include <QHBoxLayout>

#include <filesystem>

#include "core/cpu.hpp"
#include "core/mapper.hpp"
#include "core/memory.hpp"
#include "core/ppu.hpp"


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
      ui(new Ui::MainWindow()) 
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
WMain::~WMain() { delete ui; }

void WMain::clearCore() {
    cpu.reset();
    mem.reset();
    ppu.reset();
    mapper.reset();

    joyState = 0; 
    romLoaded = false; 
    paused = false;
}


/* Контроллер NES */
u8 WMain::joyMaskForKey(u32 key) {
    switch (key) {
        case Qt::Key_D: return 0x80;      /* Right  */
        case Qt::Key_A: return 0x40;      /* Left   */
        case Qt::Key_S: return 0x20;      /* Down   */
        case Qt::Key_W: return 0x10;      /* Up     */
        case Qt::Key_Return: return 0x08; /* Start  */
        case Qt::Key_Shift: return 0x04;  /* Select */
        case Qt::Key_X: return 0x02;      /* B      */
        case Qt::Key_Z: return 0x01;      /* A      */
        default: return 0;
    }
}


/* GUI */
void WMain::stpDockWidgets() {
    removeDockWidget(ui->dockCPU);
    removeDockWidget(ui->dockPPU);
    addDockWidget(Qt::RightDockWidgetArea, ui->dockCPU);
    addDockWidget(Qt::RightDockWidgetArea, ui->dockPPU);
    splitDockWidget(ui->dockCPU, ui->dockPPU, Qt::Vertical);

    ui->dockCPU->hide();
    ui->dockPPU->hide();

    ui->actionCPU->setChecked(false);
    ui->actionPPU->setChecked(false);

    connect(ui->actionCPU, &QAction::toggled, ui->dockCPU, &QDockWidget::setVisible);
    connect(ui->actionPPU, &QAction::toggled, ui->dockPPU, &QDockWidget::setVisible);

    connect(ui->dockCPU, &QDockWidget::visibilityChanged, ui->actionCPU, &QAction::setChecked);
    connect(ui->dockPPU, &QDockWidget::visibilityChanged, ui->actionPPU, &QAction::setChecked);

    auto Preview = [](QLabel* label) {
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(150, 150);
        label->setStyleSheet("background-color: #111; border: 1px solid #444;");
    };

    auto* patternRow = new QHBoxLayout();
    ppuPttrn0 = new QLabel(ui->dockPPUContents);
    ppuPttrn1 = new QLabel(ui->dockPPUContents);
    Preview(ppuPttrn0);
    Preview(ppuPttrn1);

    patternRow->addWidget(ppuPttrn0);
    patternRow->addWidget(ppuPttrn1);
    ui->verticalLayoutPPU->addLayout(patternRow);
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

        if (ui->actionCPU->isChecked() || ui->actionPPU->isChecked())
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

        mem = std::make_unique<Memory>(mapper.get(), ppu.get());
        cpu = std::make_unique<Cpu>(mem.get());
        cpu->reset();

        currRomPath = romPath;
        romLoaded = true;
        paused = false;
        joyState = 0;

        ui->actionPause->setChecked(false);
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
    if (!mapper || !ppu || !mem || !cpu) return;

    mem->setJoy1(joyState);

    bool frameCompleted = false;
    while (!frameCompleted) {
        cpu->exec();

        const u32 ppuSteps = cpu->getCycles() * 3;
        for (u32 i=0; i<ppuSteps; ++i) {
            ppu->step();

            if (ppu->frameReady) {
                ppu->frameReady = false;
                frameCompleted = true;
                break;
            }
        }

        if (ppu->nmiPending()) {
            cpu->do_nmi = true;
            ppu->clearNmi();
        }

        if (mapper->irqFlag)
            cpu->do_irq = true;
    }

    ui->nesScreen->setFrameBuffer(ppu->getFrame());
}

void WMain::updDebugPanels() {
    if (!cpu || !ppu) return;

    if (ui->actionCPU->isChecked()) {
        ui->cpuDebugText->setPlainText(QString(
            "OP: %1\n"
            "A:  0x%2\n"
            "X:  0x%3\n"
            "Y:  0x%4\n"
            "\n"
            "    NV1BDIZC\n"
            "P:  %5\n"
            "\n"
            "SP: 0x%6\n"
            "PC: 0x%7\n"
            "\n"
            "Cycles (last):  %8\n"
            "Cycles (total): %9"
        )
        .arg(cpu->getOp())
        .arg(cpu->regs.A, 2, 16, QChar('0'))
        .arg(cpu->regs.X, 2, 16, QChar('0'))
        .arg(cpu->regs.Y, 2, 16, QChar('0'))
        .arg(cpu->regs.P, 8, 2, QChar('0'))
        .arg(cpu->regs.SP, 2, 16, QChar('0'))
        .arg(cpu->regs.PC, 4, 16, QChar('0'))
        .arg(cpu->getCycles())
        .arg(cpu->getTotalCycles())
        );
    }

    if (ui->actionPPU->isChecked()) {
        ui->ppuDebugText->setPlainText(QString(
            "PPUCTRL:   0x%1\n"
            "PPUMASK:   0x%2\n"
            "PPUSTATUS: 0x%3\n"
            "OAMADDR:   0x%4\n"
            "\n"
            "V:     0x%5\n"
            "T:     0x%6\n"
            "fineX: %7\n"
            "W:     %8\n"
        )
        .arg(ppu->ppuctrl, 2, 16, QChar('0'))
        .arg(ppu->ppumask, 2, 16, QChar('0'))
        .arg(ppu->ppustatus, 2, 16, QChar('0'))
        .arg(ppu->oamaddr, 2, 16, QChar('0'))
        .arg(ppu->v, 4, 16, QChar('0'))
        .arg(ppu->t, 4, 16, QChar('0'))
        .arg(ppu->fineX)
        .arg(ppu->w)
        );

        const QImage pt0 = buildPttrn(ppu->getPatternTable(0));
        const QImage pt1 = buildPttrn(ppu->getPatternTable(1));

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
    QString title = QStringLiteral("NES Emulator");

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
