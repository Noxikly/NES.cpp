#include "nes_frame.hpp"

#include <QPainter>

#include <cstring>


NESFrame::NESFrame(QWidget* parent)
    : QFrame(parent),
      frameImage(WIDTH, HEIGHT, QImage::Format_ARGB32) 
{
    setFrameStyle(QFrame::NoFrame);
    clear();
}


void NESFrame::setFrameBuffer(const u32* frameData) {
    if (frameData == nullptr) return;

    std::memcpy(frameImage.bits(), frameData, WIDTH * HEIGHT * sizeof(u32));
    update();
}

void NESFrame::clear() {
    frameImage.fill(Qt::black);
    update();
}

void NESFrame::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.fillRect(rect(), Qt::black);
    painter.drawImage(rect(), frameImage);
}
