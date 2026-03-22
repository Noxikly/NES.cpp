#pragma once

#include <memory>

#include "common/types.h"

class WMain;

class WUpdate {
public:
    explicit WUpdate(WMain *p);
    ~WUpdate();

    auto suspendForCriticalSection() -> bool;
    void resumeAfterCriticalSection(bool wasRunning);

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
    struct EmuWorker;

    void handleEmuWorkerFailure();
    void emuWorkerTick();

    void startEmuWorker();
    void stopEmuWorker();
    void emulateFrameCore();
    auto applyReadyEmuFrame() -> bool;
    void syncDbgSafe();

    WMain *main{nullptr};
    std::unique_ptr<EmuWorker> emuWorker;

#if defined(DEBUG)
    struct DebugWorker;

    auto shouldSyncDebugFlags() const -> bool;

    void startDebugWorker();
    void stopDebugWorker();
    void submitDebugSnapshot();
    void applyReadyDebugResult();

    std::unique_ptr<DebugWorker> debugWorker;
#endif
};
