#pragma once

#include <QDialog>
#include <QHash>

class QEvent;
class QPushButton;
class WMain;

namespace Ui {
class SettingsDialog;
class SettingsLayoutPage;
class SettingsAudioPage;
} // namespace Ui

class WSettings : public QDialog {
public:
    explicit WSettings(WMain *mainWindow, QWidget *parent = nullptr);
    ~WSettings() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void initLayoutBindings();
    void initAudioSettings();
    void bindButton(QPushButton *button, const QString &bindId, int defaultKey);
    void beginRebind(QPushButton *button, const QString &bindId);
    void applyButtonCaption(QPushButton *button, int key) const;
    void updateAudioLabel();
    void updateApplyButtonState();
    static QString keyName(int key);

private:
    WMain *main{nullptr};

    std::unique_ptr<Ui::SettingsDialog> ui;
    std::unique_ptr<Ui::SettingsLayoutPage> layoutUi;
    std::unique_ptr<Ui::SettingsAudioPage> audioUi;

    QHash<QString, int> stagedBinds;
    QHash<QString, int> appliedBinds;

    bool stagedAudioEnabled{true};
    bool appliedAudioEnabled{true};
    int stagedAudioVolume{100};
    int appliedAudioVolume{100};

    QPushButton *pendingButton{nullptr};
    QString pendingBindId;
};
