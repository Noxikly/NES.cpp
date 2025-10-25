#include "cpu.hpp"


/* ОФИЦИАЛЬНЫЕ ОПКОДЫ */

/* Хранение/Загрузка */
void Cpu::LDA(u16 addr) {
    regs.A = mem->read(addr);
    set_nz(regs.A);
}

void Cpu::STA(u16 addr) {
    mem->write(addr, regs.A);
}

void Cpu::LDX(u16 addr) {
    regs.X = mem->read(addr);
    set_nz(regs.X);
}

void Cpu::STX(u16 addr) {
    mem->write(addr, regs.X);
}

void Cpu::LDY(u16 addr) {
    regs.Y = mem->read(addr);
    set_nz(regs.Y);
}

void Cpu::STY(u16 addr) {
    mem->write(addr, regs.Y);
}


/* Передача */
void Cpu::TAX() {
    regs.X = regs.A;
    set_nz(regs.X);
}

void Cpu::TXA() {
    regs.A = regs.X;
    set_nz(regs.A);
}

void Cpu::TAY() {
    regs.Y = regs.A;
    set_nz(regs.Y);
}

void Cpu::TYA() {
    regs.A = regs.Y;
    set_nz(regs.A);
}


/* Арифметические операции */
void Cpu::ADC(u16 addr) {
    u16 value = mem->read(addr);
    u16 res = regs.A + value + (regs.P & C);

    set_flag(C, res > 0xFF);
    set_flag(V, (((res ^ regs.A) & (res ^ value)) & N) != 0);
    set_nz(res);

    regs.A = res & 0xFF;
}

void Cpu::SBC(u16 addr) {
    u16 value = mem->read(addr) ^ 0x00FF;
    u16 res = regs.A + value + (regs.P & C);

    set_flag(C, res > 0xFF);
    set_flag(V, (((res ^ regs.A) & (res ^ value)) & N) != 0);
    set_nz(res & 0xFF);

    regs.A = res & 0x00FF;
}

void Cpu::INC(u16 addr) {
    u8 value = mem->read(addr);
    value++;
    set_nz(value);
    mem->write(addr, value);
}

void Cpu::DEC(u16 addr) {
    u8 value = mem->read(addr);
    value--;
    set_nz(value);
    mem->write(addr, value);
}

void Cpu::INX() {
    regs.X++;
    set_nz(regs.X);
}

void Cpu::DEX() {
    regs.X--;
    set_nz(regs.X);
}

void Cpu::INY() {
    regs.Y++;
    set_nz(regs.Y);
}

void Cpu::DEY() {
    regs.Y--;
    set_nz(regs.Y);
}


/* Сдвиг */
void Cpu::ASL(u16 addr) {
    u8 value = mem->read(addr);
    set_flag(C, value & N);
    value <<= 1;
    set_nz(value);
    mem->write(addr, value);
}

void Cpu::LSR(u16 addr) {
    u8 value = mem->read(addr);
    set_flag(C, value & 0x01);
    value >>= 1;
    set_nz(value);
    mem->write(addr, value);
}

void Cpu::ROL(u16 addr) {
    u8 value = mem->read(addr);
    u8 old_c = regs.P & C;
    set_flag(C, value & N);
    value <<= 1;
    value |= old_c;
    set_nz(value);
    mem->write(addr, value);
}

void Cpu::ROR(u16 addr) {
    u8 value = mem->read(addr);
    u8 old_c = regs.P & C;
    set_flag(C, value & 0x01);
    value >>= 1;
    value |= (old_c << 7);
    set_nz(value);
    mem->write(addr, value);
}


/* Сдвиг аккумулятора */
void Cpu::ASL_ACC() {
    set_flag(C, regs.A & N);
    regs.A <<= 1;
    set_nz(regs.A);
}

void Cpu::LSR_ACC() {
    set_flag(C, regs.A & 0x01);
    regs.A >>= 1;
    set_nz(regs.A);
}

