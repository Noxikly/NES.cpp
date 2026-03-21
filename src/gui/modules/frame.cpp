#include "gui/modules/frame.h"

WFrame::WFrame(QWidget *parent)
: QFrame(parent), 
  frameImage(WIDTH, 
             HEIGHT, 
             QImage::Format_ARGB32
) {
    setFixedSize(WIDTH * 3, HEIGHT * 3);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    clear();
}


void WFrame::setFrameBuffer(
    const std::array<u32, WIDTH * HEIGHT> &frameData) 
{
    this->frameData = &frameData;
    update();
}

void WFrame::clear() {
    frameData = nullptr;
    frameImage.fill(Qt::transparent);
    update();
}


void WFrame::paintEvent(QPaintEvent *event) {
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
