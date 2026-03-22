local ffi = require("ffi")
local bit = require("bit")


ffi.cdef [[
    void Cartridge_resize(void* instance, uint8_t vecType, size_t size);
    void Cartridge_setMirror(void* instance, uint8_t mode);
    void Cartridge_triggerIRQ(void* instance);
    void Cartridge_clearIRQ(void* instance);
]]


local M = {}


M.VEC_PRG_ROM = 0 -- константа PRG_ROM
M.VEC_PRG_RAM = 1 -- константа PRG_RAM
M.VEC_CHR_ROM = 2 -- константа CHR_ROM

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

-- API для работы с картриджем

-- Маска банка (ограничивает банк до допустимого диапазона)
function M.maskBank(bank, count)
    if count == 0 then return 0 end
    return M.bit_and(bank, count-1)
end


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

return M
