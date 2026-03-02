#include "nes_audio.hpp"
#include "constants.hpp"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QMediaDevices>
#include <QIODevice>

NesAudio::NesAudio() {
    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(CHANNELS);
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice dev = QMediaDevices::defaultAudioOutput();
    sink = std::make_unique<QAudioSink>(dev, format);
    sink->setBufferSize(2048 * 4);

    io = sink->start();
}

NesAudio::~NesAudio() {
    if (sink)
        sink->stop();
}

void NesAudio::reset() {
    pcmBuffer.clear();
    if (!sink) return;

    sink->stop();
    io = sink->start();
}

void NesAudio::pushSamples(const std::vector<float>& samples) {
    if (!sink || !io || samples.empty()) return;

    QByteArray incoming;
    incoming.resize(static_cast<qsizetype>(samples.size()* sizeof(qint16) * 2));

    for (qsizetype i = 0; i < static_cast<qsizetype>(samples.size()); ++i) {
        const qint16 pcm = toI16(samples[static_cast<std::size_t>(i)]);
        /* Stereo: L, R */
        std::memcpy(incoming.data() + (i * 2 + 0) * static_cast<qsizetype>(sizeof(qint16)), &pcm, sizeof(qint16));
        std::memcpy(incoming.data() + (i * 2 + 1) * static_cast<qsizetype>(sizeof(qint16)), &pcm, sizeof(qint16));
    }

    pcmBuffer.append(incoming);

    constexpr qsizetype maxBytes = 4096 * 4;
    if (pcmBuffer.size() > maxBytes)
        pcmBuffer.remove(0, pcmBuffer.size() - maxBytes);

    while (!pcmBuffer.isEmpty()) {
        const qint64 freeBytes = sink->bytesFree();
        if (freeBytes <= 0) break;

        qint64 toWrite = std::min<qint64>(freeBytes, pcmBuffer.size());
        toWrite &= ~qint64(3); /* stereo 16-bit (4 bytes) */
        if (toWrite <= 0) break;

        const qint64 written = io->write(pcmBuffer.constData(), toWrite);
        if (written <= 0) break;

        pcmBuffer.remove(0, static_cast<qsizetype>(written));
    }
}

auto NesAudio::toI16(float sample) -> qint16 {
    const float clamp = std::clamp(sample, -1.0f, 1.0f);
    return static_cast<qint16>(clamp * 32767.0f);
}
