#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

int main() {
    // Definimos manualmente la secuencia exacta de bytes que queremos probar
    std::vector<uint8_t> test_bytes = {
        // 1. BOM UTF-8 (3 bytes)
        0xEF, 0xBB, 0xBF,

        // 2. Texto ASCII Imprimible ("A" -> 0x41)
        0x41,

        // 3. Secuencia Válida 2 Bytes ("ñ" -> U+00F1: 0xC3 0xB1)
        0xC3, 0xB1,

        // 4. Secuencia Válida 3 Bytes ("€" -> U+20AC: 0xE2 0x82 0xAC)
        0xE2, 0x82, 0xAC,

        // 5. Secuencia Válida 4 Bytes (Emoji "😀" -> U+1F600: 0xF0 0x9F 0x98 0x80)
        0xF0, 0x9F, 0x98, 0x80,

        // 6. ERROR: Byte Huérfano / Continuación inesperado
        0x80,

        // 7. ERROR: Byte Líder Inválido (Rango 0xF8 - 0xFF)
        0xFF,

        // 8. ERROR: Byte de continuación no encontrado (Secuencia de 2 bytes corrupta)
        0xC3, 0x20, // 0x20 es un espacio, no un byte de continuación (0x80..0xBF)

        // 9. ERROR: Secuencia Incompleta al final del archivo (Falta el 2do byte)
        0xC3
    };

    std::ofstream file("archivo_prueba.bin", std::ios::binary);
    if (!file) {
        std::cerr << "Error al crear el archivo de prueba." << std::endl;
        return 1;
    }

    file.write(reinterpret_cast<const char*>(test_bytes.data()), test_bytes.size());
    file.close();

    std::cout << "Archivo 'archivo_prueba.bin' generado exitosamente con " 
              << test_bytes.size() << " bytes." << std::endl;

    return 0;
}