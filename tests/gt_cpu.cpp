#include <gtest/gtest.h>
#include "../src/cpu.hpp"
#include "../src/cartridge.hpp"
#include "../src/mappers/mapper0.hpp"


class CpuTest : public testing::Test {
protected:
    Memory mem;
    Cpu cpu{&mem};


    void SetUp() override {
        cpu.reset();
        cpu.regs.PC = 0x0200;
    }

    void loadCode(std::initializer_list<u8> code) {
        u16 addr = cpu.regs.PC;
        for(u8 byte : code)
            mem.write(addr++, byte);
    }
};



/* ОПКОДЫ */

/* Хранение/Загрузка */
TEST_F(CpuTest, LDA) {
    loadCode({0xA9, 0x32});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x32);
}

TEST_F(CpuTest, STA) {
    loadCode({0x85, 0x58});
    cpu.regs.A = 0x06;
    cpu.exec();

    EXPECT_EQ(mem.read(0x0058), 0x06);
}

TEST_F(CpuTest, LDX) {
    loadCode({0xA2, 0x71});
    cpu.exec();

    EXPECT_EQ(cpu.regs.X, 0x71);
}

TEST_F(CpuTest, STX) {
    loadCode({0x86, 0x65});
    cpu.regs.X = 0x07;
    cpu.exec();

    EXPECT_EQ(mem.read(0x0065), 0x07);
}

TEST_F(CpuTest, LDY) {
    loadCode({0xA0, 0x23});
    cpu.exec();

    EXPECT_EQ(cpu.regs.Y, 0x23);
}

TEST_F(CpuTest, STY) {
    loadCode({0x84, 0x57});
    cpu.regs.Y = 0x08;
    cpu.exec();

    EXPECT_EQ(mem.read(0x0057), 0x08);
}


/* Передача */
TEST_F(CpuTest, TAX) {
    cpu.regs.A = 0x10;
    loadCode({0xAA});
    cpu.exec();

    EXPECT_EQ(cpu.regs.X, 0x10);
}

TEST_F(CpuTest, TXA) {
    cpu.regs.X = 0x11;
    loadCode({0x8A});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x11);
}

TEST_F(CpuTest, TAY) {
    cpu.regs.A = 0x11;
    loadCode({0xA8});
    cpu.exec();

    EXPECT_EQ(cpu.regs.Y, 0x11);
}

TEST_F(CpuTest, TYA) {
    cpu.regs.Y = 0x12;
    loadCode({0x98});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x12);
}

TEST_F(CpuTest, TSX) {
    cpu.regs.SP = 0xF3;
    loadCode({0xBA});
    cpu.exec();

    EXPECT_EQ(cpu.regs.X, 0xF3);
}

TEST_F(CpuTest, TXS) {
    cpu.regs.X = 0xE4;
    loadCode({0x9A});
    cpu.exec();

    EXPECT_EQ(cpu.regs.SP, 0xE4);
}


/* Арифметика */
TEST_F(CpuTest, ADC) {
    cpu.regs.A = 0x10;
    cpu.set_flag(Cpu::C, false);
    loadCode({0x69, 0x20});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x30);
}

TEST_F(CpuTest, SBC) {
    cpu.regs.A = 0x40;
    cpu.set_flag(Cpu::C, true);
    loadCode({0xE9, 0x0F});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x31);
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
    EXPECT_TRUE(cpu.regs.P & Cpu::Z);
}

TEST_F(CpuTest, INX) {
    cpu.regs.X = 0xFF;
    loadCode({0xE8});
    cpu.exec();

    EXPECT_EQ(cpu.regs.X, 0x00);
    EXPECT_TRUE(cpu.regs.P & Cpu::Z);
}

TEST_F(CpuTest, DEX) {
    cpu.regs.X = 0x01;
    loadCode({0xCA});
    cpu.exec();

    EXPECT_EQ(cpu.regs.X, 0x00);
    EXPECT_TRUE(cpu.regs.P & Cpu::Z);
}

TEST_F(CpuTest, INY) {
    cpu.regs.Y = 0x7F;
    loadCode({0xC8});
    cpu.exec();

    EXPECT_EQ(cpu.regs.Y, 0x80);
    EXPECT_TRUE(cpu.regs.P & Cpu::N);
}

TEST_F(CpuTest, DEY) {
    cpu.regs.Y = 0x00;
    loadCode({0x88});
    cpu.exec();

    EXPECT_EQ(cpu.regs.Y, 0xFF);
    EXPECT_TRUE(cpu.regs.P & Cpu::N);
}


