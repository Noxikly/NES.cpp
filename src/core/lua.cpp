#include "lua.hpp"


enum VecType : int
{
    VEC_PRG_ROM = 0,
    VEC_PRG_RAM = 1,
    VEC_CHR_ROM = 2,
};

API_EXPORT void Cartridge_resize(void* instance, u8 vecType, size_t size) {
    auto* cart = static_cast<Cartridge*>(instance);

    switch (vecType) {
        case VEC_PRG_ROM: cart->PRG_ROM.resize(size); break;
        case VEC_PRG_RAM: cart->PRG_RAM.resize(size); break;
        case VEC_CHR_ROM: cart->CHR_ROM.resize(size); break;
        default: break;
    }
}

API_EXPORT void Cartridge_setMirror(void* instance, u8 mode) {
    static_cast<Cartridge*>(instance)->mirrorMode = mode;
}

API_EXPORT void Cartridge_triggerIRQ(void* instance) {
    static_cast<Cartridge*>(instance)->irqFlag = true;
}

API_EXPORT void Cartridge_clearIRQ(void* instance) {
    static_cast<Cartridge*>(instance)->irqFlag = false;
}
