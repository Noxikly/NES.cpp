#pragma once
#include <string>
#include "lua.hpp"


class Mapper : public Lua {
public:
    Mapper() = default;

    void load(const std::string& srcPath = "mappers/") {
        std::string path = srcPath + "mp" + std::to_string(mapperNumber) + ".lua";
        open(path);
    }

public:
    inline u8 readPRG(u16 addr) {
        u32 mappedAddr = readPRGAddr(addr);
        if (mappedAddr != 0xFFFFFFFF)
            return PRG_ROM[mappedAddr];
        return 0;
    }

    inline u8 readCHR(u16 addr) {
        u32 mappedAddr = readCHRAddr(addr);
        if (mappedAddr != 0xFFFFFFFF)
            return CHR_ROM[mappedAddr];
        return 0;
    }

    inline u8 readRAM(u16 addr) { return PRG_RAM[addr & 0x1FFF]; }


    inline void writePRG(u16 addr, u8 value) {
        u32 mappedAddr = writePRGAddr(addr, value);
        if (mappedAddr != 0xFFFFFFFF) {
            PRG_ROM[mappedAddr] = value;
        }
    }

    inline void writeCHR(u16 addr, u8 value) {
        u32 mappedAddr = writeCHRAddr(addr, value);
        if (mappedAddr != 0xFFFFFFFF) {
            CHR_ROM[mappedAddr] = value;
        }
    }

    inline void writeRAM(u16 addr, u8 value) { PRG_RAM[addr & 0x1FFF] = value; }

};