/* Логические */
TEST_F(CpuTest, AND) {
    cpu.regs.A = 0x5A;
    loadCode({0x29, 0x0F});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x0A);
}

TEST_F(CpuTest, ORA) {
    cpu.regs.A = 0x40;
    loadCode({0x09, 0x80});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0xC0);
}

TEST_F(CpuTest, EOR) {
    cpu.regs.A = 0xAA;
    loadCode({0x49, 0xFF});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x55);
}

TEST_F(CpuTest, BIT) {
    cpu.regs.A = 0x40;
    mem.write(0x0030, 0xC0);
    loadCode({0x24, 0x30});
    cpu.exec();

    EXPECT_TRUE(cpu.regs.P & Cpu::N);
    EXPECT_TRUE(cpu.regs.P & Cpu::V);
    EXPECT_FALSE(cpu.regs.P & Cpu::Z);
}


/* Сравнение */
TEST_F(CpuTest, CMP) {
    cpu.regs.A = 0x50;
    loadCode({0xC9, 0x50});
    cpu.exec();

    EXPECT_TRUE(cpu.regs.P & Cpu::Z);
    EXPECT_TRUE(cpu.regs.P & Cpu::C);
}

TEST_F(CpuTest, CPX) {
    cpu.regs.X = 0x40;
    loadCode({0xE0, 0x30});
    cpu.exec();

    EXPECT_TRUE(cpu.regs.P & Cpu::C);
}

TEST_F(CpuTest, CPY) {
    cpu.regs.Y = 0x7F;
    loadCode({0xC0, 0x80});
    cpu.exec();

    EXPECT_FALSE(cpu.regs.P & Cpu::C);
    EXPECT_TRUE(cpu.regs.P & Cpu::N);
}


/* Сдвиги */
TEST_F(CpuTest, ASL) {
    cpu.regs.A = 0x80;
    loadCode({0x0A});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x00);
    EXPECT_TRUE(cpu.regs.P & Cpu::C);
    EXPECT_TRUE(cpu.regs.P & Cpu::Z);
}

TEST_F(CpuTest, LSR) {
    cpu.regs.A = 0x01;
    loadCode({0x4A});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x00);
    EXPECT_TRUE(cpu.regs.P & Cpu::C);
}

TEST_F(CpuTest, ROL) {
    cpu.regs.A = 0x88;
    loadCode({0x2A});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x10);
    EXPECT_TRUE(cpu.regs.P & Cpu::C);
}

TEST_F(CpuTest, ROR) {
    cpu.set_flag(Cpu::C, true);
    cpu.regs.A = 0x02;
    loadCode({0x6A});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x81);
    EXPECT_FALSE(cpu.regs.P & Cpu::C);
}


/* Флаги */
TEST_F(CpuTest, CLC) {
    cpu.regs.P |= Cpu::C;
    loadCode({0x18});
    cpu.exec();

    EXPECT_FALSE(cpu.regs.P & Cpu::C);
}

TEST_F(CpuTest, SEC) {
    loadCode({0x38});
    cpu.exec();

    EXPECT_TRUE(cpu.regs.P & Cpu::C);
}

TEST_F(CpuTest, CLI) {
    cpu.regs.P |= Cpu::I;
    loadCode({0x58});
    cpu.exec();

    EXPECT_FALSE(cpu.regs.P & Cpu::I);
}

TEST_F(CpuTest, SEI) {
    loadCode({0x78});
    cpu.exec();

    EXPECT_TRUE(cpu.regs.P & Cpu::I);
}

TEST_F(CpuTest, CLD) {
    cpu.regs.P |= Cpu::D;
    loadCode({0xD8});
    cpu.exec();

    EXPECT_FALSE(cpu.regs.P & Cpu::D);
}

TEST_F(CpuTest, SED) {
    loadCode({0xF8});
    cpu.exec();

    EXPECT_TRUE(cpu.regs.P & Cpu::D);
}

TEST_F(CpuTest, CLV) {
    cpu.regs.P |= Cpu::V;
    loadCode({0xB8});
    cpu.exec();

    EXPECT_FALSE(cpu.regs.P & Cpu::V);
}


/* Стек */
TEST_F(CpuTest, PHA) {
    cpu.regs.A = 0x37;
    loadCode({0x48});
    cpu.exec();

    EXPECT_EQ(mem.read(STACK + cpu.regs.SP + 1), 0x37);
}

