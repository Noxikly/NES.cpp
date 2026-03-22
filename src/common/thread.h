#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QWaitCondition>

namespace Common::Thread {

class FnThread : public QThread {
public:
    explicit FnThread(std::function<void()> fn) : fn_(std::move(fn)) {}

protected:
    void run() override {
        if (fn_)
            fn_();
    }

private:
    std::function<void()> fn_;
};

inline void waitThread(std::unique_ptr<FnThread> &t) {
    if (t && t->isRunning() && QThread::currentThread() != t.get())
        t->wait();
}

class PausableLoopWorker {
public:
    using TickFn = std::function<void()>;
    using ErrorFn = std::function<void(std::exception_ptr)>;

    explicit PausableLoopWorker(TickFn tick, ErrorFn onError = {})
        : tick_(std::move(tick)), onErr_(std::move(onError)),
          th_(std::make_unique<FnThread>([this]() { run(); })) {
        th_->start();
    }

    ~PausableLoopWorker() { stop(); }

    PausableLoopWorker(const PausableLoopWorker &) = delete;
    auto operator=(const PausableLoopWorker &) -> PausableLoopWorker & = delete;

    void pause() {
        QMutexLocker lk(&mu_);
        paused_ = true;
    }

    void resume() {
        {
            QMutexLocker lk(&mu_);
            paused_ = false;
        }
        cv_.wakeAll();
    }

    auto isPaused() const -> bool {
        QMutexLocker lk(&mu_);
        return paused_;
    }

    void stop() {
        reqStop();
        waitThread(th_);
    }

private:
    void reqStop() {
        {
            QMutexLocker lk(&mu_);
            stop_ = true;
        }
        cv_.wakeAll();
    }

    auto waitReady() -> bool {
        mu_.lock();
        while (!stop_ && paused_)
            cv_.wait(&mu_);

        const bool ok = !stop_;
        mu_.unlock();
        return ok;
    }

    void run() {
        while (waitReady()) {

            try {
                if (tick_)
                    tick_();
            } catch (...) {
                reqStop();

                if (onErr_)
                    onErr_(std::current_exception());

                return;
            }
        }
    }

    TickFn tick_;
    ErrorFn onErr_;

    mutable QMutex mu_;
    QWaitCondition cv_;
    bool paused_{false};
    bool stop_{false};

    std::unique_ptr<FnThread> th_;
};

template <typename T> class LatestValue {
public:
    void publish(T value) {
        QMutexLocker lk(&mu_);
        val_ = std::move(value);
    }

    auto tryTake() -> std::optional<T> {
        QMutexLocker lk(&mu_);
        if (!val_.has_value())
            return std::nullopt;

        auto out = std::move(val_);
        val_.reset();
        return out;
    }

private:
    QMutex mu_;
    std::optional<T> val_;
};

template <typename InputT, typename OutputT> class LatestTaskWorker {
public:
    using ProcessFn = std::function<OutputT(InputT &&)>;

    explicit LatestTaskWorker(ProcessFn process)
        : fn_(std::move(process)),
          th_(std::make_unique<FnThread>([this]() { run(); })) {
        th_->start();
    }

    ~LatestTaskWorker() { stop(); }

    LatestTaskWorker(const LatestTaskWorker &) = delete;
    auto operator=(const LatestTaskWorker &) -> LatestTaskWorker & = delete;

    void submit(InputT input) {
        {
            QMutexLocker lk(&mu_);
            in_ = std::move(input);
        }

        cv_.wakeOne();
    }

    auto tryTake() -> std::optional<OutputT> { return out_.tryTake(); }

    void stop() {
        reqStop();
        waitThread(th_);
    }

private:
    void reqStop() {
        {
            QMutexLocker lk(&mu_);
            stop_ = true;
        }

        cv_.wakeAll();
    }

    auto takeIn(std::optional<InputT> &task) -> bool {
        mu_.lock();
        while (!stop_ && !in_.has_value())
            cv_.wait(&mu_);

        if (stop_) {
            mu_.unlock();
            return false;
        }

        task = std::move(in_);
        in_.reset();
        mu_.unlock();
        return true;
    }

    void run() {
        for (;;) {
            std::optional<InputT> task;

            if (!takeIn(task))
                return;

            if (!task.has_value())
                continue;

            try {
                if (fn_)
                    out_.publish(fn_(std::move(*task)));
            } catch (...) {
                reqStop();
                return;
            }
        }
    }

    ProcessFn fn_;

    QMutex mu_;
    QWaitCondition cv_;
    std::optional<InputT> in_;
    bool stop_{false};

    LatestValue<OutputT> out_;
    std::unique_ptr<FnThread> th_;
};

} /* namespace Common::Thread */
