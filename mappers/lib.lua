local ffi = require("ffi")
local bit = require("bit")


ffi.cdef [[
    typedef uint8_t u8;
    typedef uint16_t u16;
    typedef uint32_t u32;


    typedef struct {
        u8* _M_start;
        u8* _M_finish;
        u8* _M_end_of_storage;
    } vector_u8;


    typedef struct {
        vector_u8 PRG_ROM;
        vector_u8 PRG_RAM;
        vector_u8 CHR_ROM;
        u8 mirrorMode;
        u16 mapperNumber;
        bool irqFlag;
    } Cartridge;


    enum VectorType {
        VEC_PRG_ROM = 0,
        VEC_PRG_RAM = 1,
        VEC_CHR_ROM = 2,
    };


    void Cartridge_resize(void* instance, int vecType, size_t size);
    void Cartridge_setMirror(void* instance, u8 mode);
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
M.MIRROR_SINGLE_SCREEN_A = 2
M.MIRROR_SINGLE_SCREEN_B = 3
M.MIRROR_FOUR_SCREEN = 4

-- Битовые операции

M.band = bit.band
M.bor = bit.bor
M.bxor = bit.bxor
M.bnot = bit.bnot
M.lshift = bit.lshift
M.rshift = bit.rshift
M.arshift = bit.arshift

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
function M.readPRGDirect(addr)
    if addr < prgSize then
        return cart.PRG_ROM._M_start[addr]
    end
    return 0
end

-- Запись в PRG
function M.writePRGDirect(addr, value)
    if addr < prgSize then
        cart.PRG_ROM._M_start[addr] = value
    end
end

-- Прямое чтение/запись CHR_ROM

-- Чтение из CHR
function M.readCHRDirect(addr)
    if addr < chrSize then
        return cart.CHR_ROM._M_start[addr]
    end
    return 0
end

-- Запись в CHR
function M.writeCHRDirect(addr, value)
    if addr < chrSize then
        cart.CHR_ROM._M_start[addr] = value
    end
end

-- Прямое чтение/запись PRG_RAM

-- чтение из PRG_RAM
function M.readRAMDirect(addr)
    addr = M.band(addr, 0x1FFF)
    if addr < prgRamSize then
        return cart.PRG_RAM._M_start[addr]
    end
    return 0
end

-- запись в PRG_RAM
function M.writeRAMDirect(addr, value)
    addr = M.band(addr, 0x1FFF)
    if addr < prgRamSize then
        cart.PRG_RAM._M_start[addr] = value
    end
end

-- Хелперы для работы с битами

function M.getBit(value, bit)
    return M.band(M.rshift(value, bit), 1)
end

function M.setBit(value, bit)
    return M.bor(value, M.lshift(1, bit))
end

function M.clearBit(value, bit)
    return M.band(value, M.bnot(M.lshift(1, bit)))
end

function M.toggleBit(value, bit)
    return M.bxor(value, M.lshift(1, bit))
end

function M.testBit(value, bit)
    return M.band(value, M.lshift(1, bit)) ~= 0
end

return M
