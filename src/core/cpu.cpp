#include <array>

#include "core/cpu.h"
#include "core/cpu_op.h"

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

    static const std::array<const OpEntry *, 256> opLut = []() {
        std::array<const OpEntry *, 256> lut{};
        lut.fill(nullptr);

        for (const auto &kv : OP_TABLE) {
            const u8 op = static_cast<u8>(kv.first & 0xFF);
            lut[op] = &kv.second;
        }

        return lut;
    }();

    const OpEntry *op = opLut[opcode];
    if (!op) {
        p->c.opEntry.op_name = "???";
        p->c.opEntry.am = IMP;
        op_cycles = 2;
        page_crossed = 0;
        return;
    }

    p->c.opEntry.op_name = op->op_name;
    p->c.opEntry.am = op->am;
    op_cycles = op->cycles;
    page_crossed = 0;

    if (op->op_imp != nullptr) {
        (this->*op->op_imp)();
        return;
    }

    if (op->op_addr != nullptr) {
        const u16 addr = resolveAddr(op->am);
        (this->*op->op_addr)(addr);

        if (op->page_crossed && page_crossed)
            op_cycles += 1;
    }
}

void CPU::exec() {
    if (!mem)
        return;

    const auto tickCycles = [this](u32 cycles) { mem->tickCpuCycles(cycles); };

    if (const u32 d = mem->getDma(); d != 0) {
        c.op_cycles = d;
        tickCycles(d);
        return;
    }

    if (c.do_nmi) {
        c.do_nmi = 0;
        c.nmi();
        tickCycles(c.op_cycles);
        return;
    }

    if (c.do_irq) {
        c.do_irq = 0;
        if (!(c.regs.P & C6502::I)) {
            c.irq();
            tickCycles(c.op_cycles);
            return;
        }
    }

    c.step();
    tickCycles(c.op_cycles);
}

} /* namespace Core */
