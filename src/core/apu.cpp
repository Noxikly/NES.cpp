#include <algorithm>

#include "core/apu.h"

void Core::APU::powerUp() {
    state.noise.timer = NOISE_TABLE[0];
    state.noise.shiftReg = 1;
    samples.clear();
}

void Core::APU::reset() {
    state = State{};
    state.noise.timer = NOISE_TABLE[0];
    state.noise.shiftReg = 1;

    samples.clear();
}

/* Запись в регистры Core::APU (0x4000-0x4017) */
void Core::APU::writeReg(u16 addr, u8 value) {
    switch (addr) {
    /* Pulse 1 */
    case 0x4000:
        state.pulse1.duty = (value >> 6) & 0x03;
        state.pulse1.lengthHalt = (value & 0x20) != 0;
        state.pulse1.constVol = (value & 0x10) != 0;
        state.pulse1.volPeriod = value & 0x0F;
        break;

    case 0x4001:
        state.pulse1.swpEnabled = (value & 0x80) != 0;
        state.pulse1.swpPeriod = (value >> 4) & 0x07;
        state.pulse1.swpNegate = (value & 0x08) != 0;
        state.pulse1.swpShift = value & 0x07;
        state.pulse1.swpDiv = state.pulse1.swpPeriod;
        state.pulse1.swpReload = true;
        break;

    case 0x4002:
        state.pulse1.timerPeriod =
            static_cast<u16>((state.pulse1.timerPeriod & 0x0700) | value);
        break;

    case 0x4003:
        state.pulse1.timerPeriod = static_cast<u16>(
            (state.pulse1.timerPeriod & 0x00FF) | ((value & 0x07) << 8));
        state.pulse1.seqPos = 0;
        state.pulse1.envelStart = true;
        if (state.pulse1.enabled) {
            state.pulse1.lenCnt = LENGTH_TABLE[(value >> 3) & 0x1F];
        }
        break;

    /* Pulse 2 */
    case 0x4004:
        state.pulse2.duty = (value >> 6) & 0x03;
        state.pulse2.lengthHalt = (value & 0x20) != 0;
        state.pulse2.constVol = (value & 0x10) != 0;
        state.pulse2.volPeriod = value & 0x0F;
        break;

    case 0x4005:
        state.pulse2.swpEnabled = (value & 0x80) != 0;
        state.pulse2.swpPeriod = (value >> 4) & 0x07;
        state.pulse2.swpNegate = (value & 0x08) != 0;
        state.pulse2.swpShift = value & 0x07;
        state.pulse2.swpDiv = state.pulse2.swpPeriod;
        state.pulse2.swpReload = true;
        break;

    case 0x4006:
        state.pulse2.timerPeriod =
            static_cast<u16>((state.pulse2.timerPeriod & 0x0700) | value);
        break;

    case 0x4007:
        state.pulse2.timerPeriod = static_cast<u16>(
            (state.pulse2.timerPeriod & 0x00FF) | ((value & 0x07) << 8));
        state.pulse2.seqPos = 0;
        state.pulse2.envelStart = true;
        if (state.pulse2.enabled) {
            state.pulse2.lenCnt = LENGTH_TABLE[(value >> 3) & 0x1F];
        }
        break;

    /* Triangle */
    case 0x4008:
        state.triangle.ctrlFlag = (value & 0x80) != 0;
        state.triangle.linearReloadValue = value & 0x7F;
        break;

    case 0x400A:
        state.triangle.timerPeriod =
            static_cast<u16>((state.triangle.timerPeriod & 0x0700) | value);
        break;

    case 0x400B:
        state.triangle.timerPeriod = static_cast<u16>(
            (state.triangle.timerPeriod & 0x00FF) | ((value & 0x07) << 8));
        if (state.triangle.enabled) {
            state.triangle.lenCnt = LENGTH_TABLE[(value >> 3) & 0x1F];
        }
        state.triangle.linearReloadFlag = true;
        break;

    /* Noise */
    case 0x400C:
        state.noise.lenHalt = (value & 0x20) != 0;
        state.noise.constVol = (value & 0x10) != 0;
        state.noise.volPeriod = value & 0x0F;
        break;

    case 0x400E:
        state.noise.mode = (value & 0x80) != 0;
        state.noise.periodIndex = value & 0x0F;
        state.noise.timer = NOISE_TABLE[state.noise.periodIndex & 0x0F];
        break;

    case 0x400F:
        if (state.noise.enabled) {
            state.noise.lenCnt = LENGTH_TABLE[(value >> 3) & 0x1F];
        }
        state.noise.envelStart = true;
        break;

    /* DMC */
    case 0x4010:
        state.dmc.irqEnabled = (value & 0x80) != 0;
        state.dmc.loop = (value & 0x40) != 0;
        state.dmc.rateIndex = value & 0x0F;
        state.dmc.timer =
            static_cast<u16>(DMC_TABLE[state.dmc.rateIndex & 0x0F] - 1);
        if (!state.dmc.irqEnabled) {
            state.dmc.irqFlag = false;
        }
        break;

    case 0x4011:
        state.dmc.outLevel = value & 0x7F;
        break;

    case 0x4012:
        state.dmc.sampleAddrReg = value;
        break;

    case 0x4013:
        state.dmc.sampleLenReg = value;
        break;

    /* Каналы enable/disable + DMC start/stop */
    case 0x4015: {
        state.pulse1.enabled = (value & 0x01) != 0;
        state.pulse2.enabled = (value & 0x02) != 0;
        state.triangle.enabled = (value & 0x04) != 0;
        state.noise.enabled = (value & 0x08) != 0;

        if (!state.pulse1.enabled)
            state.pulse1.lenCnt = 0;
        if (!state.pulse2.enabled)
            state.pulse2.lenCnt = 0;
        if (!state.triangle.enabled)
            state.triangle.lenCnt = 0;
        if (!state.noise.enabled)
            state.noise.lenCnt = 0;

        const bool dmcEnable = (value & 0x10) != 0;
        state.dmc.enabled = dmcEnable;
        state.dmc.irqFlag = false;

        if (!dmcEnable) {
            state.dmc.active = false;
            state.dmc.bytesRemain = 0;
            state.dmc.bitsRemain = 0;
            state.dmc.shiftReg = 0;
            state.dmc.bufferEmpty = true;
        } else if (state.dmc.bytesRemain == 0) {
            state.dmc.active = true;
            reloadDmc();
            if (state.dmc.bufferEmpty)
                fetchDmc();
        }
        break;
    }

    /* Frame counter control */
    case 0x4017:
        state.frameCntMode5 = (value & 0x80) != 0;
        state.irqInhibit = (value & 0x40) != 0;
        if (state.irqInhibit)
            state.frameIrq = false;
        state.frameCntDelay = state.oddCycle ? 4 : 3;
        state.pendQuarterFrame = state.frameCntMode5;
        state.pendHalfFrame = state.frameCntMode5;
        state.frameCycle = 0;
        break;

    default:
        break;
    }
}

