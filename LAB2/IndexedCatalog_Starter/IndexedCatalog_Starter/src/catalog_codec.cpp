#include "catalog_codec.hpp"

#include "binary_io.hpp"
#include "crc32.hpp"

#include <algorithm>
#include <limits>

namespace lab2 {
namespace {

void append_u16(std::vector<std::byte>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xFFU));
    bytes.push_back(static_cast<std::byte>((value >> 8U) & 0xFFU));
}

void append_string(std::vector<std::byte>& bytes, const std::string& value) {
    append_u16(bytes, static_cast<std::uint16_t>(value.size()));
    for (const unsigned char character : value) {
        bytes.push_back(static_cast<std::byte>(character));
    }
}

bool take_u16(std::span<const std::byte> payload,
              std::size_t& position,
              std::uint16_t& value) {
    if (position + 2U > payload.size()) {
        return false;
    }
    value = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(payload[position])) |
            (static_cast<std::uint16_t>(
                 std::to_integer<std::uint8_t>(payload[position + 1U])) << 8U);
    position += 2U;
    return true;
}

bool take_string(std::span<const std::byte> payload,
                 std::size_t& position,
                 std::string& value) {
    std::uint16_t length = 0;
    if (!take_u16(payload, position, length) || position + length > payload.size()) {
        return false;
    }
    value.assign(reinterpret_cast<const char*>(payload.data() + position), length);
    position += length;
    return true;
}

bool contains_forbidden_separator(const std::string& value) {
    return value.find('\0') != std::string::npos ||
           value.find('\n') != std::string::npos ||
           value.find('\r') != std::string::npos;
}

} // namespace

std::vector<std::byte> encode_payload(const Record& record) {
    std::vector<std::byte> payload;
    payload.reserve(6U + record.label_id.size() + record.composer.size() + record.title.size());
    append_string(payload, record.label_id);
    append_string(payload, record.composer);
    append_string(payload, record.title);
    return payload;
}

PayloadDecodeResult decode_payload(std::span<const std::byte> payload) {
    PayloadDecodeResult result;
    std::size_t position = 0;
    Record record;

    if (!take_string(payload, position, record.label_id) ||
        !take_string(payload, position, record.composer) ||
        !take_string(payload, position, record.title)) {
        result.detail = "Las longitudes internas exceden el payload disponible.";
        return result;
    }
    if (position != payload.size()) {
        result.detail = "El payload contiene bytes adicionales no declarados.";
        return result;
    }
    if (record.label_id.empty() || record.composer.empty() || record.title.empty()) {
        result.detail = "label_id, composer y title deben ser no vacíos.";
        return result;
    }
    if (contains_forbidden_separator(record.label_id) ||
        contains_forbidden_separator(record.composer) ||
        contains_forbidden_separator(record.title)) {
        result.detail = "Los campos no pueden contener NUL ni saltos de línea.";
        return result;
    }

    result.record = std::move(record);
    return result;
}

bool write_record(std::ostream& output,
                  const Record& record,
                  std::uint64_t* written_offset,
                  std::string& error) {
    const auto label_limit = static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max());
    if (record.label_id.empty() || record.composer.empty() || record.title.empty()) {
        error = "Todos los campos son obligatorios.";
        return false;
    }
    if (record.label_id.size() > label_limit || record.composer.size() > label_limit ||
        record.title.size() > label_limit) {
        error = "Un campo excede el máximo representable de 65535 bytes.";
        return false;
    }
    if (contains_forbidden_separator(record.label_id) ||
        contains_forbidden_separator(record.composer) ||
        contains_forbidden_separator(record.title)) {
        error = "Los campos no pueden contener NUL ni saltos de línea.";
        return false;
    }

    const std::vector<std::byte> payload = encode_payload(record);
    if (payload.size() > MAX_PAYLOAD_SIZE) {
        error = "El payload excede MAX_PAYLOAD_SIZE.";
        return false;
    }

    const auto position = output.tellp();
    if (position < 0) {
        error = "No se pudo determinar el offset de escritura.";
        return false;
    }
    if (written_offset != nullptr) {
        *written_offset = static_cast<std::uint64_t>(position);
    }

    if (!write_u32_le(output, RECORD_MAGIC) ||
        !write_u16_le(output, RECORD_VERSION) ||
        !write_u32_le(output, static_cast<std::uint32_t>(payload.size())) ||
        !write_exact(output, payload) ||
        !write_u32_le(output, crc32(payload))) {
        error = "Falló la escritura del registro.";
        return false;
    }
    return true;
}

} // namespace lab2
