-- Mapper 3 (CNROM) --
local bit = require("bit")
local band = bit.band


local mp3 = {}

function mp3:init()
    self.chrBank = 0
    self.prgMask = prgSize == 0x4000 and 0x3FFF or 0x7FFF
end

function mp3:readPRGAddr(addr)
    return band(addr - 0x8000, self.prgMask)
end

function mp3:writePRGAddr(addr, value)
    self.chrBank = band(value, 0x03)
    return nil
end

function mp3:readCHRAddr(addr)
    local bankOffset = self.chrBank * 0x2000
    return (bankOffset + addr) % chrSize
end

function mp3:writeCHRAddr(addr, value)
    return nil
end

return mp3
