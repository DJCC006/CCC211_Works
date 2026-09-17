#include "binary_io.hpp"

#include <array>
#include <limits>

namespace lab2 {

bool read_exact(std::istream& input, std::span<std::byte> destination) {
    if (destination.empty()) {
        return true;
    }
    input.read(reinterpret_cast<char*>(destination.data()),
               static_cast<std::streamsize>(destination.size()));
    return input.gcount() == static_cast<std::streamsize>(destination.size());
}

bool write_exact(std::ostream& output, std::span<const std::byte> source) {
    if (source.empty()) {
        return true;
    }
    output.write(reinterpret_cast<const char*>(source.data()),
                 static_cast<std::streamsize>(source.size()));
    return static_cast<bool>(output);
}

bool read_u16_le(std::istream& input, std::uint16_t& value) {
    std::array<std::byte, 2> bytes{};
    if (!read_exact(input, bytes)) {
        return false;
    }
    value = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[0])) |
            (static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[1])) << 8U);
    return true;
}

bool read_u32_le(std::istream& input, std::uint32_t& value) {
    std::array<std::byte, 4> bytes{};
    if (!read_exact(input, bytes)) {
        return false;
    }
    value = static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[0])) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[1])) << 8U) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[2])) << 16U) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[3])) << 24U);
    return true;
}

bool read_u64_le(std::istream& input, std::uint64_t& value) {
    std::array<std::byte, 8> bytes{};
    if (!read_exact(input, bytes)) {
        return false;
    }
    value = 0;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes[i]))
                 << (8U * i);
    }
    return true;
}

bool write_u16_le(std::ostream& output, std::uint16_t value) {
    const std::array bytes{
        static_cast<std::byte>(value & 0xFFU),
        static_cast<std::byte>((value >> 8U) & 0xFFU)};
    return write_exact(output, bytes);
}

bool write_u32_le(std::ostream& output, std::uint32_t value) {
    const std::array bytes{
        static_cast<std::byte>(value & 0xFFU),
        static_cast<std::byte>((value >> 8U) & 0xFFU),
        static_cast<std::byte>((value >> 16U) & 0xFFU),
        static_cast<std::byte>((value >> 24U) & 0xFFU)};
    return write_exact(output, bytes);
}

bool write_u64_le(std::ostream& output, std::uint64_t value) {
    std::array<std::byte, 8> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::byte>((value >> (8U * i)) & 0xFFU);
    }
    return write_exact(output, bytes);
}

std::optional<std::uint64_t> stream_size(std::istream& input) {
    const auto previous_state = input.rdstate();
    input.clear();
    const auto previous_position = input.tellg();

    input.seekg(0, std::ios::end);
    const auto end_position = input.tellg();
    if (end_position < 0) {
        input.clear(previous_state);
        return std::nullopt;
    }

    input.clear();
    if (previous_position >= 0) {
        input.seekg(previous_position);
    }
    if (previous_state != std::ios::goodbit) {
        input.setstate(previous_state);
    }
    return static_cast<std::uint64_t>(end_position);
}

bool seek_absolute(std::istream& input, std::uint64_t offset) {
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        return false;
    }
    input.clear();
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    return static_cast<bool>(input);
}

} // namespace lab2
