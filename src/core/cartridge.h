#pragma once

#include <array>
#include <filesystem>
#include <vector>

#include "common/types.h"

namespace Core {
class Cartridge {
public:
    explicit Cartridge() = default;
    ~Cartridge() = default;

    void loadNES(const std::filesystem::path &path);

public:
    enum MirrorMode : u8 {
        HORIZONTAL = 0,  /* [A,A,B,B] */
        VERTICAL = 1,    /* [A,B,A,B] */
        FOUR = 2,        /* [A,B,C,D] */
        SINGLE_DOWN = 3, /* [A,A,A,A] */
        SINGLE_UP = 4,   /* [B,B,B,B] */
    } mirror{HORIZONTAL};

    struct NESFormat {
        std::array<u8, 16> raw{};

        inline bool valid() const {
            return raw[0] == 'N' && raw[1] == 'E' && raw[2] == 'S' &&
                   raw[3] == 0x1A;
        }

        inline u8 prg_banks() const { return raw[4]; }
        inline u8 chr_banks() const { return raw[5]; }
        inline u8 flags6() const { return raw[6]; }
        inline u8 flags7() const { return raw[7]; }

        inline bool has_trainer() const { return (flags6() & 0x04) != 0; }
        inline u8 mapper() const {
            return static_cast<u8>((flags7() & 0xF0) | (flags6() >> 4));
        }
        inline MirrorMode mirroring() const {
            return (flags6() & 0x01) ? VERTICAL : HORIZONTAL;
        }
        inline bool chr_is_ram() const { return chr_banks() == 0; }

    } fmt{};

public:
    std::vector<u8> PRG_ROM; /* Program ROM (код)       */
    std::vector<u8> PRG_RAM{std::vector<u8>(0x2000)}; /* SRAM */
    std::vector<u8> CHR_ROM; /* Character ROM (спрайты) */

    u8 mapperNumber{0}; /* Номер маппера (0-255)   */
    bool chrRam{false};
    bool irqFlag{false};
};

} /* namespace Core */