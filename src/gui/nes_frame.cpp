#include "nes_frame.hpp"

#include <QPainter>

NESFrame::NESFrame(QWidget *parent)
    : QFrame(parent), frameImage(WIDTH, HEIGHT, QImage::Format_ARGB32) {
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    clear();
}

void NESFrame::setFrameBuffer(
    const std::array<u32, WIDTH * HEIGHT> &frameData) {
    this->frameData = &frameData;
    update();
}

void NESFrame::clear() {
    frameData = nullptr;
    frameImage.fill(Qt::transparent);
    update();
}

void NESFrame::paintEvent(QPaintEvent *event) {
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    if (frameData) {
        const QImage frameView(
            reinterpret_cast<const uchar *>(frameData->data()), WIDTH, HEIGHT,
            static_cast<qsizetype>(WIDTH * sizeof(u32)), QImage::Format_ARGB32);
        painter.drawImage(rect(), frameView);
        return;
    }

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
}
