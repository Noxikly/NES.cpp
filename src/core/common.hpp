#pragma once

#include <array>
#include <cstdint>

#ifdef _WIN32
#define API_EXPORT extern "C" __declspec(dllexport)
#else
#define API_EXPORT extern "C"
#endif

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using i8 = int8_t;
using i16 = int16_t;

/* Common */
enum Region : u8 {
    NTSC = 0,
    PAL = 1,
    DENDY = 2,
};

enum VecType : u8 {
    VEC_PRG_ROM = 0,
    VEC_PRG_RAM = 1,
    VEC_CHR_ROM = 2,
};

/* For Memory */
inline constexpr u16 MIRROR = 0x07FF;
inline constexpr u16 STACK = 0x0100;

/* For CPU */
inline constexpr u8 CONSTANT = 0xEE;

/* For PPU */
static constexpr u16 WIDTH = 256;
static constexpr u16 HEIGHT = 240;
static constexpr u32 OPENBUS_DECAY_TICKS_NTSC = 5369318;
static constexpr u32 OPENBUS_DECAY_TICKS_PAL = 5320342;

inline constexpr std::array<u32, 64> PALETTE = {
    0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0700,
    0x561D00, 0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000,
    0x000000, 0x000000, 0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC,
    0xB71E7B, 0xB53120, 0x994E00, 0x6B6D00, 0x388700, 0x0C9300, 0x008F32,
    0x007C8D, 0x000000, 0x000000, 0x000000, 0xFFFEFF, 0x64B0FF, 0x9290FF,
    0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22, 0xBCBE00, 0x88D800,
    0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000, 0xFFFEFF,
    0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
    0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000,
    0x000000};

/* For APU */
inline constexpr u8 AUDIO_CHANNELS = 2;
inline constexpr double AUDIO_SAMPLE_RATE = 44100.0;
inline constexpr double NTSC_CPU_HZ = 1789773.0;
inline constexpr double PAL_CPU_HZ = 1652097.0;
inline constexpr double DENDY_CPU_HZ = 1773448.0;
inline constexpr double NTSC_CYCLES = AUDIO_SAMPLE_RATE / NTSC_CPU_HZ;
inline constexpr double PAL_CYCLES = AUDIO_SAMPLE_RATE / PAL_CPU_HZ;
inline constexpr double DENDY_CYCLES = AUDIO_SAMPLE_RATE / DENDY_CPU_HZ;

inline constexpr std::array<u8, 32> LENGTH_TABLE = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

inline constexpr std::array<std::array<u8, 8>, 4> DUTY_TABLE = {{
    {{0, 1, 0, 0, 0, 0, 0, 0}},
    {{0, 1, 1, 0, 0, 0, 0, 0}},
    {{0, 1, 1, 1, 1, 0, 0, 0}},
    {{1, 0, 0, 1, 1, 1, 1, 1}},
}};

inline constexpr std::array<u8, 32> TRIANGLE_TABLE = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
    0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

inline constexpr std::array<u16, 16> NOISE_TABLE = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

inline constexpr std::array<u16, 16> DMC_TABLE = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
};
