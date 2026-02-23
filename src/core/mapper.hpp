#pragma once

#include <string>
#include <filesystem>

#include "lua.hpp"


class Mapper : public Lua {
public:
    Mapper() = default;

    void load(const std::filesystem::path& srcPath = "mappers/") {
        std::filesystem::path path = srcPath / ("mp" + std::to_string(mapperNumber) + ".lua");
        open(path);
    }

public:
    inline u8 readPRG(u16 addr) {
        const u32 mappedAddr = (!hasReadPRG) ? addr : callFunc(IDX_READ_PRG, addr);
        if (mappedAddr != 0xFFFFFFFF)
            return PRG_ROM[mappedAddr];
        return 0;
    }

    inline u8 readCHR(u16 addr) {
        const u32 mappedAddr = (!hasReadCHR) ? addr : callFunc(IDX_READ_CHR, addr);
        if (mappedAddr != 0xFFFFFFFF)
            return CHR_ROM[mappedAddr];
        return 0;
    }

    inline u8 readRAM(u16 addr) { return PRG_RAM[addr & 0x1FFF]; }


    inline void writePRG(u16 addr, u8 value) {
        const u32 mappedAddr = (!hasWritePRG) ? addr : callFunc(IDX_WRITE_PRG, addr, value);
        if (mappedAddr != 0xFFFFFFFF) {
            PRG_ROM[mappedAddr] = value;
        }
    }

    inline void writeCHR(u16 addr, u8 value) {
        const u32 mappedAddr = (!hasWriteCHR) ? addr : callFunc(IDX_WRITE_CHR, addr, value);
        if (mappedAddr != 0xFFFFFFFF) {
            CHR_ROM[mappedAddr] = value;
        }
    }

    inline void writeRAM(u16 addr, u8 value) { PRG_RAM[addr & 0x1FFF] = value; }

};
