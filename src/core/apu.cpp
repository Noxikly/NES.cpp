#include "apu.hpp"

#include <algorithm>


void Apu::powerUp() {
    /* State */
    state.frameCntMode5 = false;
    state.irqInhibit = false;
    state.frameIrq = false;
    state.frameCycle = 0;
    state.oddCycle = false;
    state.frameCntDelay = 0;
    state.pendQuarterFrame = false;
    state.pendHalfFrame = false;
    state.delayHalfFrame = false;

    /* Pulse 1 и Pulse 2 */
    state.pulse1.enabled = state.pulse2.enabled = false;
    state.pulse1.duty = state.pulse2.duty = 0;
    state.pulse1.volPeriod = state.pulse2.volPeriod = 0;
    state.pulse1.lengthHalt = state.pulse2.lengthHalt = false;
    state.pulse1.constVol = state.pulse2.constVol = false;
    state.pulse1.envelDiv = state.pulse2.envelDiv = 0;
    state.pulse1.envelDecay = state.pulse2.envelDecay = 0;
    state.pulse1.envelStart = state.pulse2.envelStart = false;
    state.pulse1.swpPeriod = state.pulse2.swpPeriod = 0;
    state.pulse1.swpShift = state.pulse2.swpShift = 0;
    state.pulse1.swpDiv = state.pulse2.swpDiv = 0;
    state.pulse1.swpEnabled = state.pulse2.swpEnabled = false;
    state.pulse1.swpNegate = state.pulse2.swpNegate = false;
    state.pulse1.swpReload = state.pulse2.swpReload = false;
    state.pulse1.timerPeriod = state.pulse2.timerPeriod = 0;
    state.pulse1.timer = state.pulse2.timer = 0;
    state.pulse1.seqPos = state.pulse2.seqPos = 0;
    state.pulse1.lenCnt = state.pulse2.lenCnt = 0;

    /* Triangle */
    state.triangle.enabled = false;
    state.triangle.linearReloadValue = 0;
    state.triangle.linearCnt = 0;
    state.triangle.ctrlFlag = false;
    state.triangle.linearReloadFlag = false;
    state.triangle.timerPeriod = 0;
    state.triangle.timer = 0;
    state.triangle.seqPos = 0;
    state.triangle.lenCnt = 0;

    /* Noise */
    state.noise.enabled = false;
    state.noise.volPeriod = 0;
    state.noise.lenHalt = false;
    state.noise.constVol = false;
    state.noise.envelDiv = 0;
    state.noise.envelDecay = 0;
    state.noise.envelStart = false;
    state.noise.periodIndex = 0;
    state.noise.timer = NOISE_TABLE[0];
    state.noise.lenCnt = 0;
    state.noise.mode = false;
    state.noise.shiftReg = 1;

    /* DMC */
    state.dmc.enabled = false;
    state.dmc.irqEnabled = false;
    state.dmc.rateIndex = 0;
    state.dmc.outLevel = 0;
    state.dmc.loop = false;
    state.dmc.irqFlag = false;
    state.dmc.bitsRemain = 0;
    state.dmc.shiftReg = 0;
    state.dmc.sampleBuffer = 0;
    state.dmc.bufferEmpty = true;
    state.dmc.bytesRemain = 0;
    state.dmc.timer = 0;
    state.dmc.active = false;
    state.dmc.sampleAddrReg = 0;
    state.dmc.sampleLenReg = 0;

    state.sampleAcc = 0.0;
    sampleBuffer.clear();
}

void Apu::reset() {
    /* State */
    state.irqInhibit = false;
    state.frameIrq = false;
    state.frameCycle = 0;
    state.oddCycle = false;
    state.frameCntDelay = 0;
    state.pendQuarterFrame = false;
    state.pendHalfFrame = false;
    state.delayHalfFrame = false;

    /* PulseX/Triangle/Noise */
    state.pulse1.enabled = state.pulse2.enabled = false;
    state.pulse1.lenCnt = state.pulse2.lenCnt = 0;
    state.triangle.enabled = false;
    state.triangle.lenCnt = 0;
    state.noise.enabled = false;
    state.noise.lenCnt = 0;

    /* DMC */
    state.dmc.enabled = false;
    state.dmc.active = false;
    state.dmc.bytesRemain = 0;
    state.dmc.bitsRemain = 0;
    state.dmc.shiftReg = 0;
    state.dmc.sampleBuffer = 0;
    state.dmc.bufferEmpty = true;
    state.dmc.irqFlag = false;

    sampleBuffer.clear();
}

