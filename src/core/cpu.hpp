#pragma once

#include "memory.hpp"


class Cpu {
public:
    explicit Cpu(Memory* m): mem(m){}
    ~Cpu() = default;


    void powerUp();
    void reset();
    void exec();

public:
    enum : u8 {
        N = (1<<7), /* Отрицательный */
        V = (1<<6), /* Переполнение  */
        D = (1<<3), /* Десятичный    */
        I = (1<<2), /* Прерывание    */
        Z = (1<<1), /* Нуль          */
        C = (1<<0), /* Перенос       */
    };

    struct Regs {
        u8 A{0};          /* Аккумулятор                       */
        u8 X{0};          /* Индексный регистр X               */
        u8 Y{0};          /* Индексный регистр Y               */
        u8 P{UNUSED | I}; /* Регистр флагов (U=1 по умолчанию) */

        u8  SP{0xFD}; /* Указатель стека (инициализируется как 0xFD) */
        u16 PC{};     /* Счетчик команд                              */
    };

public:
    /* Дебаг режим */
    struct Dbg {
        std::string op; /* Опкод           */
        std::string am; /* Режим адресации */
        u16 addr;       /* Адрес           */
    };

    /* Состояние CPU */
    struct State {
        Regs regs{};
        Dbg dbg{};

        bool do_nmi{false};
        bool nmi_delay{false};
        bool do_irq{false};

        u32 cycles{0};            /* Счетчик циклов для текущей инструкции */
        u64 total_cycles{0};      /* Счетчик циклов для всех инструкций    */
        bool page_crossed{false}; /* Флаг пересечения границы страницы     */
    };

public:
    auto isPageCrossed() const -> bool   { return state.page_crossed; }
    auto getTotalCycles() const -> u64   { return state.total_cycles; }
    auto getCycles() const -> u32        { return state.cycles; }
    auto getDbg()   const -> const auto& { return state.dbg; }
    auto getState() -> auto&             { return state; }

    void loadState(State state);

private:
    State state{};
    Memory* mem;

private:
    inline void clear();
    void step();
    void nmi();
    void irq();

public:
    auto read16(u16 addr) const -> u16 { const u8 high = mem->read(addr+1);
                                         const u8 low  = mem->read(addr);
                                         return (high << 8) | low; }

    auto read16_zp(u8 addr) const -> u16 { const u8 low  = mem->read(addr);
                                           const u8 high = mem->read((addr + 1) & 0xFF);
                                           return ((high) << 8) | low; }

    void push(u8 value) { mem->write(STACK + state.regs.SP--, value); }
    auto pop() -> u8 { return mem->read(STACK + (++state.regs.SP)); }


    inline auto high(u16 addr) -> u8 { return addr >> 8; }
    inline void set_flag(u8 flag, bool value) { (value) ? (state.regs.P |= flag) : (state.regs.P &= ~flag); }

    inline void set_nz(u8 value) {
        set_flag(Z, value == 0);
        set_flag(N, value & N);
    }

private:
    inline void dbgSet(const char* am, u16 addr) { state.dbg.am = am; state.dbg.addr = addr; }

