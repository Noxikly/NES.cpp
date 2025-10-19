#pragma once
#include "cartridge.hpp"

class Mapper {
public:
    enum MapperType : u8 {
        NROM = 0,
    };

protected:
    explicit Mapper(Cartridge& c, MapperType t)
                    : cartridge(c), type(t){}
    ~Mapper() = default;

public:
    virtual u8 readPRG(u16 addr) = 0;
    virtual void writePRG(u16 addr, u8 value) = 0;

    virtual u8 readCHR(u16 addr) = 0;
    virtual void writeCHR(u16 addr, u8 value) = 0;

protected:
    Cartridge &cartridge;
    MapperType type;
};
