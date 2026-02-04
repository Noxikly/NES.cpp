local ffi = require("ffi")
local bit = require("bit")


ffi.cdef [[
    typedef struct {
        uint8_t* _M_start;
        uint8_t* _M_finish;
        uint8_t* _M_end_of_storage;
    } vector_u8;


    typedef struct {
        vector_u8 PRG_ROM;
        vector_u8 PRG_RAM;
        vector_u8 CHR_ROM;
        uint8_t mirrorMode;
        uint16_t mapperNumber;
        bool irqFlag;
    } Cartridge;


    enum VectorType {
        VEC_PRG_ROM = 0,
        VEC_PRG_RAM = 1,
        VEC_CHR_ROM = 2,
    };


    void Cartridge_resize(void* instance, uint8_t vecType, size_t size);
    void Cartridge_setMirror(void* instance, uint8_t mode);
    void Cartridge_triggerIRQ(void* instance);
    void Cartridge_clearIRQ(void* instance);
]]


local M = {}


M.VEC_PRG_ROM = ffi.C.VEC_PRG_ROM -- константа PRG_ROM
M.VEC_PRG_RAM = ffi.C.VEC_PRG_RAM -- константа PRG_RAM
M.VEC_CHR_ROM = ffi.C.VEC_CHR_ROM -- константа CHR_ROM

-- Константы зеркал

M.MIRROR_HORIZONTAL = 0
M.MIRROR_VERTICAL = 1
M.MIRROR_FOUR_SCREEN = 2
M.MIRROR_SINGLE_SCREEN_A = 3
M.MIRROR_SINGLE_SCREEN_B = 4

-- Битовые операции

M.bit_and = bit.band
M.bit_or = bit.bor
M.bit_xor = bit.bxor
M.bit_not = bit.bnot
M.bit_lshift = bit.lshift
M.bit_rshift = bit.rshift
M.bit_arshift = bit.arshift

-- Получаем указатель на Cartridge
local cart = ffi.cast("Cartridge*", __instance)
M.cart = cart


-- API для работы с картриджем

-- изменить размер вектора
function M.resize(vectorType, size)
    ffi.C.Cartridge_resize(__instance, vectorType, size)
end

-- Обертки для resize

-- изменить размер PRG_ROM
function M.resizePRG(size)
    M.resize(M.VEC_PRG_ROM, size)
end

-- изменить размер PRG_RAM
function M.resizePRG_RAM(size)
    M.resize(M.VEC_PRG_RAM, size)
end

-- изменить размер CHR_ROM
function M.resizeCHR(size)
    M.resize(M.VEC_CHR_ROM, size)
end

-- установить режим зеркала
function M.setMirror(mode)
    ffi.C.Cartridge_setMirror(__instance, mode)
end

-- установить irqFlag (true)
function M.triggerIRQ()
    ffi.C.Cartridge_triggerIRQ(__instance)
end

-- очистить irqFlag (false)
function M.clearIRQ()
    ffi.C.Cartridge_clearIRQ(__instance)
end

-- получить текущий mirrorMode
function M.getMirror()
    return cart.mirrorMode
end

-- получить irqFlag
function M.getIRQ()
    return cart.irqFlag
end

-- Прямое чтение/запись PRG_ROM

-- Чтение из PRG
function M.readPRG(addr)
    if addr < prgSize then
        return cart.PRG_ROM._M_start[addr]
    end
    return 0
end

-- Запись в PRG
function M.writePRG(addr, value)
    if addr < prgSize then
        cart.PRG_ROM._M_start[addr] = value
    end
end

-- Прямое чтение/запись CHR_ROM

-- Чтение из CHR
function M.readCHR(addr)
    if addr < chrSize then
        return cart.CHR_ROM._M_start[addr]
    end
    return 0
end

-- Запись в CHR
function M.writeCHR(addr, value)
    if addr < chrSize then
        cart.CHR_ROM._M_start[addr] = value
    end
end

-- Прямое чтение/запись PRG_RAM

-- чтение из PRG_RAM
function M.readRAM(addr)
    addr = M.bit_and(addr, 0x1FFF)
    if addr < prgRamSize then
        return cart.PRG_RAM._M_start[addr]
    end
    return 0
end

-- запись в PRG_RAM
function M.writeRAM(addr, value)
    addr = M.bit_and(addr, 0x1FFF)
    if addr < prgRamSize then
        cart.PRG_RAM._M_start[addr] = value
    end
end

return M
