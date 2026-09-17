#include "catalog.hpp"

#include "binary_io.hpp"
#include "catalog_codec.hpp"
#include "crc32.hpp"

#include <algorithm>
#include <utility>
#include <span>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>

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
    std::vector<std::byte> payload(length);
    if(!input.read(reinterpret_cast<char*>(payload.data()),length)){
        result.status= ReadStatus::TruncatedPayload;
        result.detail = "ErRor en cargo de payload.";
        return result;
    }

    //Lectura de crc
    std::uint32_t stored_crc=0;
    if(!read_u32_le(input, stored_crc)){
        result.status = ReadStatus::MissingChecksum;
        result.detail= "CRC faltante o truncado";
        return result;
    }

    const std::uint32_t crc_creado = lab2::crc32(payload);

    if(crc_creado!=stored_crc){
        result.status = ReadStatus::ChecksumMismatch;
        result.detail = "CRC32 mismatch: calculated " + std::to_string(crc_creado) +
                        ", stored " + std::to_string(stored_crc);
        return result;
    }



    //otra funcion auxiliar
    PayloadDecodeResult decoded_record = lab2::decode_payload(payload);
    if(!decoded_record.record.has_value()){
        result.status = ReadStatus::MalformedPayload;
        result.detail= decoded_record.detail;
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

    PrimaryBuildResult result;
    std::uint64_t current_offset=0;

    //Construccion de lista 0de indices
    while(true){
        //Se lee el registro en la posicion actual
        ReadResult read_res = read_record_at(input, current_offset);

        //Se maneja el tope del archivo de forma limpia
        if(read_res.status == ReadStatus::InvalidOffset){
            break;
        }

        //parar al encontrar fallo en lectura
        if(read_res.status != ReadStatus::Ok){
            result.status = BuildStatus::ReadError;
            result.error_offset=current_offset;
            result.detail=read_res.detail;
            result.entries.clear();
            return result;
        }

        //Se agrega el registro al conjunto de indices primarios
        result.entries.push_back(PrimaryEntry{
            .label_id = read_res.record->label_id,
            .offset = current_offset
        });

        //Se avanza al siguiente record
        current_offset = read_res.next_offset;
    }

    //Ordenar la lista de indices alfabeticamente
    std::sort(result.entries.begin(), result.entries.end(), 
        [](const PrimaryEntry& a, const PrimaryEntry& b){
            return a.label_id < b.label_id;
        }
    );

    //detectar claves duplicadas
    for(std::size_t i =1; i <result.entries.size(); i++){
        if(result.entries[i].label_id == result.entries[i+1].label_id){
            result.status =  BuildStatus::DuplicateKey;
            result.error_key = result.entries[i].label_id;
            result.error_offset =  result.entries[i].offset;
            result.detail = "Clave duplicada encontrada: "+result.entries[i].label_id;
            result.entries.clear();
            return result;
        }
    }

    //Caso de Exito
    result.status = BuildStatus::Ok;
    result.detail = "Indice primario construido exitosamente";
    return result;
}

