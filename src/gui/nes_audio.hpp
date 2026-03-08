#pragma once

#include <QByteArray>
#include <QtGlobal>

#include <memory>
#include <vector>

inline constexpr qsizetype maxBytes = 4096 * 4;
inline const qsizetype maxFrames = maxBytes / static_cast<qsizetype>(sizeof(qint16) * 2);

class QAudioSink;
class QIODevice;

class NesAudio {
public:
    NesAudio();
    ~NesAudio();

    void reset();
    void pushSamples(const std::vector<float>& samples);

private:
    std::unique_ptr<QAudioSink> sink;
    QIODevice* io{nullptr};
    QByteArray pcmBuffer;

private:
    static auto toI16(float sample) -> qint16;
};