    inline auto AM_IMP() -> u16 { dbgSet("IMP", 0); return 0; }
    inline auto AM_IMM() -> u16 { const u16 addr = state.regs.PC++; dbgSet("IMM", addr); return addr; };
    inline auto AM_ZPG() -> u16 { const u16 addr = static_cast<u16>(mem->read(state.regs.PC++)); 
                                  dbgSet("ZPG", addr); return addr; }
    inline auto AM_ZPX() -> u16 { return (AM_ZPG() + static_cast<u16>(state.regs.X)) & 0x00FF; }
    inline auto AM_ZPY() -> u16 { return (AM_ZPG() + static_cast<u16>(state.regs.Y)) & 0x00FF; }
    inline auto AM_REL() -> u16 { const i8 offset = static_cast<i8>(mem->read(state.regs.PC++));
                                  const u16 addr = state.regs.PC + offset;
                                  dbgSet("REL", addr);
                                  return addr; }
    inline auto AM_ABS() -> u16 { const u16 addr = read16(state.regs.PC); state.regs.PC+=2; dbgSet("ABS", addr); return addr; }
    inline auto AM_ABX() -> u16 { const u16 addr = AM_ABS();
                                  const u16 indexed_addr = addr + state.regs.X;
                                  state.page_crossed = (addr & 0xFF00) != (indexed_addr & 0xFF00);
                                  dbgSet("ABX", indexed_addr);
                                  return indexed_addr; }
    inline auto AM_ABY() -> u16 { const u16 addr = AM_ABS();
                                  const u16 indexed_addr = addr + state.regs.Y;
                                  state.page_crossed = (addr & 0xFF00) != (indexed_addr & 0xFF00);
                                  dbgSet("ABY", indexed_addr);
                                  return indexed_addr; }
    inline auto AM_IND() -> u16 { const u16 addr = AM_ABS();
                                  const u8 low = mem->read(addr);
                                  const u8 high = mem->read((addr & 0xFF00) | ((addr + 1) & 0x00FF));
                                  const u16 ind = (static_cast<u16>(high) << 8 | low);
                                  dbgSet("IND", ind);
                                  return ind; }
    inline auto AM_INX() -> u16 { const u16 addr = read16_zp(AM_ZPX()); dbgSet("INX", addr); return addr; }
    inline auto AM_INY() -> u16 { const u16 addr = read16_zp(AM_ZPG());
                                  const u16 indexed_addr = addr + state.regs.Y;
                                  state.page_crossed = (addr & 0xFF00) != (indexed_addr & 0xFF00);
                                  dbgSet("INY", indexed_addr);
                                  return indexed_addr; }

private:
    /* Официальные опкоды */
    void LDA(u16 addr); void STA(u16 addr); void LDX(u16 addr); void STX(u16 addr); void LDY(u16 addr); void STY(u16 addr);
    void TAX(); void TXA(); void TAY(); void TYA(); void TSX(); void TXS();
    void ADC(u16 addr); void SBC(u16 addr); void INC(u16 addr); void DEC(u16 addr); void INX(); void DEX(); void INY(); void DEY();
    void ASL(u16 addr); void LSR(u16 addr); void ROL(u16 addr); void ROR(u16 addr);
    void ASL_ACC(); void LSR_ACC(); void ROL_ACC(); void ROR_ACC();
    void AND(u16 addr); void ORA(u16 addr); void EOR(u16 addr); void BIT(u16 addr);
    void CMP(u16 addr); void CPX(u16 addr); void CPY(u16 addr);
    void BCC(u16 addr); void BCS(u16 addr); void BEQ(u16 addr); void BNE(u16 addr); void BPL(u16 addr); void BMI(u16 addr); void BVC(u16 addr); void BVS(u16 addr);
    void JMP(u16 addr); void JSR(u16 addr); void RTS(); void BRK(); void RTI();
    void PHA(); void PLA(); void PHP(); void PLP();
    void CLC(); void SEC(); void CLI(); void SEI(); void CLD(); void SED(); void CLV();
    void NOP();


    /* Неофициальные опкоды */
    void ALR(u16 addr); void ANC(u16 addr); void ANE(u16 addr); void ARR(u16 addr);
    void DCP(u16 addr); void ISC(u16 addr); void LAS(u16 addr); void LAX(u16 addr); void LXA(u16 addr);
    void RLA(u16 addr); void RRA(u16 addr); void SAX(u16 addr); void SBX(u16 addr); void SHA(u16 addr); void SHX(u16 addr); void SHY(u16 addr);
    void SLO(u16 addr); void SRE(u16 addr); void TAS(u16 addr); void USBC(u16 addr); void KIL(u16 addr);
    void NOP_IGN(u16 addr);
};
