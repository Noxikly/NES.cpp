-- Mapper 4 (MMC3) --
local lib = require("lib")

local mp4 = {}

function mp4:init()
    self.cntPRG = prgSize/0x2000
    self.cntCHR = chrSize/0x0400
    self.chrRAM = chrSize==0
    
    if self.chrRAM then
        lib.resizeCHR(0x2000)
        self.cntCHR = 8
    end
    
    -- R0-R7 регистры
    self.regs = {0, 0, 0, 0, 0, 0, 0, 0}
    self.bankSelect = 0
    self.modePRG = false
    self.modeCHR = false
    
    -- IRQ
    self.irqLatch = 0
    self.irqCounter = 0
    self.irqReload = false
    self.irqEnabled = false
    
    lib.clearIRQ()
end

function mp4:getPRGBank(addr)
    addr = addr - 0x8000
    local bankIdx = lib.bit_rshift(addr, 13)
    
    local r6 = lib.maskBank(self.regs[7], self.cntPRG)
    local r7 = lib.maskBank(self.regs[8], self.cntPRG)
    local lastSecond = lib.maskBank(self.cntPRG-2, self.cntPRG)
    local last = lib.maskBank(self.cntPRG-1, self.cntPRG)
    
    if not self.modePRG then
        if bankIdx==0 then return r6
        elseif bankIdx==1 then return r7
        elseif bankIdx==2 then return lastSecond
        else return last
        end
    else
        if bankIdx==0 then return lastSecond
        elseif bankIdx==1 then return r7
        elseif bankIdx==2 then return r6
        else return last
        end
    end
end

function mp4:getCHRBank(addr)
    local bankIdx = lib.bit_rshift(lib.bit_and(addr, 0x1FFF), 10)
    

    local r0 = lib.bit_and(self.regs[1], 0xFE)
    local r1 = lib.bit_and(self.regs[2], 0xFE)
    local r2 = self.regs[3]
    local r3 = self.regs[4]
    local r4 = self.regs[5]
    local r5 = self.regs[6]
    
    -- Применяем маску
    r0 = lib.maskBank(r0, self.cntCHR)
    r1 = lib.maskBank(r1, self.cntCHR)
    r2 = lib.maskBank(r2, self.cntCHR)
    r3 = lib.maskBank(r3, self.cntCHR)
    r4 = lib.maskBank(r4, self.cntCHR)
    r5 = lib.maskBank(r5, self.cntCHR)
    
    if not self.modeCHR then
        if bankIdx == 0 then return r0
        elseif bankIdx == 1 then return r0+1
        elseif bankIdx == 2 then return r1
        elseif bankIdx == 3 then return r1+1
        elseif bankIdx == 4 then return r2
        elseif bankIdx == 5 then return r3
        elseif bankIdx == 6 then return r4
        else return r5
        end
    else
        if bankIdx == 0 then return r2
        elseif bankIdx == 1 then return r3
        elseif bankIdx == 2 then return r4
        elseif bankIdx == 3 then return r5
        elseif bankIdx == 4 then return r0
        elseif bankIdx == 5 then return r0+1
        elseif bankIdx == 6 then return r1
        else return r1+1
        end
    end
end

function mp4:readPRGAddr(addr)
    local bank = self:getPRGBank(addr)
    local offset = lib.bit_and(addr, 0x1FFF)
    return bank * 0x2000 + offset
end

function mp4:writePRGAddr(addr, value)
    if addr < 0x8000 then return nil end
    
    local evenAddr = lib.bit_and(addr, 0x01) == 0
    
    if addr >= 0x8000 and addr <= 0x9FFF then
        if evenAddr then
            self.bankSelect = lib.bit_and(value, 0x07)
            self.modePRG = lib.bit_and(value, 0x40) ~= 0
            self.modeCHR = lib.bit_and(value, 0x80) ~= 0
        else
            local v = lib.bit_and(value, 0xFF)
            local r = self.bankSelect

            if r == 0 or r == 1 then
                v = lib.bit_and(v, 0xFE)
            elseif r == 6 or r == 7 then
                v = lib.bit_and(v, 0x3F)
            end

            self.regs[r + 1] = v
        end
        
    elseif addr >= 0xA000 and addr <= 0xBFFF then
        if evenAddr then
            -- 0 = vertical, 1 = horizontal
            local mode = lib.bit_and(value, 0x01) == 0 and lib.MIRROR_VERTICAL or lib.MIRROR_HORIZONTAL
            lib.setMirror(mode)
        end
        
    elseif addr >= 0xC000 and addr <= 0xDFFF then
        -- IRQ latch / IRQ reload
        if evenAddr then
            -- $C000: IRQ latch
            self.irqLatch = value
        else
            -- $C001: IRQ reload
            self.irqCounter = 0
            self.irqReload = true
        end
        
    else  -- 0xE000-0xFFFF
        -- IRQ disable / IRQ enable
        if evenAddr then
            -- $E000: IRQ disable
            self.irqEnabled = false
            lib.clearIRQ()
        else
            -- $E001: IRQ enable
            self.irqEnabled = true
        end
    end
    
    return nil
end

function mp4:readCHRAddr(addr)
    local bank = self:getCHRBank(addr)
    local offset = lib.bit_and(addr, 0x03FF)
    return bank*0x0400 + offset
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
        self.irqCounter = self.irqCounter-1
    end
    
    if self.irqCounter== 0 and self.irqEnabled then
        lib.triggerIRQ()
    end
end

return mp4