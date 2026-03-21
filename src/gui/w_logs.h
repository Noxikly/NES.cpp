#pragma once

#include <QDialog>
#include <QString>

#include "common/types.h"

class QImage;
class QTimer;

namespace Ui {
class LogsDialog;
}

class WLogs : public QDialog {
    Q_OBJECT

public:
    explicit WLogs(QWidget *parent = nullptr);
    ~WLogs() override;

    void appendLine(const QString &line);
    void setCpuDebugText(const QString &text);
    void setPpuDebugText(const QString &text);
    void setPpuPatternTables(const QImage &pt0, const QImage &pt1);
    void setStopped(bool stopped);

    bool allowCpu() const;
    bool allowPpu() const;
    bool allowApu() const;
    bool allowMemory() const;
    bool allowMapper() const;
    bool allowGui() const;

signals:
    void stopToggled(bool stopped);
    void stepBackRequested();
    void stepForwardRequested();
    void clearLogsRequested();

private:
    void flushPending();

private:
    std::unique_ptr<Ui::LogsDialog> ui;
    QString pendingChunk;
    sz pendingLines{0};
    std::unique_ptr<QTimer> flushTimer;
};
