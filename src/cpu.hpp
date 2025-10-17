#pragma once
#include "memory.hpp"


class Cpu {
public:
    explicit Cpu(Memory *m): mem(m){};
    ~Cpu() = default;


    void reset();
    void exec();


    auto isPageCrossed() const -> bool { return page_crossed; }

public:
    enum : u8 {
        N = (1<<7), /* Отрицательный   */
        V = (1<<6), /* Переполнение    */
        D = (1<<3), /* Десятичный      */
        I = (1<<2), /* Прерывание      */
        Z = (1<<1), /* Нуль            */
        C = (1<<0), /* Перенос         */
    };

    struct {
        u8 A{0};      /* Аккумулятор                       */
        u8 X{0};      /* Индексный регистр X               */
        u8 Y{0};      /* Индексный регистр Y               */
        u8 P{UNUSED}; /* Регистр флагов (U=1 по умолчанию) */

        u8  SP{0xFD}; /* Указатель стека (инициализируется как 0xFD) */
        u16 PC{};     /* Счетчик команд                              */
    } regs;

private:
    Memory *mem{nullptr};

    bool do_nmi{false};
    bool do_irq{false};

    u8  cycles{0};            /* Счетчик циклов для текущей инструкции */
    u64 total_cycles{0};      /* Счетчик циклов для всех инструкций    */
    bool page_crossed{false}; /* Флаг пересечения границы страницы     */

private:
    void step();
    void nmi();
    void irq();

public:
    auto read16(u16 addr) -> u16 { u8 high = mem->read(addr+1);
                                   u8 low  = mem->read(addr);
                                   return (high << 8) | low; }

    void push(u8 value) { mem->write(STACK + regs.SP--, value); }
    auto pop() -> u8 { return mem->read(STACK + (++regs.SP)); }


    inline void set_flag(u8 flag, bool value) { (value) ? (regs.P |= flag) : (regs.P &= ~flag); }

    inline void set_nz(u8 value) {
        set_flag(Z, value == 0);
        set_flag(N, value & N);
    }

private:
    static inline auto AM_ACC() -> u16 { return 0; }
    static inline auto AM_IMP() -> u16 { return 0; }
    inline auto AM_IMM() -> u16 { return regs.PC++; };
    inline auto AM_ZPG() -> u16 { return static_cast<u16>(mem->read(regs.PC++)); }
    inline auto AM_ZPX() -> u16 { return (AM_ZPG() + static_cast<u16>(regs.X)) & 0x00FF; }
    inline auto AM_ZPY() -> u16 { return (AM_ZPG() + static_cast<u16>(regs.Y)) & 0x00FF; }
    inline auto AM_REL() -> u16 { const i8 offset = static_cast<i8>(mem->read(regs.PC++));
                                  return regs.PC + offset; }
    inline auto AM_ABS() -> u16 { const u16 addr = read16(regs.PC); regs.PC+=2; return addr; }
    inline auto AM_ABX() -> u16 { const u16 addr = AM_ABS();
                                  const u16 indexed_addr = addr + regs.X;
                                  page_crossed = (addr & 0xFF00) != (indexed_addr & 0xFF00);
                                  return indexed_addr; }
    inline auto AM_ABY() -> u16 { const u16 addr = AM_ABS();
                                  const u16 indexed_addr = addr + regs.Y;
                                  page_crossed = (addr & 0xFF00) != (indexed_addr & 0xFF00);
                                  return indexed_addr; }
    inline auto AM_IND() -> u16 { const u16 addr = AM_ABS();
                                  u8 low = mem->read(addr);
                                  u8 high = mem->read((addr & 0xFF00) | ((addr + 1) & 0x00FF));
                                  return (static_cast<u16>(high) << 8 | low); }
    inline auto AM_INX() -> u16 { return read16(AM_ZPX()); }
    inline auto AM_INY() -> u16 { const u16 addr = read16(AM_ZPG());
                                  const u16 indexed_addr = addr + regs.Y;
                                  page_crossed = (addr & 0xFF00) != (indexed_addr & 0xFF00);
                                  return indexed_addr; }


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

};