/* Чтение статуса Core::APU (0x4015) */
u8 Core::APU::readStatus() {
    u8 status = 0;
    if (state.pulse1.lenCnt > 0)
        status |= 0x01;
    if (state.pulse2.lenCnt > 0)
        status |= 0x02;
    if (state.triangle.lenCnt > 0)
        status |= 0x04;
    if (state.noise.lenCnt > 0)
        status |= 0x08;
    if (state.dmc.bytesRemain > 0)
        status |= 0x10;
    if (state.frameIrq)
        status |= 0x40;
    if (state.dmc.irqFlag)
        status |= 0x80;

    state.frameIrq = false;
    return status;
}

/* Продвинуть Core::APU на указанное число CPU-циклов */
void Core::APU::step(u32 cpuCycles) {
    for (u32 i = 0; i < cpuCycles; ++i) {
        if (state.delayHalfFrame) {
            halfFrame();
            state.delayHalfFrame = false;
        }

        /* Отложенная перезапись frame counter после записи в $4017 */
        if (state.frameCntDelay > 0) {
            --state.frameCntDelay;
            if (state.frameCntDelay == 0) {
                state.frameCycle = 0;

                if (state.pendQuarterFrame) {
                    quarterFrame();
                    state.pendQuarterFrame = false;
                }
                if (state.pendHalfFrame) {
                    halfFrame();
                    state.pendHalfFrame = false;
                }

                if (!state.oddCycle) {
                    ++state.frameCycle;
                    tickFrameCounter();
                }
            }
        } else if (!state.oddCycle) {
            ++state.frameCycle;
            tickFrameCounter();
        }

        /* Pulse/Noise тикают на чётных CPU-циклах */
        if (!state.oddCycle) {
            tickPulseTimer(state.pulse1);
            tickPulseTimer(state.pulse2);
            tickNoiseTimer();
        }

        /* Triangle и DMC тикают каждый цикл */
        tickTriangleTimer();
        tickDmc();

        /* Накопление и генерация аудиосэмплов */
        for (state.sampleAcc += cyclesPerSample; state.sampleAcc >= 1.0;
             state.sampleAcc -= 1.0)
            samples.push_back(mixSample());

        state.oddCycle = !state.oddCycle;
    }
}

