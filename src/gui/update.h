#pragma once

#include "common/types.h"

class WMain;

class WUpdate {
public:
    explicit WUpdate(WMain *parent);

    void stpTimer();
    void updFrameCap();
    void doFrame();
    void updDebugPanels();
    void updFpsCounter();
    void updWindowTitle();
    void syncDebugFlagsFromLogWindow();
    void syncInputToMemory();
    void runCpuInstruction();
    void presentAudioAndVideo();
    auto ppuPerCpu() const -> f64;

private:
    WMain *main{nullptr};
};
