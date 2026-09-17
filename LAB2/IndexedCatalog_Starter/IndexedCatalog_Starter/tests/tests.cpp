#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "catalog.hpp"
#include "catalog_codec.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Fixture {
    std::string bytes;
    std::vector<std::uint64_t> offsets;
};

Fixture make_fixture(const std::vector<lab2::Record>& records) {
    std::ostringstream output(std::ios::binary | std::ios::out);
    Fixture fixture;
    for (const auto& record : records) {
        std::uint64_t offset = 0;
        std::string error;
        REQUIRE(lab2::write_record(output, record, &offset, error));
        fixture.offsets.push_back(offset);
    }
    fixture.bytes = output.str();
    return fixture;
}

std::stringstream open_fixture(const std::string& bytes) {
    return std::stringstream(bytes, std::ios::binary | std::ios::in | std::ios::out);
}

const std::vector<lab2::Record> sample_records{
    {"WAR23699", "COREA", "TOUCHSTONE"},
    {"ANG3795", "BEETHOVEN", "SYMPHONY NO. 9"},
    {"DG18807", "BEETHOVEN", "SYMPHONY NO. 9"},
    {"COL31809", "DVORAK", "SYMPHONY NO. 9"}
};

} // namespace

TEST_CASE("E01 - read_record_at lee un registro válido desde su offset") {
    const Fixture fixture = make_fixture(sample_records);
    auto input = open_fixture(fixture.bytes);

    const lab2::ReadResult result = lab2::read_record_at(input, fixture.offsets[1]);

    REQUIRE(result.ok());
    CHECK(*result.record == sample_records[1]);
    CHECK(result.offset == fixture.offsets[1]);
    CHECK(result.next_offset == fixture.offsets[2]);
}

TEST_CASE("E02 - read_record_at distingue magic inválido y CRC incorrecto") {
    Fixture fixture = make_fixture({sample_records[0]});

    SUBCASE("magic inválido") {
        fixture.bytes[0] = static_cast<char>(fixture.bytes[0] ^ 0x01);
        auto input = open_fixture(fixture.bytes);
        CHECK(lab2::read_record_at(input, 0).status == lab2::ReadStatus::BadMagic);
    }

    SUBCASE("payload modificado") {
        // Header = 10 bytes; los primeros 2 bytes del payload son label_id_length.
        fixture.bytes[12] = static_cast<char>(fixture.bytes[12] ^ 0x01);
        auto input = open_fixture(fixture.bytes);
        CHECK(lab2::read_record_at(input, 0).status ==
              lab2::ReadStatus::ChecksumMismatch);
    }

    SUBCASE("offset fuera del archivo") {
        auto input = open_fixture(fixture.bytes);
        CHECK(lab2::read_record_at(input, fixture.bytes.size()).status ==
              lab2::ReadStatus::InvalidOffset);
    }
}

TEST_CASE("E03 - read_record_at detecta checksum truncado") {
    Fixture fixture = make_fixture({sample_records[0]});
    fixture.bytes.resize(fixture.bytes.size() - 2U);
    auto input = open_fixture(fixture.bytes);

    CHECK(lab2::read_record_at(input, 0).status == lab2::ReadStatus::MissingChecksum);
}

TEST_CASE("E04 - build_primary_index produce entradas ordenadas y offsets correctos") {
    const Fixture fixture = make_fixture(sample_records);
    auto input = open_fixture(fixture.bytes);

    const lab2::PrimaryBuildResult result = lab2::build_primary_index(input);

    REQUIRE(result.ok());
    REQUIRE(result.entries.size() == 4);
    CHECK(result.entries[0] == lab2::PrimaryEntry{"ANG3795", fixture.offsets[1]});
    CHECK(result.entries[1] == lab2::PrimaryEntry{"COL31809", fixture.offsets[3]});
    CHECK(result.entries[2] == lab2::PrimaryEntry{"DG18807", fixture.offsets[2]});
    CHECK(result.entries[3] == lab2::PrimaryEntry{"WAR23699", fixture.offsets[0]});
}

