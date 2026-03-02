#include "cpu.hpp"


/* ОФИЦИАЛЬНЫЕ ОПКОДЫ */

/* Хранение/Загрузка */
void Cpu::LDA(u16 addr) {
    state.regs.A = mem->read(addr);
    set_nz(state.regs.A);
}

void Cpu::STA(u16 addr) {
    mem->write(addr, state.regs.A);
}

void Cpu::LDX(u16 addr) {
    state.regs.X = mem->read(addr);
    set_nz(state.regs.X);
}

void Cpu::STX(u16 addr) {
    mem->write(addr, state.regs.X);
}

void Cpu::LDY(u16 addr) {
    state.regs.Y = mem->read(addr);
    set_nz(state.regs.Y);
}

void Cpu::STY(u16 addr) {
    mem->write(addr, state.regs.Y);
}


/* Передача */
void Cpu::TAX() {
    state.regs.X = state.regs.A;
    set_nz(state.regs.X);
}

void Cpu::TXA() {
    state.regs.A = state.regs.X;
    set_nz(state.regs.A);
}

void Cpu::TAY() {
    state.regs.Y = state.regs.A;
    set_nz(state.regs.Y);
}

void Cpu::TYA() {
    state.regs.A = state.regs.Y;
    set_nz(state.regs.A);
}


/* Арифметические операции */
void Cpu::ADC(u16 addr) {
    const u16 value = mem->read(addr);
    const u16 res = state.regs.A + value + (state.regs.P & C);

    set_flag(C, res > 0xFF);
    set_flag(V, (((res ^ state.regs.A) & (res ^ value)) & N) != 0);
    set_nz(res);

    state.regs.A = res & 0xFF;
}

void Cpu::SBC(u16 addr) {
    const u16 value = mem->read(addr) ^ 0x00FF;
    const u16 res = state.regs.A + value + (state.regs.P & C);

    set_flag(C, res > 0xFF);
    set_flag(V, (((res ^ state.regs.A) & (res ^ value)) & N) != 0);
    set_nz(res & 0xFF);

    state.regs.A = res & 0x00FF;
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
    state.regs.X++;
    set_nz(state.regs.X);
}

void Cpu::DEX() {
    state.regs.X--;
    set_nz(state.regs.X);
}

void Cpu::INY() {
    state.regs.Y++;
    set_nz(state.regs.Y);
}

void Cpu::DEY() {
    state.regs.Y--;
    set_nz(state.regs.Y);
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
    const u8 old_c = state.regs.P & C;
    set_flag(C, value & N);
    value <<= 1;
    value |= old_c;
    set_nz(value);
    mem->write(addr, value);
}

void Cpu::ROR(u16 addr) {
    u8 value = mem->read(addr);
    const u8 old_c = state.regs.P & C;
    set_flag(C, value & 0x01);
    value >>= 1;
    value |= (old_c << 7);
    set_nz(value);
    mem->write(addr, value);
}


/* Сдвиг аккумулятора */
void Cpu::ASL_ACC() {
    set_flag(C, state.regs.A & N);
    state.regs.A <<= 1;
    set_nz(state.regs.A);
}

void Cpu::LSR_ACC() {
    set_flag(C, state.regs.A & 0x01);
    state.regs.A >>= 1;
    set_nz(state.regs.A);
}

void Cpu::ROL_ACC() {
    const u8 old_c = state.regs.P & C;
    set_flag(C, state.regs.A & N);
    state.regs.A <<= 1;
    state.regs.A |= old_c;
    set_nz(state.regs.A);
}

void Cpu::ROR_ACC() {
    const u8 old_c = state.regs.P & C;
    set_flag(C, state.regs.A & 0x01);
    state.regs.A >>= 1;
    state.regs.A |= (old_c << 7);
    set_nz(state.regs.A);
}


/* Логические операции */
void Cpu::AND(u16 addr) {
    state.regs.A &= mem->read(addr);
    set_nz(state.regs.A);
}

void Cpu::ORA(u16 addr) {
    state.regs.A |= mem->read(addr);
    set_nz(state.regs.A);
}

void Cpu::EOR(u16 addr) {
    state.regs.A ^= mem->read(addr);
    set_nz(state.regs.A);
}

void Cpu::BIT(u16 addr) {
    const u8 value = mem->read(addr);
    set_flag(N, value & N);
    set_flag(V, value & V);
    set_flag(Z, (state.regs.A & value) == 0);
}


/* Сравнение */
void Cpu::CMP(u16 addr) {
    const u8 value = mem->read(addr);
    set_flag(C, state.regs.A >= value);
    set_nz(state.regs.A - value);
}

