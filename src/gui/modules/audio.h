#pragma once

#include <QByteArray>

#include "common/types.h"

class QAudioSink;
class QIODevice;

class NesAudio {
public:
    static inline constexpr qsizetype MAX_BYTES = 4096 * 4;
    static inline const qsizetype MAX_FRAMES =
        MAX_BYTES / static_cast<qsizetype>(sizeof(qint16) * 2);

public:
    explicit NesAudio();
    ~NesAudio();

    void reset();
    void pushSamples(const std::vector<f32> &samples);

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled; }

    void setVolume(f32 volume);
    f32 getVolume() const { return volume; }

private:
    static qint16 toI16(f32 sample);

private:
    std::unique_ptr<QAudioSink> sink;
    QIODevice *io{nullptr};
    QByteArray pcmBuffer;

    bool enabled{true};
    f32 volume{1.0f};
};
