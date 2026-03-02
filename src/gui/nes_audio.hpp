#pragma once

#include <QByteArray>
#include <QtGlobal>

#include <memory>
#include <vector>

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