/* Запись в регистры APU (0x4000-0x4017) */
void Apu::writeReg(u16 addr, u8 value) {
    switch (addr) {
        case 0x4000: {
            state.pulse1.duty = (value >> 6) & 0x03;
            state.pulse1.lengthHalt = (value & 0x20) != 0;
            state.pulse1.constVol = (value & 0x10) != 0;
            state.pulse1.volPeriod = value & 0x0F;
            break;
        }
        case 0x4001: {
            state.pulse1.swpEnabled = (value & 0x80) != 0;
            state.pulse1.swpPeriod = (value >> 4) & 0x07;
            state.pulse1.swpNegate = (value & 0x08) != 0;
            state.pulse1.swpShift = value & 0x07;
            state.pulse1.swpReload = true;
            break;
        }
        case 0x4002: {
            state.pulse1.timerPeriod = (state.pulse1.timerPeriod & 0x0700) | value;
            break;
        }
        case 0x4003: {
            state.pulse1.timerPeriod = (state.pulse1.timerPeriod & 0x00FF) | ((value & 0x07) << 8);
            state.pulse1.seqPos = 0;
            state.pulse1.envelStart = true;
            if (state.pulse1.enabled)
                state.pulse1.lenCnt = LENGTH_TABLE[(value >> 3) & 0x1F];
            break;
        }

        case 0x4004: {
            state.pulse2.duty = (value >> 6) & 0x03;
            state.pulse2.lengthHalt = (value & 0x20) != 0;
            state.pulse2.constVol = (value & 0x10) != 0;
            state.pulse2.volPeriod = value & 0x0F;
            break;
        }
        case 0x4005: {
            state.pulse2.swpEnabled = (value & 0x80) != 0;
            state.pulse2.swpPeriod = (value >> 4) & 0x07;
            state.pulse2.swpNegate = (value & 0x08) != 0;
            state.pulse2.swpShift = value & 0x07;
            state.pulse2.swpReload = true;
            break;
        }
        case 0x4006: {
            state.pulse2.timerPeriod = (state.pulse2.timerPeriod & 0x0700) | value;
            break;
        }
        case 0x4007: {
            state.pulse2.timerPeriod = (state.pulse2.timerPeriod & 0x00FF) | ((value & 0x07) << 8);
            state.pulse2.seqPos = 0;
            state.pulse2.envelStart = true;
            if (state.pulse2.enabled)
                state.pulse2.lenCnt = LENGTH_TABLE[(value >> 3) & 0x1F];
            break;
        }

        case 0x4008: {
            state.triangle.ctrlFlag = (value & 0x80) != 0;
            state.triangle.linearReloadValue = value & 0x7F;
            break;
        }
        case 0x400A: {
            state.triangle.timerPeriod = (state.triangle.timerPeriod & 0x0700) | value;
            break;
        }
        case 0x400B: {
            state.triangle.timerPeriod = (state.triangle.timerPeriod & 0x00FF) | ((value & 0x07) << 8);
            if (state.triangle.enabled)
                state.triangle.lenCnt = LENGTH_TABLE[(value >> 3) & 0x1F];
            state.triangle.linearReloadFlag = true;
            break;
        }

        case 0x400C: {
            state.noise.lenHalt = (value & 0x20) != 0;
            state.noise.constVol = (value & 0x10) != 0;
            state.noise.volPeriod = value & 0x0F;
            break;
        }
        case 0x400E: {
            state.noise.mode = (value & 0x80) != 0;
            state.noise.periodIndex = value & 0x0F;
            break;
        }
        case 0x400F: {
            if (state.noise.enabled)
                state.noise.lenCnt = LENGTH_TABLE[(value >> 3) & 0x1F];
            state.noise.envelStart = true;
            break;
        }

        case 0x4010: {
            state.dmc.irqEnabled = (value & 0x80) != 0;
            state.dmc.loop = (value & 0x40) != 0;
            state.dmc.rateIndex = value & 0x0F;
            if (!state.dmc.irqEnabled)
                state.dmc.irqFlag = false;
            break;
        }
        case 0x4011: {
            state.dmc.outLevel = value & 0x7F;
            break;
        }
        case 0x4012: {
            state.dmc.sampleAddrReg = value;
            break;
        }
        case 0x4013: {
            state.dmc.sampleLenReg = value;
            break;
        }
        case 0x4015: {
            state.pulse1.enabled = (value & 0x01) != 0;
            state.pulse2.enabled = (value & 0x02) != 0;
            state.triangle.enabled = (value & 0x04) != 0;
            state.noise.enabled = (value & 0x08) != 0;

            if (!state.pulse1.enabled) state.pulse1.lenCnt = 0;
            if (!state.pulse2.enabled) state.pulse2.lenCnt = 0;
            if (!state.triangle.enabled) state.triangle.lenCnt = 0;
            if (!state.noise.enabled) state.noise.lenCnt = 0;

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
                state.dmc.bytesRemain = static_cast<u16>(state.dmc.sampleLenReg) * 16 + 1;

                if (state.dmc.bufferEmpty && state.dmc.bytesRemain > 0) {
                    state.dmc.sampleBuffer = 0x00;
                    state.dmc.bufferEmpty = false;
                    --state.dmc.bytesRemain;
                    if (state.dmc.bytesRemain == 0 && !state.dmc.loop)
                        state.dmc.irqFlag |= state.dmc.irqEnabled;
                }
            }

            break;
        }

        case 0x4017: {
            state.frameCntMode5 = (value & 0x80) != 0;
            state.irqInhibit = (value & 0x40) != 0;
            if (state.irqInhibit) {
                state.frameIrq = false;
                state.frameIrqRepeat = 0;
            }

            state.frameCntDelay = 3;
            state.pendQuarterFrame = state.pendHalfFrame = state.frameCntMode5;
            break;
        }

        default:
            break;
    }
}

