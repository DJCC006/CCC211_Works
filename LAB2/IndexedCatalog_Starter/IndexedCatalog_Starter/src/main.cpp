#include "catalog.hpp"
#include "index_io.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void print_usage(const char* program) {
    std::cerr
        << "Uso:\n"
        << "  " << program << " build DATA.bin INDEX.idx\n"
        << "  " << program << " find DATA.bin INDEX.idx LABEL_ID\n"
        << "  " << program << " composer DATA.bin INDEX.idx COMPOSER\n"
        << "  " << program << " verify DATA.bin INDEX.idx\n";
}

void print_record(const lab2::Record& record) {
    std::cout << record.label_id << " | " << record.composer
              << " | " << record.title << '\n';
}

std::ifstream open_data(const std::filesystem::path& path) {
    return std::ifstream(path, std::ios::binary);
}

int command_build(const std::filesystem::path& data_path,
                  const std::filesystem::path& index_path) {
    std::ifstream data = open_data(data_path);
    if (!data) {
        std::cerr << "No se pudo abrir " << data_path << '\n';
        return 2;
    }

    const lab2::PrimaryBuildResult built = lab2::build_primary_index(data);
    if (!built.ok()) {
        std::cerr << "No se pudo construir el índice: "
                  << lab2::to_string(built.status)
                  << " offset=" << built.error_offset;
        if (!built.error_key.empty()) {
            std::cerr << " key=" << built.error_key;
        }
        std::cerr << " " << built.detail << '\n';
        return 3;
    }

    std::string error;
    if (!lab2::save_primary_index(index_path, built.entries, error)) {
        std::cerr << error << '\n';
        return 4;
    }
    std::cout << "Índice creado: " << built.entries.size() << " entradas.\n";
    return 0;
}

lab2::IndexLoadResult load_index_or_report(const std::filesystem::path& path) {
    lab2::IndexLoadResult loaded = lab2::load_primary_index(path);
    if (!loaded.success) {
        std::cerr << loaded.error << '\n';
    }
    return loaded;
}

int command_find(const std::filesystem::path& data_path,
                 const std::filesystem::path& index_path,
                 std::string_view label_id) {
    lab2::IndexLoadResult loaded = load_index_or_report(index_path);
    if (!loaded.success) {
        return 2;
    }
    std::ifstream data = open_data(data_path);
    if (!data) {
        std::cerr << "No se pudo abrir " << data_path << '\n';
        return 2;
    }

    const lab2::ReadResult result = lab2::find_record(data, loaded.entries, label_id);
    if (!result.ok()) {
        std::cerr << lab2::to_string(result.status) << ": " << result.detail << '\n';
        return result.status == lab2::ReadStatus::NotFound ? 1 : 3;
    }
    print_record(*result.record);
    return 0;
}

int command_composer(const std::filesystem::path& data_path,
                     const std::filesystem::path& index_path,
                     std::string_view composer) {
    lab2::IndexLoadResult loaded = load_index_or_report(index_path);
    if (!loaded.success) {
        return 2;
    }
    std::ifstream data = open_data(data_path);
    if (!data) {
        std::cerr << "No se pudo abrir " << data_path << '\n';
        return 2;
    }

    const lab2::ComposerBuildResult secondary =
        lab2::build_composer_index(data, loaded.entries);
    const std::span<const std::string> ids =
        lab2::find_by_composer(secondary.entries, composer);

    for (const std::string& id : ids) {
        const lab2::ReadResult result = lab2::find_record(data, loaded.entries, id);
        if (result.ok()) {
            print_record(*result.record);
        }
    }
    std::cout << "Coincidencias: " << ids.size()
              << "; referencias corruptas omitidas: " << secondary.skipped.size()
              << '\n';
    return 0;
}

int command_verify(const std::filesystem::path& data_path,
                   const std::filesystem::path& index_path) {
    lab2::IndexLoadResult loaded = load_index_or_report(index_path);
    if (!loaded.success) {
        return 2;
    }
    std::ifstream data = open_data(data_path);
    if (!data) {
        std::cerr << "No se pudo abrir " << data_path << '\n';
        return 2;
    }

    const lab2::VerificationReport report =
        lab2::verify_primary_index(data, loaded.entries);
    std::cout << "Entradas revisadas: " << report.entries_checked << '\n'
              << "Referencias legibles y coincidentes: "
              << report.readable_matching_entries << '\n'
              << "Problemas: " << report.issues.size() << '\n';

    for (const lab2::VerificationIssue& issue : report.issues) {
        std::cout << "- " << lab2::to_string(issue.type)
                  << " key=" << issue.index_key
                  << " offset=" << issue.offset;
        if (issue.type == lab2::VerificationIssueType::RecordReadError) {
            std::cout << " read_status=" << lab2::to_string(issue.read_status);
        }
        if (!issue.detail.empty()) {
            std::cout << " — " << issue.detail;
        }
        std::cout << '\n';
    }
    return report.consistent() ? 0 : 5;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 64;
    }

    const std::string command = argv[1];
    if (command == "build" && argc == 4) {
        return command_build(argv[2], argv[3]);
    }
    if (command == "find" && argc == 5) {
        return command_find(argv[2], argv[3], argv[4]);
    }
    if (command == "composer" && argc == 5) {
        return command_composer(argv[2], argv[3], argv[4]);
    }
    if (command == "verify" && argc == 4) {
        return command_verify(argv[2], argv[3]);
    }

    print_usage(argv[0]);
    return 64;
}
