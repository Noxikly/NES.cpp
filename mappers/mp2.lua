-- Mapper 2 (UxROM) --
local lib = require("lib")

local mp2 = {}

function mp2:init()
    self.prgBank = 0
    self.cntBank = prgSize / 0x4000
    self.chrRAM  = chrSize == 0

    if self.chrRAM then
        lib.resizeCHR(0x2000)
    end
end

function mp2:readPRGAddr(addr)
    addr = addr - 0x8000
    
    if addr < 0x4000 then
        local bankOffset = self.prgBank * 0x4000
        return bankOffset + addr
    else
        local lastBankOffset = (self.cntBank - 1) * 0x4000
        return lastBankOffset + (addr - 0x4000)
    end
end

function mp2:writePRGAddr(addr, value)
    self.prgBank = lib.bit_and(value, self.cntBank - 1)
    return nil
end

function mp2:readCHRAddr(addr)
    return lib.bit_and(addr, 0x1FFF)
end

function mp2:writeCHRAddr(addr, value)
    if self.chrRAM then
        return lib.bit_and(addr, 0x1FFF)
    end
    return nil
end

return mp2