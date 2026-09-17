#pragma once

#include "catalog.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace lab2 {

inline constexpr std::uint32_t RECORD_MAGIC = 0x3253554DU; // bytes: M U S 2
inline constexpr std::uint16_t RECORD_VERSION = 1;
inline constexpr std::uint32_t MAX_PAYLOAD_SIZE = 64U * 1024U;
inline constexpr std::uint64_t RECORD_HEADER_SIZE = 10U;
inline constexpr std::uint64_t RECORD_CHECKSUM_SIZE = 4U;

struct PayloadDecodeResult {
    std::optional<Record> record;
    std::string detail;
};

std::vector<std::byte> encode_payload(const Record& record);
PayloadDecodeResult decode_payload(std::span<const std::byte> payload);

// Utilidad provista para generar fixtures; no forma parte de los TODO del estudiante.
bool write_record(
    std::ostream& output,
    const Record& record,
    std::uint64_t* written_offset,
    std::string& error);

} // namespace lab2