auto Apu::readStatus() -> u8 {
    /* Формирование статуса 0x4015 */
    u8 status = 0;
    if (state.pulse1.lenCnt > 0) status |= 0x01;
    if (state.pulse2.lenCnt > 0) status |= 0x02;
    if (state.triangle.lenCnt > 0) status |= 0x04;
    if (state.noise.lenCnt > 0) status |= 0x08;
    if (state.dmc.bytesRemain > 0) status |= 0x10;
    if (state.frameIrq) status |= 0x40;
    if (state.dmc.irqFlag) status |= 0x80;

    state.frameIrq = false;
    return status;
}

void Apu::step(u32 cpuCycles) {
    for (u32 i=0; i<cpuCycles; ++i) {
        if (state.frameIrqRepeat > 0) {
            if (!state.irqInhibit)
                state.frameIrq = true;
            --state.frameIrqRepeat;
        }

        if (state.delayHalfFrame) {
            halfFrame();
            state.delayHalfFrame = false;
        }

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

        if (!state.oddCycle) {
            tickPulseTimer(state.pulse1);
            tickPulseTimer(state.pulse2);
            tickNoiseTimer();
        }

        tickTriangleTimer();
        tickDmc();

        /* Ресемплинг */
        for (state.sampleAcc += CYCLES; 
             state.sampleAcc >= 1.0; 
             state.sampleAcc -= 1.0)
            sampleBuffer.push_back(mixSample());

        state.oddCycle = !state.oddCycle;
    }
}

auto Apu::takeSamples() -> std::vector<float> {
    std::vector<float> out;
    out.swap(sampleBuffer);
    return out;
}

void Apu::tickFrameCounter() {
    switch (state.frameCycle) {
        case 3729:
        case 11186: quarterFrame(); break;

        case 7457:
            quarterFrame();
            state.delayHalfFrame = true;
            break;
    }

    if (state.frameCntMode5) {
        /* 5-step seq */
        if (state.frameCycle == 18641) {
            quarterFrame();
            state.delayHalfFrame = true;
            state.frameCycle = 0;
        }
    } else {
        /* 4-step seq */
        if (state.frameCycle == 14915) {
            quarterFrame();
            state.delayHalfFrame = true;
            if (!state.irqInhibit) {
                state.frameIrq = true;
                state.frameIrqRepeat = 2;
            }
            state.frameCycle = 0;
        }
    }
}

