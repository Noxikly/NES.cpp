#include "gui/modules/frame.h"

WFrame::WFrame(QWidget *parent) : QFrame(parent) {
    setFixedSize(WIDTH * 3, HEIGHT * 3);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(false);
    clear();
}

void WFrame::setFrameBuffer(
    const std::array<u32, WIDTH * HEIGHT> &newFrameData) {
    frameData = newFrameData;

    if (frameView.isNull()) {
        frameView = QImage(
            reinterpret_cast<const uchar *>(frameData.data()), WIDTH, HEIGHT,
            static_cast<qsizetype>(WIDTH * sizeof(u32)), QImage::Format_RGB32);
    }

    hasFrame = true;
    update();
}

void WFrame::clear() {
    hasFrame = false;
    frameView = QImage();
    update();
}

void WFrame::paintEvent(QPaintEvent *event) {
    (void)event;

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    if (hasFrame && !frameView.isNull()) {
        painter.drawImage(rect(), frameView);
        return;
    }

    painter.fillRect(rect(), Qt::black);
}