void Cpu::ROL_ACC() {
    u8 old_c = regs.P & C;
    set_flag(C, regs.A & N);
    regs.A <<= 1;
    regs.A |= old_c;
    set_nz(regs.A);
}

void Cpu::ROR_ACC() {
    u8 old_c = regs.P & C;
    set_flag(C, regs.A & 0x01);
    regs.A >>= 1;
    regs.A |= (old_c << 7);
    set_nz(regs.A);
}


/* Логические операции */
void Cpu::AND(u16 addr) {
    regs.A &= mem->read(addr);
    set_nz(regs.A);
}

void Cpu::ORA(u16 addr) {
    regs.A |= mem->read(addr);
    set_nz(regs.A);
}

void Cpu::EOR(u16 addr) {
    regs.A ^= mem->read(addr);
    set_nz(regs.A);
}

void Cpu::BIT(u16 addr) {
    u8 value = mem->read(addr);
    set_flag(N, value & N);
    set_flag(V, value & V);
    set_flag(Z, (regs.A & value) == 0);
}


/* Сравнение */
void Cpu::CMP(u16 addr) {
    u8 value = mem->read(addr);
    set_flag(C, regs.A >= value);
    set_nz(regs.A - value);
}

void Cpu::CPX(u16 addr) {
    u8 value = mem->read(addr);
    set_flag(C, regs.X >= value);
    set_nz(regs.X - value);
}

void Cpu::CPY(u16 addr) {
    u8 value = mem->read(addr);
    set_flag(C, regs.Y >= value);
    set_nz(regs.Y - value);
}