TEST_F(CpuTest, PLA) {
    cpu.regs.A = 0x00;
    cpu.push(0x42);
    loadCode({0x68});
    cpu.exec();

    EXPECT_EQ(cpu.regs.A, 0x42);
}

TEST_F(CpuTest, PHP) {
    cpu.regs.P = 0x30;
    loadCode({0x08});
    cpu.exec();

    EXPECT_EQ(mem.read(STACK + cpu.regs.SP + 1), 0x30 | UNUSED | BREAK);
}

TEST_F(CpuTest, PLP) {
    mem.ram.fill(0);
    cpu.regs.P = 0x00;
    cpu.regs.SP = 0xFF;
    mem.write(0x0000, 0x70);
    loadCode({0x28});
    cpu.exec();
}


/* Переходы */
TEST_F(CpuTest, JMP) {
    loadCode({0x4C, 0x00, 0x03});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0300);
}

TEST_F(CpuTest, JSR) {
    loadCode({0x20, 0x05, 0x02});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0205);
}

TEST_F(CpuTest, RTS) {
    cpu.push(0x02);
    cpu.push(0x03);
    loadCode({0x60});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0204);
}

TEST_F(CpuTest, BRK) {
    mem.write(0xFFFE, 0x00);
    mem.write(0xFFFF, 0x04);
    loadCode({0x00});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0400);
}

TEST_F(CpuTest, RTI) {
    cpu.push(0x02);
    cpu.push(0x03);
    cpu.push(0x30 | UNUSED);
    loadCode({0x40});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0203);
}


/* Ветвления */
TEST_F(CpuTest, BEQ) {
    cpu.regs.P |= Cpu::Z;
    loadCode({0xF0, 0x05});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0207);
}

TEST_F(CpuTest, BNE) {
    cpu.regs.P &= ~Cpu::Z;
    loadCode({0xD0, 0x05});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0207);
}

TEST_F(CpuTest, BCC) {
    cpu.regs.P &= ~Cpu::C;
    loadCode({0x90, 0x03});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0205);
}

TEST_F(CpuTest, BCS) {
    cpu.regs.P |= Cpu::C;
    loadCode({0xB0, 0x03});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0205);
}

TEST_F(CpuTest, BMI) {
    cpu.regs.P |= Cpu::N;
    loadCode({0x30, 0x03});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0205);
}

TEST_F(CpuTest, BPL) {
    cpu.regs.P &= ~Cpu::N;
    loadCode({0x10, 0x03});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0205);
}

TEST_F(CpuTest, BVC) {
    cpu.regs.P &= ~Cpu::V;
    loadCode({0x50, 0x03});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0205);
}

TEST_F(CpuTest, BVS) {
    cpu.regs.P |= Cpu::V;
    loadCode({0x70, 0x03});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, 0x0205);
}


/* NOP */
TEST_F(CpuTest, NOP) {
    u16 old = cpu.regs.PC;
    loadCode({0xEA});
    cpu.exec();

    EXPECT_EQ(cpu.regs.PC, old + 1);
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
                          /* loop:     */
        0x6D, 0x01, 0x00, /* ADC $0001 */
        0x88,             /* DEY       */
        0xD0, 0xFA,       /* BNE loop  */
        0x8D, 0x02, 0x00, /* STA $0002 */
        0xEA, 0xEA, 0xEA  /* NOP       */
    });

    while (mem.read(0x0002) == 0)
        cpu.exec();

    /* 10 * 3 = 30 (0x1E) */
    EXPECT_EQ(mem.read(0x0002), 0x1E);
}


TEST_F(CpuTest, NESTEST) {
    Cartridge cart;
    cart.loadNES("tests/roms/nestest.nes");

    Mapper0 mapper(cart);
    Memory memory(&mapper);
    Cpu cpu(&memory);


    cpu.reset();
    cpu.regs.PC = 0xC000;
    cpu.regs.P = 0x24;


    for(int i=0;i<8991;++i) {
        /* printf("PC:%04X A:%02X X:%02X Y:%02X P:%02x SP:%02X CYC:%lu\n",
            cpu.regs.PC,
            cpu.regs.A,
            cpu.regs.X,
            cpu.regs.Y,
            cpu.regs.P,
            cpu.regs.SP,
            cpu.getTotalCycles());
        */
        cpu.exec();
    }

    EXPECT_EQ(mem.read(0x0002), 0x00); /* Код выполнения */
    EXPECT_EQ(memory.read(0x0003), 0x00); /* Код ошибки  */
}



int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
