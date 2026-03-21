#pragma once

#include <QFrame>
#include <QPainter>

#include "core/ppu.h"

class WFrame : public QFrame {
public:
    static inline constexpr u16 WIDTH = Core::PPU::WIDTH;
    static inline constexpr u16 HEIGHT = Core::PPU::HEIGHT;

public:
    explicit WFrame(QWidget *p = nullptr);
    ~WFrame() = default;

    void setFrameBuffer(const std::array<u32, WIDTH * HEIGHT> &frameData);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const std::array<u32, WIDTH * HEIGHT> *frameData{nullptr};
    QImage frameImage;
};