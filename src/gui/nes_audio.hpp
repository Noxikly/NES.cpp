#pragma once

#include <QByteArray>
#include <QtGlobal>

#include <memory>
#include <vector>

inline constexpr qsizetype maxBytes = 4096 * 4;
inline const qsizetype maxFrames =
    maxBytes / static_cast<qsizetype>(sizeof(qint16) * 2);

class QAudioSink;
class QIODevice;

class NesAudio {
  public:
    NesAudio();
    ~NesAudio();

    void reset();
    void pushSamples(const std::vector<float> &samples);
    void setEnabled(bool enabled);
    auto isEnabled() const -> bool { return enabled; }
    void setVolume(float volume);
    auto getVolume() const -> float { return volume; }

  private:
    std::unique_ptr<QAudioSink> sink;
    QIODevice *io{nullptr};
    QByteArray pcmBuffer;
    bool enabled{true};
    float volume{1.0f};

  private:
    static auto toI16(float sample) -> qint16;
};