std::optional<std::uint64_t> find_offset(
    std::span<const PrimaryEntry> index,
    std::string_view label_id) {
    // TODO 3
    // Implemente búsqueda binaria manual. No use std::lower_bound,
    // std::binary_search ni std::equal_range.

    //Verificar que el indice no sea vacio
    if(index.empty()){
        return std::nullopt;
    }

    size_t left = 0;
    size_t right = index.size();

    while(left< right){

        //obtener el punto medio
        size_t mid =left + (right-left)/2;

        if(index[mid].label_id == label_id){
            return index[mid].offset;
        }

        //acortar dependiendo de los ids.
        if(index[mid].label_id < label_id){
            left = mid +1;
        }else{
            right =  mid;
        }
    }

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

    
    ComposerBuildResult result;
    std::uint64_t current_offset =0;

    //Tabla hash temporal para agrupar los indices
    std::unordered_map<std::string, std::vector<std::string>> grouped_map;


    //recorrir cada entrada del indice primario recibido
    for(const auto& entry : primary){
        //leemos el registro usando el indice del registro primary 
        ReadResult read_res = read_record_at(input, entry.offset);

        //abortar el proceso si se encuentra error
        if(read_res.status != ReadStatus::Ok){
            result.skipped.push_back(SkippedRecord{
                .offset = entry.offset,
                .status = read_res.status,
            });
            continue;
        }

        //agrupar el label bajo su respectivo compositor
        grouped_map[read_res.record->composer].push_back(read_res.record->label_id);
    }

    //transferir el mapa al vector
    for(auto& [composer_name, labels] : grouped_map){
        std::sort(labels.begin(), labels.end());

        result.entries.push_back(ComposerEntry{
            .composer = composer_name,
            .label_ids = std::move(labels)
        });
    }

    //ordenrar los indices secundarios
    std::sort(result.entries.begin(), result.entries.end(),
        [](const ComposerEntry& a, const ComposerEntry& b){
            return a.composer < b.composer;
        }
    );

    //Modificacion
    //TERMINAR DE VER CONFIGURACION MANUAL
    return result;
}

std::span<const std::string> find_by_composer(
    const ComposerIndex& index,
    std::string_view composer) {
    // TODO 5
    // El ComposerIndex está ordenado por compositor: use búsqueda binaria.

    if(index.empty()){
        return {};
    }

    std::size_t left =0;
    std::size_t right = index.size();

    while(left<right){
        std::size_t mid = left + (right-left)/2;

        if(index[mid].composer == composer){
            return std::span<const std::string>(index[mid].label_ids);
        }

        if(index[mid].composer<composer){
            left = mid+1;
        }else{
            right=mid;
        }
    }

    return {};

}

VerificationReport verify_primary_index(
    std::istream& input,
    std::span<const PrimaryEntry> index) {
    // TODO 6
    // Haga primero las verificaciones estructurales del índice y luego valide
    // cada referencia con read_record_at. No imprima desde esta función.

    VerificationReport report;
    report.entries_checked = index.size();

    std::unordered_set<std::string_view> seen_keys;
    std::unordered_set<std::uint64_t> seen_offsets;
    bool is_unsorted_reported = false;

    //verificaciones estructurales
    for(std::size_t i=0; i<index.size(); i++){
        const auto& entry = index[i];

        //verificacion de orden
        if(i>0 && entry.label_id < index[i-1].label_id){
            if(!is_unsorted_reported){
                report.issues.push_back(VerificationIssue{
                    .type = VerificationIssueType::UnsortedIndex
                });
                is_unsorted_reported=true;
            }
        }

        //verificacion de llaves duplicadas
        if(seen_keys.contains(entry.label_id)){
            report.issues.push_back(VerificationIssue{
                .type= VerificationIssueType::DuplicateKey,
            });
        }else{
            seen_keys.insert(entry.label_id);
        }

        //verificacion de offsets duplicados
        if(seen_offsets.contains(entry.offset)){
            report.issues.push_back(VerificationIssue{
                .type= VerificationIssueType::DuplicateOffset,
                .offset= entry.offset
            });
        }else{
            seen_offsets.insert(entry.offset);
        }
    }

    //Verificaciones contra archivos de datos
    for(const auto& entry: index){
        ReadResult read_res = read_record_at(input, entry.offset);

        if(read_res.status!= ReadStatus::Ok){
            report.issues.push_back(VerificationIssue{
                .type= VerificationIssueType::RecordReadError,
                .offset=entry.offset,
                .read_status=read_res.status
            });
        }else if(read_res.record->label_id != entry.label_id){
            report.issues.push_back(VerificationIssue{
                .type = VerificationIssueType::KeyMismatch,
                .offset = entry.offset
            });
        }else{
            report.readable_matching_entries++;
        }
    }

    return report;
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
