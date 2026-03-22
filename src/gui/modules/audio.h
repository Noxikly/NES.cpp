#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include <QtGlobal>

#include "common/types.h"

class QAudioSink;
class QIODevice;

class NesAudio {
public:
    static inline constexpr qsizetype MAX_BYTES = 16384 * 4;
    static inline constexpr qsizetype MAX_FRAMES =
        MAX_BYTES / static_cast<qsizetype>(sizeof(qint16) * 2);
    static inline constexpr qsizetype RING_SAMPLES = MAX_FRAMES * 2;

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
    void clearRing();
    void appendSamples(const std::vector<f32> &samples);
    void drainSink();

private:
    std::unique_ptr<QAudioSink> sink;
    QIODevice *io{nullptr};
    std::array<qint16, static_cast<size_t>(RING_SAMPLES)> pcmRing{};
    qsizetype ringRead{0};
    qsizetype ringWrite{0};
    qsizetype ringUsed{0};

    bool enabled{true};
    f32 volume{1.0f};
};