/* Ход frame counter (4-step / 5-step) */
void Core::APU::tickFrameCounter() {
    switch (state.frameCycle) {
    case 3728:
    case 11185:
        quarterFrame();
        break;

    case 7456:
        quarterFrame();
        state.delayHalfFrame = true;
        break;

    default:
        break;
    }

    /* NTSC-схема тактов frame counter */
    if (state.frameCntMode5) {
        if (state.frameCycle == 18640) {
            quarterFrame();
            state.delayHalfFrame = true;
            state.frameCycle = 0;
        }
    } else {
        if (state.frameCycle == 14914) {
            quarterFrame();
            state.delayHalfFrame = true;
            if (!state.irqInhibit)
                state.frameIrq = true;
            state.frameCycle = 0;
        }
    }
}

/* Quarter-frame: envelope + linear counter */
void Core::APU::quarterFrame() {
    clockEnvelope(state.pulse1.lengthHalt, state.pulse1.volPeriod,
                  state.pulse1.envelStart, state.pulse1.envelDiv,
                  state.pulse1.envelDecay);

    clockEnvelope(state.pulse2.lengthHalt, state.pulse2.volPeriod,
                  state.pulse2.envelStart, state.pulse2.envelDiv,
                  state.pulse2.envelDecay);

    clockEnvelope(state.noise.lenHalt, state.noise.volPeriod,
                  state.noise.envelStart, state.noise.envelDiv,
                  state.noise.envelDecay);

    if (state.triangle.linearReloadFlag)
        state.triangle.linearCnt = state.triangle.linearReloadValue;
    else if (state.triangle.linearCnt > 0)
        --state.triangle.linearCnt;

    if (!state.triangle.ctrlFlag)
        state.triangle.linearReloadFlag = false;
}

/* Half-frame: length counters + sweep */
void Core::APU::halfFrame() {
    clockLengthCounter(state.pulse1.lengthHalt, state.pulse1.lenCnt);
    clockLengthCounter(state.pulse2.lengthHalt, state.pulse2.lenCnt);
    clockLengthCounter(state.triangle.ctrlFlag, state.triangle.lenCnt);
    clockLengthCounter(state.noise.lenHalt, state.noise.lenCnt);

    clockSweep(state.pulse1, false);
    clockSweep(state.pulse2, true);
}

/* Envelope clock для Pulse/Noise */
void Core::APU::clockEnvelope(bool lenHalt, u8 volPeriod, bool &startFlag,
                              u8 &div, u8 &decay) {
    if (startFlag) {
        startFlag = false;
        decay = 15;
        div = volPeriod;
        return;
    }

    if (div == 0) {
        div = volPeriod;
        if (decay == 0) {
            if (lenHalt)
                decay = 15;
        } else
            --decay;
    } else
        --div;
}

/* Sweep unit для Pulse-каналов */
void Core::APU::clockSweep(Pulse &pulse, bool secondChannel) {
    const bool doSweep =
        pulse.swpEnabled && (pulse.swpShift > 0) && (pulse.lenCnt > 0);

    if (pulse.swpDiv == 0) {
        if (doSweep) {
            const u16 change =
                static_cast<u16>(pulse.timerPeriod >> pulse.swpShift);
            u16 target = pulse.timerPeriod;
            if (pulse.swpNegate) {
                const u16 bias = secondChannel ? 0 : 1;
                target = static_cast<u16>(pulse.timerPeriod - change - bias);
            } else
                target = static_cast<u16>(pulse.timerPeriod + change);

            if (target <= 0x07FF)
                pulse.timerPeriod = target;
        }
        pulse.swpDiv = pulse.swpPeriod;
    } else
        --pulse.swpDiv;

    if (pulse.swpReload) {
        pulse.swpReload = false;
        pulse.swpDiv = pulse.swpPeriod;
    }
}

/* Тик таймера Pulse-канала */
void Core::APU::tickPulseTimer(Pulse &pulse) {
    if (pulse.timer == 0) {
        pulse.timer = pulse.timerPeriod;
        pulse.seqPos = static_cast<u8>((pulse.seqPos + 1) & 0x07);
    } else
        --pulse.timer;
}

/* Тик таймера Triangle-канала */
void Core::APU::tickTriangleTimer() {
    if (state.triangle.timer == 0) {
        state.triangle.timer = state.triangle.timerPeriod;
        if (state.triangle.lenCnt > 0 && state.triangle.linearCnt > 0)
            state.triangle.seqPos =
                static_cast<u8>((state.triangle.seqPos + 1) & 0x1F);
    } else
        --state.triangle.timer;
}

