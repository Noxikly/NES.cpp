#include "nes_audio.hpp"
#include "core/common.hpp"

#include <QAudioFormat>
#include <QAudioSink>
#include <QMediaDevices>
#include <QIODevice>

#include <algorithm>
#include <cstring>

NesAudio::NesAudio() {
    QAudioFormat format;
    format.setSampleRate(static_cast<int>(AUDIO_SAMPLE_RATE));
    format.setChannelCount(AUDIO_CHANNELS);
    format.setSampleFormat(QAudioFormat::Int16);

    sink = std::make_unique<QAudioSink>(QMediaDevices::defaultAudioOutput(), format);
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

    const qsizetype totalFrames = static_cast<qsizetype>(samples.size());
    const qsizetype kFrames = std::min(totalFrames, maxFrames);
    const qsizetype startFrame = totalFrames - kFrames;

    QByteArray in;
    in.resize(kFrames * static_cast<qsizetype>(sizeof(qint16) * 2));

    auto* out = reinterpret_cast<qint16*>(in.data());

    for (qsizetype i=0; i < kFrames; ++i) {
        const qint16 pcm = toI16(samples[static_cast<std::size_t>(startFrame + i)]);
        out[i * 2 + 0] = pcm;
        out[i * 2 + 1] = pcm;
    }

    pcmBuffer.append(in);

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
