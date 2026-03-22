#pragma once

#include <optional>
#include <variant>

#include "common/types.h"

namespace Common::Error {

enum class ErrorCode : u8 {
    None = 0,

    /* common failures */
    NullPointer,
    InvalidAddress,
    ComponentUnavailable,
    UnmappedRead,
    UnmappedWrite,

    /* CPU failures */
    InvalidOpcode,

    /* Bus failures */
    BusConflict,
};

inline const char *toString(ErrorCode code) {
    switch (code) {
    case ErrorCode::None:
        return "None";
    case ErrorCode::NullPointer:
        return "NullPointer";
    case ErrorCode::InvalidAddress:
        return "InvalidAddress";
    case ErrorCode::ComponentUnavailable:
        return "ComponentUnavailable";
    case ErrorCode::UnmappedRead:
        return "UnmappedRead";
    case ErrorCode::UnmappedWrite:
        return "UnmappedWrite";
    case ErrorCode::InvalidOpcode:
        return "InvalidOpcode";
    case ErrorCode::BusConflict:
        return "BusConflict";
    default:
        return "Unknown";
    }
}

struct Error {
    ErrorCode code;
    u32 addr;

    Error() : code(ErrorCode::None), addr(0) {}
    Error(ErrorCode code) : code(code), addr(0) {}
    Error(ErrorCode code, u32 addr) : code(code), addr(addr) {}
};

template <typename T> using Result = std::variant<T, Error>;

using Status = std::optional<Error>;

} // namespace Common::Error
