-- Mapper 3 (CNROM) --
local lib = require("lib")

local mp3 = {}

function mp3:init()
    self.chrBank = 0
    self.prgMask = prgSize == 0x4000 and 0x3FFF or 0x7FFF
    self.chrRAM  = chrSize == 0

    if self.chrRAM then
        lib.resizeCHR(0x2000)
        self.chrBankCount = 1
    else
        self.chrBankCount = chrSize / 0x2000
    end
end

function mp3:readPRGAddr(addr)
    return lib.bit_and(addr-0x8000, self.prgMask)
end

function mp3:writePRGAddr(addr, value)
    self.chrBank = lib.maskBank(value, self.chrBankCount)
    return nil
end

function mp3:readCHRAddr(addr)
    local bankOffset = self.chrBank*0x2000
    return bankOffset+addr
end

function mp3:writeCHRAddr(addr, value)
    if self.chrRAM then
        return addr
    end
    return nil
end

return mp3