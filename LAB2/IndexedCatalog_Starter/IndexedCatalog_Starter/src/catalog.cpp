#include "catalog.hpp"

#include "binary_io.hpp"
#include "catalog_codec.hpp"
#include "crc32.hpp"

#include <algorithm>
#include <utility>
#include <span>
#include <cstddef>

namespace lab2 {

const char* to_string(ReadStatus status) {
    switch (status) {
    case ReadStatus::Ok: return "Ok";
    case ReadStatus::NotFound: return "NotFound";
    case ReadStatus::IndexKeyMismatch: return "IndexKeyMismatch";
    case ReadStatus::InvalidOffset: return "InvalidOffset";
    case ReadStatus::TruncatedHeader: return "TruncatedHeader";
    case ReadStatus::BadMagic: return "BadMagic";
    case ReadStatus::UnsupportedVersion: return "UnsupportedVersion";
    case ReadStatus::InvalidLength: return "InvalidLength";
    case ReadStatus::TruncatedPayload: return "TruncatedPayload";
    case ReadStatus::MissingChecksum: return "MissingChecksum";
    case ReadStatus::ChecksumMismatch: return "ChecksumMismatch";
    case ReadStatus::MalformedPayload: return "MalformedPayload";
    }
    return "UnknownReadStatus";
}

const char* to_string(BuildStatus status) {
    switch (status) {
    case BuildStatus::Ok: return "Ok";
    case BuildStatus::ReadError: return "ReadError";
    case BuildStatus::DuplicateKey: return "DuplicateKey";
    }
    return "UnknownBuildStatus";
}

const char* to_string(VerificationIssueType type) {
    switch (type) {
    case VerificationIssueType::UnsortedIndex: return "UnsortedIndex";
    case VerificationIssueType::DuplicateKey: return "DuplicateKey";
    case VerificationIssueType::DuplicateOffset: return "DuplicateOffset";
    case VerificationIssueType::RecordReadError: return "RecordReadError";
    case VerificationIssueType::KeyMismatch: return "KeyMismatch";
    }
    return "UnknownVerificationIssue";
}


//Funcion 1 auxiliar agregada
std::uint64_t file_size(std::istream& input){
    const auto current = input.tellg();
    if(current==-1){
        return 0;
    }

    //se mueve el puntero al final para obtener su tamanio
    input.seekg(0,std::ios::end);
    const auto end_pos = input.tellg();

    //se vuelve a recomodar el puntero
    input.seekg(current);

    return static_cast<std::uint64_t>(end_pos);
}


ReadResult read_record_at(std::istream& input, std::uint64_t offset) {
    // TODO 1
    // Orden obligatorio:
    // 1) comprobar tamaño/offset y hacer seek;
    // 2) leer magic, version y payload_length;
    // 3) validar el header ANTES de reservar memoria;
    // 4) leer payload y CRC almacenado;
    // 5) recalcular CRC;
    // 6) decodificar el payload solamente si el CRC coincide.

    //Creacion de variables a manejar internamente
    ReadResult result;
    result.offset= offset;

    //Validamos que estamos dentro de los limites del archivo
    const std::uint64_t archivo_size = file_size(input);
    if(offset >= archivo_size){
        //Nos salimos del archivo
        result.status = ReadStatus::InvalidOffset;
        result.detail = "Offset "+std::to_string(offset) + "Esta fuera de los limites del archivo";
        return result;
    }

    //Recomodar el offset del archivo al offset buscado
    input.seekg(static_cast<std::streamoff>(offset));
    if(!input){
        //Deteccion de error al ubicar offset
        result.status= ReadStatus::InvalidOffset;
        result.detail = "Error para ubicar offset"+std::to_string(offset);
        return result;
    }


    //Lectura de header completo - los 10 bytes
    char magicb[4];
    std::uint16_t version =0;
    std::uint32_t length =0;

    //Se realiza cada extraccion con verificacion
    //Lectura de magic
    if(!input.read(magicb, 4)){
        //Error en leer magic -> header truncado
        result.status= ReadStatus::TruncatedHeader;
        result.detail = "Header Truncado: error al leer el magic";
        return result;
    }

    //Lectura de version
    if(!read_u16_le(input, version)){
        //Error de leer version ->header truncado
        result.status = ReadStatus::TruncatedHeader;
        result.detail = "Header Truncado: Error al leer version";
        return result;
    }

    //lectura de length
    if(!read_u32_le(input, length)){
        result.status = ReadStatus::TruncatedHeader;
        result.detail = "Error al leer la longitud del payload";
        return result;
    }

    //Validaciones de valores de cabezera
    std::string_view magic_sv(magicb,4);
    if(magic_sv != RECORD_MAGIC){
        //Magic erroneo -> BadMagic
        result.status= ReadStatus::BadMagic;
        result.detail= "Magic Invalido";
        return result;
    }

    //validar version
    if(version != RECORD_VERSION){
        //Error en version -> UnsupportedVersion
        result.status = ReadStatus::UnsupportedVersion;
        result.detail = "Version No Soportada: "+std::to_string(version);
        return result;
    }

    //Validar length
    if(length == 0 || length > MAX_PAYLOAD_SIZE){
        //mal length -> invalidLength
        result.status = ReadStatus::InvalidLength;
        result.detail = "Valor de Length invalido";
        return result;
    }

    //Lectura de payload
    std::vector<std::uint8_t> payload(length);
    if(!input.read(reinterpret_cast<char*>(payload.data()),length)){
        result.status= ReadStatus::TruncatedPayload;
        result.detail = "eRor en cargo de payload.";
        return result;
    }

    //Lectura de crc
    std::uint32_t stored_crc=0;
    if(!read_u32_le(input, stored_crc)){
        result.status = ReadStatus::MissingChecksum;
        result.detail= "CRC faltante o truncado";
        return result;
    }

    //Verificar el crc
    //Se hace la conversion de uint8 a bytes
    const std::uint32_t crc_creado = lab2::crc32(
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(payload.data()),payload.size())); //ver si esto aca se buguea
    
