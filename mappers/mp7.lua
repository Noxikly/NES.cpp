-- Mapper 7 (AOROM) --
local lib = require("lib")

local mp7 = {}

function mp7:init()
    self.prgBank = 0
    self.prgBankCount = prgSize / 0x8000
    self.chrRAM = chrSize == 0

    if self.chrRAM then
        lib.resizeCHR(0x2000)
    end
end

function mp7:readPRGAddr(addr)
    addr = addr - 0x8000
    local bankOffset = self.prgBank * 0x8000
    return (bankOffset + addr)
end

function mp7:writePRGAddr(addr, value)
    if addr < 0x8000 then
        return nil
    end

    self.prgBank = lib.bit_and(value, 0x0F)

    if lib.bit_and(value, 0x10) == 0 then
        lib.setMirror(lib.MIRROR_SINGLE_SCREEN_A)
    else
        lib.setMirror(lib.MIRROR_SINGLE_SCREEN_B)
    end

    return nil
end

function mp7:readCHRAddr(addr)
    return lib.bit_and(addr, 0x1FFF)
end

function mp7:writeCHRAddr(addr, value)
    if self.chrRAM then
        return lib.bit_and(addr, 0x1FFF)
    end
    return nil
end

return mp7
