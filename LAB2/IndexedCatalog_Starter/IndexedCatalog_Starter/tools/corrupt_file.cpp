#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <system_error>

namespace {

bool parse_u64(std::string_view text, std::uint64_t& value) {
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

void usage(const char* program) {
    std::cerr
        << "Uso:\n"
        << "  " << program << " flip INPUT OUTPUT OFFSET\n"
        << "  " << program << " truncate INPUT OUTPUT BYTES_TO_REMOVE\n";
}

bool copy_input(const std::filesystem::path& input,
                const std::filesystem::path& output) {
    std::error_code error;
    std::filesystem::copy_file(
        input, output, std::filesystem::copy_options::overwrite_existing, error);
    if (error) {
        std::cerr << "No se pudo copiar el archivo: " << error.message() << '\n';
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 5) {
        usage(argv[0]);
        return 64;
    }

    const std::string_view command = argv[1];
    const std::filesystem::path input = argv[2];
    const std::filesystem::path output = argv[3];
    std::uint64_t amount = 0;
    if (!parse_u64(argv[4], amount)) {
        std::cerr << "El valor numérico es inválido.\n";
        return 2;
    }
    if (!copy_input(input, output)) {
        return 2;
    }

    std::error_code error;
    const std::uint64_t size = std::filesystem::file_size(output, error);
    if (error) {
        std::cerr << "No se pudo obtener el tamaño: " << error.message() << '\n';
        return 2;
    }

    if (command == "flip") {
        if (amount >= size) {
            std::cerr << "OFFSET está fuera del archivo.\n";
            return 3;
        }
        std::fstream file(output, std::ios::binary | std::ios::in | std::ios::out);
        file.seekg(static_cast<std::streamoff>(amount));
        char byte = 0;
        file.read(&byte, 1);
        byte = static_cast<char>(static_cast<unsigned char>(byte) ^ 0x01U);
        file.seekp(static_cast<std::streamoff>(amount));
        file.write(&byte, 1);
        if (!file) {
            std::cerr << "Falló la modificación del byte.\n";
            return 4;
        }
        std::cout << "Se invirtió el bit menos significativo del byte "
                  << amount << ".\n";
        return 0;
    }

    if (command == "truncate") {
        if (amount > size) {
            std::cerr << "No se pueden eliminar más bytes que el tamaño del archivo.\n";
            return 3;
        }
        std::filesystem::resize_file(output, size - amount, error);
        if (error) {
            std::cerr << "No se pudo truncar: " << error.message() << '\n';
            return 4;
        }
        std::cout << "Se eliminaron " << amount << " bytes del final.\n";
        return 0;
    }

    usage(argv[0]);
    return 64;
}