void Apu::quarterFrame() {
    clockEnvel(state.pulse1.lengthHalt, state.pulse1.volPeriod,
               state.pulse1.envelStart, state.pulse1.envelDiv, state.pulse1.envelDecay);

    clockEnvel(state.pulse2.lengthHalt, state.pulse2.volPeriod,
               state.pulse2.envelStart, state.pulse2.envelDiv, state.pulse2.envelDecay);

    clockEnvel(state.noise.lenHalt, state.noise.volPeriod,
               state.noise.envelStart, state.noise.envelDiv, state.noise.envelDecay);

    if (state.triangle.linearReloadFlag)
        state.triangle.linearCnt = state.triangle.linearReloadValue;
    else if (state.triangle.linearCnt > 0)
        --state.triangle.linearCnt;

    if (!state.triangle.ctrlFlag)
        state.triangle.linearReloadFlag = false;
}

void Apu::halfFrame() {
    clockLenCnt(state.pulse1.lengthHalt, state.pulse1.lenCnt);
    clockLenCnt(state.pulse2.lengthHalt, state.pulse2.lenCnt);
    clockLenCnt(state.triangle.ctrlFlag, state.triangle.lenCnt);
    clockLenCnt(state.noise.lenHalt, state.noise.lenCnt);

    clockSweep(state.pulse1, false);
    clockSweep(state.pulse2, true);
}

void Apu::clockEnvel(bool lenHalt, u8 volPeriod,
                     bool& startFlag, u8& div, u8& decay) {
    if (startFlag) {
        startFlag = false;
        decay = 15;
        div = volPeriod;
        return;
    }

    if (div == 0) {
        div = volPeriod;
        if (decay == 0) {
            if (lenHalt) decay = 15;
        } else --decay;
    } else --div;
}

void Apu::clockSweep(Pulse& pulse, bool secondChannel) {
    const bool doSweep = pulse.swpEnabled && (pulse.swpShift > 0) && (pulse.lenCnt > 0);

    if (pulse.swpDiv == 0) {
        if (doSweep) {
            const u16 change = pulse.timerPeriod >> pulse.swpShift;
            if (pulse.swpNegate) {
                const u16 bias = secondChannel ? 0 : 1;
                pulse.timerPeriod = static_cast<u16>(pulse.timerPeriod - change - bias);
            } else
                pulse.timerPeriod = static_cast<u16>(pulse.timerPeriod + change);
        }
        pulse.swpDiv = pulse.swpPeriod;
    } else --pulse.swpDiv;

    if (pulse.swpReload) {
        pulse.swpReload = false;
        pulse.swpDiv = pulse.swpPeriod;
    }
}

void Apu::tickPulseTimer(Pulse& pulse) {
    if (pulse.timer == 0) {
        pulse.timer = pulse.timerPeriod;
        pulse.seqPos = static_cast<u8>((pulse.seqPos + 1) & 0x07);
    } else --pulse.timer;
}

void Apu::tickTriangleTimer() {
    if (state.triangle.timer == 0) {
        state.triangle.timer = state.triangle.timerPeriod;
        if (state.triangle.lenCnt > 0 && state.triangle.linearCnt > 0)
            state.triangle.seqPos = static_cast<u8>((state.triangle.seqPos + 1) & 0x1F);
    } else --state.triangle.timer;
}

void Apu::tickNoiseTimer() {
    if (state.noise.timer == 0) {
        state.noise.timer = NOISE_TABLE[state.noise.periodIndex & 0x0F];

        const u8 tap = state.noise.mode ? 6 : 1;
        const u16 bit0 = state.noise.shiftReg & 0x0001;
        const u16 bitN = (state.noise.shiftReg >> tap) & 0x0001;
        const u16 fb = bit0 ^ bitN;

        state.noise.shiftReg >>= 1;
        state.noise.shiftReg |= static_cast<u16>(fb << 14);
    } else --state.noise.timer;
}

