#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include "constants.hpp"


class Cartridge {
public:
    void loadNES(const std::string& path) {
        std::ifstream ROM(path, std::ios::binary);
        if (!ROM) throw std::runtime_error("[LOAD]: Не удалось открыть ROM файл");


        std::array<u8, 16> header{};
        ROM.read(reinterpret_cast<char*>(header.data()), 16);
        if (!ROM || std::string(&header[0], &header[4]) != "NES\x1A")
            throw std::runtime_error("[LOAD]: Неверный iNES заголовок");

        /* Парсинг заголовка */
        const u8 prgBanks = header[4];  /* Количество 16KB банков PRG ROM */
        const u8 chrBanks = header[5];  /* Количество 8KB банков CHR ROM  */
        const u8 flags6 = header[6];    /* Флаги 6 */
        const u8 flags7 = header[7];    /* Флаги 7 */


        mirrorMode = (flags6 & 0x01);  /* 0 = horizontal, 1 = vertical */
        mapperNumber = (flags7 & 0xF0) | (flags6 >> 4);

        /* Чтение PRG ROM */
        PRG_ROM.resize(prgBanks * 0x4000);
        ROM.read(reinterpret_cast<char*>(PRG_ROM.data()), PRG_ROM.size());
        if (!ROM) throw std::runtime_error("[LOAD]: Не удалось прочитать PRG");

        /* Чтение CHR ROM */
        if (chrBanks) {
            CHR_ROM.resize(chrBanks * 0x2000);
            ROM.read(reinterpret_cast<char*>(CHR_ROM.data()), CHR_ROM.size());
            if (!ROM) throw std::runtime_error("[LOAD]: Не удалось прочитать CHR");
        }
    }

public:
    std::vector<u8> PRG_ROM; /* Program ROM (код)                 */
    std::vector<u8> PRG_RAM = std::vector<u8>(0x2000, 0); /* SRAM */
    std::vector<u8> CHR_ROM; /* Character ROM (спрайты)           */
    u8 mapperNumber{0};      /* Номер маппера (0-255)             */
    u8 mirrorMode{0};        /* 0 = horizontal, 1 = vertical      */

    bool irqFlag{false};
};