void Cpu::CPX(u16 addr) {
    const u8 value = mem->read(addr);
    set_flag(C, state.regs.X >= value);
    set_nz(state.regs.X - value);
}

void Cpu::CPY(u16 addr) {
    const u8 value = mem->read(addr);
    set_flag(C, state.regs.Y >= value);
    set_nz(state.regs.Y - value);
}


/* Ветвления */
void Cpu::BCC(u16 addr) {
    if (!(state.regs.P & C)) {
        const u16 old = state.regs.PC;
        state.regs.PC = addr;
        state.cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BCS(u16 addr) {
    if (state.regs.P & C) {
        const u16 old = state.regs.PC;
        state.regs.PC = addr;
        state.cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BEQ(u16 addr) {
    if (state.regs.P & Z) {
        const u16 old = state.regs.PC;
        state.regs.PC = addr;
        state.cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BNE(u16 addr) {
    if (!(state.regs.P & Z)) {
        const u16 old = state.regs.PC;
        state.regs.PC = addr;
        state.cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BMI(u16 addr) {
    if (state.regs.P & N) {
        const u16 old = state.regs.PC;
        state.regs.PC = addr;
        state.cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BPL(u16 addr) {
    if (!(state.regs.P & N)) {
        const u16 old = state.regs.PC;
        state.regs.PC = addr;
        state.cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BVC(u16 addr) {
    if (!(state.regs.P & V)) {
        const u16 old = state.regs.PC;
        state.regs.PC = addr;
        state.cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}

void Cpu::BVS(u16 addr) {
    if (state.regs.P & V) {
        const u16 old = state.regs.PC;
        state.regs.PC = addr;
        state.cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
    }
}


/* Переходы */
void Cpu::JMP(u16 addr) {
    state.regs.PC = addr;
}

void Cpu::JSR(u16 addr) {
    state.regs.PC--;
    push((state.regs.PC >> 8) & 0xFF);
    push(state.regs.PC & 0xFF);
    state.regs.PC = addr;
}

void Cpu::RTS() {
    const u8 low = pop();
    const u8 high = pop();
    state.regs.PC = (static_cast<u16>(high) << 8) | low;
    state.regs.PC++;
}

void Cpu::BRK() {
    state.regs.PC++;
    push((state.regs.PC >> 8) & 0xFF);
    push(state.regs.PC & 0xFF);
    push(state.regs.P | BREAK);
    set_flag(I, true);
    state.regs.PC = read16(0xFFFE);
}

void Cpu::RTI() {
    state.regs.P = pop();
    const u8 low = pop();
    const u8 high = pop();
    set_flag(BREAK, false);
    set_flag(UNUSED, true);
    state.regs.PC = (static_cast<u16>(high) << 8) | low;
}


/* Стек */
void Cpu::PHA() {
    push(state.regs.A);
}

void Cpu::PLA() {
    state.regs.A = pop();
    set_nz(state.regs.A);
}

void Cpu::PHP() {
    push(state.regs.P | BREAK | UNUSED);
}

void Cpu::PLP() {
    state.regs.P = pop();
    set_flag(BREAK, false);
    set_flag(UNUSED, true);
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
    state.regs.X = state.regs.SP;
    set_nz(state.regs.X);
}

void Cpu::TXS() {
    state.regs.SP = state.regs.X;
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
    set_flag(C, state.regs.A & N);
}

void Cpu::ANE(u16 addr) {
    state.regs.A = ((state.regs.A | CONSTANT) & state.regs.X & mem->read(addr));
    set_nz(state.regs.A);
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
    const u8 value = state.regs.SP & mem->read(addr);
    state.regs.X = state.regs.A = state.regs.SP = value;
    set_nz(value);
}

void Cpu::LAX(u16 addr) {
    state.regs.A = state.regs.X = mem->read(addr);
    set_nz(state.regs.A);
}

void Cpu::LXA(u16 addr) {
    const u8 value = (state.regs.A | CONSTANT) & mem->read(addr);
    state.regs.A = state.regs.X = value;
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
    mem->write(addr, state.regs.A & state.regs.X);
}

void Cpu::SBX(u16 addr) {
    const u8 value = mem->read(addr);
    state.regs.X = (state.regs.A & state.regs.X) - value;

    set_flag(C, (state.regs.A & state.regs.X) >= value);
    set_nz(state.regs.X);
}

void Cpu::SHA(u16 addr) {
    u8 h = high(addr) + 1;
    if ((addr & 0xFF) + state.regs.Y >= 0x100) h--;
    const u8 val = state.regs.A & state.regs.X & h;
    mem->write(addr, val);
}

void Cpu::SHX(u16 addr) {
    u8 h = (addr >> 8) + 1;
    if ((addr & 0xFF) + state.regs.Y >= 0x100) h--;
    const u8 val = state.regs.X & h;
    mem->write(addr, val);
}

void Cpu::SHY(u16 addr) {
    u8 h = (addr >> 8) + 1;
    if ((addr & 0xFF) + state.regs.X >= 0x100) h--;
    const u8 val = state.regs.Y & h;
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
    state.regs.SP = state.regs.A & state.regs.X;
    u8 h = (addr >> 8) + 1;
    if ((addr & 0xFF) + state.regs.Y >= 0x100) h--;
    const u8 val = state.regs.SP & h;
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
inline void Cpu::clear() {
    state.regs.A = 0;
    state.regs.X = 0;
    state.regs.Y = 0;
    state.regs.P = UNUSED | I;
    state.regs.SP = 0xFD;
    state.regs.PC = read16(0xFFFC);
    state.cycles = 7;
}

void Cpu::powerUp() {
    clear();
    state.dbg.op.clear();
    mem->state.ram.fill(0);

    state.do_nmi = false;
    state.nmi_delay = false;
    state.do_irq = false;
    state.page_crossed = false;

    if (mem && mem->apu) {
        mem->apu->powerUp();
        mem->apu->step(state.cycles);
    }

    state.total_cycles = 0;
}

void Cpu::reset() {
    clear();

    state.do_nmi = false;
    state.nmi_delay = false;
    state.do_irq = false;

    if (mem && mem->apu) {
        mem->apu->reset();
        mem->apu->step(state.cycles);
    }

    state.total_cycles += state.cycles;
}

void Cpu::loadState(State state) {
    this->state = state;
}

void Cpu::nmi() {
    push((state.regs.PC >> 8) & 0xFF);
    push(state.regs.PC & 0xFF);
    push(state.regs.P & ~BREAK);
    set_flag(I, true);
    state.regs.PC = read16(0xFFFA);
    state.cycles = 8;
    state.total_cycles += state.cycles;
}

void Cpu::irq() {
    push((state.regs.PC >> 8) & 0xFF);
    push(state.regs.PC & 0xFF);
    push(state.regs.P & ~BREAK);
    set_flag(I, true);
    state.regs.PC = read16(0xFFFE);
    state.cycles = 7;
    state.total_cycles += state.cycles;
}



/* Выполнение одной инструкции */
void Cpu::step() {
    const u8 opcode = mem->read(state.regs.PC++);
    state.page_crossed = false;

    state.dbg.am = "IMP";
    state.dbg.addr = 0;

    switch (opcode) {
        /* ОФИЦИАЛЬНЫЕ ОПКОДЫ */

        /* ADC */
        case 0x69: ADC(AM_IMM()); state.cycles = 2; goto ADC;
        case 0x65: ADC(AM_ZPG()); state.cycles = 3; goto ADC;
        case 0x75: ADC(AM_ZPX()); state.cycles = 4; goto ADC;
        case 0x6D: ADC(AM_ABS()); state.cycles = 4; goto ADC;
        case 0x7D: ADC(AM_ABX()); state.cycles = 4 + state.page_crossed; goto ADC;
        case 0x79: ADC(AM_ABY()); state.cycles = 4 + state.page_crossed; goto ADC;
        case 0x61: ADC(AM_INX()); state.cycles = 6; goto ADC;
        case 0x71: ADC(AM_INY()); state.cycles = 5 + state.page_crossed; goto ADC;
        ADC: state.dbg.op = "ADC"; break;

        /* AND */
        case 0x29: AND(AM_IMM()); state.cycles = 2; goto AND;
        case 0x25: AND(AM_ZPG()); state.cycles = 3; goto AND;
        case 0x35: AND(AM_ZPX()); state.cycles = 4; goto AND;
        case 0x2D: AND(AM_ABS()); state.cycles = 4; goto AND;
        case 0x3D: AND(AM_ABX()); state.cycles = 4 + state.page_crossed; goto AND;
        case 0x39: AND(AM_ABY()); state.cycles = 4 + state.page_crossed; goto AND;
        case 0x21: AND(AM_INX()); state.cycles = 6; goto AND;
        case 0x31: AND(AM_INY()); state.cycles = 5 + state.page_crossed; goto AND;
        AND: state.dbg.op = "AND"; break;

        /* ASL */
        case 0x0A: ASL_ACC(); state.cycles = 2; goto ASL;
        case 0x06: ASL(AM_ZPG()); state.cycles = 5; goto ASL;
        case 0x16: ASL(AM_ZPX()); state.cycles = 6; goto ASL;
        case 0x0E: ASL(AM_ABS()); state.cycles = 6; goto ASL;
        case 0x1E: ASL(AM_ABX()); state.cycles = 7; goto ASL;
        ASL: state.dbg.op = "ASL"; break;

        /* BCC */
        case 0x90: state.dbg.op="BCC"; state.cycles = 2; BCC(AM_REL()); break;

        /* BCS */
        case 0xB0: state.dbg.op="BCS"; state.cycles = 2; BCS(AM_REL()); break;

        /* BEQ */
        case 0xF0: state.dbg.op="BEQ"; state.cycles = 2; BEQ(AM_REL()); break;

        /* BIT */
        case 0x24: state.dbg.op="BIT"; state.cycles = 3; BIT(AM_ZPG()); break;
        case 0x2C: state.dbg.op="BIT"; state.cycles = 4; BIT(AM_ABS()); break;

        /* BMI */
        case 0x30: state.dbg.op="BMI"; state.cycles = 2; BMI(AM_REL()); break;

        /* BNE */
        case 0xD0: state.dbg.op="BNE"; state.cycles = 2; BNE(AM_REL()); break;

        /* BPL */
        case 0x10: state.dbg.op="BPL"; state.cycles = 2; BPL(AM_REL()); break;

        /* BRK */
        case 0x00: BRK(); state.dbg.op="BRK"; state.cycles = 7; break;

        /* BVC */
        case 0x50: state.dbg.op="BVC"; state.cycles = 2; BVC(AM_REL()); break;

        /* BVS */
        case 0x70: state.dbg.op="BVS"; state.cycles = 2; BVS(AM_REL()); break;

        /* CLC */
        case 0x18: CLC(); state.dbg.op="CLC"; state.cycles = 2; break;

        /* CLD */
        case 0xD8: CLD(); state.dbg.op="CLD"; state.cycles = 2; break;

        /* CLI */
        case 0x58: CLI(); state.dbg.op="CLI"; state.cycles = 2; break;

        /* CLV */
        case 0xB8: CLV(); state.dbg.op="CLV"; state.cycles = 2; break;

        /* CMP */
        case 0xC9: CMP(AM_IMM()); state.cycles = 2; goto CMP;
        case 0xC5: CMP(AM_ZPG()); state.cycles = 3; goto CMP;
        case 0xD5: CMP(AM_ZPX()); state.cycles = 4; goto CMP;
        case 0xCD: CMP(AM_ABS()); state.cycles = 4; goto CMP;
        case 0xDD: CMP(AM_ABX()); state.cycles = 4 + state.page_crossed; goto CMP;
        case 0xD9: CMP(AM_ABY()); state.cycles = 4 + state.page_crossed; goto CMP;
        case 0xC1: CMP(AM_INX()); state.cycles = 6; goto CMP;
        case 0xD1: CMP(AM_INY()); state.cycles = 5 + state.page_crossed; goto CMP;
        CMP: state.dbg.op = "CMP"; break;

        /* CPX */
        case 0xE0: CPX(AM_IMM()); state.cycles = 2; goto CPX;
        case 0xE4: CPX(AM_ZPG()); state.cycles = 3; goto CPX;
        case 0xEC: CPX(AM_ABS()); state.cycles = 4; goto CPX;
        CPX: state.dbg.op = "CPX"; break;

        /* CPY */
        case 0xC0: CPY(AM_IMM()); state.cycles = 2; goto CPY;
        case 0xC4: CPY(AM_ZPG()); state.cycles = 3; goto CPY;
        case 0xCC: CPY(AM_ABS()); state.cycles = 4; goto CPY;
        CPY: state.dbg.op = "CPY"; break;

        /* DEC */
        case 0xC6: DEC(AM_ZPG()); state.cycles = 5; goto DEC;
        case 0xD6: DEC(AM_ZPX()); state.cycles = 6; goto DEC;
        case 0xCE: DEC(AM_ABS()); state.cycles = 6; goto DEC;
        case 0xDE: DEC(AM_ABX()); state.cycles = 7; goto DEC;
        DEC: state.dbg.op = "DEC"; break;

        /* DEX */
        case 0xCA: DEX(); state.dbg.op="DEX"; state.cycles = 2; break;

        /* DEY */
        case 0x88: DEY(); state.dbg.op="DEY"; state.cycles = 2; break;

        /* EOR */
        case 0x49: EOR(AM_IMM()); state.cycles = 2; goto EOR;
        case 0x45: EOR(AM_ZPG()); state.cycles = 3; goto EOR;
        case 0x55: EOR(AM_ZPX()); state.cycles = 4; goto EOR;
        case 0x4D: EOR(AM_ABS()); state.cycles = 4; goto EOR;
        case 0x5D: EOR(AM_ABX()); state.cycles = 4 + state.page_crossed; goto EOR;
        case 0x59: EOR(AM_ABY()); state.cycles = 4 + state.page_crossed; goto EOR;
        case 0x41: EOR(AM_INX()); state.cycles = 6; goto EOR;
        case 0x51: EOR(AM_INY()); state.cycles = 5 + state.page_crossed; goto EOR;
        EOR: state.dbg.op = "EOR"; break;

        /* INC */
        case 0xE6: INC(AM_ZPG()); state.cycles = 5; goto INC;
        case 0xF6: INC(AM_ZPX()); state.cycles = 6; goto INC;
        case 0xEE: INC(AM_ABS()); state.cycles = 6; goto INC;
        case 0xFE: INC(AM_ABX()); state.cycles = 7; goto INC;
        INC: state.dbg.op = "INC"; break;

        /* INX */
        case 0xE8: INX(); state.dbg.op="INX"; state.cycles = 2; break;

        /* INY */
        case 0xC8: INY(); state.dbg.op="INY"; state.cycles = 2; break;

        /* JMP */
        case 0x4C: JMP(AM_ABS()); state.dbg.op="JMP"; state.cycles = 3; break;
        case 0x6C: JMP(AM_IND()); state.dbg.op="JMP"; state.cycles = 5; break;

        /* JSR */
        case 0x20: JSR(AM_ABS()); state.dbg.op="JSR"; state.cycles = 6; break;

        /* LDA */
        case 0xA9: LDA(AM_IMM()); state.cycles = 2; goto LDA;
        case 0xA5: LDA(AM_ZPG()); state.cycles = 3; goto LDA;
        case 0xB5: LDA(AM_ZPX()); state.cycles = 4; goto LDA;
        case 0xAD: LDA(AM_ABS()); state.cycles = 4; goto LDA;
        case 0xBD: LDA(AM_ABX()); state.cycles = 4 + state.page_crossed; goto LDA;
        case 0xB9: LDA(AM_ABY()); state.cycles = 4 + state.page_crossed; goto LDA;
        case 0xA1: LDA(AM_INX()); state.cycles = 6; goto LDA;
        case 0xB1: LDA(AM_INY()); state.cycles = 5 + state.page_crossed; goto LDA;
        LDA: state.dbg.op = "LDA"; break;

        /* LDX */
        case 0xA2: LDX(AM_IMM()); state.cycles = 2; goto LDX;
        case 0xA6: LDX(AM_ZPG()); state.cycles = 3; goto LDX;
        case 0xB6: LDX(AM_ZPY()); state.cycles = 4; goto LDX;
        case 0xAE: LDX(AM_ABS()); state.cycles = 4; goto LDX;
        case 0xBE: LDX(AM_ABY()); state.cycles = 4 + state.page_crossed; goto LDX;
        LDX: state.dbg.op = "LDX"; break;

        /* LDY */
        case 0xA0: LDY(AM_IMM()); state.cycles = 2; goto LDY;
        case 0xA4: LDY(AM_ZPG()); state.cycles = 3; goto LDY;
        case 0xB4: LDY(AM_ZPX()); state.cycles = 4; goto LDY;
        case 0xAC: LDY(AM_ABS()); state.cycles = 4; goto LDY;
        case 0xBC: LDY(AM_ABX()); state.cycles = 4 + state.page_crossed; goto LDY;
        LDY: state.dbg.op = "LDY"; break;

        /* LSR */
        case 0x4A: LSR_ACC(); state.cycles = 2; goto LSR;
        case 0x46: LSR(AM_ZPG()); state.cycles = 5; goto LSR;
        case 0x56: LSR(AM_ZPX()); state.cycles = 6; goto LSR;
        case 0x4E: LSR(AM_ABS()); state.cycles = 6; goto LSR;
        case 0x5E: LSR(AM_ABX()); state.cycles = 7; goto LSR;
        LSR: state.dbg.op = "LSR"; break;

        /* NOP */
        case 0xEA: NOP(); state.dbg.op="NOP"; state.cycles = 2; break;

        /* ORA */
        case 0x09: ORA(AM_IMM()); state.cycles = 2; goto ORA;
        case 0x05: ORA(AM_ZPG()); state.cycles = 3; goto ORA;
        case 0x15: ORA(AM_ZPX()); state.cycles = 4; goto ORA;
        case 0x0D: ORA(AM_ABS()); state.cycles = 4; goto ORA;
        case 0x1D: ORA(AM_ABX()); state.cycles = 4 + state.page_crossed; goto ORA;
        case 0x19: ORA(AM_ABY()); state.cycles = 4 + state.page_crossed; goto ORA;
        case 0x01: ORA(AM_INX()); state.cycles = 6; goto ORA;
        case 0x11: ORA(AM_INY()); state.cycles = 5 + state.page_crossed; goto ORA;
        ORA: state.dbg.op = "ORA"; break;

        /* PHA */
        case 0x48: PHA(); state.dbg.op="PHA"; state.cycles = 3; break;

        /* PHP */
        case 0x08: PHP(); state.dbg.op="PHP"; state.cycles = 3; break;

        /* PLA */
        case 0x68: PLA(); state.dbg.op="PLA"; state.cycles = 4; break;

        /* PLP */
        case 0x28: PLP(); state.dbg.op="PLP"; state.cycles = 4; break;

        /* ROL */
        case 0x2A: ROL_ACC(); state.cycles = 2; goto ROL;
        case 0x26: ROL(AM_ZPG()); state.cycles = 5; goto ROL;
        case 0x36: ROL(AM_ZPX()); state.cycles = 6; goto ROL;
        case 0x2E: ROL(AM_ABS()); state.cycles = 6; goto ROL;
        case 0x3E: ROL(AM_ABX()); state.cycles = 7; goto ROL;
        ROL: state.dbg.op = "ROL"; break;

        /* ROR */
        case 0x6A: ROR_ACC(); state.cycles = 2; goto ROR;
        case 0x66: ROR(AM_ZPG()); state.cycles = 5; goto ROR;
        case 0x76: ROR(AM_ZPX()); state.cycles = 6; goto ROR;
        case 0x6E: ROR(AM_ABS()); state.cycles = 6; goto ROR;
        case 0x7E: ROR(AM_ABX()); state.cycles = 7; goto ROR;
        ROR: state.dbg.op = "ROR"; break;

        /* RTI */
        case 0x40: RTI(); state.dbg.op="RTI"; state.cycles = 6; break;

        /* RTS */
        case 0x60: RTS(); state.dbg.op="RTS"; state.cycles = 6; break;

        /* SBC */
        case 0xE9: SBC(AM_IMM()); state.cycles = 2; goto SBC;
        case 0xE5: SBC(AM_ZPG()); state.cycles = 3; goto SBC;
        case 0xF5: SBC(AM_ZPX()); state.cycles = 4; goto SBC;
        case 0xED: SBC(AM_ABS()); state.cycles = 4; goto SBC;
        case 0xFD: SBC(AM_ABX()); state.cycles = 4 + state.page_crossed; goto SBC;
        case 0xF9: SBC(AM_ABY()); state.cycles = 4 + state.page_crossed; goto SBC;
        case 0xE1: SBC(AM_INX()); state.cycles = 6; goto SBC;
        case 0xF1: SBC(AM_INY()); state.cycles = 5 + state.page_crossed; goto SBC;
        SBC: state.dbg.op = "SBC"; break;

        /* SEC */
        case 0x38: SEC(); state.dbg.op="SEC"; state.cycles = 2; break;

        /* SED */
        case 0xF8: SED(); state.dbg.op="SED"; state.cycles = 2; break;

        /* SEI */
        case 0x78: SEI(); state.dbg.op="SEI"; state.cycles = 2; break;

        /* STA */
        case 0x85: STA(AM_ZPG()); state.cycles = 3; goto STA;
        case 0x95: STA(AM_ZPX()); state.cycles = 4; goto STA;
        case 0x8D: STA(AM_ABS()); state.cycles = 4; goto STA;
        case 0x9D: STA(AM_ABX()); state.cycles = 5; goto STA;
        case 0x99: STA(AM_ABY()); state.cycles = 5; goto STA;
        case 0x81: STA(AM_INX()); state.cycles = 6; goto STA;
        case 0x91: STA(AM_INY()); state.cycles = 6; goto STA;
        STA: state.dbg.op = "STA"; break;

        /* STX */
        case 0x86: STX(AM_ZPG()); state.cycles = 3; goto STX;
        case 0x96: STX(AM_ZPY()); state.cycles = 4; goto STX;
        case 0x8E: STX(AM_ABS()); state.cycles = 4; goto STX;
        STX: state.dbg.op = "STX"; break;

        /* STY */
        case 0x84: STY(AM_ZPG()); state.cycles = 3; goto STY;
        case 0x94: STY(AM_ZPX()); state.cycles = 4; goto STY;
        case 0x8C: STY(AM_ABS()); state.cycles = 4; goto STY;
        STY: state.dbg.op = "STY"; break;

        /* TAX */
        case 0xAA: TAX(); state.dbg.op="TAX"; state.cycles = 2; break;

        /* TAY */
        case 0xA8: TAY(); state.dbg.op="TAY"; state.cycles = 2; break;

        /* TSX */
        case 0xBA: TSX(); state.dbg.op="TSX"; state.cycles = 2; break;

        /* TXA */
        case 0x8A: TXA(); state.dbg.op="TXA"; state.cycles = 2; break;

        /* TXS */
        case 0x9A: TXS(); state.dbg.op="TXS"; state.cycles = 2; break;

        /* TYA */
        case 0x98: TYA(); state.dbg.op="TYA"; state.cycles = 2; break;



        /* НЕОФИЦИАЛЬНЫЕ ОПКОДЫ */

        /* ALR */
        case 0x4B: ALR(AM_IMM()); state.dbg.op="ALR"; state.cycles = 2; break;

        /* ANC */
        case 0x0B:
        case 0x2B: ANC(AM_IMM()); state.dbg.op="ANC"; state.cycles = 2; break;

        /* ANE */
        case 0x8B: ANE(AM_IMM()); state.dbg.op="ANE"; state.cycles = 2; break;

        /* ARR */
        case 0x6B: ARR(AM_IMM()); state.dbg.op="ARR"; state.cycles = 2; break;

        /* DCP */
        case 0xC7: DCP(AM_ZPG()); state.cycles = 5; goto DCP;
        case 0xD7: DCP(AM_ZPX()); state.cycles = 6; goto DCP;
        case 0xCF: DCP(AM_ABS()); state.cycles = 6; goto DCP;
        case 0xDF: DCP(AM_ABX()); state.cycles = 7; goto DCP;
        case 0xDB: DCP(AM_ABY()); state.cycles = 7; goto DCP;
        case 0xC3: DCP(AM_INX()); state.cycles = 8; goto DCP;
        case 0xD3: DCP(AM_INY()); state.cycles = 8; goto DCP;
        DCP: state.dbg.op = "DCP"; break;

        /* ISC */
        case 0xE7: ISC(AM_ZPG()); state.cycles = 5; goto ISC;
        case 0xF7: ISC(AM_ZPX()); state.cycles = 6; goto ISC;
        case 0xEF: ISC(AM_ABS()); state.cycles = 6; goto ISC;
        case 0xFF: ISC(AM_ABX()); state.cycles = 7; goto ISC;
        case 0xFB: ISC(AM_ABY()); state.cycles = 7; goto ISC;
        case 0xE3: ISC(AM_INX()); state.cycles = 8; goto ISC;
        case 0xF3: ISC(AM_INY()); state.cycles = 8; goto ISC;
        ISC: state.dbg.op = "ISC"; break;

        /* LAS */
        case 0xBB: LAS(AM_ABY()); state.dbg.op = "LAS"; state.cycles = 4 + state.page_crossed; break;

        /* LAX */
        case 0xA7: LAX(AM_ZPG()); state.cycles = 3; goto LAX;
        case 0xB7: LAX(AM_ZPY()); state.cycles = 4; goto LAX;
        case 0xAF: LAX(AM_ABS()); state.cycles = 4; goto LAX;
        case 0xBF: LAX(AM_ABY()); state.cycles = 4 + state.page_crossed; goto LAX;
        case 0xA3: LAX(AM_INX()); state.cycles = 6; goto LAX;
        case 0xB3: LAX(AM_INY()); state.cycles = 5 + state.page_crossed; goto LAX;
        LAX: state.dbg.op = "LAX"; break;

        /* LXA */
        case 0xAB: LXA(AM_IMM()); state.dbg.op = "LXA"; state.cycles = 2; break;

        /* RLA */
        case 0x27: RLA(AM_ZPG()); state.cycles = 5; goto RLA;
        case 0x37: RLA(AM_ZPX()); state.cycles = 6; goto RLA;
        case 0x23: RLA(AM_INX()); state.cycles = 8; goto RLA;
        case 0x33: RLA(AM_INY()); state.cycles = 8; goto RLA;
        case 0x2F: RLA(AM_ABS()); state.cycles = 6; goto RLA;
        case 0x3F: RLA(AM_ABX()); state.cycles = 7; goto RLA;
        case 0x3B: RLA(AM_ABY()); state.cycles = 7; goto RLA;
        RLA: state.dbg.op = "RLA"; break;

        /* RRA */
        case 0x67: RRA(AM_ZPG()); state.cycles = 5; goto RRA;
        case 0x77: RRA(AM_ZPX()); state.cycles = 6; goto RRA;
        case 0x63: RRA(AM_INX()); state.cycles = 8; goto RRA;
        case 0x73: RRA(AM_INY()); state.cycles = 8; goto RRA;
        case 0x6F: RRA(AM_ABS()); state.cycles = 6; goto RRA;
        case 0x7F: RRA(AM_ABX()); state.cycles = 7; goto RRA;
        case 0x7B: RRA(AM_ABY()); state.cycles = 7; goto RRA;
        RRA: state.dbg.op = "RRA"; break;

        /* SAX */
        case 0x87: SAX(AM_ZPG()); state.cycles = 3; goto SAX;
        case 0x97: SAX(AM_ZPY()); state.cycles = 4; goto SAX;
        case 0x8F: SAX(AM_ABS()); state.cycles = 4; goto SAX;
        case 0x83: SAX(AM_INX()); state.cycles = 6; goto SAX;
        SAX: state.dbg.op = "SAX"; break;

        /* SBX */
        case 0xCB: SBX(AM_IMM()); state.dbg.op = "SBX"; state.cycles = 2; break;

        /* SHA */
        case 0x93: SHA(AM_INY()); state.cycles = 6; goto SHA;
        case 0x9F: SHA(AM_ABS()); state.cycles = 5; goto SHA;
        SHA: state.dbg.op = "SHA"; break;

        /* SHX */
        case 0x9E: SHX(AM_ABY()); state.dbg.op = "SHX"; state.cycles = 5; break;

        /* SHY */
        case 0x9C: SHY(AM_ABX()); state.dbg.op = "SHY"; state.cycles = 5; break;

        /* SLO */
        case 0x07: SLO(AM_ZPG()); state.cycles = 5; goto SLO;
        case 0x17: SLO(AM_ZPX()); state.cycles = 6; goto SLO;
        case 0x03: SLO(AM_INX()); state.cycles = 8; goto SLO;
        case 0x13: SLO(AM_INY()); state.cycles = 8; goto SLO;
        case 0x0F: SLO(AM_ABS()); state.cycles = 6; goto SLO;
        case 0x1F: SLO(AM_ABX()); state.cycles = 7; goto SLO;
        case 0x1B: SLO(AM_ABY()); state.cycles = 7; goto SLO;
        SLO: state.dbg.op = "SLO"; break;

        /* SRE */
        case 0x47: SRE(AM_ZPG()); state.cycles = 5; goto SRE;
        case 0x57: SRE(AM_ZPX()); state.cycles = 6; goto SRE;
        case 0x43: SRE(AM_INX()); state.cycles = 8; goto SRE;
        case 0x53: SRE(AM_INY()); state.cycles = 8; goto SRE;
        case 0x4F: SRE(AM_ABS()); state.cycles = 6; goto SRE;
        case 0x5F: SRE(AM_ABX()); state.cycles = 7; goto SRE;
        case 0x5B: SRE(AM_ABY()); state.cycles = 7; goto SRE;
        SRE: state.dbg.op = "SRE"; break;

        /* TAS */
        case 0x9B: TAS(AM_ABY()); state.dbg.op="TAS"; state.cycles = 5; break;

        /* USBC */
        case 0xEB: USBC(AM_IMM()); state.dbg.op="USBC"; state.cycles = 2; break;

        /* KIL */
        case 0x02: case 0x12: case 0x22: case 0x32:
        case 0x42: case 0x52: case 0x62: case 0x72:
        case 0x92: case 0xB2: case 0xD2: case 0xF2:
            KIL(AM_IMP()); state.dbg.op="KIL"; state.cycles = 0;
            break;

        /* NOP */
        case 0x80:
        case 0x82:
        case 0x89:
        case 0xC2:
        case 0xE2: NOP_IGN(AM_IMM()); state.dbg.op="NOP"; state.cycles = 2; break;

        case 0x04:
        case 0x44:
        case 0x64: NOP_IGN(AM_ZPG()); state.dbg.op="NOP"; state.cycles = 3; break;

        case 0x14:
        case 0x34:
        case 0x54:
        case 0x74:
        case 0xD4:
        case 0xF4: NOP_IGN(AM_ZPX()); state.dbg.op="NOP"; state.cycles = 4; break;

        case 0x0C: NOP_IGN(AM_ABS()); state.dbg.op="NOP"; state.cycles = 4; break;

        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC: NOP_IGN(AM_ABX()); state.dbg.op="NOP"; state.cycles = 4 + state.page_crossed; break;

        default:
            /* Неизвестный опкод */
            state.dbg.op="UNK"; state.cycles = 2; break;
    }
}


void Cpu::exec() {
    if (const u32 d = mem->getDma(); d != 0) {
        state.cycles = d;
        state.total_cycles += state.cycles;
        return;
    }
    if (state.nmi_delay) {
        state.nmi_delay = false;
        state.do_nmi = false;
        nmi();
        return;
    }
    if (state.do_nmi) {
        state.nmi_delay = true;
    }
    if(state.do_irq) {
        state.do_irq = false;
        if (!(state.regs.P & I)) {
            irq();
            return;
        }
    }

    state.cycles = 0;
    step();
    state.total_cycles += state.cycles;
}
