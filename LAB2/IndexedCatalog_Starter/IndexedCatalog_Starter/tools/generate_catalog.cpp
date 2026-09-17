#include "catalog_codec.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace {

bool parse_line(const std::string& line, lab2::Record& record) {
    const std::size_t first = line.find('|');
    const std::size_t second = first == std::string::npos
        ? std::string::npos
        : line.find('|', first + 1U);
    if (first == std::string::npos || second == std::string::npos ||
        line.find('|', second + 1U) != std::string::npos) {
        return false;
    }
    record.label_id = line.substr(0, first);
    record.composer = line.substr(first + 1U, second - first - 1U);
    record.title = line.substr(second + 1U);
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " INPUT.psv OUTPUT.bin\n";
        return 64;
    }

    std::ifstream source(argv[1]);
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!source || !output) {
        std::cerr << "No se pudo abrir la entrada o la salida.\n";
        return 2;
    }

    std::string line;
    std::size_t line_number = 0;
    std::size_t records = 0;
    while (std::getline(source, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line_number == 1U && line == "label_id|composer|title") {
            continue;
        }
        if (line.empty()) {
            continue;
        }

        lab2::Record record;
        if (!parse_line(line, record)) {
            std::cerr << "Línea PSV inválida: " << line_number << '\n';
            return 3;
        }
        std::string error;
        if (!lab2::write_record(output, record, nullptr, error)) {
            std::cerr << "Error en línea " << line_number << ": " << error << '\n';
            return 4;
        }
        ++records;
    }

    std::cout << "Registros escritos: " << records << '\n';
    return 0;
}
