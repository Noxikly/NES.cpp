#include <gtest/gtest.h>
#include <initializer_list>

#include "../src/core/cpu.hpp"
#include "../src/core/common.hpp"


class CpuTest : public testing::Test {
protected:
    Memory mem;
    CPU cpu{&mem};

    void SetUp() override {
        cpu.reset();
        cpu.c.regs.PC = 0x0200;
    }

    void loadCode(std::initializer_list<u8> code) {
        u16 addr = cpu.c.regs.PC;
        for (const u8 byte : code)
            mem.write(addr++, byte);
    }
};


/* Хранение/Загрузка */
TEST_F(CpuTest, LDA) {
    loadCode({0xA9, 0x32});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x32);
}

TEST_F(CpuTest, STA) {
    loadCode({0x85, 0x58});
    cpu.c.regs.A = 0x06;
    cpu.exec();
    EXPECT_EQ(mem.read(0x0058), 0x06);
}

TEST_F(CpuTest, LDX) {
    loadCode({0xA2, 0x71});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.X, 0x71);
}

TEST_F(CpuTest, STX) {
    loadCode({0x86, 0x65});
    cpu.c.regs.X = 0x07;
    cpu.exec();
    EXPECT_EQ(mem.read(0x0065), 0x07);
}

TEST_F(CpuTest, LDY) {
    loadCode({0xA0, 0x23});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.Y, 0x23);
}

TEST_F(CpuTest, STY) {
    loadCode({0x84, 0x57});
    cpu.c.regs.Y = 0x08;
    cpu.exec();
    EXPECT_EQ(mem.read(0x0057), 0x08);
}


/* Передача */
TEST_F(CpuTest, TAX) {
    cpu.c.regs.A = 0x10;
    loadCode({0xAA});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.X, 0x10);
}

TEST_F(CpuTest, TXA) {
    cpu.c.regs.X = 0x11;
    loadCode({0x8A});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x11);
}

TEST_F(CpuTest, TAY) {
    cpu.c.regs.A = 0x11;
    loadCode({0xA8});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.Y, 0x11);
}

TEST_F(CpuTest, TYA) {
    cpu.c.regs.Y = 0x12;
    loadCode({0x98});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x12);
}

TEST_F(CpuTest, TSX) {
    cpu.c.regs.SP = 0xF3;
    loadCode({0xBA});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.X, 0xF3);
}

TEST_F(CpuTest, TXS) {
    cpu.c.regs.X = 0xE4;
    loadCode({0x9A});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.SP, 0xE4);
}


/* Арифметика */
TEST_F(CpuTest, ADC) {
    cpu.c.regs.A = 0x10;
    cpu.c.regs.P &= ~CPU::C6502::C;
    loadCode({0x69, 0x20});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x30);
}

TEST_F(CpuTest, SBC) {
    cpu.c.regs.A = 0x40;
    cpu.c.regs.P |= CPU::C6502::C;
    loadCode({0xE9, 0x0F});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x31);
}

TEST_F(CpuTest, INC) {
    mem.write(0x0045, 0x2F);
    loadCode({0xE6, 0x45});
    cpu.exec();
    EXPECT_EQ(mem.read(0x0045), 0x30);
}

TEST_F(CpuTest, DEC) {
    mem.write(0x0060, 0x01);
    loadCode({0xC6, 0x60});
    cpu.exec();
    EXPECT_EQ(mem.read(0x0060), 0x00);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::Z);
}

TEST_F(CpuTest, INX) {
    cpu.c.regs.X = 0xFF;
    loadCode({0xE8});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.X, 0x00);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::Z);
}

TEST_F(CpuTest, DEX) {
    cpu.c.regs.X = 0x01;
    loadCode({0xCA});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.X, 0x00);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::Z);
}

TEST_F(CpuTest, INY) {
    cpu.c.regs.Y = 0x7F;
    loadCode({0xC8});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.Y, 0x80);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::N);
}

TEST_F(CpuTest, DEY) {
    cpu.c.regs.Y = 0x00;
    loadCode({0x88});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.Y, 0xFF);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::N);
}


/* Логические */
TEST_F(CpuTest, AND) {
    cpu.c.regs.A = 0x5A;
    loadCode({0x29, 0x0F});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x0A);
}

