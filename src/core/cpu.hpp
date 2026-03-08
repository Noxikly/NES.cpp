#pragma once

#include <unordered_map>

#include "mem.hpp"
#include "common.hpp"

namespace Core {
class CPU;
}


class CPU {
public:
    explicit CPU(Memory* m = nullptr) : mem(m) {}
    ~CPU() = default;

    void exec();
    void reset();

private:
    Memory *mem{nullptr};

public:
    struct State {
        struct Regs {
            u8 A{0};     /* Аккумулятор         */
            u8 X{0};     /* Индексный регистр X */
            u8 Y{0};     /* Индексный регистр Y */
            u8 P{0};     /* Регистр флагов      */

            u8  SP{0xFD}; /* Указатель стека */
            u16 PC{};     /* Счетчик команд  */
        } regs;

        bool do_nmi{0};
        bool nmi_delay{0};
        bool do_irq{0};
        u32  op_cycles{0};
        bool page_crossed{0};
    } state{};

    const State& getState() const { return state; }
    void loadState(const State& s) { state = s; }

public:
    /* CPU 6502 namespace */
    class C6502 {

    public:
        explicit C6502(CPU* p)
            : p(p)
            , regs(p->state.regs)
            , do_nmi(p->state.do_nmi)
            , nmi_delay(p->state.nmi_delay)
            , do_irq(p->state.do_irq)
            , op_cycles(p->state.op_cycles)
            , page_crossed(p->state.page_crossed) {}
        ~C6502() = default;

        void step();
        void nmi();
        void irq();

    private:
        CPU* p; /* Parent */

    public:
        enum : u8 {
            N = (1<<7), /* Отрицательный */
            V = (1<<6), /* Переполнение  */
            U = (1<<5), /* Всегда 1      */
            B = (1<<4), /* BRK           */
            D = (1<<3), /* Десятичный    */
            I = (1<<2), /* Прерывание    */
            Z = (1<<1), /* Нуль          */
            C = (1<<0), /* Перенос       */
        };

        enum AddrMode {
            IMM, /* Immediate    */
            IMP, /* Implied      */
            ZPG, /* Zeropage     */
            ZPGX, /* Zeropage, X */
            ZPGY, /* Zeropage, Y */
            REL, /* Relative     */
            ABS, /* Absolute     */
            ABSX, /* Absolute, X */
            ABSY, /* Absolute, Y */
            IND, /* Indirect     */
            INDX, /* Indirect, X */
            INDY, /* Indirect, Y */
        } AM;

    public:
        State::Regs& regs;
        bool& do_nmi;
        bool& nmi_delay;
        bool& do_irq;
        u32& op_cycles;
        bool& page_crossed;

    private:
        struct OpEntry {
            const char* op_name;
            C6502::AddrMode am;
            u8 cycles;
            void (C6502::*op_addr)(u16);
            void (C6502::*op_imp)();
            bool page_crossed;
        };

        static const std::unordered_map<u16, OpEntry> OP_TABLE;
    
    public:
        OpEntry opEntry{};

    private:
        /* Утилиты */
        inline u16 read16(u16 addr) const {
            const u8 high = p->mem->read(addr+1);
            const u8 low  = p->mem->read(addr);
            return (high << 8) | low;
        }

        inline u16 read16_zp(u16 addr) const {
            const u8 high = p->mem->read((addr+1) & 0xFF);
            const u8 low  = p->mem->read(addr);
            return (high << 8) | low;
        }

        /* Флаги */
        inline void set_flag(u8 flag, bool value) { (value) ? (regs.P |= flag) : (regs.P &= ~flag); }
        inline void set_nz(u8 value) { set_flag(Z, value == 0); set_flag(N, value & N); }
        inline void set_nz16(u16 value) { set_flag(Z, (value & 0x00FF) == 0); set_flag(N, value & 0x0080); }

        /* Стек */
        void push(u8 value) { p->mem->write(STACK + regs.SP--, value); }
        u8 pop() { return p->mem->read(STACK + (++regs.SP)); }

    private:
        /* Режимы адресации */
        inline u16 AM_IMP() const { return 0; }

