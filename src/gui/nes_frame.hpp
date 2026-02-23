#pragma once

#include <QFrame>
#include <QImage>

#include "core/constants.hpp"

class NESFrame : public QFrame {
public:
    explicit NESFrame(QWidget* parent = nullptr);

    void setFrameBuffer(const u32* frameData);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage frameImage;
};
