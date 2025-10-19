#include <cstdint>


using u8 = std::uint8_t;
using i8 = std::int8_t;
using u16 = std::uint16_t;
using u64 = std::uint64_t;


inline constexpr u8  UNUSED = (1<<5);
inline constexpr u8  BREAK  = (1<<4);

inline constexpr u8 CONSTANT = 0xEE;

inline constexpr u16 MIRROR = 0x07FF;
inline constexpr u16 STACK = 0x0100;
