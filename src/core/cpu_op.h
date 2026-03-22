#include <unordered_map>

#include "common/types.h"

#include "core/cpu.h"

namespace Core {

/* ДОКУМЕНТИРОВАННЫЕ ОПКОДЫ */

/* Операции загрузки/хранения */

/* LDA: M -> A */
inline void CPU::C6502::LDA(u16 addr) {
    regs.A = p->memRead(addr);
    set_nz(regs.A);
}

/* STA: A -> M */
inline void CPU::C6502::STA(u16 addr) { p->memWrite(addr, regs.A); }

/* LDX: M -> X */
inline void CPU::C6502::LDX(u16 addr) {
    regs.X = p->memRead(addr);
    set_nz(regs.X);
}

/* STX: X -> M */
inline void CPU::C6502::STX(u16 addr) { p->memWrite(addr, regs.X); }

/* LDY: M -> Y */
inline void CPU::C6502::LDY(u16 addr) {
    regs.Y = p->memRead(addr);
    set_nz(regs.Y);
}

/* STY: Y -> M */
inline void CPU::C6502::STY(u16 addr) { p->memWrite(addr, regs.Y); }

/* Операции передачи */

/* TAX: A -> X */
inline void CPU::C6502::TAX() {
    regs.X = regs.A;
    set_nz(regs.X);
}

/* TXA: X -> A */
inline void CPU::C6502::TXA() {
    regs.A = regs.X;
    set_nz(regs.A);
}

/* TAY: A -> Y */
inline void CPU::C6502::TAY() {
    regs.Y = regs.A;
    set_nz(regs.Y);
}

/* TYA: Y -> A */
inline void CPU::C6502::TYA() {
    regs.A = regs.Y;
    set_nz(regs.A);
}

/* Арифметические операции */

/* Вспомогательная функция */
inline void CPU::C6502::ArithmWithCarry(const u16 val) {
    const u16 tmpA =
        static_cast<u16>(regs.A) + val + static_cast<u16>(regs.P & C);

    set_flag(C, tmpA > 0xFF);
    set_flag(Z, (tmpA & 0x00FF) == 0);
    set_flag(N, tmpA & N);

    /* (A ^ R) & ~(A ^ M) */
    set_flag(V, ((static_cast<u16>(regs.A) ^ tmpA) &
                 (~(static_cast<u16>(regs.A) ^ val))) &
                    0x0080);

    regs.A = tmpA & 0x00FF;
}

/* ADC: A += M + C */
inline void CPU::C6502::ADC(u16 addr) {
    const u16 val = p->memRead(addr);
    ArithmWithCarry(val);
}

/* SBC: A += -M + C */
inline void CPU::C6502::SBC(u16 addr) {
    const u16 val = p->memRead(addr) ^ 0x00FF; /* -M */
    ArithmWithCarry(val);
}

/* INC: M + 1 -> M */
inline void CPU::C6502::INC(u16 addr) {
    u8 val = p->memRead(addr);
    val++;
    set_nz(val);
    p->memWrite(addr, val);
}

/* INC: M - 1 -> M */
inline void CPU::C6502::DEC(u16 addr) {
    u8 val = p->memRead(addr);
    val--;
    set_nz(val);
    p->memWrite(addr, val);
}

/* INX: X + 1 -> X */
inline void CPU::C6502::INX() { set_nz(++regs.X); }

/* DEX: X - 1 -> X */
inline void CPU::C6502::DEX() { set_nz(--regs.X); }

/* INY: Y + 1 -> Y */
inline void CPU::C6502::INY() { set_nz(++regs.Y); }

/* DEY: Y - 1 -> Y */
inline void CPU::C6502::DEY() { set_nz(--regs.Y); }

/* Сдвиги */

/* ASL: C <- [A или M] <- 0 */
inline void CPU::C6502::ASL(u16 addr) {
    const bool cond = p->c.opEntry.am == C6502::IMP;
    const u8 val = (cond) ? regs.A : p->memRead(addr);
    const u16 tmp = static_cast<u16>(val << 1);

    set_flag(C, (tmp & 0xFF00) > 0);
    set_flag(Z, (tmp & 0x00FF) == 0);
    set_flag(N, tmp & N);

    if (cond)
        regs.A = tmp & 0x00FF;
    else
        p->memWrite(addr, tmp & 0x00FF);
}

/* LSR: 0 -> [A или M] -> C */
inline void CPU::C6502::LSR(u16 addr) {
    const bool cond = p->c.opEntry.am == C6502::IMP;
    const u8 val = (cond) ? regs.A : p->memRead(addr);
    const u16 tmp = val >> 1;

    set_flag(C, val & 0x0001);
    set_nz16(tmp);

    if (cond)
        regs.A = tmp & 0x00FF;
    else
        p->memWrite(addr, tmp & 0x00FF);
}

/* ROL: C <- [76543210] <- C */
inline void CPU::C6502::ROL(u16 addr) {
    const bool cond = p->c.opEntry.am == C6502::IMP;
    const u8 val = (cond) ? regs.A : p->memRead(addr);
    const u16 tmp = static_cast<u16>(val << 1) | (regs.P & C);

    set_flag(C, tmp & 0xFF00);
    set_nz16(tmp);

    if (cond)
        regs.A = tmp & 0x00FF;
    else
        p->memWrite(addr, tmp & 0x00FF);
}

/* ROR: C -> [76543210] -> C */
inline void CPU::C6502::ROR(u16 addr) {
    const bool cond = p->c.opEntry.am == C6502::IMP;
    const u8 val = (cond) ? regs.A : p->memRead(addr);
    const u16 tmp = static_cast<u16>((regs.P & C) << 7) | (val >> 1);

    set_flag(C, val & 0x01);
    set_nz16(tmp);

    if (cond)
        regs.A = tmp & 0x00FF;
    else
        p->memWrite(addr, tmp & 0x00FF);
}

/* Логические операции */

/* AND: A = A & M */
inline void CPU::C6502::AND(u16 addr) {
    regs.A &= p->memRead(addr);
    set_nz(regs.A);
}

/* ORA: A = A | memory */
inline void CPU::C6502::ORA(u16 addr) {
    regs.A |= p->memRead(addr);
    set_nz(regs.A);
}

/* EOR: A = A ^ memory */
inline void CPU::C6502::EOR(u16 addr) {
    regs.A ^= p->memRead(addr);
    set_nz(regs.A);
}

/* BIT: A & M */
inline void CPU::C6502::BIT(u16 addr) {
    const u8 val = p->memRead(addr);
    set_flag(Z, (regs.A & val & 0x00FF) == 0);
    set_flag(N, val & N);
    set_flag(V, val & V);
}

/* Операции сравнения */

/* CMP: A - M */
inline void CPU::C6502::CMP(u16 addr) {
    const u8 val = p->memRead(addr);
    const u16 tmp = static_cast<u16>(regs.A) - static_cast<u16>(val);
    set_flag(C, regs.A >= val);
    set_nz16(tmp);
}

/* CPX: X - M */
inline void CPU::C6502::CPX(u16 addr) {
    const u8 val = p->memRead(addr);
    const u16 tmp = static_cast<u16>(regs.X) - static_cast<u16>(val);
    set_flag(C, regs.X >= val);
    set_nz16(tmp);
}

/* CPY: Y - M */
inline void CPU::C6502::CPY(u16 addr) {
    const u8 val = p->memRead(addr);
    const u16 tmp = static_cast<u16>(regs.Y) - static_cast<u16>(val);
    set_flag(C, regs.Y >= val);
    set_nz16(tmp);
}

/* Операции ветвления */

/* Вспомогательная функция */
inline void CPU::C6502::Branch(u16 addr) {
    const u16 old = regs.PC;
    regs.PC = addr;
    op_cycles += 1 + ((old ^ addr) & 0xFF00 ? 1 : 0);
}

/* BCC: Branch on Carry Clear */
inline void CPU::C6502::BCC(u16 addr) {
    if (!(regs.P & C))
        Branch(addr);
}

/* BCS: Branch on Carry Set */
inline void CPU::C6502::BCS(u16 addr) {
    if (regs.P & C)
        Branch(addr);
}

/* BEQ: Branch on Equal */
inline void CPU::C6502::BEQ(u16 addr) {
    if (regs.P & Z)
        Branch(addr);
}

/* BNE: Branch on Not Equal */
inline void CPU::C6502::BNE(u16 addr) {
    if (!(regs.P & Z))
        Branch(addr);
}

/* BMI: Branch on Minus */
inline void CPU::C6502::BMI(u16 addr) {
    if (regs.P & N)
        Branch(addr);
}

/* BPL: Branch on Plus */
inline void CPU::C6502::BPL(u16 addr) {
    if (!(regs.P & N))
        Branch(addr);
}

/* BVS: Branch on Overflow Set */
inline void CPU::C6502::BVS(u16 addr) {
    if (regs.P & V)
        Branch(addr);
}

/* BVC: Branch on Overflow Clear */
inline void CPU::C6502::BVC(u16 addr) {
    if (!(regs.P & V))
        Branch(addr);
}

/* Операции перехода */

/* JMP: PC = M */
inline void CPU::C6502::JMP(u16 addr) { regs.PC = addr; }

/* JSR: PC+2 -> (S+3000), S - 2 -> S, (PC+1) -> PC */
inline void CPU::C6502::JSR(u16 addr) {
    regs.PC--;
    push((regs.PC >> 8) & 0x00FF);
    push(regs.PC & 0x00FF);
    regs.PC = addr;
}

/* RTS: (4400 + S + 1) -> PС, S + 2 -> S */
inline void CPU::C6502::RTS() {
    const u8 low = pop();
    const u8 high = pop();
    regs.PC = (static_cast<u16>(high) << 8) | static_cast<u16>(low);
    regs.PC++;
}

/* BRK:
    push PC + 2 high байта на стек
    push PC + 2 low байта на стек
    push NV11DIZC флаги на стек
    PC = ($FFFE)
*/
inline void CPU::C6502::BRK() {
    regs.PC++;
    push((regs.PC >> 8) & 0x00FF);
    push(regs.PC & 0x00FF);
    push(regs.P | B | U);
    set_flag(I, 1);
    regs.PC = read16(0xFFFE);
}

/* RTI:
    pull NVxxDIZC флаги из стека
    pull PC low байт из стека
    pull PC high байт из стека
*/
inline void CPU::C6502::RTI() {
    regs.P = pop();
    const u8 low = pop();
    const u8 high = pop();
    set_flag(B, 0);
    set_flag(U, 1);
    regs.PC = (static_cast<u16>(high) << 8) | static_cast<u16>(low);
}

/* Операции со стеком */

/* PHA: Push A на стек */
inline void CPU::C6502::PHA() { push(regs.A); }

/* PLA: Pull A со стека */
inline void CPU::C6502::PLA() {
    regs.A = pop();
    set_nz(regs.A);
}

/* PHP: Push P на стек */
inline void CPU::C6502::PHP() { push(regs.P | U | B); }

/* PLP: Pull P со стека */
inline void CPU::C6502::PLP() {
    regs.P = pop();
    set_flag(B, 0);
    set_flag(U, 1);
}

/* TSX: SP = X */
inline void CPU::C6502::TSX() {
    regs.X = regs.SP;
    set_nz(regs.X);
}

/* TXS: X = SP */
inline void CPU::C6502::TXS() { regs.SP = regs.X; }

/* Операции с флагами */

/* SEC: Set Carry */
inline void CPU::C6502::SEC() { set_flag(C, 1); }

/* CLC: Clear Carry */
inline void CPU::C6502::CLC() { set_flag(C, 0); }

/* SEI: Set Interrupt */
inline void CPU::C6502::SEI() { set_flag(I, 1); }

/* CLI: Clear Interrupt */
inline void CPU::C6502::CLI() { set_flag(I, 0); }

/* SED: Set Decimal*/
inline void CPU::C6502::SED() { set_flag(D, 1); }

/* CLD: Clear Decimal */
inline void CPU::C6502::CLD() { set_flag(D, 0); }

/* CLV: Clear Overflow */
inline void CPU::C6502::CLV() { set_flag(V, 0); }

/* NOP: ничего не делать */
inline void CPU::C6502::NOP() { /* Ничего */ }

/* НЕДОКУМЕНТИРОВАННЫЕ ОПКОДЫ */

inline void CPU::C6502::ALR(u16 addr) {
    AND(addr);
    LSR(0);
}

inline void CPU::C6502::ANC(u16 addr) {
    AND(addr);
    set_flag(C, regs.A & N);
}

inline void CPU::C6502::ANE(u16 addr) {
    regs.A =
        static_cast<u8>((regs.A | CPU::CONSTANT) & regs.X & p->memRead(addr));
    set_nz(regs.A);
}

inline void CPU::C6502::ARR(u16 addr) {
    AND(addr);
    ROR(0);
}

inline void CPU::C6502::DCP(u16 addr) {
    DEC(addr);
    CMP(addr);
}

inline void CPU::C6502::ISC(u16 addr) {
    INC(addr);
    SBC(addr);
}

inline void CPU::C6502::LAS(u16 addr) {
    const u8 val = static_cast<u8>(regs.SP & p->memRead(addr));
    regs.X = val;
    regs.A = val;
    regs.SP = val;
    set_nz(val);
}

inline void CPU::C6502::LAX(u16 addr) {
    regs.A = p->memRead(addr);
    regs.X = regs.A;
    set_nz(regs.A);
}

inline void CPU::C6502::LXA(u16 addr) {
    const u8 val = static_cast<u8>((regs.A | CONSTANT) & p->memRead(addr));
    regs.A = val;
    regs.X = val;
    set_nz(val);
}

inline void CPU::C6502::RLA(u16 addr) {
    ROL(addr);
    AND(addr);
}

inline void CPU::C6502::RRA(u16 addr) {
    ROR(addr);
    ADC(addr);
}

inline void CPU::C6502::SAX(u16 addr) {
    p->memWrite(addr, static_cast<u8>(regs.A & regs.X));
}

inline void CPU::C6502::SBX(u16 addr) {
    const u8 val = p->memRead(addr);
    const u8 ax = static_cast<u8>(regs.A & regs.X);
    const u16 tmp = static_cast<u16>(ax) - static_cast<u16>(val);
    set_flag(C, ax >= val);
    regs.X = static_cast<u8>(tmp & 0x00FF);
    set_nz(regs.X);
}

inline void CPU::C6502::SHA(u16 addr) {
    u8 h = static_cast<u8>((addr >> 8) + 1);
    if (((addr & 0x00FF) + regs.Y) >= 0x0100)
        h--;
    p->memWrite(addr, static_cast<u8>(regs.A & regs.X & h));
}

inline void CPU::C6502::SHX(u16 addr) {
    u8 h = static_cast<u8>((addr >> 8) + 1);
    if (((addr & 0x00FF) + regs.Y) >= 0x0100)
        h--;
    p->memWrite(addr, static_cast<u8>(regs.X & h));
}

inline void CPU::C6502::SHY(u16 addr) {
    u8 h = static_cast<u8>((addr >> 8) + 1);
    if (((addr & 0x00FF) + regs.X) >= 0x0100)
        h--;
    p->memWrite(addr, static_cast<u8>(regs.Y & h));
}

inline void CPU::C6502::SLO(u16 addr) {
    ASL(addr);
    ORA(addr);
}

inline void CPU::C6502::SRE(u16 addr) {
    LSR(addr);
    EOR(addr);
}

inline void CPU::C6502::TAS(u16 addr) {
    regs.SP = static_cast<u8>(regs.A & regs.X);
    u8 h = static_cast<u8>((addr >> 8) + 1);
    if (((addr & 0x00FF) + regs.Y) >= 0x0100)
        h--;
    p->memWrite(addr, static_cast<u8>(regs.SP & h));
}

inline void CPU::C6502::USBC(u16 addr) { SBC(addr); }

inline void CPU::C6502::KIL(u16 /*addr*/) { regs.PC--; }

inline void CPU::C6502::NOP_IGN(u16 addr) { (void)p->memRead(addr); }

/* Таблица опкодов (opcode, функция, адресация, такты) */
inline const std::unordered_map<u16, CPU::C6502::OpEntry> CPU::C6502::OP_TABLE =
    {
        /* ADC */
        {0x69, {"ADC", IMM, 2, &C6502::ADC, nullptr, 0}},
        {0x65, {"ADC", ZPG, 3, &C6502::ADC, nullptr, 0}},
        {0x75, {"ADC", ZPGX, 4, &C6502::ADC, nullptr, 0}},
        {0x6D, {"ADC", ABS, 4, &C6502::ADC, nullptr, 0}},
        {0x7D, {"ADC", ABSX, 4, &C6502::ADC, nullptr, 1}},
        {0x79, {"ADC", ABSY, 4, &C6502::ADC, nullptr, 1}},
        {0x61, {"ADC", INDX, 6, &C6502::ADC, nullptr, 0}},
        {0x71, {"ADC", INDY, 5, &C6502::ADC, nullptr, 1}},

        /* AND */
        {0x29, {"AND", IMM, 2, &C6502::AND, nullptr, 0}},
        {0x25, {"AND", ZPG, 3, &C6502::AND, nullptr, 0}},
        {0x35, {"AND", ZPGX, 4, &C6502::AND, nullptr, 0}},
        {0x2D, {"AND", ABS, 4, &C6502::AND, nullptr, 0}},
        {0x3D, {"AND", ABSX, 4, &C6502::AND, nullptr, 1}},
        {0x39, {"AND", ABSY, 4, &C6502::AND, nullptr, 1}},
        {0x21, {"AND", INDX, 6, &C6502::AND, nullptr, 0}},
        {0x31, {"AND", INDY, 5, &C6502::AND, nullptr, 1}},

        /* ASL */
        {0x0A, {"ASL", IMP, 2, &C6502::ASL, nullptr, 0}},
        {0x06, {"ASL", ZPG, 5, &C6502::ASL, nullptr, 0}},
        {0x16, {"ASL", ZPGX, 6, &C6502::ASL, nullptr, 0}},
        {0x0E, {"ASL", ABS, 6, &C6502::ASL, nullptr, 0}},
        {0x1E, {"ASL", ABSX, 7, &C6502::ASL, nullptr, 0}},

        /* Branch */
        {0x90, {"BCC", REL, 2, &C6502::BCC, nullptr, 0}},
        {0xB0, {"BCS", REL, 2, &C6502::BCS, nullptr, 0}},
        {0xF0, {"BEQ", REL, 2, &C6502::BEQ, nullptr, 0}},
        {0x30, {"BMI", REL, 2, &C6502::BMI, nullptr, 0}},
        {0xD0, {"BNE", REL, 2, &C6502::BNE, nullptr, 0}},
        {0x10, {"BPL", REL, 2, &C6502::BPL, nullptr, 0}},
        {0x50, {"BVC", REL, 2, &C6502::BVC, nullptr, 0}},
        {0x70, {"BVS", REL, 2, &C6502::BVS, nullptr, 0}},

        /* BIT */
        {0x24, {"BIT", ZPG, 3, &C6502::BIT, nullptr, 0}},
        {0x2C, {"BIT", ABS, 4, &C6502::BIT, nullptr, 0}},

        /* BRK */
        {0x00, {"BRK", IMP, 7, nullptr, &C6502::BRK, 0}},

        /* Flag ops */
        {0x18, {"CLC", IMP, 2, nullptr, &C6502::CLC, 0}},
        {0xD8, {"CLD", IMP, 2, nullptr, &C6502::CLD, 0}},
        {0x58, {"CLI", IMP, 2, nullptr, &C6502::CLI, 0}},
        {0xB8, {"CLV", IMP, 2, nullptr, &C6502::CLV, 0}},
        {0x38, {"SEC", IMP, 2, nullptr, &C6502::SEC, 0}},
        {0xF8, {"SED", IMP, 2, nullptr, &C6502::SED, 0}},
        {0x78, {"SEI", IMP, 2, nullptr, &C6502::SEI, 0}},

        /* CMP */
        {0xC9, {"CMP", IMM, 2, &C6502::CMP, nullptr, 0}},
        {0xC5, {"CMP", ZPG, 3, &C6502::CMP, nullptr, 0}},
        {0xD5, {"CMP", ZPGX, 4, &C6502::CMP, nullptr, 0}},
        {0xCD, {"CMP", ABS, 4, &C6502::CMP, nullptr, 0}},
        {0xDD, {"CMP", ABSX, 4, &C6502::CMP, nullptr, 1}},
        {0xD9, {"CMP", ABSY, 4, &C6502::CMP, nullptr, 1}},
        {0xC1, {"CMP", INDX, 6, &C6502::CMP, nullptr, 0}},
        {0xD1, {"CMP", INDY, 5, &C6502::CMP, nullptr, 1}},

        /* CPX / CPY */
        {0xE0, {"CPX", IMM, 2, &C6502::CPX, nullptr, 0}},
        {0xE4, {"CPX", ZPG, 3, &C6502::CPX, nullptr, 0}},
        {0xEC, {"CPX", ABS, 4, &C6502::CPX, nullptr, 0}},

        {0xC0, {"CPY", IMM, 2, &C6502::CPY, nullptr, 0}},
        {0xC4, {"CPY", ZPG, 3, &C6502::CPY, nullptr, 0}},
        {0xCC, {"CPY", ABS, 4, &C6502::CPY, nullptr, 0}},

        /* DEC */
        {0xC6, {"DEC", ZPG, 5, &C6502::DEC, nullptr, 0}},
        {0xD6, {"DEC", ZPGX, 6, &C6502::DEC, nullptr, 0}},
        {0xCE, {"DEC", ABS, 6, &C6502::DEC, nullptr, 0}},
        {0xDE, {"DEC", ABSX, 7, &C6502::DEC, nullptr, 0}},

        /* DEX / DEY */
        {0xCA, {"DEX", IMP, 2, nullptr, &C6502::DEX, 0}},
        {0x88, {"DEY", IMP, 2, nullptr, &C6502::DEY, 0}},

        /* EOR */
        {0x49, {"EOR", IMM, 2, &C6502::EOR, nullptr, 0}},
        {0x45, {"EOR", ZPG, 3, &C6502::EOR, nullptr, 0}},
        {0x55, {"EOR", ZPGX, 4, &C6502::EOR, nullptr, 0}},
        {0x4D, {"EOR", ABS, 4, &C6502::EOR, nullptr, 0}},
        {0x5D, {"EOR", ABSX, 4, &C6502::EOR, nullptr, 1}},
        {0x59, {"EOR", ABSY, 4, &C6502::EOR, nullptr, 1}},
        {0x41, {"EOR", INDX, 6, &C6502::EOR, nullptr, 0}},
        {0x51, {"EOR", INDY, 5, &C6502::EOR, nullptr, 1}},

        /* INC */
        {0xE6, {"INC", ZPG, 5, &C6502::INC, nullptr, 0}},
        {0xF6, {"INC", ZPGX, 6, &C6502::INC, nullptr, 0}},
        {0xEE, {"INC", ABS, 6, &C6502::INC, nullptr, 0}},
        {0xFE, {"INC", ABSX, 7, &C6502::INC, nullptr, 0}},

        /* INX / INY */
        {0xE8, {"INX", IMP, 2, nullptr, &C6502::INX, 0}},
        {0xC8, {"INY", IMP, 2, nullptr, &C6502::INY, 0}},

        /* JMP / JSR */
        {0x4C, {"JMP", ABS, 3, &C6502::JMP, nullptr, 0}},
        {0x6C, {"JMP", IND, 5, &C6502::JMP, nullptr, 0}},
        {0x20, {"JSR", ABS, 6, &C6502::JSR, nullptr, 0}},

        /* LDA */
        {0xA9, {"LDA", IMM, 2, &C6502::LDA, nullptr, 0}},
        {0xA5, {"LDA", ZPG, 3, &C6502::LDA, nullptr, 0}},
        {0xB5, {"LDA", ZPGX, 4, &C6502::LDA, nullptr, 0}},
        {0xAD, {"LDA", ABS, 4, &C6502::LDA, nullptr, 0}},
        {0xBD, {"LDA", ABSX, 4, &C6502::LDA, nullptr, 1}},
        {0xB9, {"LDA", ABSY, 4, &C6502::LDA, nullptr, 1}},
        {0xA1, {"LDA", INDX, 6, &C6502::LDA, nullptr, 0}},
        {0xB1, {"LDA", INDY, 5, &C6502::LDA, nullptr, 1}},

        /* LDX */
        {0xA2, {"LDX", IMM, 2, &C6502::LDX, nullptr, 0}},
        {0xA6, {"LDX", ZPG, 3, &C6502::LDX, nullptr, 0}},
        {0xB6, {"LDX", ZPGY, 4, &C6502::LDX, nullptr, 0}},
        {0xAE, {"LDX", ABS, 4, &C6502::LDX, nullptr, 0}},
        {0xBE, {"LDX", ABSY, 4, &C6502::LDX, nullptr, 1}},

        /* LDY */
        {0xA0, {"LDY", IMM, 2, &C6502::LDY, nullptr, 0}},
        {0xA4, {"LDY", ZPG, 3, &C6502::LDY, nullptr, 0}},
        {0xB4, {"LDY", ZPGX, 4, &C6502::LDY, nullptr, 0}},
        {0xAC, {"LDY", ABS, 4, &C6502::LDY, nullptr, 0}},
        {0xBC, {"LDY", ABSX, 4, &C6502::LDY, nullptr, 1}},

        /* LSR */
        {0x4A, {"LSR", IMP, 2, &C6502::LSR, nullptr, 0}},
        {0x46, {"LSR", ZPG, 5, &C6502::LSR, nullptr, 0}},
        {0x56, {"LSR", ZPGX, 6, &C6502::LSR, nullptr, 0}},
        {0x4E, {"LSR", ABS, 6, &C6502::LSR, nullptr, 0}},
        {0x5E, {"LSR", ABSX, 7, &C6502::LSR, nullptr, 0}},

        /* NOP */
        {0xEA, {"NOP", IMP, 2, nullptr, &C6502::NOP, 0}},
        {0x1A, {"NOP", IMP, 2, nullptr, &C6502::NOP, 0}},
        {0x3A, {"NOP", IMP, 2, nullptr, &C6502::NOP, 0}},
        {0x5A, {"NOP", IMP, 2, nullptr, &C6502::NOP, 0}},
        {0x7A, {"NOP", IMP, 2, nullptr, &C6502::NOP, 0}},
        {0xDA, {"NOP", IMP, 2, nullptr, &C6502::NOP, 0}},
        {0xFA, {"NOP", IMP, 2, nullptr, &C6502::NOP, 0}},

        /* ORA */
        {0x09, {"ORA", IMM, 2, &C6502::ORA, nullptr, 0}},
        {0x05, {"ORA", ZPG, 3, &C6502::ORA, nullptr, 0}},
        {0x15, {"ORA", ZPGX, 4, &C6502::ORA, nullptr, 0}},
        {0x0D, {"ORA", ABS, 4, &C6502::ORA, nullptr, 0}},
        {0x1D, {"ORA", ABSX, 4, &C6502::ORA, nullptr, 1}},
        {0x19, {"ORA", ABSY, 4, &C6502::ORA, nullptr, 1}},
        {0x01, {"ORA", INDX, 6, &C6502::ORA, nullptr, 0}},
        {0x11, {"ORA", INDY, 5, &C6502::ORA, nullptr, 1}},

        /* Stack */
        {0x48, {"PHA", IMP, 3, nullptr, &C6502::PHA, 0}},
        {0x08, {"PHP", IMP, 3, nullptr, &C6502::PHP, 0}},
        {0x68, {"PLA", IMP, 4, nullptr, &C6502::PLA, 0}},
        {0x28, {"PLP", IMP, 4, nullptr, &C6502::PLP, 0}},

        /* ROL */
        {0x2A, {"ROL", IMP, 2, &C6502::ROL, nullptr, 0}},
        {0x26, {"ROL", ZPG, 5, &C6502::ROL, nullptr, 0}},
        {0x36, {"ROL", ZPGX, 6, &C6502::ROL, nullptr, 0}},
        {0x2E, {"ROL", ABS, 6, &C6502::ROL, nullptr, 0}},
        {0x3E, {"ROL", ABSX, 7, &C6502::ROL, nullptr, 0}},

        /* ROR */
        {0x6A, {"ROR", IMP, 2, &C6502::ROR, nullptr, 0}},
        {0x66, {"ROR", ZPG, 5, &C6502::ROR, nullptr, 0}},
        {0x76, {"ROR", ZPGX, 6, &C6502::ROR, nullptr, 0}},
        {0x6E, {"ROR", ABS, 6, &C6502::ROR, nullptr, 0}},
        {0x7E, {"ROR", ABSX, 7, &C6502::ROR, nullptr, 0}},

        /* RTI / RTS */
        {0x40, {"RTI", IMP, 6, nullptr, &C6502::RTI, 0}},
        {0x60, {"RTS", IMP, 6, nullptr, &C6502::RTS, 0}},

        /* SBC */
        {0xE9, {"SBC", IMM, 2, &C6502::SBC, nullptr, 0}},
        {0xE5, {"SBC", ZPG, 3, &C6502::SBC, nullptr, 0}},
        {0xF5, {"SBC", ZPGX, 4, &C6502::SBC, nullptr, 0}},
        {0xED, {"SBC", ABS, 4, &C6502::SBC, nullptr, 0}},
        {0xFD, {"SBC", ABSX, 4, &C6502::SBC, nullptr, 1}},
        {0xF9, {"SBC", ABSY, 4, &C6502::SBC, nullptr, 1}},
        {0xE1, {"SBC", INDX, 6, &C6502::SBC, nullptr, 0}},
        {0xF1, {"SBC", INDY, 5, &C6502::SBC, nullptr, 1}},

        /* STA */
        {0x85, {"STA", ZPG, 3, &C6502::STA, nullptr, 0}},
        {0x95, {"STA", ZPGX, 4, &C6502::STA, nullptr, 0}},
        {0x8D, {"STA", ABS, 4, &C6502::STA, nullptr, 0}},
        {0x9D, {"STA", ABSX, 5, &C6502::STA, nullptr, 0}},
        {0x99, {"STA", ABSY, 5, &C6502::STA, nullptr, 0}},
        {0x81, {"STA", INDX, 6, &C6502::STA, nullptr, 0}},
        {0x91, {"STA", INDY, 6, &C6502::STA, nullptr, 0}},

        /* STX */
        {0x86, {"STX", ZPG, 3, &C6502::STX, nullptr, 0}},
        {0x96, {"STX", ZPGY, 4, &C6502::STX, nullptr, 0}},
        {0x8E, {"STX", ABS, 4, &C6502::STX, nullptr, 0}},

        /* STY */
        {0x84, {"STY", ZPG, 3, &C6502::STY, nullptr, 0}},
        {0x94, {"STY", ZPGX, 4, &C6502::STY, nullptr, 0}},
        {0x8C, {"STY", ABS, 4, &C6502::STY, nullptr, 0}},

        /* Transfer */
        {0xAA, {"TAX", IMP, 2, nullptr, &C6502::TAX, 0}},
        {0xA8, {"TAY", IMP, 2, nullptr, &C6502::TAY, 0}},
        {0xBA, {"TSX", IMP, 2, nullptr, &C6502::TSX, 0}},
        {0x8A, {"TXA", IMP, 2, nullptr, &C6502::TXA, 0}},
        {0x9A, {"TXS", IMP, 2, nullptr, &C6502::TXS, 0}},
        {0x98, {"TYA", IMP, 2, nullptr, &C6502::TYA, 0}},

        /* ALR / ANC / ANE / ARR */
        {0x4B, {"ALR", IMM, 2, &C6502::ALR, nullptr, 0}},
        {0x0B, {"ANC", IMM, 2, &C6502::ANC, nullptr, 0}},
        {0x2B, {"ANC", IMM, 2, &C6502::ANC, nullptr, 0}},
        {0x8B, {"ANE", IMM, 2, &C6502::ANE, nullptr, 0}},
        {0x6B, {"ARR", IMM, 2, &C6502::ARR, nullptr, 0}},

        /* DCP */
        {0xC7, {"DCP", ZPG, 5, &C6502::DCP, nullptr, 0}},
        {0xD7, {"DCP", ZPGX, 6, &C6502::DCP, nullptr, 0}},
        {0xCF, {"DCP", ABS, 6, &C6502::DCP, nullptr, 0}},
        {0xDF, {"DCP", ABSX, 7, &C6502::DCP, nullptr, 0}},
        {0xDB, {"DCP", ABSY, 7, &C6502::DCP, nullptr, 0}},
        {0xC3, {"DCP", INDX, 8, &C6502::DCP, nullptr, 0}},
        {0xD3, {"DCP", INDY, 8, &C6502::DCP, nullptr, 0}},

        /* ISC */
        {0xE7, {"ISC", ZPG, 5, &C6502::ISC, nullptr, 0}},
        {0xF7, {"ISC", ZPGX, 6, &C6502::ISC, nullptr, 0}},
        {0xEF, {"ISC", ABS, 6, &C6502::ISC, nullptr, 0}},
        {0xFF, {"ISC", ABSX, 7, &C6502::ISC, nullptr, 0}},
        {0xFB, {"ISC", ABSY, 7, &C6502::ISC, nullptr, 0}},
        {0xE3, {"ISC", INDX, 8, &C6502::ISC, nullptr, 0}},
        {0xF3, {"ISC", INDY, 8, &C6502::ISC, nullptr, 0}},

        /* LAS / LAX / LXA */
        {0xBB, {"LAS", ABSY, 4, &C6502::LAS, nullptr, 1}},

        {0xA7, {"LAX", ZPG, 3, &C6502::LAX, nullptr, 0}},
        {0xB7, {"LAX", ZPGY, 4, &C6502::LAX, nullptr, 0}},
        {0xAF, {"LAX", ABS, 4, &C6502::LAX, nullptr, 0}},
        {0xBF, {"LAX", ABSY, 4, &C6502::LAX, nullptr, 1}},
        {0xA3, {"LAX", INDX, 6, &C6502::LAX, nullptr, 0}},
        {0xB3, {"LAX", INDY, 5, &C6502::LAX, nullptr, 1}},

        {0xAB, {"LXA", IMM, 2, &C6502::LXA, nullptr, 0}},

        /* RLA */
        {0x27, {"RLA", ZPG, 5, &C6502::RLA, nullptr, 0}},
        {0x37, {"RLA", ZPGX, 6, &C6502::RLA, nullptr, 0}},
        {0x2F, {"RLA", ABS, 6, &C6502::RLA, nullptr, 0}},
        {0x3F, {"RLA", ABSX, 7, &C6502::RLA, nullptr, 0}},
        {0x3B, {"RLA", ABSY, 7, &C6502::RLA, nullptr, 0}},
        {0x23, {"RLA", INDX, 8, &C6502::RLA, nullptr, 0}},
        {0x33, {"RLA", INDY, 8, &C6502::RLA, nullptr, 0}},

        /* RRA */
        {0x67, {"RRA", ZPG, 5, &C6502::RRA, nullptr, 0}},
        {0x77, {"RRA", ZPGX, 6, &C6502::RRA, nullptr, 0}},
        {0x6F, {"RRA", ABS, 6, &C6502::RRA, nullptr, 0}},
        {0x7F, {"RRA", ABSX, 7, &C6502::RRA, nullptr, 0}},
        {0x7B, {"RRA", ABSY, 7, &C6502::RRA, nullptr, 0}},
        {0x63, {"RRA", INDX, 8, &C6502::RRA, nullptr, 0}},
        {0x73, {"RRA", INDY, 8, &C6502::RRA, nullptr, 0}},

        /* SAX / SBX */
        {0x87, {"SAX", ZPG, 3, &C6502::SAX, nullptr, 0}},
        {0x97, {"SAX", ZPGY, 4, &C6502::SAX, nullptr, 0}},
        {0x8F, {"SAX", ABS, 4, &C6502::SAX, nullptr, 0}},
        {0x83, {"SAX", INDX, 6, &C6502::SAX, nullptr, 0}},

        {0xCB, {"SBX", IMM, 2, &C6502::SBX, nullptr, 0}},

        /* SHA / SHX / SHY */
        {0x93, {"SHA", INDY, 6, &C6502::SHA, nullptr, 0}},
        {0x9F, {"SHA", ABSY, 5, &C6502::SHA, nullptr, 0}},
        {0x9E, {"SHX", ABSY, 5, &C6502::SHX, nullptr, 0}},
        {0x9C, {"SHY", ABSX, 5, &C6502::SHY, nullptr, 0}},

        /* SLO */
        {0x07, {"SLO", ZPG, 5, &C6502::SLO, nullptr, 0}},
        {0x17, {"SLO", ZPGX, 6, &C6502::SLO, nullptr, 0}},
        {0x0F, {"SLO", ABS, 6, &C6502::SLO, nullptr, 0}},
        {0x1F, {"SLO", ABSX, 7, &C6502::SLO, nullptr, 0}},
        {0x1B, {"SLO", ABSY, 7, &C6502::SLO, nullptr, 0}},
        {0x03, {"SLO", INDX, 8, &C6502::SLO, nullptr, 0}},
        {0x13, {"SLO", INDY, 8, &C6502::SLO, nullptr, 0}},

        /* SRE */
        {0x47, {"SRE", ZPG, 5, &C6502::SRE, nullptr, 0}},
        {0x57, {"SRE", ZPGX, 6, &C6502::SRE, nullptr, 0}},
        {0x4F, {"SRE", ABS, 6, &C6502::SRE, nullptr, 0}},
        {0x5F, {"SRE", ABSX, 7, &C6502::SRE, nullptr, 0}},
        {0x5B, {"SRE", ABSY, 7, &C6502::SRE, nullptr, 0}},
        {0x43, {"SRE", INDX, 8, &C6502::SRE, nullptr, 0}},
        {0x53, {"SRE", INDY, 8, &C6502::SRE, nullptr, 0}},

        {0x9B, {"TAS", ABSY, 5, &C6502::TAS, nullptr, 0}},
        {0xEB, {"USBC", IMM, 2, &C6502::USBC, nullptr, 0}},

        /* KIL */
        {0x02, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0x12, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0x22, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0x32, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0x42, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0x52, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0x62, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0x72, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0x92, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0xB2, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0xD2, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},
        {0xF2, {"KIL", IMP, 0, &C6502::KIL, nullptr, 0}},

        /* NOP */
        {0x80, {"NOP", IMM, 2, &C6502::NOP_IGN, nullptr, 0}},
        {0x82, {"NOP", IMM, 2, &C6502::NOP_IGN, nullptr, 0}},
        {0x89, {"NOP", IMM, 2, &C6502::NOP_IGN, nullptr, 0}},
        {0xC2, {"NOP", IMM, 2, &C6502::NOP_IGN, nullptr, 0}},
        {0xE2, {"NOP", IMM, 2, &C6502::NOP_IGN, nullptr, 0}},

        {0x04, {"NOP", ZPG, 3, &C6502::NOP_IGN, nullptr, 0}},
        {0x44, {"NOP", ZPG, 3, &C6502::NOP_IGN, nullptr, 0}},
        {0x64, {"NOP", ZPG, 3, &C6502::NOP_IGN, nullptr, 0}},

        {0x14, {"NOP", ZPGX, 4, &C6502::NOP_IGN, nullptr, 0}},
        {0x34, {"NOP", ZPGX, 4, &C6502::NOP_IGN, nullptr, 0}},
        {0x54, {"NOP", ZPGX, 4, &C6502::NOP_IGN, nullptr, 0}},
        {0x74, {"NOP", ZPGX, 4, &C6502::NOP_IGN, nullptr, 0}},
        {0xD4, {"NOP", ZPGX, 4, &C6502::NOP_IGN, nullptr, 0}},
        {0xF4, {"NOP", ZPGX, 4, &C6502::NOP_IGN, nullptr, 0}},

        {0x0C, {"NOP", ABS, 4, &C6502::NOP_IGN, nullptr, 0}},

        {0x1C, {"NOP", ABSX, 4, &C6502::NOP_IGN, nullptr, 1}},
        {0x3C, {"NOP", ABSX, 4, &C6502::NOP_IGN, nullptr, 1}},
        {0x5C, {"NOP", ABSX, 4, &C6502::NOP_IGN, nullptr, 1}},
        {0x7C, {"NOP", ABSX, 4, &C6502::NOP_IGN, nullptr, 1}},
        {0xDC, {"NOP", ABSX, 4, &C6502::NOP_IGN, nullptr, 1}},
        {0xFC, {"NOP", ABSX, 4, &C6502::NOP_IGN, nullptr, 1}},
};

} /* namespace Core */
