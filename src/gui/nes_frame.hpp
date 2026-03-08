#pragma once

#include <QFrame>
#include <QImage>

#include <array>

#include "core/common.hpp"

class NESFrame : public QFrame {
public:
    explicit NESFrame(QWidget* parent = nullptr);

    void setFrameBuffer(const std::array<u32, WIDTH * HEIGHT>& frameData);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    const std::array<u32, WIDTH * HEIGHT>* frameData{nullptr};
    QImage frameImage;
};
