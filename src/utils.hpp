#include <cstdint>
#include <array>


using u8 = std::uint8_t;
using i8 = std::int8_t;
using u16 = std::uint16_t;
using i16 = std::int16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;


inline constexpr u8  UNUSED = (1<<5);
inline constexpr u8  BREAK  = (1<<4);

inline constexpr u8 CONSTANT = 0xEE;

inline constexpr u16 MIRROR = 0x07FF;
inline constexpr u16 STACK = 0x0100;


inline constexpr u16 DOTS   = 341;
inline constexpr u16 LINES  = 262;
inline constexpr u16 WIDTH  = 256;
inline constexpr u16 HEIGHT = 240;
inline constexpr u8  SCALE  = 3;

constexpr std::array<u8, 192> paletteRGB = {
/*  RR    GG   BB   RR   GG   BB   RR   GG   BB   RR   GG   BB  */
    84,   84,  84,   0,  30, 116,   8,  16, 144,  48,   0, 136,
    68,    0, 100,  92,   0,  48,  84,   4,   0,  60,  24,   0,
    32,   42,   0,   8,  58,   0,   0,  64,   0,   0,  60,   0,
    0,    50,  60,   0,   0,   0,   0,   0,   0,   0,   0,   0,

    152, 150, 152,   8,  76, 196,  48,  50, 236,  92,  30, 228,
    136,  20, 176, 160,  20, 100, 152,  34,  32, 120,  60,   0,
    84,   90,   0,  40, 114,   0,   8, 124,   0,   0, 118,  40,
    0,  102,  120,   0,   0,   0,   0,   0,   0,   0,   0,   0,

    236, 238, 236,  76, 154, 236, 120, 124, 236, 176,  98, 236,
    228,  84, 236, 236,  88, 180, 236, 106, 100, 212, 136,  32,
    160, 170,   0, 116, 196,   0,  76, 208,  32,  56, 204, 108,
    56,  180, 204,  60,  60,  60,   0,   0,   0,   0,   0,   0,

    236, 238, 236,  76, 154, 236, 120, 124, 236, 176,  98, 236,
    228,  84, 236, 236,  88, 180, 236, 106, 100, 212, 136,  32,
    160, 170,   0, 116, 196,   0,  76, 208,  32,  56, 204, 108,
    56,  180, 204,  60,  60,  60,  60,  60,  60,  60,  60,  60,
};