    if(crc_creado!=stored_crc){
        result.status = ReadStatus::ChecksumMismatch;
        result.detail = "CRC32 mismatch: calculated " + std::to_string(crc_creado) +
                        ", stored " + std::to_string(stored_crc);
        return result;
    }



    //otra funcion auxiliar
    std::optional<Record> decoded_record = lab2::decode_payload(payload);
    if (!decoded_record.has_value() ||
        decoded_record->label_id.empty() ||
        decoded_record->composer.empty() ||
        decoded_record->title.empty()) {
        result.status = ReadStatus::MalformedPayload;
        result.detail = "El payload esta malformado";
        return result;
    }


    //Retornado de valor en caso que si sea un valor valido
    result.status= ReadStatus::Ok;
    result.record= std::move(decoded_record);
    result.next_offset = offset +10 +length+4;
    result.detail = "Se leyo el registro exitosamente";

    return result;
}

PrimaryBuildResult build_primary_index(std::istream& input) {
    // TODO 2
    // Recorra el archivo con next_offset, ordene por label_id y detecte duplicados.
    (void)input;
    return {BuildStatus::ReadError, {}, 0, {},
            "TODO: implementar build_primary_index"};
}

std::optional<std::uint64_t> find_offset(
    std::span<const PrimaryEntry> index,
    std::string_view label_id) {
    // TODO 3
    // Implemente búsqueda binaria manual. No use std::lower_bound,
    // std::binary_search ni std::equal_range.
    (void)index;
    (void)label_id;
    return std::nullopt;
}

ReadResult find_record(
    std::istream& input,
    std::span<const PrimaryEntry> index,
    std::string_view label_id) {
    const auto offset = find_offset(index, label_id);
    if (!offset.has_value()) {
        return {ReadStatus::NotFound, std::nullopt, 0, 0,
                "La clave no existe en el índice primario."};
    }
    ReadResult result = read_record_at(input, *offset);
    if (result.ok() && result.record->label_id != label_id) {
        result.status = ReadStatus::IndexKeyMismatch;
        result.record.reset();
        result.detail = "La clave del índice no coincide con la clave del registro.";
    }
    return result;
}

ComposerBuildResult build_composer_index(
    std::istream& input,
    std::span<const PrimaryEntry> primary) {
    // TODO 4
    // Recomendación: reúna pares (composer, label_id), ordénelos y agrúpelos.
    // No almacene offsets en este índice secundario.
    (void)input;
    (void)primary;
    return {};
}

std::span<const std::string> find_by_composer(
    const ComposerIndex& index,
    std::string_view composer) {
    // TODO 5
    // El ComposerIndex está ordenado por compositor: use búsqueda binaria.
    (void)index;
    (void)composer;
    return {};
}

VerificationReport verify_primary_index(
    std::istream& input,
    std::span<const PrimaryEntry> index) {
    // TODO 6
    // Haga primero las verificaciones estructurales del índice y luego valide
    // cada referencia con read_record_at. No imprima desde esta función.
    (void)input;
    (void)index;
    return {};
}

std::vector<std::string> intersect_sorted(
    std::span<const std::string> left,
    std::span<const std::string> right) {
    // TODO BONO: dos punteros, O(n + m), sin duplicados.
    (void)left;
    (void)right;
    return {};
}

} // namespace lab2