/* Тик таймера Noise-канала и сдвиг LFSR */
void Core::APU::tickNoiseTimer() {
    if (state.noise.timer == 0) {
        state.noise.timer = NOISE_TABLE[state.noise.periodIndex & 0x0F];

        const u8 tap = state.noise.mode ? 6 : 1;
        const u16 bit0 = state.noise.shiftReg & 0x0001;
        const u16 bitN = (state.noise.shiftReg >> tap) & 0x0001;
        const u16 fb = bit0 ^ bitN;

        state.noise.shiftReg >>= 1;
        state.noise.shiftReg |= static_cast<u16>(fb << 14);
    } else
        --state.noise.timer;
}

/* Тик DMC-канала: сдвиг битов и обновление DAC */
void Core::APU::tickDmc() {
    if (!state.dmc.enabled || !state.dmc.active)
        return;

    if (state.dmc.timer > 0) {
        --state.dmc.timer;
        return;
    }

    state.dmc.timer =
        static_cast<u16>(DMC_TABLE[state.dmc.rateIndex & 0x0F] - 1);

    if (state.dmc.bitsRemain == 0) {
        if (state.dmc.bufferEmpty) {
            if (state.dmc.bytesRemain == 0 && state.dmc.loop)
                reloadDmc();

            if (state.dmc.bytesRemain == 0) {
                state.dmc.active = false;
                return;
            }

            fetchDmc();
            if (state.dmc.bufferEmpty)
                return;
        }

        state.dmc.shiftReg = state.dmc.sampleBuffer;
        state.dmc.bufferEmpty = true;
        state.dmc.bitsRemain = 8;
    }

    if (state.dmc.bitsRemain > 0) {
        const u8 bit = state.dmc.shiftReg & 0x01;
        if (bit) {
            if (state.dmc.outLevel <= 125)
                state.dmc.outLevel += 2;
        } else {
            if (state.dmc.outLevel >= 2)
                state.dmc.outLevel -= 2;
        }
        state.dmc.shiftReg >>= 1;
        --state.dmc.bitsRemain;
    }

    if (state.dmc.bufferEmpty) {
        if (state.dmc.bytesRemain == 0 && state.dmc.loop)
            reloadDmc();
        if (state.dmc.bytesRemain > 0)
            fetchDmc();
    }

    state.dmc.active = (state.dmc.bytesRemain > 0) ||
                       (!state.dmc.bufferEmpty) || (state.dmc.bitsRemain > 0);
}

/* Выход Pulse-канала с учётом mute-условий */
u8 Core::APU::pulseOut(const Pulse &pulse, bool secondChannel) const {
    if (!pulse.enabled || pulse.lenCnt == 0)
        return 0;
    if (pulse.timerPeriod < 8)
        return 0;

    if (pulse.swpEnabled && pulse.swpShift > 0) {
        const u16 change =
            static_cast<u16>(pulse.timerPeriod >> pulse.swpShift);
        u16 target = 0;
        if (pulse.swpNegate) {
            const u16 bias = secondChannel ? 0 : 1;
            target = static_cast<u16>(pulse.timerPeriod - change - bias);
        } else
            target = static_cast<u16>(pulse.timerPeriod + change);
        if (target > 0x07FF)
            return 0;
    }

    if (DUTY_TABLE[pulse.duty & 0x03][pulse.seqPos & 0x07] == 0)
        return 0;

    return pulse.constVol ? pulse.volPeriod : pulse.envelDecay;
}

/* Нелинейный миксер NES Core::APU (Pulse + TND) */
f32 Core::APU::mixSample() const {
    const f64 p1 = static_cast<f64>(pulseOut(state.pulse1, false));
    const f64 p2 = static_cast<f64>(pulseOut(state.pulse2, true));
    const f64 t = static_cast<f64>(triangleOut());
    const f64 n = static_cast<f64>(noiseOut());
    const f64 d = static_cast<f64>(dmcOut());

    const f64 pulse =
        (p1 + p2 == 0.0) ? 0.0 : 95.88 / ((8128.0 / (p1 + p2)) + 100.0);
    const f64 tndIn = (t / 8227.0) + (n / 12241.0) + (d / 22638.0);
    const f64 tnd = (tndIn == 0.0) ? 0.0 : 159.79 / ((1.0 / tndIn) + 100.0);

    return static_cast<f32>(std::clamp(pulse + tnd, 0.0, 1.0));
}
