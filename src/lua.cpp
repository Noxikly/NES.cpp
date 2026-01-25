#include "lua.hpp"


enum VecType : int
{
    VEC_PRG_ROM = 0,
    VEC_PRG_RAM = 1,
    VEC_CHR_ROM = 2,
};

extern "C"
{
    void Cartridge_resize(void* instance, int vecType, size_t size) {
        auto* cart = static_cast<Cartridge*>(instance);

        switch (vecType) {
            case VEC_PRG_ROM: cart->PRG_ROM.resize(size); break;
            case VEC_PRG_RAM: cart->PRG_RAM.resize(size); break;
            case VEC_CHR_ROM: cart->CHR_ROM.resize(size); break;
            default: break;
        }
    }

    void Cartridge_setMirror(void* instance, u8 mode) {
        static_cast<Cartridge*>(instance)->mirrorMode = mode;
    }

    void Cartridge_triggerIRQ(void* instance) {
        static_cast<Cartridge*>(instance)->irqFlag = true;
    }

    void Cartridge_clearIRQ(void* instance) {
        static_cast<Cartridge*>(instance)->irqFlag = false;
    }
} /* extern "C" */