void Apu::tickDmc() {
    if (!state.dmc.enabled || !state.dmc.active) return;
    if (state.dmc.timer > 0) {
        --state.dmc.timer;
        return;
    }

    state.dmc.timer = static_cast<u16>(DMC_TABLE[state.dmc.rateIndex & 0x0F] - 1);
    if (state.dmc.bitsRemain == 0) {
        if (state.dmc.bufferEmpty) {
            if (state.dmc.bytesRemain > 0) {
                state.dmc.sampleBuffer = 0x00;
                state.dmc.bufferEmpty = false;
                --state.dmc.bytesRemain;
                if (state.dmc.bytesRemain == 0 && !state.dmc.loop)
                    state.dmc.irqFlag |= state.dmc.irqEnabled;
            } else if (state.dmc.loop) {
                state.dmc.bytesRemain = static_cast<u16>(state.dmc.sampleLenReg) * 16 + 1;
                if (state.dmc.bytesRemain > 0) {
                    state.dmc.sampleBuffer = 0x00;
                    state.dmc.bufferEmpty = false;
                    --state.dmc.bytesRemain;
                }
            } else {
                state.dmc.active = false;
                return;
            }

            if (state.dmc.bufferEmpty) return;
        }

        state.dmc.shiftReg = state.dmc.sampleBuffer;
        state.dmc.bufferEmpty = true;
        state.dmc.bitsRemain = 8;
    }

    if (state.dmc.bitsRemain > 0) {
        const u8 bit = state.dmc.shiftReg & 0x01;
        if (bit) {
            if (state.dmc.outLevel <= 125) state.dmc.outLevel += 2;
        } else {
            if (state.dmc.outLevel >= 2) state.dmc.outLevel -= 2;
        }
        state.dmc.shiftReg >>= 1;
        --state.dmc.bitsRemain;
    }

    if (state.dmc.bufferEmpty) {
        if (state.dmc.bytesRemain > 0) {
            state.dmc.sampleBuffer = 0x00;
            state.dmc.bufferEmpty = false;
            --state.dmc.bytesRemain;
            if (state.dmc.bytesRemain == 0 && !state.dmc.loop)
                state.dmc.irqFlag |= state.dmc.irqEnabled;
        } else if (state.dmc.loop) {
            state.dmc.bytesRemain = static_cast<u16>(state.dmc.sampleLenReg) * 16 + 1;
            if (state.dmc.bytesRemain > 0) {
                state.dmc.sampleBuffer = 0x00;
                state.dmc.bufferEmpty = false;
                --state.dmc.bytesRemain;
            }
        }
    }

    state.dmc.active = (state.dmc.bytesRemain > 0)
                    || (!state.dmc.bufferEmpty)
                    || (state.dmc.bitsRemain > 0);
}

auto Apu::pulseOut(const Pulse& pulse, bool secondChannel) const -> u8 {
    if (!pulse.enabled || pulse.lenCnt == 0) return 0;

    /* Проверка sweep мута */
    if (pulse.timerPeriod < 8) return 0;

    /* Проверка sweep */
    if (pulse.swpEnabled && pulse.swpShift > 0) {
        const u16 change = pulse.timerPeriod >> pulse.swpShift;
        u16 target;
        if (pulse.swpNegate) {
            const u16 bias = secondChannel ? 0 : 1;
            target = static_cast<u16>(pulse.timerPeriod - change - bias);
        } else
            target = static_cast<u16>(pulse.timerPeriod + change);
        
        if (target > 0x07FF) return 0;
    }

    /* Проверка duty */
    if (DUTY_TABLE[pulse.duty & 0x03][pulse.seqPos & 0x07] == 0) return 0;

    return pulse.constVol ? pulse.volPeriod : pulse.envelDecay;
}

auto Apu::mixSample() const -> float {
    const double p1 = static_cast<double>(pulseOut(state.pulse1, false));
    const double p2 = static_cast<double>(pulseOut(state.pulse2, true));
    const double t = static_cast<double>(triangleOut());
    const double n = static_cast<double>(noiseOut());
    const double d = static_cast<double>(dmcOut());

    const double pulse = (p1 + p2 == 0.0) ? 0.0 : 95.88 / ((8128.0 / (p1 + p2)) + 100.0);
    const double tndIn = (t / 8227.0) + (n / 12241.0) + (d / 22638.0);
    const double tnd = (tndIn == 0.0) ? 0.0 : 159.79 / ((1.0 / tndIn) + 100.0);

    return static_cast<float>(std::clamp(pulse + tnd, 0.0, 1.0));
}
