#include "gui/modules/audio.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>

#include "core/apu.h"

NesAudio::NesAudio() {
    QAudioFormat format;
    format.setSampleRate(static_cast<int>(Core::APU::AUDIO_SAMPLE_RATE));
    format.setChannelCount(Core::APU::AUDIO_CHANNELS);
    format.setSampleFormat(QAudioFormat::Int16);

    sink = std::make_unique<QAudioSink>(QMediaDevices::defaultAudioOutput(), format);
    sink->setBufferSize(2048 * 4);
    sink->setVolume(volume);

    io = sink->start();
}

NesAudio::~NesAudio() {
    if (sink)
        sink->stop();
}

void NesAudio::reset() {
    pcmBuffer.clear();
    if (!sink)
        return;

    sink->stop();
    io = sink->start();
}

void NesAudio::pushSamples(const std::vector<f32> &samples) {
    if (!enabled || !sink || !io || samples.empty())
        return;

    const qsizetype totalFrames = static_cast<qsizetype>(samples.size());
    const qsizetype frameCount = std::min(totalFrames, MAX_FRAMES);
    const qsizetype startFrame = totalFrames - frameCount;

    QByteArray in;
    in.resize(frameCount * static_cast<qsizetype>(sizeof(qint16) * 2));

    auto *out = reinterpret_cast<qint16 *>(in.data());
    for (qsizetype i = 0; i < frameCount; ++i) {
        const qint16 pcm = toI16(samples[static_cast<sz>(startFrame + i)]);
        out[i * 2 + 0] = pcm;
        out[i * 2 + 1] = pcm;
    }

    pcmBuffer.append(in);

    if (pcmBuffer.size() > MAX_BYTES)
        pcmBuffer.remove(0, pcmBuffer.size() - MAX_BYTES);

    while (!pcmBuffer.isEmpty()) {
        const qint64 freeBytes = sink->bytesFree();
        if (freeBytes <= 0)
            break;

        qint64 toWrite = std::min<qint64>(freeBytes, pcmBuffer.size());
        toWrite &= ~qint64(3); // stereo 16-bit (4 bytes)
        if (toWrite <= 0)
            break;

        const qint64 written = io->write(pcmBuffer.constData(), toWrite);
        if (written <= 0)
            break;

        pcmBuffer.remove(0, static_cast<qsizetype>(written));
    }
}

void NesAudio::setEnabled(bool value) {
    enabled = value;
    if (!sink)
        return;

    if (enabled) {
        if (!io)
            io = sink->start();
        return;
    }

    sink->stop();
    io = nullptr;
    pcmBuffer.clear();
}

void NesAudio::setVolume(f32 v) {
    volume = std::clamp(v, 0.0f, 1.0f);
    if (sink)
        sink->setVolume(volume * volume);
}

qint16 NesAudio::toI16(f32 sample) {
    const f32 clamped = std::clamp(sample, -1.0f, 1.0f);
    return static_cast<qint16>(clamped * 32767.0f);
}
