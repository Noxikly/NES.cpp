-- Mapper 9 (MMC2) --
local lib = require("lib")

local mp9 = {}

function mp9:init()
    self.prgBankCount = prgSize / 0x2000
    self.chrBankCount = chrSize / 0x1000

    self.chrRAM = chrSize == 0
    if self.chrRAM then
        lib.resizeCHR(0x2000)
        self.chrBankCount = 2
    end

    self.prgBank = 0

    self.chrFD0 = 0
    self.chrFE0 = 0
    self.chrFD1 = 0
    self.chrFE1 = 0

    self.latch0 = 0xFD
    self.latch1 = 0xFD
end

function mp9:readPRGAddr(addr)
    addr = addr - 0x8000

    local bank
    local offset = lib.bit_and(addr, 0x1FFF)
    local region = lib.bit_rshift(addr, 13) -- 0..3 для 8КБ

    if region == 0 then
        bank = lib.maskBank(self.prgBank, self.prgBankCount)
    else
        local fixedBase = self.prgBankCount - 3
        if fixedBase < 0 then
            fixedBase = 0
        end
        bank = fixedBase + (region - 1)
        bank = lib.maskBank(bank, self.prgBankCount)
    end

    return bank * 0x2000 + offset
end

function mp9:writePRGAddr(addr, value)
    if addr >= 0xA000 and addr <= 0xAFFF then
        -- CHR $8000-$9FFF
        self.prgBank = lib.bit_and(value, 0x0F)
    elseif addr >= 0xB000 and addr <= 0xBFFF then
        -- CHR $0000-$0FFF при latch0 == $FD
        self.chrFD0 = lib.bit_and(value, 0x1F)
    elseif addr >= 0xC000 and addr <= 0xCFFF then
        -- CHR $0000-$0FFF при latch0 == $FE
        self.chrFE0 = lib.bit_and(value, 0x1F)
    elseif addr >= 0xD000 and addr <= 0xDFFF then
        -- CHR $1000-$1FFF при latch1 == $FD
        self.chrFD1 = lib.bit_and(value, 0x1F)
    elseif addr >= 0xE000 and addr <= 0xEFFF then
        -- CHR $1000-$1FFF при latch1 == $FE
        self.chrFE1 = lib.bit_and(value, 0x1F)
    elseif addr >= 0xF000 then
        -- Mirroring: 0 vertical, 1 horizontal
        if lib.bit_and(value, 1) == 0 then
            lib.setMirror(lib.MIRROR_VERTICAL)
        else
            lib.setMirror(lib.MIRROR_HORIZONTAL)
        end
    end

    return nil
end

local function latchUpdate(self, addr)
    -- MMC2 latch тригеры
    if addr == 0x0FD8 then
        self.latch0 = 0xFD
    elseif addr == 0x0FE8 then
        self.latch0 = 0xFE
    elseif addr >= 0x1FD8 and addr <= 0x1FDF then
        self.latch1 = 0xFD
    elseif addr >= 0x1FE8 and addr <= 0x1FEF then
        self.latch1 = 0xFE
    end
end

function mp9:readCHRAddr(addr)
    addr = lib.bit_and(addr, 0x1FFF)

    local bank
    local offset

    if addr < 0x1000 then
        bank = (self.latch0 == 0xFD) and self.chrFD0 or self.chrFE0
        bank = lib.maskBank(bank, self.chrBankCount)
        offset = addr
    else
        bank = (self.latch1 == 0xFD) and self.chrFD1 or self.chrFE1
        bank = lib.maskBank(bank, self.chrBankCount)
        offset = addr - 0x1000
    end

    local mapped = bank * 0x1000 + offset
    latchUpdate(self, addr)

    return mapped
end

function mp9:writeCHRAddr(addr, value)
    if not self.chrRAM then
        return nil
    end
    return self:readCHRAddr(addr)
end

return mp9
