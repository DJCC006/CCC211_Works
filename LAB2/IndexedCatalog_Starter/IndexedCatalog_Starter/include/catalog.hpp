#pragma once

#include <cstddef>
#include <cstdint>
#include <istream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lab2 {

struct Record {
    std::string label_id;
    std::string composer;
    std::string title;

    bool operator==(const Record&) const = default;
};

enum class ReadStatus {
    Ok,
    NotFound,
    IndexKeyMismatch,
    InvalidOffset,
    TruncatedHeader,
    BadMagic,
    UnsupportedVersion,
    InvalidLength,
    TruncatedPayload,
    MissingChecksum,
    ChecksumMismatch,
    MalformedPayload
};

const char* to_string(ReadStatus status);

struct ReadResult {
    ReadStatus status{ReadStatus::InvalidOffset};
    std::optional<Record> record;
    std::uint64_t offset{};
    std::uint64_t next_offset{};
    std::string detail;

    [[nodiscard]] bool ok() const {
        return status == ReadStatus::Ok && record.has_value();
    }
};

struct PrimaryEntry {
    std::string label_id;
    std::uint64_t offset{};

    bool operator==(const PrimaryEntry&) const = default;
};

enum class BuildStatus {
    Ok,
    ReadError,
    DuplicateKey
};

const char* to_string(BuildStatus status);

struct PrimaryBuildResult {
    BuildStatus status{BuildStatus::ReadError};
    std::vector<PrimaryEntry> entries;
    std::uint64_t error_offset{};
    std::string error_key;
    std::string detail;

    [[nodiscard]] bool ok() const { return status == BuildStatus::Ok; }
};

struct ComposerEntry {
    std::string composer;
    std::vector<std::string> label_ids;

    bool operator==(const ComposerEntry&) const = default;
};

using ComposerIndex = std::vector<ComposerEntry>;

struct SkippedRecord {
    std::string index_key;
    std::uint64_t offset{};
    ReadStatus status{ReadStatus::InvalidOffset};
};

struct ComposerBuildResult {
    ComposerIndex entries;
    std::vector<SkippedRecord> skipped;
};

enum class VerificationIssueType {
    UnsortedIndex,
    DuplicateKey,
    DuplicateOffset,
    RecordReadError,
    KeyMismatch
};

const char* to_string(VerificationIssueType type);

struct VerificationIssue {
    VerificationIssueType type{VerificationIssueType::RecordReadError};
    std::string index_key;
    std::uint64_t offset{};
    ReadStatus read_status{ReadStatus::Ok};
    std::string detail;
};

struct VerificationReport {
    std::size_t entries_checked{};
    std::size_t readable_matching_entries{};
    std::vector<VerificationIssue> issues;

    [[nodiscard]] bool consistent() const { return issues.empty(); }
};

// TODO 1: lectura segura y validada de un registro.
ReadResult read_record_at(std::istream& input, std::uint64_t offset);

// TODO 2: recorrido secuencial y construcción del índice primario ordenado.
PrimaryBuildResult build_primary_index(std::istream& input);

// TODO 3: búsqueda binaria manual sobre el índice primario.
std::optional<std::uint64_t> find_offset(
    std::span<const PrimaryEntry> index,
    std::string_view label_id);

// Esta función integra TODO 1 y TODO 3.
ReadResult find_record(
    std::istream& input,
    std::span<const PrimaryEntry> index,
    std::string_view label_id);

// TODO 4: índice invertido compositor -> lista ordenada de label_id.
ComposerBuildResult build_composer_index(
    std::istream& input,
    std::span<const PrimaryEntry> primary);

// TODO 5: búsqueda por compositor sobre el índice secundario ordenado.
std::span<const std::string> find_by_composer(
    const ComposerIndex& index,
    std::string_view composer);

// TODO 6: auditoría de consistencia entre índice y archivo de datos.
VerificationReport verify_primary_index(
    std::istream& input,
    std::span<const PrimaryEntry> index);

// BONO: intersección de dos listas ordenadas en O(n + m).
std::vector<std::string> intersect_sorted(
    std::span<const std::string> left,
    std::span<const std::string> right);

} // namespace lab2
