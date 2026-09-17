#include "index_io.hpp"

#include <charconv>
#include <fstream>
#include <system_error>

namespace lab2 {

IndexLoadResult load_primary_index(const std::filesystem::path& path) {
    IndexLoadResult result;
    std::ifstream input(path);
    if (!input) {
        result.error = "No se pudo abrir el archivo de índice: " + path.string();
        return result;
    }

    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const std::size_t tab = line.find('\t');
        if (tab == std::string::npos || tab == 0U ||
            line.find('\t', tab + 1U) != std::string::npos) {
            result.error = "Línea de índice inválida: " + std::to_string(line_number);
            result.entries.clear();
            return result;
        }

        std::uint64_t offset = 0;
        const char* first = line.data() + tab + 1U;
        const char* last = line.data() + line.size();
        const auto parsed = std::from_chars(first, last, offset);
        if (parsed.ec != std::errc{} || parsed.ptr != last) {
            result.error = "Offset inválido en línea " + std::to_string(line_number);
            result.entries.clear();
            return result;
        }
        result.entries.push_back({line.substr(0, tab), offset});
    }

    if (!input.eof()) {
        result.error = "Error mientras se leía el archivo de índice.";
        result.entries.clear();
        return result;
    }

    result.success = true;
    return result;
}

bool save_primary_index(const std::filesystem::path& path,
                        std::span<const PrimaryEntry> entries,
                        std::string& error) {
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "No se pudo crear el archivo de índice: " + path.string();
        return false;
    }

    for (const PrimaryEntry& entry : entries) {
        if (entry.label_id.empty() || entry.label_id.find_first_of("\t\r\n") != std::string::npos) {
            error = "El índice contiene una clave que no puede serializarse.";
            return false;
        }
        output << entry.label_id << '\t' << entry.offset << '\n';
    }
    if (!output) {
        error = "Falló la escritura del archivo de índice.";
        return false;
    }
    return true;
}

} // namespace lab2
