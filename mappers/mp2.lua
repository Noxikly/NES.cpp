-- Mapper 2 (UxROM) --
local bit = require("bit")
local band = bit.band


local mp2 = {}

function mp2:init()
    self.prgBank = 0
    self.cntBank = prgSize / 0x4000
    if chrSize == 0 then
        CHRresize(0x2000)
    end
end

function mp2:readPRGAddr(addr)
    addr = addr - 0x8000

    if addr < 0x4000 then
        local bankOffset = self.prgBank * 0x4000
        return (bankOffset + addr) % prgSize
    else
        local lastBank = (self.cntBank - 1) * 0x4000
        return lastBank + (addr - 0x4000)
    end
end

function mp2:writePRGAddr(addr, value)
    self.prgBank = band(value, 0x0F)
    return nil
end

function mp2:readCHRAddr(addr)
    return band(addr, 0x1FFF)
end

function mp2:writeCHRAddr(addr, value)
    return band(addr, 0x1FFF)
end

return mp2
