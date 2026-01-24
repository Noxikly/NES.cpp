-- Mapper 4 (MMC3) --
local bit = require("bit")
local band, rshift = bit.band, bit.rshift


local mp4 = {}

function mp4:init()
    self.cntPRG = prgSize / 0x2000
    self.cntCHR = chrSize / 0x0400

    self.chrRAM = chrSize == 0
    if self.chrRAM then
        CHRresize(0x2000)
        self.cntCHR = 8
    end

    self.regs = {0, 2, 4, 5, 6, 7, 0, 1}
    self.bankSelect = 0
    self.modePRG = false
    self.modeCHR = false

    self.irqLatch = 0
    self.irqCounter = 0
    self.irqReload = false
    self.irqEnabled = false
end

function mp4:getPRGBank(addr)
    addr = addr - 0x8000
    local bankIndex = band(rshift(addr, 13), 0x03)

    if not self.modePRG then
        if bankIndex == 0 then return self.regs[7] % self.cntPRG
        elseif bankIndex == 1 then return self.regs[8] % self.cntPRG
        elseif bankIndex == 2 then return (self.cntPRG - 2) % self.cntPRG
        else return (self.cntPRG - 1) % self.cntPRG
        end
    else
        if bankIndex == 0 then return (self.cntPRG - 2) % self.cntPRG
        elseif bankIndex == 1 then return self.regs[8] % self.cntPRG
        elseif bankIndex == 2 then return self.regs[7] % self.cntPRG
        else return (self.cntPRG - 1) % self.cntPRG
        end
    end
end

function mp4:getCHRBank(addr)
    addr = band(addr, 0x1FFF)
    local bankIndex = band(rshift(addr, 10), 0x07)

    if not self.modeCHR then
        if bankIndex == 0 or bankIndex == 1 then
            return (band(self.regs[1], 0xFE) + band(bankIndex, 1)) % self.cntCHR
        elseif bankIndex == 2 or bankIndex == 3 then
            return (band(self.regs[2], 0xFE) + band(bankIndex, 1)) % self.cntCHR
        elseif bankIndex == 4 then return self.regs[3] % self.cntCHR
        elseif bankIndex == 5 then return self.regs[4] % self.cntCHR
        elseif bankIndex == 6 then return self.regs[5] % self.cntCHR
        else return self.regs[6] % self.cntCHR
        end
    else
        if bankIndex == 0 then return self.regs[3] % self.cntCHR
        elseif bankIndex == 1 then return self.regs[4] % self.cntCHR
        elseif bankIndex == 2 then return self.regs[5] % self.cntCHR
        elseif bankIndex == 3 then return self.regs[6] % self.cntCHR
        elseif bankIndex == 4 or bankIndex == 5 then
            return (band(self.regs[1], 0xFE) + band(bankIndex, 1)) % self.cntCHR
        else
            return (band(self.regs[2], 0xFE) + band(bankIndex, 1)) % self.cntCHR
        end
    end
end

function mp4:readPRGAddr(addr)
    local bank = self:getPRGBank(addr)
    local offset = band(addr - 0x8000, 0x1FFF)
    return bank * 0x2000 + offset
end

function mp4:writePRGAddr(addr, value)
    if addr < 0x8000 then return nil end

    local evenAddr = band(addr, 0x01) == 0

    if addr <= 0x9FFF then
        if evenAddr then
            self.bankSelect = band(value, 0x07)
            self.modePRG = band(value, 0x40) ~= 0
            self.modeCHR = band(value, 0x80) ~= 0
        else
            self.regs[self.bankSelect + 1] = value
        end
    elseif addr <= 0xBFFF then
        if evenAddr then
            SetMirror(band(value, 0x01) == 1 and 0 or 1)
        end
    elseif addr <= 0xDFFF then
        if evenAddr then
            self.irqLatch = value
        else
            self.irqReload = true
        end
    else
        self.irqEnabled = not evenAddr
        if evenAddr then
            ClearIRQ()
        end
    end

    return nil
end

function mp4:readCHRAddr(addr)
    addr = band(addr, 0x1FFF)
    local bank = self:getCHRBank(addr)
    local offset = band(addr, 0x03FF)
    return bank * 0x0400 + offset
end

function mp4:writeCHRAddr(addr, value)
    if not self.chrRAM then return nil end
    return self:readCHRAddr(addr)
end

function mp4:step()
    if self.irqCounter == 0 or self.irqReload then
        self.irqCounter = self.irqLatch
        self.irqReload = false
    else
        self.irqCounter = self.irqCounter - 1
    end

    if self.irqCounter == 0 and self.irqEnabled then
        TriggerIRQ()
    end
end

return mp4
