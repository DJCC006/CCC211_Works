#pragma once

#include <cstddef>
#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <span>

namespace lab2 {

bool read_exact(std::istream& input, std::span<std::byte> destination);
bool write_exact(std::ostream& output, std::span<const std::byte> source);

bool read_u16_le(std::istream& input, std::uint16_t& value);
bool read_u32_le(std::istream& input, std::uint32_t& value);
bool read_u64_le(std::istream& input, std::uint64_t& value);

bool write_u16_le(std::ostream& output, std::uint16_t value);
bool write_u32_le(std::ostream& output, std::uint32_t value);
bool write_u64_le(std::ostream& output, std::uint64_t value);

// Conserva la posición lógica del stream cuando es posible.
std::optional<std::uint64_t> stream_size(std::istream& input);
bool seek_absolute(std::istream& input, std::uint64_t offset);

} // namespace lab2
