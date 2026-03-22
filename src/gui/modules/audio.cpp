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

    sink = std::make_unique<QAudioSink>(QMediaDevices::defaultAudioOutput(),
                                        format);
    sink->setBufferSize(MAX_BYTES);
    sink->setVolume(volume);

    io = sink->start();
}

NesAudio::~NesAudio() {
    if (sink)
        sink->stop();
}

void NesAudio::reset() {
    clearRing();

    if (!sink)
        return;

    sink->stop();
    io = sink->start();
}

void NesAudio::pushSamples(const std::vector<f32> &samples) {
    if (!enabled || !sink || !io)
        return;

    appendSamples(samples);
    drainSink();
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
    clearRing();
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

void NesAudio::clearRing() {
    ringRead = 0;
    ringWrite = 0;
    ringUsed = 0;
}

void NesAudio::appendSamples(const std::vector<f32> &samples) {
    if (samples.empty())
        return;

    const qsizetype totalFrames = static_cast<qsizetype>(samples.size());
    const qsizetype frameCount = std::min(totalFrames, MAX_FRAMES);
    const qsizetype startFrame = totalFrames - frameCount;

    const auto pushSample = [this](qint16 sample) {
        if (ringUsed == RING_SAMPLES) {
            ringRead = (ringRead + 1) % RING_SAMPLES;
            --ringUsed;
        }

        pcmRing[static_cast<size_t>(ringWrite)] = sample;
        ringWrite = (ringWrite + 1) % RING_SAMPLES;
        ++ringUsed;
    };

    for (qsizetype i = 0; i < frameCount; ++i) {
        const qint16 pcm = toI16(samples[static_cast<sz>(startFrame + i)]);
        pushSample(pcm);
        pushSample(pcm);
    }
}

void NesAudio::drainSink() {
    while (ringUsed > 1) {
        const qint64 freeBytes = sink->bytesFree();
        if (freeBytes <= 0)
            break;

        qint64 writableBytes =
            freeBytes & ~qint64(3); /* stereo frame aligned */
        if (writableBytes <= 0)
            break;

        qsizetype writableSamples = static_cast<qsizetype>(
            writableBytes / static_cast<qint64>(sizeof(qint16)));
        writableSamples &= ~qsizetype(1);
        if (writableSamples <= 0)
            break;

        const qsizetype contiguous =
            std::min(ringUsed, RING_SAMPLES - ringRead);
        qsizetype samplesToWrite = std::min(contiguous, writableSamples);
        samplesToWrite &= ~qsizetype(1);
        if (samplesToWrite <= 0)
            break;

        const qint64 written = io->write(
            reinterpret_cast<const char *>(pcmRing.data() +
                                           static_cast<size_t>(ringRead)),
            static_cast<qint64>(samplesToWrite *
                                static_cast<qsizetype>(sizeof(qint16))));
        if (written <= 0)
            break;

        qsizetype writtenSamples = static_cast<qsizetype>(
            written / static_cast<qint64>(sizeof(qint16)));
        writtenSamples &= ~qsizetype(1);
        if (writtenSamples <= 0)
            break;

        ringRead = (ringRead + writtenSamples) % RING_SAMPLES;
        ringUsed -= writtenSamples;
    }
}