/* Ветвления */
void Cpu::BCC(u16 addr) {
    if (!(regs.P & C)) {
        u16 old = regs.PC;
        regs.PC = addr;
        cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BCS(u16 addr) {
    if (regs.P & C) {
        u16 old = regs.PC;
        regs.PC = addr;
        cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BEQ(u16 addr) {
    if (regs.P & Z) {
        u16 old = regs.PC;
        regs.PC = addr;
        cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BNE(u16 addr) {
    if (!(regs.P & Z)) {
        u16 old = regs.PC;
        regs.PC = addr;
        cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BMI(u16 addr) {
    if (regs.P & N) {
        u16 old = regs.PC;
        regs.PC = addr;
        cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BPL(u16 addr) {
    if (!(regs.P & N)) {
        u16 old = regs.PC;
        regs.PC = addr;
        cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BVC(u16 addr) {
    if (!(regs.P & V)) {
        u16 old = regs.PC;
        regs.PC = addr;
        cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BVS(u16 addr) {
    if (regs.P & V) {
        u16 old = regs.PC;
        regs.PC = addr;
        cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}


/* Переходы */
void Cpu::JMP(u16 addr) {
    regs.PC = addr;
}

void Cpu::JSR(u16 addr) {
    regs.PC--;
    push((regs.PC >> 8) & 0xFF);
    push(regs.PC & 0xFF);
    regs.PC = addr;
}

void Cpu::RTS() {
    u8 low = pop();
    u8 high = pop();
    regs.PC = (static_cast<u16>(high) << 8) | low;
    regs.PC++;
}

void Cpu::BRK() {
    regs.PC++;
    push((regs.PC >> 8) & 0xFF);
    push(regs.PC & 0xFF);
    push(regs.P | BREAK);
    set_flag(I, true);
    regs.PC = mem->ram[0xFFFE] | (mem->ram[0xFFFF] << 8);
}

void Cpu::RTI() {
    regs.P = pop();
    regs.P &= ~BREAK;
    regs.P |= UNUSED;
    u8 low = pop();
    u8 high = pop();
    regs.PC = (static_cast<u16>(high) << 8) | low;
}


/* Стек */
void Cpu::PHA() {
    push(regs.A);
}

void Cpu::PLA() {
    regs.A = pop();
    set_nz(regs.A);
}

void Cpu::PHP() {
    push(regs.P | BREAK | UNUSED);
}

void Cpu::PLP() {
    regs.P = pop();
    regs.P &= ~BREAK;
    regs.P |= UNUSED;
}


/* Флаги */
void Cpu::CLC() {
    set_flag(C, false);
}

void Cpu::SEC() {
    set_flag(C, true);
}

void Cpu::CLI() {
    set_flag(I, false);
}

void Cpu::SEI() {
    set_flag(I, true);
}

void Cpu::CLD() {
    set_flag(D, false);
}

void Cpu::SED() {
    set_flag(D, true);
}

void Cpu::CLV() {
    set_flag(V, false);
}


/* Передача стека */
void Cpu::TSX() {
    regs.X = regs.SP;
    set_nz(regs.X);
}

void Cpu::TXS() {
    regs.SP = regs.X;
}


/* NOP */
void Cpu::NOP() {
    /* Ничего не делает */
}



/* НЕОФИЦИАЛЬНЫЕ ОПКОДЫ */

void Cpu::ALR(u16 addr) {
    AND(addr);
    LSR_ACC();
}

void Cpu::ANC(u16 addr) {
    AND(addr);
    set_flag(C, regs.A & N);
}

void Cpu::ANE(u16 addr) {
    regs.A = ((regs.A | CONSTANT) & regs.X & mem->read(addr));
    set_nz(regs.A);
}

void Cpu::ARR(u16 addr) {
    AND(addr);
    ROR_ACC();
}

void Cpu::DCP(u16 addr) {
    DEC(addr);
    CMP(addr);
}

void Cpu::ISC(u16 addr) {
    INC(addr);
    SBC(addr);
}

void Cpu::LAS(u16 addr) {
    u8 value = regs.SP & mem->read(addr);
    regs.X = regs.A = regs.SP = value;
    set_nz(value);
}

void Cpu::LAX(u16 addr) {
    regs.A = regs.X = mem->read(addr);
    set_nz(regs.A);
}

void Cpu::LXA(u16 addr) {
    u8 value = (regs.A | CONSTANT) & mem->read(addr);
    regs.A = regs.X = value;
    set_nz(value);
}

void Cpu::RLA(u16 addr) {
    ROL(addr);
    AND(addr);
}

void Cpu::RRA(u16 addr) {
    ROR(addr);
    ADC(addr);
}

void Cpu::SAX(u16 addr) {
    mem->write(addr, regs.A & regs.X);
}

void Cpu::SBX(u16 addr) {
    u8 value = mem->read(addr);
    regs.X = (regs.A & regs.X) - value;

    set_flag(C, (regs.A & regs.X) >= value);
    set_nz(regs.X);
}

void Cpu::SHA(u16 addr) {
    u8 h = high(addr) + 1;
    if ((addr & 0xFF) + regs.Y >= 0x100) h--;
    u8 val = regs.A & regs.X & h;
    mem->write(addr, val);
}

void Cpu::SHX(u16 addr) {
    u8 h = (addr >> 8) + 1;
    if ((addr & 0xFF) + regs.Y >= 0x100) h--;
    u8 val = regs.X & h;
    mem->write(addr, val);
}

void Cpu::SHY(u16 addr) {
    u8 h = (addr >> 8) + 1;
    if ((addr & 0xFF) + regs.X >= 0x100) h--;
    u8 val = regs.Y & h;
    mem->write(addr, val);
}

void Cpu::SLO(u16 addr) {
    ASL(addr);
    ORA(addr);
}

void Cpu::SRE(u16 addr) {
    LSR(addr);
    EOR(addr);
}

void Cpu::TAS(u16 addr) {
    regs.SP = regs.A & regs.X;
    u8 h = (addr >> 8) + 1;
    if ((addr & 0xFF) + regs.Y >= 0x100) h--;
    u8 val = regs.SP & h;
    mem->write(addr, val);
}

void Cpu::USBC(u16 addr) {
    SBC(addr);
}

void Cpu::KIL(u16 /*addr*/) {
    /* Пока-что без остановки шины */
}

void Cpu::NOP_IGN(u16 addr) {
    (void)mem->read(addr);
}


/* Системные функции */
void Cpu::reset() {
    regs.A = 0;
    regs.X = 0;
    regs.Y = 0;
    regs.P = UNUSED;
    regs.SP = 0xFD;
    regs.PC = read16(0xFFFC);
    cycles = 7;
    total_cycles += cycles;
}

void Cpu::nmi() {
    push((regs.PC >> 8) & 0xFF);
    push(regs.PC & 0xFF);
    push(regs.P & ~BREAK);
    set_flag(I, true);
    regs.PC = read16(0xFFFA);
    cycles = 8;
    total_cycles += cycles;
}

void Cpu::irq() {
    if (!(regs.P & I)) {
        push((regs.PC >> 8) & 0xFF);
        push(regs.PC & 0xFF);
        push(regs.P & ~BREAK);
        set_flag(I, true);
        regs.PC = read16(0xFFFE);
        cycles = 7;
        total_cycles += cycles;
    }
}



/* Выполнение одной инструкции */
void Cpu::step() {
    u8 opcode = mem->read(regs.PC++);
    page_crossed = false;

    switch (opcode) {
        /* ОФИЦИАЛЬНЫЕ ОПКОДЫ */

        /* ADC */
        case 0x69: ADC(AM_IMM()); cycles = 2; break;
        case 0x65: ADC(AM_ZPG()); cycles = 3; break;
        case 0x75: ADC(AM_ZPX()); cycles = 4; break;
        case 0x6D: ADC(AM_ABS()); cycles = 4; break;
        case 0x7D: ADC(AM_ABX()); cycles = 4 + page_crossed; break;
        case 0x79: ADC(AM_ABY()); cycles = 4 + page_crossed; break;
        case 0x61: ADC(AM_INX()); cycles = 6; break;
        case 0x71: ADC(AM_INY()); cycles = 5 + page_crossed; break;

        /* AND */
        case 0x29: AND(AM_IMM()); cycles = 2; break;
        case 0x25: AND(AM_ZPG()); cycles = 3; break;
        case 0x35: AND(AM_ZPX()); cycles = 4; break;
        case 0x2D: AND(AM_ABS()); cycles = 4; break;
        case 0x3D: AND(AM_ABX()); cycles = 4 + page_crossed; break;
        case 0x39: AND(AM_ABY()); cycles = 4 + page_crossed; break;
        case 0x21: AND(AM_INX()); cycles = 6; break;
        case 0x31: AND(AM_INY()); cycles = 5 + page_crossed; break;

        /* ASL */
        case 0x0A: ASL_ACC(); cycles = 2; break;
        case 0x06: ASL(AM_ZPG()); cycles = 5; break;
        case 0x16: ASL(AM_ZPX()); cycles = 6; break;
        case 0x0E: ASL(AM_ABS()); cycles = 6; break;
        case 0x1E: ASL(AM_ABX()); cycles = 7; break;

        /* BCC */
        case 0x90: cycles = 2; BCC(AM_REL()); break;

        /* BCS */
        case 0xB0: cycles = 2; BCS(AM_REL()); break;

        /* BEQ */
        case 0xF0: cycles = 2; BEQ(AM_REL()); break;

        /* BIT */
        case 0x24: cycles = 3; BIT(AM_ZPG()); break;
        case 0x2C: cycles = 4; BIT(AM_ABS()); break;

        /* BMI */
        case 0x30: cycles = 2; BMI(AM_REL()); break;

        /* BNE */
        case 0xD0: cycles = 2; BNE(AM_REL()); break;

        /* BPL */
        case 0x10: cycles = 2; BPL(AM_REL()); break;

        /* BRK */
        case 0x00: BRK(); cycles = 7; break;

        /* BVC */
        case 0x50: cycles = 2; BVC(AM_REL()); break;

        /* BVS */
        case 0x70: cycles = 2; BVS(AM_REL()); break;

        /* CLC */
        case 0x18: CLC(); cycles = 2; break;

        /* CLD */
        case 0xD8: CLD(); cycles = 2; break;

        /* CLI */
        case 0x58: CLI(); cycles = 2; break;

        /* CLV */
        case 0xB8: CLV(); cycles = 2; break;

        /* CMP */
        case 0xC9: CMP(AM_IMM()); cycles = 2; break;
        case 0xC5: CMP(AM_ZPG()); cycles = 3; break;
        case 0xD5: CMP(AM_ZPX()); cycles = 4; break;
        case 0xCD: CMP(AM_ABS()); cycles = 4; break;
        case 0xDD: CMP(AM_ABX()); cycles = 4 + page_crossed; break;
        case 0xD9: CMP(AM_ABY()); cycles = 4 + page_crossed; break;
        case 0xC1: CMP(AM_INX()); cycles = 6; break;
        case 0xD1: CMP(AM_INY()); cycles = 5 + page_crossed; break;

        /* CPX */
        case 0xE0: CPX(AM_IMM()); cycles = 2; break;
        case 0xE4: CPX(AM_ZPG()); cycles = 3; break;
        case 0xEC: CPX(AM_ABS()); cycles = 4; break;

        /* CPY */
        case 0xC0: CPY(AM_IMM()); cycles = 2; break;
        case 0xC4: CPY(AM_ZPG()); cycles = 3; break;
        case 0xCC: CPY(AM_ABS()); cycles = 4; break;

        /* DEC */
        case 0xC6: DEC(AM_ZPG()); cycles = 5; break;
        case 0xD6: DEC(AM_ZPX()); cycles = 6; break;
        case 0xCE: DEC(AM_ABS()); cycles = 6; break;
        case 0xDE: DEC(AM_ABX()); cycles = 7; break;

        /* DEX */
        case 0xCA: DEX(); cycles = 2; break;

        /* DEY */
        case 0x88: DEY(); cycles = 2; break;

        /* EOR */
        case 0x49: EOR(AM_IMM()); cycles = 2; break;
        case 0x45: EOR(AM_ZPG()); cycles = 3; break;
        case 0x55: EOR(AM_ZPX()); cycles = 4; break;
        case 0x4D: EOR(AM_ABS()); cycles = 4; break;
        case 0x5D: EOR(AM_ABX()); cycles = 4 + page_crossed; break;
        case 0x59: EOR(AM_ABY()); cycles = 4 + page_crossed; break;
        case 0x41: EOR(AM_INX()); cycles = 6; break;
        case 0x51: EOR(AM_INY()); cycles = 5 + page_crossed; break;

        /* INC */
        case 0xE6: INC(AM_ZPG()); cycles = 5; break;
        case 0xF6: INC(AM_ZPX()); cycles = 6; break;
        case 0xEE: INC(AM_ABS()); cycles = 6; break;
        case 0xFE: INC(AM_ABX()); cycles = 7; break;

        /* INX */
        case 0xE8: INX(); cycles = 2; break;

        /* INY */
        case 0xC8: INY(); cycles = 2; break;

        /* JMP */
        case 0x4C: JMP(AM_ABS()); cycles = 3; break;
        case 0x6C: JMP(AM_IND()); cycles = 5; break;

        /* JSR */
        case 0x20: JSR(AM_ABS()); cycles = 6; break;

        /* LDA */
        case 0xA9: LDA(AM_IMM()); cycles = 2; break;
        case 0xA5: LDA(AM_ZPG()); cycles = 3; break;
        case 0xB5: LDA(AM_ZPX()); cycles = 4; break;
        case 0xAD: LDA(AM_ABS()); cycles = 4; break;
        case 0xBD: LDA(AM_ABX()); cycles = 4 + page_crossed; break;
        case 0xB9: LDA(AM_ABY()); cycles = 4 + page_crossed; break;
        case 0xA1: LDA(AM_INX()); cycles = 6; break;
        case 0xB1: LDA(AM_INY()); cycles = 5 + page_crossed; break;

        /* LDX */
        case 0xA2: LDX(AM_IMM()); cycles = 2; break;
        case 0xA6: LDX(AM_ZPG()); cycles = 3; break;
        case 0xB6: LDX(AM_ZPY()); cycles = 4; break;
        case 0xAE: LDX(AM_ABS()); cycles = 4; break;
        case 0xBE: LDX(AM_ABY()); cycles = 4 + page_crossed; break;

        /* LDY */
        case 0xA0: LDY(AM_IMM()); cycles = 2; break;
        case 0xA4: LDY(AM_ZPG()); cycles = 3; break;
        case 0xB4: LDY(AM_ZPX()); cycles = 4; break;
        case 0xAC: LDY(AM_ABS()); cycles = 4; break;
        case 0xBC: LDY(AM_ABX()); cycles = 4 + page_crossed; break;

        /* LSR */
        case 0x4A: LSR_ACC(); cycles = 2; break;
        case 0x46: LSR(AM_ZPG()); cycles = 5; break;
        case 0x56: LSR(AM_ZPX()); cycles = 6; break;
        case 0x4E: LSR(AM_ABS()); cycles = 6; break;
        case 0x5E: LSR(AM_ABX()); cycles = 7; break;

        /* NOP */
        case 0xEA: NOP(); cycles = 2; break;

        /* ORA */
        case 0x09: ORA(AM_IMM()); cycles = 2; break;
        case 0x05: ORA(AM_ZPG()); cycles = 3; break;
        case 0x15: ORA(AM_ZPX()); cycles = 4; break;
        case 0x0D: ORA(AM_ABS()); cycles = 4; break;
        case 0x1D: ORA(AM_ABX()); cycles = 4 + page_crossed; break;
        case 0x19: ORA(AM_ABY()); cycles = 4 + page_crossed; break;
        case 0x01: ORA(AM_INX()); cycles = 6; break;
        case 0x11: ORA(AM_INY()); cycles = 5 + page_crossed; break;

        /* PHA */
        case 0x48: PHA(); cycles = 3; break;

        /* PHP */
        case 0x08: PHP(); cycles = 3; break;

        /* PLA */
        case 0x68: PLA(); cycles = 4; break;

        /* PLP */
        case 0x28: PLP(); cycles = 4; break;

        /* ROL */
        case 0x2A: ROL_ACC(); cycles = 2; break;
        case 0x26: ROL(AM_ZPG()); cycles = 5; break;
        case 0x36: ROL(AM_ZPX()); cycles = 6; break;
        case 0x2E: ROL(AM_ABS()); cycles = 6; break;
        case 0x3E: ROL(AM_ABX()); cycles = 7; break;

        /* ROR */
        case 0x6A: ROR_ACC(); cycles = 2; break;
        case 0x66: ROR(AM_ZPG()); cycles = 5; break;
        case 0x76: ROR(AM_ZPX()); cycles = 6; break;
        case 0x6E: ROR(AM_ABS()); cycles = 6; break;
        case 0x7E: ROR(AM_ABX()); cycles = 7; break;

        /* RTI */
        case 0x40: RTI(); cycles = 6; break;

        /* RTS */
        case 0x60: RTS(); cycles = 6; break;

        /* SBC */
        case 0xE9: SBC(AM_IMM()); cycles = 2; break;
        case 0xE5: SBC(AM_ZPG()); cycles = 3; break;
        case 0xF5: SBC(AM_ZPX()); cycles = 4; break;
        case 0xED: SBC(AM_ABS()); cycles = 4; break;
        case 0xFD: SBC(AM_ABX()); cycles = 4 + page_crossed; break;
        case 0xF9: SBC(AM_ABY()); cycles = 4 + page_crossed; break;
        case 0xE1: SBC(AM_INX()); cycles = 6; break;
        case 0xF1: SBC(AM_INY()); cycles = 5 + page_crossed; break;

        /* SEC */
        case 0x38: SEC(); cycles = 2; break;

        /* SED */
        case 0xF8: SED(); cycles = 2; break;

        /* SEI */
        case 0x78: SEI(); cycles = 2; break;

        /* STA */
        case 0x85: STA(AM_ZPG()); cycles = 3; break;
        case 0x95: STA(AM_ZPX()); cycles = 4; break;
        case 0x8D: STA(AM_ABS()); cycles = 4; break;
        case 0x9D: STA(AM_ABX()); cycles = 5; break;
        case 0x99: STA(AM_ABY()); cycles = 5; break;
        case 0x81: STA(AM_INX()); cycles = 6; break;
        case 0x91: STA(AM_INY()); cycles = 6; break;

        /* STX */
        case 0x86: STX(AM_ZPG()); cycles = 3; break;
        case 0x96: STX(AM_ZPY()); cycles = 4; break;
        case 0x8E: STX(AM_ABS()); cycles = 4; break;

        /* STY */
        case 0x84: STY(AM_ZPG()); cycles = 3; break;
        case 0x94: STY(AM_ZPX()); cycles = 4; break;
        case 0x8C: STY(AM_ABS()); cycles = 4; break;

        /* TAX */
        case 0xAA: TAX(); cycles = 2; break;

        /* TAY */
        case 0xA8: TAY(); cycles = 2; break;

        /* TSX */
        case 0xBA: TSX(); cycles = 2; break;

        /* TXA */
        case 0x8A: TXA(); cycles = 2; break;

        /* TXS */
        case 0x9A: TXS(); cycles = 2; break;

        /* TYA */
        case 0x98: TYA(); cycles = 2; break;



        /* НЕОФИЦИАЛЬНЫЕ ОПКОДЫ */

        /* ALR */
        case 0x4B: ALR(AM_IMM()); cycles = 2; break;

        /* ANC */
        case 0x0B:
        case 0x2B: ANC(AM_IMM()); cycles = 2; break;

        /* ANE */
        case 0x8B: ANE(AM_IMM()); cycles = 2; break;

        /* ARR */
        case 0x6B: ARR(AM_IMM()); cycles = 2; break;

        /* DCP */
        case 0xC7: DCP(AM_ZPG()); cycles = 5; break;
        case 0xD7: DCP(AM_ZPX()); cycles = 6; break;
        case 0xCF: DCP(AM_ABS()); cycles = 6; break;
        case 0xDF: DCP(AM_ABX()); cycles = 7; break;
        case 0xDB: DCP(AM_ABY()); cycles = 7; break;
        case 0xC3: DCP(AM_INX()); cycles = 8; break;
        case 0xD3: DCP(AM_INY()); cycles = 8; break;

        /* ISC */
        case 0xE7: ISC(AM_ZPG()); cycles = 5; break;
        case 0xF7: ISC(AM_ZPX()); cycles = 6; break;
        case 0xEF: ISC(AM_ABS()); cycles = 6; break;
        case 0xFF: ISC(AM_ABX()); cycles = 7; break;
        case 0xFB: ISC(AM_ABY()); cycles = 7; break;
        case 0xE3: ISC(AM_INX()); cycles = 8; break;
        case 0xF3: ISC(AM_INY()); cycles = 8; break;

        /* LAS */
        case 0xBB: LAS(AM_ABY()); cycles = 4 + page_crossed; break;

        /* LAX */
        case 0xA7: LAX(AM_ZPG()); cycles = 3; break;
        case 0xB7: LAX(AM_ZPY()); cycles = 4; break;
        case 0xAF: LAX(AM_ABS()); cycles = 4; break;
        case 0xBF: LAX(AM_ABY()); cycles = 4 + page_crossed; break;
        case 0xA3: LAX(AM_INX()); cycles = 6; break;
        case 0xB3: LAX(AM_INY()); cycles = 5 + page_crossed; break;

        /* LXA */
        case 0xAB: LXA(AM_IMM()); cycles = 2; break;

        /* RLA */
        case 0x27: RLA(AM_ZPG()); cycles = 5; break;
        case 0x37: RLA(AM_ZPX()); cycles = 6; break;
        case 0x23: RLA(AM_INX()); cycles = 8; break;
        case 0x33: RLA(AM_INY()); cycles = 8; break;
        case 0x2F: RLA(AM_ABS()); cycles = 6; break;
        case 0x3F: RLA(AM_ABX()); cycles = 7; break;
        case 0x3B: RLA(AM_ABY()); cycles = 7; break;

        /* RRA */
        case 0x67: RRA(AM_ZPG()); cycles = 5; break;
        case 0x77: RRA(AM_ZPX()); cycles = 6; break;
        case 0x63: RRA(AM_INX()); cycles = 8; break;
        case 0x73: RRA(AM_INY()); cycles = 8; break;
        case 0x6F: RRA(AM_ABS()); cycles = 6; break;
        case 0x7F: RRA(AM_ABX()); cycles = 7; break;
        case 0x7B: RRA(AM_ABY()); cycles = 7; break;

        /* SAX */
        case 0x87: SAX(AM_ZPG()); cycles = 3; break;
        case 0x97: SAX(AM_ZPY()); cycles = 4; break;
        case 0x8F: SAX(AM_ABS()); cycles = 4; break;
        case 0x83: SAX(AM_INX()); cycles = 6; break;

        /* SBX */
        case 0xCB: SBX(AM_IMM()); cycles = 2; break;

        /* SHA */
        case 0x93: SHA(AM_INY()); cycles = 6; break;
        case 0x9F: SHA(AM_ABS()); cycles = 5; break;

        /* SHX */
        case 0x9E: SHX(AM_ABY()); cycles = 5; break;

        /* SHY */
        case 0x9C: SHY(AM_ABX()); cycles = 5; break;

        /* SLO */
        case 0x07: SLO(AM_ZPG()); cycles = 5; break;
        case 0x17: SLO(AM_ZPX()); cycles = 6; break;
        case 0x03: SLO(AM_INX()); cycles = 8; break;
        case 0x13: SLO(AM_INY()); cycles = 8; break;
        case 0x0F: SLO(AM_ABS()); cycles = 6; break;
        case 0x1F: SLO(AM_ABX()); cycles = 7; break;
        case 0x1B: SLO(AM_ABY()); cycles = 7; break;

        /* SRE */
        case 0x47: SRE(AM_ZPG()); cycles = 5; break;
        case 0x57: SRE(AM_ZPX()); cycles = 6; break;
        case 0x43: SRE(AM_INX()); cycles = 8; break;
        case 0x53: SRE(AM_INY()); cycles = 8; break;
        case 0x4F: SRE(AM_ABS()); cycles = 6; break;
        case 0x5F: SRE(AM_ABX()); cycles = 7; break;
        case 0x5B: SRE(AM_ABY()); cycles = 7; break;

        /* TAS */
        case 0x9B: TAS(AM_ABY()); cycles = 5; break;

        /* USBC */
        case 0xEB: USBC(AM_IMM()); cycles = 2; break;

        /* KIL */
        case 0x02: case 0x12: case 0x22: case 0x32:
        case 0x42: case 0x52: case 0x62: case 0x72:
        case 0x92: case 0xB2: case 0xD2: case 0xF2:
            KIL(AM_IMP()); cycles = 0;
            break;

        /* NOP */
        case 0x80:
        case 0x82:
        case 0x89:
        case 0xC2:
        case 0xE2: NOP_IGN(AM_IMM()); cycles = 2; break;

        case 0x04:
        case 0x44:
        case 0x64: NOP_IGN(AM_ZPG()); cycles = 3; break;

        case 0x14:
        case 0x34:
        case 0x54:
        case 0x74:
        case 0xD4:
        case 0xF4: NOP_IGN(AM_ZPX()); cycles = 4; break;

        case 0x0C: NOP_IGN(AM_ABS()); cycles = 4; break;

        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC: NOP_IGN(AM_ABX()); cycles = 4 + page_crossed; break;

        default:
            /* Неизвестный опкод */
            cycles = 2;
            break;
    }
}


void Cpu::exec() {
    if(do_nmi) {
        nmi();
        do_nmi = false;
        return;
    }
    if(do_irq) {
        irq();
        do_irq = false;
        return;
    }

    cycles = 0;
    step();
    total_cycles += cycles;
}