TEST_CASE("E05 - build_primary_index rechaza claves primarias duplicadas") {
    const Fixture fixture = make_fixture({
        {"DUP1", "COMPOSER A", "TITLE A"},
        {"DUP1", "COMPOSER B", "TITLE B"}
    });
    auto input = open_fixture(fixture.bytes);

    const lab2::PrimaryBuildResult result = lab2::build_primary_index(input);

    CHECK(result.status == lab2::BuildStatus::DuplicateKey);
    CHECK(result.error_key == "DUP1");
}

TEST_CASE("E06 - find_offset realiza búsquedas en límites y ausencia") {
    const std::vector<lab2::PrimaryEntry> index{
        {"ANG3795", 20}, {"COL31809", 80}, {"DG18807", 140}, {"WAR23699", 300}
    };

    CHECK(lab2::find_offset(index, "ANG3795") == 20);
    CHECK(lab2::find_offset(index, "DG18807") == 140);
    CHECK(lab2::find_offset(index, "WAR23699") == 300);
    CHECK_FALSE(lab2::find_offset(index, "ZZZ").has_value());
    CHECK_FALSE(lab2::find_offset({}, "ANG3795").has_value());
}

TEST_CASE("E07 - build_composer_index agrupa claves y las mantiene ordenadas") {
    const Fixture fixture = make_fixture(sample_records);
    const std::vector<lab2::PrimaryEntry> primary{
        {"ANG3795", fixture.offsets[1]},
        {"COL31809", fixture.offsets[3]},
        {"DG18807", fixture.offsets[2]},
        {"WAR23699", fixture.offsets[0]}
    };

    auto input = open_fixture(fixture.bytes);
    const lab2::ComposerBuildResult secondary =
        lab2::build_composer_index(input, primary);

    CHECK(secondary.skipped.empty());
    REQUIRE(secondary.entries.size() == 3);
    CHECK(secondary.entries[0].composer == "BEETHOVEN");
    CHECK(secondary.entries[0].label_ids ==
          std::vector<std::string>{"ANG3795", "DG18807"});
    CHECK(secondary.entries[1].composer == "COREA");
    CHECK(secondary.entries[2].composer == "DVORAK");
}

TEST_CASE("E08 - find_by_composer encuentra una lista sin escanear registros") {
    const lab2::ComposerIndex index{
        {"BEETHOVEN", {"ANG3795", "DG18807"}},
        {"COREA", {"WAR23699"}},
        {"DVORAK", {"COL31809"}}
    };

    const auto found = lab2::find_by_composer(index, "COREA");
    REQUIRE(found.size() == 1);
    CHECK(found[0] == "WAR23699");
    CHECK(lab2::find_by_composer(index, "MOZART").empty());
}

TEST_CASE("E09 - verify_primary_index acepta un índice consistente") {
    const Fixture fixture = make_fixture(sample_records);
    const std::vector<lab2::PrimaryEntry> primary{
        {"ANG3795", fixture.offsets[1]},
        {"COL31809", fixture.offsets[3]},
        {"DG18807", fixture.offsets[2]},
        {"WAR23699", fixture.offsets[0]}
    };

    auto input = open_fixture(fixture.bytes);
    const lab2::VerificationReport report =
        lab2::verify_primary_index(input, primary);

    CHECK(report.entries_checked == primary.size());
    CHECK(report.readable_matching_entries == primary.size());
    CHECK(report.consistent());
}

TEST_CASE("E10 - verify_primary_index detecta problemas estructurales y referencias") {
    const Fixture fixture = make_fixture(sample_records);
    std::vector<lab2::PrimaryEntry> damaged_index{
        {"ZZZ-WRONG-KEY", fixture.offsets[1]},
        {"ANG3795", fixture.offsets[1]},
        {"ANG3795", fixture.offsets[2]}
    };
    auto input = open_fixture(fixture.bytes);

    const lab2::VerificationReport report =
        lab2::verify_primary_index(input, damaged_index);

    CHECK_FALSE(report.consistent());
    const auto has_type = [&](lab2::VerificationIssueType type) {
        return std::any_of(report.issues.begin(), report.issues.end(),
                           [&](const auto& issue) { return issue.type == type; });
    };
    CHECK(has_type(lab2::VerificationIssueType::UnsortedIndex));
    CHECK(has_type(lab2::VerificationIssueType::DuplicateKey));
    CHECK(has_type(lab2::VerificationIssueType::DuplicateOffset));
    CHECK(has_type(lab2::VerificationIssueType::KeyMismatch));
}
