#include "cartridge.hpp"

#include <ios>
#include <fstream>
#include <stdexcept>


void Cartridge::loadNES(const std::filesystem::path& path) {
    std::ifstream rom(path, std::ios::binary);
    if (!rom)
        throw std::runtime_error("[LOAD]: Не удалось открыть ROM файл");

    rom.read(reinterpret_cast<char*>(fmt.raw.data()), static_cast<std::streamsize>(fmt.raw.size()));
    if (!rom || !fmt.valid())
        throw std::runtime_error("[LOAD]: Неверный iNES заголовок");

    /* Пропуск trainer (если есть) */
    if (fmt.has_trainer())
        rom.seekg(512, std::ios::cur);

    mapperNumber = fmt.mapper();
    mirror = fmt.mirroring();

    /* Чтение PRG ROM */
    PRG_ROM.resize(static_cast<size_t>(fmt.prg_banks()) * 0x4000);
    rom.read(reinterpret_cast<char*>(PRG_ROM.data()), static_cast<std::streamsize>(PRG_ROM.size()));
    if (!rom)
        throw std::runtime_error("[LOAD]: Не удалось прочитать PRG");

    /* Чтение CHR ROM */
    if (!fmt.chr_is_ram()) {
        chrRam = false;
        CHR_ROM.resize(static_cast<size_t>(fmt.chr_banks()) * 0x2000);
        rom.read(reinterpret_cast<char*>(CHR_ROM.data()), static_cast<std::streamsize>(CHR_ROM.size()));
        if (!rom)
            throw std::runtime_error("[LOAD]: Не удалось прочитать CHR");
    } else {
        chrRam = true;
        CHR_ROM.clear();
    }
}

