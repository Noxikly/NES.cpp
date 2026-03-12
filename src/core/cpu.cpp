#include "cpu.hpp"
#include "common.hpp"
#include "cpu_op.hpp"

namespace Core {

/* Системные функции */
void CPU::reset() {
    if (!mem)
        return;

    c.regs.A = 0;
    c.regs.X = 0;
    c.regs.Y = 0;
    c.regs.P = c.U | c.I;
    c.regs.SP = 0xFD;
    c.regs.PC = (static_cast<u16>(mem->read(0xFFFD)) << 8) |
                static_cast<u16>(mem->read(0xFFFC));
    c.op_cycles = 7;
    c.do_nmi = 0;
    c.nmi_delay = 0;
    c.do_irq = 0;
    c.page_crossed = 0;
    c.opEntry.op_name = "";
    c.opEntry.am = C6502::IMP;
}

/* Линии прерываний */
void CPU::C6502::nmi() {
    push((regs.PC >> 8) & 0x00FF);
    push(regs.PC & 0x00FF);
    push(regs.P & ~B);
    set_flag(I, 1);
    regs.PC = read16(0xFFFA);
    op_cycles = 8;
}

void CPU::C6502::irq() {
    push((regs.PC >> 8) & 0x00FF);
    push(regs.PC & 0x00FF);
    push(regs.P & ~B);
    set_flag(I, 1);
    regs.PC = read16(0xFFFE);
    op_cycles = 7;
}

/* Выполнение одной инструкции */
void CPU::C6502::step() {
    const u8 opcode = p->mem->read(regs.PC++);

    const auto it = OP_TABLE.find(opcode);
    if (it == OP_TABLE.end()) {
        p->c.opEntry.op_name = "UNK";
        p->c.opEntry.am = IMP;
        op_cycles = 2;
        page_crossed = 0;
        return;
    }

    const OpEntry &op = it->second;

    p->c.opEntry.op_name = op.op_name;
    p->c.opEntry.am = op.am;
    op_cycles = op.cycles;
    page_crossed = 0;

    auto resolve_addr = [this](AddrMode am) -> u16 {
        switch (am) {
        case IMM:
            return AM_IMM();
        case IMP:
            return AM_IMP();
        case ZPG:
            return AM_ZPG();
        case ZPGX:
            return AM_ZPX();
        case ZPGY:
            return AM_ZPY();
        case REL:
            return AM_REL();
        case ABS:
            return AM_ABS();
        case ABSX:
            return AM_ABX();
        case ABSY:
            return AM_ABY();
        case IND:
            return AM_IND();
        case INDX:
            return AM_INX();
        case INDY:
            return AM_INY();
        }

        return AM_IMP();
    };

    if (op.op_imp != nullptr) {
        (this->*op.op_imp)();
        return;
    }

    if (op.op_addr != nullptr) {
        const u16 addr = resolve_addr(op.am);
        (this->*op.op_addr)(addr);

        if (op.page_crossed && page_crossed)
            op_cycles += 1;
    }
}

void CPU::exec() {
    if (!mem)
        return;

    if (const u32 d = mem->getDma(); d != 0) {
        c.op_cycles = d;
        return;
    }

    if (c.nmi_delay) {
        c.nmi_delay = 0;
        c.do_nmi = 0;
        c.nmi();
        return;
    }

    if (c.do_nmi)
        c.nmi_delay = 1;

    if (c.do_irq) {
        c.do_irq = 0;
        if (!(c.regs.P & C6502::I)) {
            c.irq();
            return;
        }
    }

    c.step();
}

} // namespace Core
