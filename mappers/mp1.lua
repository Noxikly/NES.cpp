-- Mapper 1 (MMC1) --
local bit = require("bit")
local band, bor, rshift, lshift = bit.band, bit.bor, bit.rshift, bit.lshift


local mp1 = {}

function mp1:init()
    self.shiftReg = 0x10
    self.control = 0x0C

    self.chrBank0 = 0
    self.chrBank1 = 0
    self.prgBank = 0

    self.prgBankCount = prgSize / 0x4000
    self.chrBankCount = chrSize / 0x1000

    self.chrRAM = chrSize == 0
    if self.chrRAM then
        CHRresize(0x2000)
        self.chrBankCount = 2
    end
end

function mp1:readPRGAddr(addr)
    addr = addr - 0x8000

    local prgMode = band(rshift(self.control, 2), 0x03)
    local bankOffset

    if prgMode <= 1 then
        -- 32KB mode
        bankOffset = band(self.prgBank, 0xFE) * 0x4000
        return (bankOffset + addr) % prgSize
    elseif prgMode == 2 then
        if addr < 0x4000 then
            return addr
        else
            bankOffset = self.prgBank * 0x4000
            return (bankOffset + (addr - 0x4000)) % prgSize
        end
    else
        if addr < 0x4000 then
            bankOffset = self.prgBank * 0x4000
            return (bankOffset + addr) % prgSize
        else
            local lastBank = (self.prgBankCount - 1) * 0x4000
            return lastBank + (addr - 0x4000)
        end
    end
end

function mp1:writePRGAddr(addr, value)
    if band(value, 0x80) ~= 0 then
        self.shiftReg = 0x10
        self.control = bor(self.control, 0x0C)
        return nil
    end

    local fill = band(self.shiftReg, 1) == 1
    self.shiftReg = rshift(self.shiftReg, 1)
    self.shiftReg = bor(self.shiftReg, lshift(band(value, 1), 4))

    if fill then
        local reg = band(rshift(addr, 13), 0x03)

        if reg == 0 then
            self.control = self.shiftReg
        elseif reg == 1 then
            self.chrBank0 = self.shiftReg
        elseif reg == 2 then
            self.chrBank1 = self.shiftReg
        else
            self.prgBank = band(self.shiftReg, 0x0F)
        end

        self.shiftReg = 0x10
    end

    return nil
end

function mp1:readCHRAddr(addr)
    addr = band(addr, 0x1FFF)

    local chrMode = band(rshift(self.control, 4), 0x01)
    local bankOffset
    local finalAddr = addr

    if chrMode == 0 then
        -- 8KB mode
        bankOffset = band(self.chrBank0, 0xFE) * 0x1000
    else
        -- 4KB mode
        if addr < 0x1000 then
            bankOffset = self.chrBank0 * 0x1000
        else
            bankOffset = self.chrBank1 * 0x1000
            finalAddr = addr - 0x1000
        end
    end

    return (bankOffset + finalAddr) % (self.chrBankCount * 0x1000)
end

function mp1:writeCHRAddr(addr, value)
    if not self.chrRAM then
        return nil
    end
    return self:readCHRAddr(addr)
end

return mp1
