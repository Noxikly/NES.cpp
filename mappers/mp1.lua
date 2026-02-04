-- Mapper 1 (MMC1) --
local lib = require("lib")

local mp1 = {}

local function updateMirror(ctrl)
    local mode = lib.bit_and(ctrl, 0x03)
    if mode == 0 then
        lib.setMirror(lib.MIRROR_SINGLE_SCREEN_A)
    elseif mode == 1 then
        lib.setMirror(lib.MIRROR_SINGLE_SCREEN_B)
    elseif mode == 2 then
        lib.setMirror(lib.MIRROR_VERTICAL)
    else
        lib.setMirror(lib.MIRROR_HORIZONTAL)
    end
end

function mp1:init()
    self.shiftReg = 0x10
    self.ctrl = 0x0C

    self.chrBank0 = 0
    self.chrBank1 = 0
    self.prgBank = 0

    self.prgBankCount = prgSize / 0x4000
    self.chrBankCount = chrSize / 0x1000

    self.chrRAM = chrSize == 0
    if self.chrRAM then
        lib.resizeCHR(0x2000)
        self.chrBankCount = 2
    end

    updateMirror(self.ctrl)
end


function mp1:readPRGAddr(addr)
    addr = addr - 0x8000

    local prgMode = lib.bit_and(lib.bit_rshift(self.ctrl, 2), 0x03)
    local bankOffset


    if prgMode <= 1 then
        bankOffset = lib.bit_and(self.prgBank, 0xFE) * 0x4000
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
    if lib.bit_and(value, 0x80) ~= 0 then
        self.shiftReg = 0x10
        self.ctrl = lib.bit_or(self.ctrl, 0x0C)
        updateMirror(self.ctrl)
        return nil
    end

    local fill = lib.bit_and(self.shiftReg, 1) == 1
    self.shiftReg = lib.bit_rshift(self.shiftReg, 1)
    self.shiftReg = lib.bit_or(self.shiftReg, lib.bit_lshift(lib.bit_and(value, 1), 4))

    if fill then
        local reg = lib.bit_and(lib.bit_rshift(addr, 13), 0x03)

        if reg == 0 then
            self.ctrl = self.shiftReg
            updateMirror(self.ctrl)
        elseif reg == 1 then
            self.chrBank0 = self.shiftReg
        elseif reg == 2 then
            self.chrBank1 = self.shiftReg
        else
            self.prgBank = lib.bit_and(self.shiftReg, 0x0F)
        end

        self.shiftReg = 0x10
    end

    return nil
end


function mp1:readCHRAddr(addr)
    addr = lib.bit_and(addr, 0x1FFF)

    local chrMode = lib.bit_and(lib.bit_rshift(self.ctrl, 4), 0x01)
    local bankOffset
    local finalAddr = addr

    if chrMode == 0 then
        bankOffset = lib.bit_and(self.chrBank0, 0xFE) * 0x1000
    else
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