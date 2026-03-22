#include <stddef.h>

#include "common/types.h"

#include "core/cartridge.h"
#include "core/lua.h"

API_EXPORT void Cartridge_resize(void *instance, u8 vecType, size_t size) {
    auto *cart = static_cast<Core::Cartridge *>(instance);

    switch (vecType) {
    case 0: /* VEC_PRG_ROM */
        cart->PRG_ROM.resize(size);
        break;
    case 1: /* VEC_PRG_RAM */
        cart->PRG_RAM.resize(size);
        break;
    case 2: /* VEC_CHR_ROM */
        cart->CHR_ROM.resize(size);
        break;
    default:
        break;
    }
}

API_EXPORT void Cartridge_setMirror(void *instance, u8 mode) {
    static_cast<Core::Cartridge *>(instance)->mirror =
        static_cast<Core::Cartridge::MirrorMode>(mode);
}

API_EXPORT void Cartridge_triggerIRQ(void *instance) {
    static_cast<Core::Cartridge *>(instance)->irqFlag = true;
}

API_EXPORT void Cartridge_clearIRQ(void *instance) {
    static_cast<Core::Cartridge *>(instance)->irqFlag = false;
}
