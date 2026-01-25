-- Mapper 3 (CNROM) --
local lib = require("lib")

local mp3 = {}

function mp3:init()
    self.chrBank = 0
    self.prgMask = prgSize == 0x4000 and 0x3FFF or 0x7FFF
    if chrSize == 0 then
        lib.resizeCHR(0x2000)
        self.chrBankMask = 0
    else
        local chrBankCount = chrSize / 0x2000
        self.chrBankMask = chrBankCount-1
    end
end

function mp3:readPRGAddr(addr)
    return lib.band(addr-0x8000, self.prgMask)
end

function mp3:writePRGAddr(addr, value)
    self.chrBank = lib.band(value, self.chrBankMask)
    return nil
end

function mp3:readCHRAddr(addr)
    local bankOffset = self.chrBank*0x2000
    return bankOffset+addr
end

function mp3:writeCHRAddr(addr, value)
    if chrSize == 0 then
        return addr
    end
    return nil
end

return mp3