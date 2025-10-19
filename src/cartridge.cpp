#include <fstream>
#include <array>
#include "cartridge.hpp"


void Cartridge::loadNES(const std::string& path) {
    std::ifstream ROM(path, std::ios::binary);
    if (!ROM) throw std::runtime_error("Не удалось открыть ROM файл");


    std::array<u8, 16> header{};
    ROM.read(reinterpret_cast<char*>(header.data()), 16);
    if (!ROM || std::string(&header[0], &header[4]) != "NES\x1A")
        throw std::runtime_error("Неверный iNES заголовок");


    u8 prgBanks = header[4];
    u8 chrBanks = header[5];

    PRG_ROM.resize(prgBanks * 0x4000);
    ROM.read(reinterpret_cast<char*>(PRG_ROM.data()), PRG_ROM.size());
    if (!ROM) throw std::runtime_error("Не удалось прочитать PRG");

    if (chrBanks) {
        CHR_ROM.resize(chrBanks * 0x2000);
        ROM.read(reinterpret_cast<char*>(CHR_ROM.data()), CHR_ROM.size());
        if (!ROM) throw std::runtime_error("Не удалось прочитать CHR");
    }
}
