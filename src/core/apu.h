#pragma once

#include <vector>

#include "common/types.h"

namespace Core {
class APU {
public:
/* constants */
static inline constexpr u8 AUDIO_CHANNELS = 2;
static inline constexpr f64 AUDIO_SAMPLE_RATE = 44100.0;
static inline constexpr f64 NTSC_CPU_HZ = 1789773.0;
static inline constexpr f64 PAL_CPU_HZ = 1652097.0;
static inline constexpr f64 DENDY_CPU_HZ = 1773448.0;
static inline constexpr f64 NTSC_CYCLES = AUDIO_SAMPLE_RATE / NTSC_CPU_HZ;
static inline constexpr f64 PAL_CYCLES = AUDIO_SAMPLE_RATE / PAL_CPU_HZ;
static inline constexpr f64 DENDY_CYCLES = AUDIO_SAMPLE_RATE / DENDY_CPU_HZ;

static inline constexpr u8 LENGTH_TABLE[32] = 
{
     10, 254, 20,  2, 40,  4, 80,  6,  
    160,   8, 60, 10, 14, 12, 26, 14,
     12,  16, 24, 18, 48, 20, 96, 22,
    192,  24, 72, 26, 16, 28, 32, 30,
};

static inline constexpr u8 DUTY_TABLE[4][8] = 
{
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1},
};

static inline constexpr u8 TRIANGLE_TABLE[32] = 
{
    15, 14, 13, 12, 11, 10,  9,  8, 
    7,   6,  5,  4,  3,  2,  1,  0,
    0,   1,  2,  3,  4,  5,  6,  7, 
    8,  9,  10, 11, 12, 13, 14, 15,
};

static inline constexpr u16 NOISE_TABLE[16] = 
{
      4,   8,  16,  32,  64,   96,  128,  160, 
    202, 254, 380, 508, 762, 1016, 2034, 4068,
};

static inline constexpr u16 DMC_TABLE[16] = 
{
    428, 380, 340, 320, 286, 254, 226, 214, 
    190, 160, 142, 128, 106,  84,  72,  54,
};


public:
    explicit APU() = default;
    ~APU() = default;

    void powerUp();
    void reset();

    void writeReg(u16 addr, u8 value);
    u8 readStatus();

    void step(u32 cpuCycles);

public:
    struct Pulse {
        bool enabled{false};

        u8 duty{0};
        u8 volPeriod{0};
        bool lengthHalt{false};
        bool constVol{false};

        u8 envelDiv{0};
        u8 envelDecay{0};
        bool envelStart{false};

        u8 swpPeriod{0};
        u8 swpShift{0};
        u8 swpDiv{0};
        bool swpEnabled{false};
        bool swpNegate{false};
        bool swpReload{false};

        u16 timerPeriod{0};
        u16 timer{0};
        u8 seqPos{0};
        u8 lenCnt{0};
    };

    struct Triangle {
        bool enabled{false};

        u8 linearReloadValue{0};
        u8 linearCnt{0};
        bool ctrlFlag{false};
        bool linearReloadFlag{false};

        u16 timerPeriod{0};
        u16 timer{0};
        u8 seqPos{0};
        u8 lenCnt{0};
    };

    struct Noise {
        bool enabled{false};

        u8 volPeriod{0};
        bool lenHalt{false};
        bool constVol{false};

        u8 envelDiv{0};
        u8 envelDecay{0};
        bool envelStart{false};

        u8 periodIndex{0};
        u16 timer{0};
        u8 lenCnt{0};
        bool mode{false};

        u16 shiftReg{1};
    };

    struct DMC {
        bool enabled{false};
        bool irqEnabled{false};

        u8 rateIndex{0};
        u8 outLevel{0};
        bool loop{false};
        bool irqFlag{false};

        u8 sampleAddrReg{0};
        u8 sampleLenReg{0};
        u8 bitsRemain{0};
        u8 shiftReg{0};
        u8 sampleBuffer{0};
        bool bufferEmpty{true};
        u16 timer{0};
        u16 bytesRemain{0};
        bool active{false};
    };

public:
    bool debug{false};

public:
    f64 cyclesPerSample{NTSC_CYCLES};
    std::vector<f32> samples{};

public:
    struct State {
        Pulse pulse1{};
        Pulse pulse2{};
        Triangle triangle{};
        Noise noise{};
        DMC dmc{};

        u32 frameCycle{0};
        bool frameCntMode5{false};
        bool irqInhibit{false};
        bool frameIrq{false};
        bool oddCycle{false};

        u8 frameCntDelay{0};
        bool pendQuarterFrame{false};
        bool pendHalfFrame{false};
        bool delayHalfFrame{false};

        f64 sampleAcc{0.0};
    };

    const State &getState() const { return state; }
    void loadState(const State &s) {
        state = s;
        samples.clear();
    }

private:
    State state{};

private:
    void tickFrameCounter();
    void quarterFrame();
    void halfFrame();
    inline void reloadDmc() {
        state.dmc.bytesRemain =
            static_cast<u16>(state.dmc.sampleLenReg) * 16 + 1;
    }
    inline void fetchDmc() {
        if (state.dmc.bytesRemain == 0)
            return;

        state.dmc.sampleBuffer = 0x00;
        state.dmc.bufferEmpty = false;
        --state.dmc.bytesRemain;

        if (state.dmc.bytesRemain == 0 && !state.dmc.loop)
            state.dmc.irqFlag = state.dmc.irqEnabled;
    }

    static void clockEnvelope(bool lenHalt, u8 volPeriod, bool &startFlag,
                              u8 &div, u8 &decay);
    static void clockLengthCounter(bool halt, u8 &lenCnt) {
        if (!halt && lenCnt > 0)
            --lenCnt;
    }
    static void clockSweep(Pulse &pulse, bool secondChannel);

    void tickPulseTimer(Pulse &pulse);
    void tickTriangleTimer();
    void tickNoiseTimer();
    void tickDmc();

private:
    u8 pulseOut(const Pulse &pulse, bool secondChannel) const;
    u8 triangleOut() const {
        if (!state.triangle.enabled || state.triangle.lenCnt == 0 ||
            state.triangle.linearCnt == 0)
            return 0;
        if (state.triangle.timerPeriod < 2)
            return 0;
        return TRIANGLE_TABLE[state.triangle.seqPos & 0x1F];
    }
    u8 noiseOut() const {
        if ((state.noise.shiftReg & 0x01) != 0)
            return 0;
        if (!state.noise.enabled || state.noise.lenCnt == 0)
            return 0;
        return state.noise.constVol ? state.noise.volPeriod
                                    : state.noise.envelDecay;
    }
    u8 dmcOut() const { return state.dmc.outLevel; }

    f32 mixSample() const;
};

} /* namespace Core */
