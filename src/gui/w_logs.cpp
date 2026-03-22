#include "gui/w_logs.h"

#include <QImage>
#include <QLabel>

#include "ui_logs.h"

WLogs::WLogs(QWidget *p) : QDialog(p), ui(std::make_unique<Ui::LogsDialog>()) {
    ui->setupUi(this);

    connect(ui->btnStopStart, &QPushButton::toggled, this,
            [this](bool stopped) {
                ui->btnStopStart->setText(stopped ? tr("Start") : tr("Stop"));
                emit stopToggled(stopped);
            });
    connect(ui->btnStepBack, &QPushButton::clicked, this,
            &WLogs::stepBackRequested);
    connect(ui->btnStepForward, &QPushButton::clicked, this,
            &WLogs::stepForwardRequested);
}

WLogs::~WLogs() = default;

void WLogs::setCpuDebugText(const QString &text) {
    if (ui && ui->cpuDebugView)
        ui->cpuDebugView->setPlainText(text);
}

void WLogs::setPpuDebugText(const QString &text) {
    if (ui && ui->ppuDebugView)
        ui->ppuDebugView->setPlainText(text);
}

void WLogs::setPpuPatternTables(const QImage &pt0, const QImage &pt1) {
    if (!ui)
        return;

    const auto setScaled = [](QLabel *lbl, const QImage &img) {
        if (!lbl)
            return;

        lbl->setPixmap(QPixmap::fromImage(img.scaled(
            lbl->size(), Qt::KeepAspectRatio, Qt::FastTransformation)));
    };

    setScaled(ui->ppuPattern0Label, pt0);
    setScaled(ui->ppuPattern1Label, pt1);
}

void WLogs::setStopped(bool stopped) {
    if (!ui || !ui->btnStopStart)
        return;

    if (ui->btnStopStart->isChecked() == stopped)
        return;

    ui->btnStopStart->setChecked(stopped);
}

bool WLogs::allowCpu() const { return true; }
bool WLogs::allowPpu() const { return true; }

bool WLogs::isCpuTabActive() const {
    return ui && ui->tabs && ui->tabs->currentWidget() == ui->tabCpu;
}

bool WLogs::isPpuTabActive() const {
    return ui && ui->tabs && ui->tabs->currentWidget() == ui->tabPpu;
}