TEST_F(CpuTest, ORA) {
    cpu.c.regs.A = 0x40;
    loadCode({0x09, 0x80});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0xC0);
}

TEST_F(CpuTest, EOR) {
    cpu.c.regs.A = 0xAA;
    loadCode({0x49, 0xFF});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x55);
}

TEST_F(CpuTest, BIT) {
    cpu.c.regs.A = 0x40;
    mem.write(0x0030, 0xC0);
    loadCode({0x24, 0x30});
    cpu.exec();
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::N);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::V);
    EXPECT_FALSE(cpu.c.regs.P & CPU::C6502::Z);
}


/* Сравнение */
TEST_F(CpuTest, CMP) {
    cpu.c.regs.A = 0x50;
    loadCode({0xC9, 0x50});
    cpu.exec();
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::Z);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::C);
}

TEST_F(CpuTest, CPX) {
    cpu.c.regs.X = 0x40;
    loadCode({0xE0, 0x30});
    cpu.exec();
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::C);
}

TEST_F(CpuTest, CPY) {
    cpu.c.regs.Y = 0x7F;
    loadCode({0xC0, 0x80});
    cpu.exec();
    EXPECT_FALSE(cpu.c.regs.P & CPU::C6502::C);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::N);
}


/* Сдвиги */
TEST_F(CpuTest, ASL) {
    cpu.c.regs.A = 0x80;
    loadCode({0x0A});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x00);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::C);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::Z);
}

TEST_F(CpuTest, LSR) {
    cpu.c.regs.A = 0x01;
    loadCode({0x4A});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x00);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::C);
}

TEST_F(CpuTest, ROL) {
    cpu.c.regs.A = 0x88;
    loadCode({0x2A});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x10);
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::C);
}

TEST_F(CpuTest, ROR) {
    cpu.c.regs.P |= CPU::C6502::C;
    cpu.c.regs.A = 0x02;
    loadCode({0x6A});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x81);
    EXPECT_FALSE(cpu.c.regs.P & CPU::C6502::C);
}


/* Флаги */
TEST_F(CpuTest, CLC) {
    cpu.c.regs.P |= CPU::C6502::C;
    loadCode({0x18});
    cpu.exec();
    EXPECT_FALSE(cpu.c.regs.P & CPU::C6502::C);
}

TEST_F(CpuTest, SEC) {
    loadCode({0x38});
    cpu.exec();
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::C);
}

TEST_F(CpuTest, CLI) {
    cpu.c.regs.P |= CPU::C6502::I;
    loadCode({0x58});
    cpu.exec();
    EXPECT_FALSE(cpu.c.regs.P & CPU::C6502::I);
}

TEST_F(CpuTest, SEI) {
    loadCode({0x78});
    cpu.exec();
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::I);
}

TEST_F(CpuTest, CLD) {
    cpu.c.regs.P |= CPU::C6502::D;
    loadCode({0xD8});
    cpu.exec();
    EXPECT_FALSE(cpu.c.regs.P & CPU::C6502::D);
}

TEST_F(CpuTest, SED) {
    loadCode({0xF8});
    cpu.exec();
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::D);
}

TEST_F(CpuTest, CLV) {
    cpu.c.regs.P |= CPU::C6502::V;
    loadCode({0xB8});
    cpu.exec();
    EXPECT_FALSE(cpu.c.regs.P & CPU::C6502::V);
}


/* Стек */
TEST_F(CpuTest, PHA) {
    const u8 old_sp = cpu.c.regs.SP;
    cpu.c.regs.A = 0x37;
    loadCode({0x48});
    cpu.exec();
    EXPECT_EQ(mem.read(static_cast<u16>(STACK + old_sp)), 0x37);
}

TEST_F(CpuTest, PLA) {
    cpu.c.regs.SP = 0xFC;
    mem.write(static_cast<u16>(STACK + 0xFD), 0x42);
    loadCode({0x68});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.A, 0x42);
}

TEST_F(CpuTest, PHP) {
    const u8 old_sp = cpu.c.regs.SP;
    cpu.c.regs.P = 0x30;
    loadCode({0x08});
    cpu.exec();
    EXPECT_EQ(mem.read(static_cast<u16>(STACK + old_sp)), 
                       static_cast<u8>(0x30 | CPU::C6502::U | CPU::C6502::B));
}

