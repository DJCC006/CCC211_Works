#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace lab2 {

// CRC-32/ISO-HDLC (polinomio reflejado 0xEDB88320).
std::uint32_t crc32(std::span<const std::byte> bytes);

} // namespace lab2