        inline u16 AM_IMM() { const u16 addr = regs.PC++; return addr; };
        inline u16 AM_ZPG() { const u16 addr = static_cast<u16>(p->mem->read(regs.PC++)); 
                              return addr; }
        inline u16 AM_ZPX() { return (AM_ZPG() + static_cast<u16>(regs.X)) & 0x00FF; }
        inline u16 AM_ZPY() { return (AM_ZPG() + static_cast<u16>(regs.Y)) & 0x00FF; }

        inline u16 AM_REL() { const i8 offset = static_cast<i8>(p->mem->read(regs.PC++));
                              const u16 addr = regs.PC + offset;
                              return addr; }

        inline u16 AM_ABS() { const u16 addr = read16(regs.PC); regs.PC+=2; return addr; }
        inline u16 AM_ABX() { const u16 addr = AM_ABS();
                              const u16 idx_addr = addr + regs.X;
                              page_crossed = (addr & 0xFF00) != (idx_addr & 0xFF00);
                              return idx_addr; }
        inline u16 AM_ABY() { const u16 addr = AM_ABS();
                              const u16 idx_addr = addr + regs.Y;
                              page_crossed = (addr & 0xFF00) != (idx_addr & 0xFF00);
                              return idx_addr; }

        inline u16 AM_IND() { const u16 addr = AM_ABS();
                              const u8 low = p->mem->read(addr);
                              const u8 high = p->mem->read((addr & 0xFF00) | ((addr + 1) & 0x00FF));
                              const u16 ind = (static_cast<u16>(high) << 8 | low);
                              return ind; }
        inline u16 AM_INX() { const u16 addr = read16_zp(AM_ZPX()); return addr; }
        inline u16 AM_INY() { const u16 addr = read16_zp(AM_ZPG());
                              const u16 idx_addr = addr + regs.Y;
                              page_crossed = (addr & 0xFF00) != (idx_addr & 0xFF00);
                              return idx_addr; }
    
    private:
        /* Вспомогательные функции */
        inline void ArithmWithCarry(const u16 val);
        inline void Branch(u16 addr);

        /* Официальные опкоды */
        void LDA(u16 addr); void STA(u16 addr); void LDX(u16 addr); 
        void STX(u16 addr); void LDY(u16 addr); void STY(u16 addr);

        void TAX(); void TXA(); void TAY(); void TYA(); void TSX(); void TXS();

        void ADC(u16 addr); void SBC(u16 addr); void INC(u16 addr); void DEC(u16 addr); 
        void INX(); void DEX(); void INY(); void DEY();

        void ASL(u16 addr); void LSR(u16 addr); void ROL(u16 addr); void ROR(u16 addr);

        void AND(u16 addr); void ORA(u16 addr); void EOR(u16 addr); void BIT(u16 addr);
        void CMP(u16 addr); void CPX(u16 addr); void CPY(u16 addr);

        void BCC(u16 addr); void BCS(u16 addr); void BEQ(u16 addr); void BNE(u16 addr); 
        void BPL(u16 addr); void BMI(u16 addr); void BVC(u16 addr); void BVS(u16 addr);

        void JMP(u16 addr); void JSR(u16 addr); void RTS(); void BRK(); void RTI();

        void PHA(); void PLA(); void PHP(); void PLP();
        void CLC(); void SEC(); void CLI(); void SEI(); void CLD(); void SED(); void CLV();
        void NOP();


        /* Неофициальные опкоды */
        void ALR(u16 addr); void ANC(u16 addr); void ANE(u16 addr); void ARR(u16 addr);
        void DCP(u16 addr); void ISC(u16 addr); void LAS(u16 addr); void LAX(u16 addr); void LXA(u16 addr);
        void RLA(u16 addr); void RRA(u16 addr); void SAX(u16 addr); void SBX(u16 addr); void SHA(u16 addr); 
        void SHX(u16 addr); void SHY(u16 addr);

        void SLO(u16 addr); void SRE(u16 addr); void TAS(u16 addr); void USBC(u16 addr); void KIL(u16 addr);
        void NOP_IGN(u16 addr);
    };

    C6502 c{this};

public:
    const char* getLastOpName() const { return c.opEntry.op_name; }
    C6502::AddrMode getLastAddrMode() const { return c.opEntry.am; }
};
