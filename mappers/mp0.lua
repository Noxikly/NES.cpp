-- Mapper 0 (NROM) --
local lib = require("lib")

local mp0 = {}

function mp0:init()
    self.cntBank = prgSize == 0x4000 and 1 or 0
    if chrSize == 0 then
        lib.resizeCHR(0x2000)
    end
end

function mp0:readPRGAddr(addr)
    addr = addr - 0x8000
    if self.cntBank == 1 then
        addr = lib.band(addr, 0x3FFF)
    end
    return addr
end

function mp0:readCHRAddr(addr)
    return lib.band(addr, 0x1FFF)
end

function mp0:writeCHRAddr(addr, value)
    if chrSize == 0 then
        return lib.band(addr, 0x1FFF)
    end
    return nil
end

return mp0
