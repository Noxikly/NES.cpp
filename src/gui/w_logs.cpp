#include "gui/w_logs.h"

#include <QImage>
#include <QTimer>

#include "common/debug.h"
#include "ui_logs.h"

WLogs::WLogs(QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::LogsDialog>()) {
    ui->setupUi(this);
    ui->logView->setMaximumBlockCount(1000);

    Common::Debug::setEnabled(true);
    Common::Debug::setLevel(Common::Debug::Level::Trace);

    flushTimer = std::make_unique<QTimer>(this);
    flushTimer->setInterval(16);
    connect(flushTimer.get(), &QTimer::timeout, this, [this]() {
        if (const auto dropped = Common::Debug::popDroppedCount(); dropped > 0) {
            appendLine(QStringLiteral("[WARN] Dropped %1 debug lines")
                           .arg(static_cast<qulonglong>(dropped)));
        }

        const auto messages = Common::Debug::drain(512);
        for (const auto &msg : messages) {
            const std::string &message = msg.text;

            bool allowed = true;
            if (message.rfind("[CPU]", 0) == 0)
                allowed = allowCpu();
            else if (message.rfind("[PPU]", 0) == 0)
                allowed = allowPpu();
            else if (message.rfind("[APU]", 0) == 0)
                allowed = allowApu();
            else if (message.rfind("[MEM", 0) == 0)
                allowed = allowMemory();
            else if (message.rfind("[MAPPER]", 0) == 0)
                allowed = allowMapper();
            else if (message.rfind("[GUI]", 0) == 0)
                allowed = allowGui();

            if (!allowed)
                continue;

            const QString line =
                QStringLiteral("[%1] %2")
                    .arg(QString::fromLatin1(Common::Debug::levelName(msg.level)))
                    .arg(QString::fromStdString(message));
            appendLine(line);
        }

        flushPending();
    });
    flushTimer->start();

    connect(ui->btnStopStart, &QPushButton::toggled, this, [this](bool stopped) {
        ui->btnStopStart->setText(stopped ? tr("Start") : tr("Stop"));
        emit stopToggled(stopped);
    });
    connect(ui->btnStepBack, &QPushButton::clicked, this, &WLogs::stepBackRequested);
    connect(ui->btnStepForward, &QPushButton::clicked, this,
            &WLogs::stepForwardRequested);
    connect(ui->btnClearLogs, &QPushButton::clicked, this, [this]() {
        pendingChunk.clear();
        pendingLines = 0;
        ui->logView->clear();
        Common::Debug::clearQueue();
        emit clearLogsRequested();
    });
}

WLogs::~WLogs() = default;

void WLogs::appendLine(const QString &line) {
    if (!pendingChunk.isEmpty())
        pendingChunk += QLatin1Char('\n');
    pendingChunk += line;
    ++pendingLines;

    if (pendingLines >= 128)
        flushPending();
    else if (flushTimer && !flushTimer->isActive())
        flushTimer->start();
}

void WLogs::setCpuDebugText(const QString &text) {
    if (ui && ui->cpuDebugView)
        ui->cpuDebugView->setPlainText(text);
}

void WLogs::setPpuDebugText(const QString &text) {
    if (ui && ui->ppuDebugView)
        ui->ppuDebugView->setPlainText(text);
}

void WLogs::setPpuPatternTables(const QImage &pt0, const QImage &pt1) {
    if (ui && ui->ppuPattern0Label) {
        ui->ppuPattern0Label->setPixmap(
            QPixmap::fromImage(pt0.scaled(ui->ppuPattern0Label->size(),
                                          Qt::KeepAspectRatio,
                                          Qt::FastTransformation)));
    }

    if (ui && ui->ppuPattern1Label) {
        ui->ppuPattern1Label->setPixmap(
            QPixmap::fromImage(pt1.scaled(ui->ppuPattern1Label->size(),
                                          Qt::KeepAspectRatio,
                                          Qt::FastTransformation)));
    }
}

void WLogs::setStopped(bool stopped) {
    if (!ui || !ui->btnStopStart)
        return;

    if (ui->btnStopStart->isChecked() == stopped)
        return;

    ui->btnStopStart->setChecked(stopped);
}

bool WLogs::allowCpu() const { return ui && ui->chkCpu && ui->chkCpu->isChecked(); }
bool WLogs::allowPpu() const { return ui && ui->chkPpu && ui->chkPpu->isChecked(); }
bool WLogs::allowApu() const { return ui && ui->chkApu && ui->chkApu->isChecked(); }
bool WLogs::allowMemory() const {
    return ui && ui->chkMemory && ui->chkMemory->isChecked();
}
bool WLogs::allowMapper() const {
    return ui && ui->chkMapper && ui->chkMapper->isChecked();
}
bool WLogs::allowGui() const { return ui && ui->chkGui && ui->chkGui->isChecked(); }

void WLogs::flushPending() {
    if (!ui || !ui->logView || pendingChunk.isEmpty())
        return;

    ui->logView->appendPlainText(pendingChunk);
    pendingChunk.clear();
    pendingLines = 0;
}
