#include "gui/w_settings.h"

#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QPushButton>

#include "gui/w_main.h"
#include "ui_audio.h"
#include "ui_layout.h"
#include "ui_settings.h"

WSettings::WSettings(WMain *mainWindow, QWidget *parent)
    : QDialog(parent), main(mainWindow), ui(std::make_unique<Ui::SettingsDialog>()),
      layoutUi(std::make_unique<Ui::SettingsLayoutPage>()),
      audioUi(std::make_unique<Ui::SettingsAudioPage>()) {
    ui->setupUi(this);

    if (ui->layoutPageHost) {
        layoutUi->setupUi(ui->layoutPageHost);
        initLayoutBindings();
    }

    if (ui->audioPageHost) {
        audioUi->setupUi(ui->audioPageHost);
        initAudioSettings();
    }

    ui->btnApply->setCheckable(true);
    connect(ui->btnApply, &QPushButton::clicked, this, [this]() {
        if (main) {
            for (auto it = stagedBinds.cbegin(); it != stagedBinds.cend(); ++it)
                main->setBinding(it.key(), it.value());

            main->setAudioEnabled(stagedAudioEnabled);
            main->setAudioVolume(stagedAudioVolume);
        }

        appliedBinds = stagedBinds;
        appliedAudioEnabled = stagedAudioEnabled;
        appliedAudioVolume = stagedAudioVolume;
        updateApplyButtonState();
    });

    installEventFilter(this);

    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::close);

    updateApplyButtonState();
}

WSettings::~WSettings() = default;


void WSettings::initLayoutBindings() {
    bindButton(layoutUi->btnP1Up, "P1_UP", Qt::Key_W);
    bindButton(layoutUi->btnP1Down, "P1_DOWN", Qt::Key_S);
    bindButton(layoutUi->btnP1Left, "P1_LEFT", Qt::Key_A);
    bindButton(layoutUi->btnP1Right, "P1_RIGHT", Qt::Key_D);
    bindButton(layoutUi->btnP1Select, "P1_SELECT", Qt::Key_Shift);
    bindButton(layoutUi->btnP1Start, "P1_START", Qt::Key_Return);
    bindButton(layoutUi->btnP1B, "P1_B", Qt::Key_Z);
    bindButton(layoutUi->btnP1A, "P1_A", Qt::Key_X);

    bindButton(layoutUi->btnP2Up, "P2_UP", Qt::Key_Up);
    bindButton(layoutUi->btnP2Down, "P2_DOWN", Qt::Key_Down);
    bindButton(layoutUi->btnP2Left, "P2_LEFT", Qt::Key_Left);
    bindButton(layoutUi->btnP2Right, "P2_RIGHT", Qt::Key_Right);
    bindButton(layoutUi->btnP2Select, "P2_SELECT", Qt::Key_Period);
    bindButton(layoutUi->btnP2Start, "P2_START", Qt::Key_Slash);
    bindButton(layoutUi->btnP2B, "P2_B", Qt::Key_Semicolon);
    bindButton(layoutUi->btnP2A, "P2_A", Qt::Key_Apostrophe);
}

void WSettings::initAudioSettings() {
    if (!audioUi)
        return;

    stagedAudioEnabled = main ? main->isAudioEnabled() : true;
    appliedAudioEnabled = stagedAudioEnabled;

    stagedAudioVolume = main ? main->audioVolumePercent() : 100;
    appliedAudioVolume = stagedAudioVolume;

    if (audioUi->chkAudioEnabled)
        audioUi->chkAudioEnabled->setChecked(stagedAudioEnabled);

    if (audioUi->sliderVolume)
        audioUi->sliderVolume->setValue(stagedAudioVolume);

    if (audioUi->sliderVolume)
        audioUi->sliderVolume->setEnabled(stagedAudioEnabled);

    updateAudioLabel();

    if (audioUi->chkAudioEnabled) {
        connect(audioUi->chkAudioEnabled, &QCheckBox::toggled, this,
                [this](bool checked) {
                    stagedAudioEnabled = checked;

                    if (audioUi && audioUi->sliderVolume)
                        audioUi->sliderVolume->setEnabled(checked);

                    updateApplyButtonState();
                });
    }

    if (audioUi->sliderVolume) {
        connect(audioUi->sliderVolume, &QSlider::valueChanged, this,
                [this](int value) {
                    stagedAudioVolume = value;
                    updateAudioLabel();
                    updateApplyButtonState();
                });
    }
}

void WSettings::bindButton(QPushButton *button, const QString &bindId,
                           int defaultKey) {
    if (!button)
        return;

    const int initialKey = main ? main->bindingFor(bindId) : defaultKey;
    stagedBinds.insert(bindId, initialKey);
    appliedBinds.insert(bindId, initialKey);
    button->setProperty("bindId", bindId);
    button->setProperty("bindLabel", button->text());
    applyButtonCaption(button, initialKey);

    connect(button, &QPushButton::clicked, this,
            [this, button, bindId]() { beginRebind(button, bindId); });
}

void WSettings::beginRebind(QPushButton *button, const QString &bindId) {
    if (!button)
        return;

    pendingButton = button;
    pendingBindId = bindId;

    const QString label = button->property("bindLabel").toString();
    button->setText(QStringLiteral("%1: ...").arg(label));
    setFocus(Qt::OtherFocusReason);
}

void WSettings::applyButtonCaption(QPushButton *button, int key) const {
    if (!button)
        return;

    const QString label = button->property("bindLabel").toString();
    button->setText(QStringLiteral("%1: %2").arg(label, keyName(key)));
}

void WSettings::updateAudioLabel() {
    if (!audioUi || !audioUi->labelVolumeValue)
        return;

    audioUi->labelVolumeValue->setText(QStringLiteral("%1%").arg(stagedAudioVolume));
}

auto WSettings::keyName(int key) -> QString {
    const QString name = QKeySequence(key).toString(QKeySequence::NativeText);
    if (!name.isEmpty())
        return name;

    return QStringLiteral("Key(%1)").arg(key);
}

bool WSettings::eventFilter(QObject *watched, QEvent *event) {
    if (pendingButton && !pendingBindId.isEmpty() &&
        event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        const int key = keyEvent->key();

        if (key == Qt::Key_Escape) {
            applyButtonCaption(pendingButton,
                               main ? main->bindingFor(pendingBindId)
                                    : stagedBinds.value(pendingBindId, Qt::Key_unknown));
        } else if (key != Qt::Key_unknown) {
            stagedBinds[pendingBindId] = key;
            applyButtonCaption(pendingButton, key);
            updateApplyButtonState();
        }

        pendingButton = nullptr;
        pendingBindId.clear();
        return true;
    }

    return QDialog::eventFilter(watched, event);
}

void WSettings::updateApplyButtonState() {
    const bool hasBindChanges = (stagedBinds != appliedBinds);
    const bool hasAudioChanges = (stagedAudioEnabled != appliedAudioEnabled) ||
                                 (stagedAudioVolume != appliedAudioVolume);
    const bool hasChanges = hasBindChanges || hasAudioChanges;

    ui->btnApply->setEnabled(hasChanges);
    ui->btnApply->setChecked(!hasChanges);
}