TEST_F(CpuTest, PLP) {
    cpu.c.regs.P = 0x00;
    cpu.c.regs.SP = 0xFE;
    mem.write(static_cast<u16>(STACK + 0xFF), 0x70);
    loadCode({0x28});
    cpu.exec();
    EXPECT_TRUE(cpu.c.regs.P & CPU::C6502::U);
}


/* Переходы */
TEST_F(CpuTest, JMP) {
    loadCode({0x4C, 0x00, 0x03});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0300);
}

TEST_F(CpuTest, JSR) {
    loadCode({0x20, 0x05, 0x02});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0205);
}

TEST_F(CpuTest, RTS) {
    cpu.c.regs.SP = 0xFD;
    mem.write(static_cast<u16>(STACK + 0xFE), 0x03);
    mem.write(static_cast<u16>(STACK + 0xFF), 0x02);
    loadCode({0x60});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0204);
}

TEST_F(CpuTest, BRK) {
    loadCode({0x00});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0000);
    EXPECT_EQ(mem.read(0x01FD), 0x02);
    EXPECT_EQ(mem.read(0x01FC), 0x02);
}

TEST_F(CpuTest, RTI) {
    cpu.c.regs.SP = 0xFC;
    mem.write(static_cast<u16>(STACK + 0xFD), 
              static_cast<u8>(0x30 | CPU::C6502::U));
    mem.write(static_cast<u16>(STACK + 0xFE), 0x03);
    mem.write(static_cast<u16>(STACK + 0xFF), 0x02);
    loadCode({0x40});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0203);
}


/* Ветвления */
TEST_F(CpuTest, BEQ) {
    cpu.c.regs.P |= CPU::C6502::Z;
    loadCode({0xF0, 0x05});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0207);
}

TEST_F(CpuTest, BNE) {
    cpu.c.regs.P &= ~CPU::C6502::Z;
    loadCode({0xD0, 0x05});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0207);
}

TEST_F(CpuTest, BCC) {
    cpu.c.regs.P &= ~CPU::C6502::C;
    loadCode({0x90, 0x03});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0205);
}

TEST_F(CpuTest, BCS) {
    cpu.c.regs.P |= CPU::C6502::C;
    loadCode({0xB0, 0x03});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0205);
}

TEST_F(CpuTest, BMI) {
    cpu.c.regs.P |= CPU::C6502::N;
    loadCode({0x30, 0x03});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0205);
}

TEST_F(CpuTest, BPL) {
    cpu.c.regs.P &= ~CPU::C6502::N;
    loadCode({0x10, 0x03});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0205);
}

TEST_F(CpuTest, BVC) {
    cpu.c.regs.P &= ~CPU::C6502::V;
    loadCode({0x50, 0x03});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0205);
}

TEST_F(CpuTest, BVS) {
    cpu.c.regs.P |= CPU::C6502::V;
    loadCode({0x70, 0x03});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, 0x0205);
}

/* NOP */
TEST_F(CpuTest, NOP) {
    const u16 old_pc = cpu.c.regs.PC;
    loadCode({0xEA});
    cpu.exec();
    EXPECT_EQ(cpu.c.regs.PC, static_cast<u16>(old_pc + 1));
}


TEST_F(CpuTest, MULTIPLY_TEST) {
    loadCode({
        0xA2, 0x0A,       /* LDX #10   */
        0x8E, 0x00, 0x00, /* STX $0000 */
        0xA2, 0x03,       /* LDX #3    */
        0x8E, 0x01, 0x00, /* STX $0001 */
        0xAC, 0x00, 0x00, /* LDY $0000 */
        0xA9, 0x00,       /* LDA #0    */
        0x18,             /* CLC       */
        0x6D, 0x01, 0x00, /* ADC $0001 */
        0x88,             /* DEY       */
        0xD0, 0xFA,       /* BNE loop  */
        0x8D, 0x02, 0x00, /* STA $0002 */
        0xEA, 0xEA, 0xEA  /* NOP       */
    });

    while (mem.read(0x0002) == 0)
        cpu.exec();

    EXPECT_EQ(mem.read(0x0002), 0x1E);
}


int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
