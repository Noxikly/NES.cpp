#pragma once

#include <cstdarg>
#include <cstdio>
#include <deque>
#include <mutex>
#include <vector>

#include <algorithm>
#include <string>

#include "common/types.h"

namespace Common::Debug {

enum class Level {
    Error = 0,
    Warn,
    Info,
    Debug,
    Trace,
};

struct Message {
    Level level;
    std::string text;
};

inline const char* levelName(Level level) {
    switch (level) {
        case Level::Error: return "ERROR";
        case Level::Warn:  return "WARN";
        case Level::Info:  return "INFO";
        case Level::Debug: return "DEBUG";
        case Level::Trace: return "TRACE";
        default:           return "UNKNOWN";
    }
}

inline Level& globalLevel() {
    static Level level = Level::Info;
    return level;
}

inline bool& globalEnabled() {
    static bool enabled = true;
    return enabled;
}

inline std::mutex& globalMutex() {
    static std::mutex m;
    return m;
}

inline std::deque<Message>& globalQueue() {
    static std::deque<Message> queue;
    return queue;
}

inline sz& globalDropped() {
    static sz dropped = 0;
    return dropped;
}

inline constexpr sz kQueueLimit = 4096;

inline bool isLowPriority(Level level) {
    return level == Level::Debug || level == Level::Trace;
}

inline void enqueue(Level level, std::string text) {
    auto& queue = globalQueue();
    if (queue.size() < kQueueLimit) {
        queue.push_back({level, std::move(text)});
        return;
    }

    if (isLowPriority(level)) {
        ++globalDropped();
        return;
    }

    const auto it = std::find_if(queue.begin(), queue.end(), [](const Message& msg) {
        return isLowPriority(msg.level);
    });

    if (it != queue.end()) {
        queue.erase(it);
    } else {
        queue.pop_front();
    }

    ++globalDropped();
    queue.push_back({level, std::move(text)});
}

inline void setLevel(Level level) {
    globalLevel() = level;
}

inline Level getLevel() {
    return globalLevel();
}

inline void setEnabled(bool enabled) {
    globalEnabled() = enabled;
}

inline bool enabled(Level level) {
    return globalEnabled() &&
           static_cast<u8>(level) <= static_cast<u8>(globalLevel());
}

inline void logv(Level level, const char* fmt, va_list ap) {
    if (!enabled(level)) {
        return;
    }

    char buf[512];
    va_list ap_copy;
    va_copy(ap_copy, ap);
    std::vsnprintf(buf, sizeof(buf), fmt, ap_copy);
    va_end(ap_copy);

    std::lock_guard<std::mutex> lock(globalMutex());
    enqueue(level, std::string(buf));
}

inline void log(Level level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logv(level, fmt, ap);
    va_end(ap);
}

inline std::vector<Message> drain(sz maxItems = 512) {
    std::vector<Message> batch;

    {
        std::lock_guard<std::mutex> lock(globalMutex());
        auto& queue = globalQueue();
        const sz count = std::min(maxItems, queue.size());
        batch.reserve(count);

        for (sz i = 0; i < count; ++i) {
            batch.push_back(std::move(queue.front()));
            queue.pop_front();
        }
    }

    return batch;
}

inline sz popDroppedCount() {
    std::lock_guard<std::mutex> lock(globalMutex());
    const sz dropped = globalDropped();
    globalDropped() = 0;
    return dropped;
}

inline void clearQueue() {
    std::lock_guard<std::mutex> lock(globalMutex());
    globalQueue().clear();
    globalDropped() = 0;
}

} /* namespace Common::Debug */


#define LOG_ERROR(...) ::Common::Debug::log(::Common::Debug::Level::Error, __VA_ARGS__)
#define LOG_WARN(...)  ::Common::Debug::log(::Common::Debug::Level::Warn,  __VA_ARGS__)
#define LOG_INFO(...)  ::Common::Debug::log(::Common::Debug::Level::Info,  __VA_ARGS__)
#define LOG_DEBUG(...) ::Common::Debug::log(::Common::Debug::Level::Debug, __VA_ARGS__)
#define LOG_TRACE(...) ::Common::Debug::log(::Common::Debug::Level::Trace, __VA_ARGS__)
