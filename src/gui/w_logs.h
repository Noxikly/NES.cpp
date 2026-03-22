#pragma once

#include <QDialog>
#include <QString>

class QImage;

namespace Ui {
class LogsDialog;
}

class WLogs : public QDialog {
    Q_OBJECT

public:
    explicit WLogs(QWidget *p = nullptr);
    ~WLogs() override;

    void setCpuDebugText(const QString &text);
    void setPpuDebugText(const QString &text);
    void setPpuPatternTables(const QImage &pt0, const QImage &pt1);
    void setStopped(bool stopped);

    bool allowCpu() const;
    bool allowPpu() const;
    bool isCpuTabActive() const;
    bool isPpuTabActive() const;

signals:
    void stopToggled(bool stopped);
    void stepBackRequested();
    void stepForwardRequested();

private:
    std::unique_ptr<Ui::LogsDialog> ui;
};
