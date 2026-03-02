#pragma once

#include <vector>

#include "constants.hpp"

class Apu {
public:
    struct Pulse {
        bool enabled{false};    /* Канал включен (бит 0/1 в 0x4015)                  */

        u8 duty{0};             /* Duty cycle [0..3]                                 */
        u8 volPeriod{0};        /* Период/громкость огибающей [0..15]                */
        bool lengthHalt{false}; /* Остановка length счетчика (halt/loop envelope)    */
        bool constVol{false};   /* Использовать постоянную громкость вместо envelope */

        u8 envelDiv{0};         /* Делитель envelope                       */
        u8 envelDecay{0};       /* Текущее значение decay envelope [0..15] */
        bool envelStart{false}; /* Флаг перезапуска envelope               */

        u8 swpPeriod{0};        /* Период sweep [0..7]              */
        u8 swpShift{0};         /* Сдвиг sweep [0..7]               */
        u8 swpDiv{0};           /* Внутренний делитель sweep        */
        bool swpEnabled{false}; /* Sweep включен                    */
        bool swpNegate{false};  /* Sweep: вычитание вместо сложения */
        bool swpReload{false};  /* Перезагрузка sweep-делителя      */

        u16 timerPeriod{0};     /* Период таймера (11 бит) */
        u16 timer{0};           /* Текущий счетчик таймера */
        u8 seqPos{0};           /* Позиция в duty          */
        u8 lenCnt{0};           /* Length counter          */
    };

    struct Triangle {
        bool enabled{false};     /* Канал включен (бит 2 в 0x4015)                */

        u8 linearReloadValue{0}; /* Значение перезагрузки linear counter [0..127] */
        u8 linearCnt{0};         /* Linear counter                                */
        bool ctrlFlag{false};    /* Control flag (halt length)                    */
        bool linearReloadFlag{false}; /* Флаг перезагрузки linear counter         */

        u16 timerPeriod{0};      /* Период таймера (11 бит)                   */
        u16 timer{0};            /* Текущий счетчик таймера                   */
        u8 seqPos{0};            /* Позиция в 32-ступенчатой таблице triangle */
        u8 lenCnt{0};            /* Length counter                            */
    };

    struct Noise {
        bool enabled{false};    /* Канал включен (бит 3 в 0x4015)    */

        u8 volPeriod{0};        /* Период/громкость envelope [0..15] */
        bool lenHalt{false};    /* Halt length / loop envelope       */
        bool constVol{false};   /* Постоянная громкость              */

        u8 envelDiv{0};         /* Делитель envelope               */
        u8 envelDecay{0};       /* Текущее значение decay envelope */
        bool envelStart{false}; /* Флаг перезапуска envelope       */

        u8 periodIndex{0};      /* Индекс периода шума [0..15] */
        u16 timer{0};           /* Таймер шума                 */
        u8 lenCnt{0};           /* Length counter              */
        bool mode{false};       /* Режим LFSR (short/long)     */

        u16 shiftReg{1};        /* 15-битный LFSR сдвиговый регистр */
    };

    struct Dmc {
        bool enabled{false};    /* Канал включен (бит 4 в 0x4015) */
        bool irqEnabled{false}; /* IRQ разрешен                   */

        u8 rateIndex{0};        /* Индекс частоты DMC [0..15]     */
        u8 outLevel{0};         /* Выходной уровень [0..127]      */
        bool loop{false};       /* Зацикливание выборки           */
        bool irqFlag{false};    /* Флаг IRQ DMC                   */

        u8 sampleAddrReg{0};    /* Регистр адреса выборки (0x4012)     */
        u8 sampleLenReg{0};     /* Регистр длины выборки (0x4013)      */
        u8 bitsRemain{0};       /* Осталось бит для вывода из shiftReg */
        u8 shiftReg{0};         /* Shift register текущего байта DMC   */
        u8 sampleBuffer{0};     /* Буфер следующего байта выборки      */
        bool bufferEmpty{true}; /* Буфер выборки пуст                  */
        u16 timer{0};           /* Таймер DMC                          */
        u16 bytesRemain{0};     /* Осталось байт в текущей выборке     */
        bool active{false};     /* Воспроизведение активно             */
    };

    struct State {
        Pulse pulse1{};
        Pulse pulse2{};
        Triangle triangle{};
        Noise noise{};
        Dmc dmc{};

        u32 frameCycle{0};         /* Счетчик циклов frame счетчика           */
        bool frameCntMode5{false}; /* true: 5-step режим, false: 4-step режим */
        bool irqInhibit{false};    /* Запрет IRQ frame счетчика               */
        bool frameIrq{false};      /* Флаг IRQ frame счетчика                 */
        bool oddCycle{false};      /* Нечетный CPU cycle                      */

        u8 frameCntDelay{0};          /* Задержка записи 0x4017       */
        bool pendQuarterFrame{false}; /* quarter-frame тик            */
        bool pendHalfFrame{false};    /* half-frame тик               */
        bool delayHalfFrame{false};   /* half-frame на 1 цикл         */
        u8 frameIrqRepeat{0};         /* Установка IRQ frame счетчика */

        double sampleAcc{0.0};
    };

public:
    Apu() = default;

    void powerUp();
    void reset();

    void writeReg(u16 addr, u8 value);
    auto readStatus() -> u8;

    void step(u32 cpuCycles);

    auto getState() const -> const State& { return state; }
    void loadState(const State& state) { this->state = state; sampleBuffer.clear(); }

    auto takeSamples() -> std::vector<float>;

private:
    State state{};
    std::vector<float> sampleBuffer;

private:
    void stepCycle();
    void tickFrameCounter();
    void quarterFrame();
    void halfFrame();

    static void clockEnvel(bool lenHalt, u8 volPeriod,
                           bool& startFlag, u8& div, u8& decay);

    static void clockLenCnt(bool halt, u8& lenCnt) { if (!halt && lenCnt > 0) --lenCnt; }
    static void clockSweep(Pulse& pulse, bool secondChannel);

    void tickPulseTimer(Pulse& pulse);
    void tickTriangleTimer();
    void tickNoiseTimer();
    void tickDmc();

private:
    auto pulseOut(const Pulse& pulse, bool secondChannel) const -> u8;
    auto triangleOut() const -> u8 { if (!state.triangle.enabled 
                                         || state.triangle.lenCnt == 0 
                                         || state.triangle.linearCnt == 0) return 0;
                                     return TRIANGLE_TABLE[state.triangle.seqPos & 0x1F];}
    auto noiseOut() const -> u8 { if ((state.noise.shiftReg & 0x01) == 0) return 0;
                                  if (!state.noise.enabled || state.noise.lenCnt == 0) return 0;
                                  return state.noise.constVol ? state.noise.volPeriod : state.noise.envelDecay; }
    auto dmcOut() const -> u8 {return state.dmc.outLevel; }

    auto mixSample() const -> float;
};
