#include "common/debug.h"

#include "core/cpu.h"
#include "core/cpu_op.h"

namespace Core {

/* Системные функции */
void CPU::reset() {
    if (!mem)
        return;

    if (debug)
        LOG_DEBUG("[CPU] reset");

    c.regs.A = 0;
    c.regs.X = 0;
    c.regs.Y = 0;
    c.regs.P = c.U | c.I;
    c.regs.SP = 0xFD;
    c.regs.PC = (static_cast<u16>(memRead(0xFFFD)) << 8) |
                static_cast<u16>(memRead(0xFFFC));
    c.op_cycles = 7;
    c.do_nmi = 0;
    c.do_irq = 0;
    c.page_crossed = 0;
    debugTraceCounter = 0;
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
    op_cycles = 7;
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
    const u8 opcode = p->memRead(regs.PC++);

    const auto it = OP_TABLE.find(opcode);
    if (it == OP_TABLE.end()) {
        p->c.opEntry.op_name = "???";
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

    if (debug) {
        ++debugTraceCounter;
        if ((debugTraceCounter & 0x0F) == 0) {
            LOG_TRACE(
                "[CPU] exec PC=0x%04X A=0x%02X X=0x%02X Y=0x%02X P=0x%02X SP=0x%02X",
                static_cast<unsigned>(c.regs.PC),
                static_cast<unsigned>(c.regs.A),
                static_cast<unsigned>(c.regs.X),
                static_cast<unsigned>(c.regs.Y),
                static_cast<unsigned>(c.regs.P),
                static_cast<unsigned>(c.regs.SP));
        }
    }

    if (const u32 d = mem->getDma(); d != 0) {
        c.op_cycles = d;
        for (u32 i = 0; i < d; ++i)
            mem->tickCpuCycle();
        return;
    }

    if (c.do_nmi) {
        c.do_nmi = 0;
        c.nmi();
        for (u32 i = 0; i < c.op_cycles; ++i)
            mem->tickCpuCycle();
        return;
    }

    if (c.do_irq) {
        c.do_irq = 0;
        if (!(c.regs.P & C6502::I)) {
            c.irq();
            for (u32 i = 0; i < c.op_cycles; ++i)
                mem->tickCpuCycle();
            return;
        }
    }

    c.step();
    for (u32 i = 0; i < c.op_cycles; ++i)
        mem->tickCpuCycle();
}

} /* namespace Core */
