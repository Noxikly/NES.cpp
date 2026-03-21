#include "common/debug.h"
#include "common/error.h"

#include "core/apu.h"
#include "core/mem.h"

namespace Core {

namespace {
inline void logReadError(bool debug, u16 addr, Common::Error::ErrorCode code) {
    if (!debug)
        return;

    const char *name = Common::Error::toString(code);
    if (code == Common::Error::ErrorCode::UnmappedRead) {
        LOG_TRACE("[MEM:read] addr=0x%04X, err=%s", static_cast<unsigned>(addr),
                  name);
        return;
    }

    LOG_WARN("[MEM:read] addr=0x%04X, err=%s", static_cast<unsigned>(addr),
             name);
}

inline void logWriteError(bool debug, u16 addr, Common::Error::ErrorCode code) {
    if (!debug)
        return;

    const char *name = Common::Error::toString(code);
    if (code == Common::Error::ErrorCode::UnmappedWrite) {
        LOG_TRACE("[MEM:write] addr=0x%04X, err=%s", static_cast<unsigned>(addr),
                  name);
        return;
    }

    LOG_WARN("[MEM:write] addr=0x%04X, err=%s", static_cast<unsigned>(addr),
             name);
}
} // namespace

Common::Error::Result<u8> Memory::read(u16 addr) const {
    if (addr < 0x2000) {
        return state.ram[addr & MIRROR];
    }

    if (addr < 0x4000) {
        if (!ppu) {
            logReadError(debug, addr,
                         Common::Error::ErrorCode::ComponentUnavailable);
            return Common::Error::Error{
                Common::Error::ErrorCode::ComponentUnavailable, addr};
        }
        return ppu->readReg(addr);
    }

    if (addr < 0x4020) {
        switch (addr) {
        case 0x4015:
            if (!apu) {
                logReadError(debug, addr,
                             Common::Error::ErrorCode::ComponentUnavailable);
                return Common::Error::Error{
                    Common::Error::ErrorCode::ComponentUnavailable, addr};
            }
            return apu->readStatus();
        case 0x4016: {
            u8 value;
            if (state.joy) {
                value = ((state.joy1 & 0x01) | 0x40);
            } else {
                value = ((state.joy1Shift & 0x01) | 0x40);
                state.joy1Shift = static_cast<u8>((state.joy1Shift >> 1) | 0x80);
            }
            return value;
        }
        case 0x4017: {
            u8 value;
            if (state.joy) {
                value = ((state.joy2 & 0x01) | 0x40);
            } else {
                value = ((state.joy2Shift & 0x01) | 0x40);
                state.joy2Shift = static_cast<u8>((state.joy2Shift >> 1) | 0x80);
            }
            return value;
        }
        default:
            logReadError(debug, addr, Common::Error::ErrorCode::UnmappedRead);
            return Common::Error::Error{Common::Error::ErrorCode::UnmappedRead,
                                        addr};
        }
    }

    if (addr >= 0x6000 && addr < 0x8000) {
        if (!mapper) {
            logReadError(debug, addr,
                         Common::Error::ErrorCode::ComponentUnavailable);
            return Common::Error::Error{
                Common::Error::ErrorCode::ComponentUnavailable, addr};
        }
        return mapper->readRAM(addr);
    }

    if (addr >= 0x8000) {
        if (!mapper) {
            logReadError(debug, addr,
                         Common::Error::ErrorCode::ComponentUnavailable);
            return Common::Error::Error{
                Common::Error::ErrorCode::ComponentUnavailable, addr};
        }
        return mapper->readPRG(addr);
    }

    logReadError(debug, addr, Common::Error::ErrorCode::InvalidAddress);
    return Common::Error::Error{Common::Error::ErrorCode::InvalidAddress, addr};
}

Common::Error::Status Memory::write(u16 addr, u8 value) {
    if (addr < 0x2000) {
        state.ram[addr & MIRROR] = value;
        return std::nullopt;
    }

    if (addr < 0x4000) {
        if (!ppu) {
            logWriteError(debug, addr,
                          Common::Error::ErrorCode::ComponentUnavailable);
            return Common::Error::Error{
                Common::Error::ErrorCode::ComponentUnavailable, addr};
        }
        ppu->writeReg(addr, value);
        return std::nullopt;
    }

    if (addr < 0x4020) {
        switch (addr) {
        case 0x4000:
        case 0x4001:
        case 0x4002:
        case 0x4003:
        case 0x4004:
        case 0x4005:
        case 0x4006:
        case 0x4007:
        case 0x4008:
        case 0x4009:
        case 0x400A:
        case 0x400B:
        case 0x400C:
        case 0x400D:
        case 0x400E:
        case 0x400F:
        case 0x4010:
        case 0x4011:
        case 0x4012:
        case 0x4013:
        case 0x4015:
        case 0x4017:
            if (!apu) {
                logWriteError(debug, addr,
                              Common::Error::ErrorCode::ComponentUnavailable);
                return Common::Error::Error{
                    Common::Error::ErrorCode::ComponentUnavailable, addr};
            }
            apu->writeReg(addr, value);
            return std::nullopt;
        case 0x4014: {
            if (!ppu) {
                addDma(state.dmaOdd ? 513 : 514);
                logWriteError(debug, addr,
                              Common::Error::ErrorCode::ComponentUnavailable);
                return Common::Error::Error{
                    Common::Error::ErrorCode::ComponentUnavailable, addr};
            }

            const u16 base = value << 8;
            for (u16 i = 0; i < 256; ++i) {
                const auto dataRes = read(base + i);
                const u8 data = std::holds_alternative<u8>(dataRes)
                                    ? std::get<u8>(dataRes)
                                    : 0;
                ppu->writeReg(0x2004, data);
            }
            addDma(state.dmaOdd ? 513 : 514);
            return std::nullopt;
        }
        case 0x4016: {
            const bool newJoy = (value & 0x01) != 0;
            if (newJoy) {
                state.joy1Shift = state.joy1;
                state.joy2Shift = state.joy2;
            } else if (state.joy) {
                state.joy1Shift = state.joy1;
                state.joy2Shift = state.joy2;
            }
            state.joy = newJoy;
            return std::nullopt;
        }
        default:
            logWriteError(debug, addr, Common::Error::ErrorCode::UnmappedWrite);
            return Common::Error::Error{Common::Error::ErrorCode::UnmappedWrite,
                                        addr};
        }
    }

    if (addr >= 0x6000 && addr < 0x8000) {
        if (!mapper) {
            logWriteError(debug, addr,
                          Common::Error::ErrorCode::ComponentUnavailable);
            return Common::Error::Error{
                Common::Error::ErrorCode::ComponentUnavailable, addr};
        }
        mapper->writeRAM(addr, value);
        return std::nullopt;
    }

    if (addr >= 0x8000) {
        if (!mapper) {
            logWriteError(debug, addr,
                          Common::Error::ErrorCode::ComponentUnavailable);
            return Common::Error::Error{
                Common::Error::ErrorCode::ComponentUnavailable, addr};
        }
        mapper->writePRG(addr, value);
        return std::nullopt;
    }

    logWriteError(debug, addr, Common::Error::ErrorCode::InvalidAddress);
    return Common::Error::Error{Common::Error::ErrorCode::InvalidAddress, addr};
}

} /* namespace Core */
