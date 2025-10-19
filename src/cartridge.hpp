#pragma once
#include <vector>
#include <string>
#include "utils.hpp"


class Cartridge {
public:
    void loadNES(const std::string& path);


    std::vector<u8>& getPRG() { return PRG_ROM; }
    std::vector<u8>& getCHR() { return CHR_ROM; }

private:
    std::vector<u8> PRG_ROM; /* Program ROM (код) */
    std::vector<u8> CHR_ROM; /* Character ROM (спрайты) */
};
