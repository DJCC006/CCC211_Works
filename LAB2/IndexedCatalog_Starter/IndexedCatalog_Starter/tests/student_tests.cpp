#include "doctest/doctest.h"

#include "catalog.hpp"
#include "catalog_codec.hpp"
#include <sstream>
// Agregue aquí al menos tres casos de prueba propios.
// No defina DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN en este archivo.

// Ejemplo de estructura (reemplace o elimine este comentario):
// TEST_CASE("Mi prueba - descripción precisa") {
//     CHECK(/* condición */);
// }



TEST_CASE("Prueba para TODO 1: read_record_at"){
    SUBCASE("InvalidOffset: Offset sobrepasa limites archivos"){
        std::stringstream ss("data prueba");
        auto res= lab2::read_record_at(ss, 100);
        CHECK(res.status== lab2::ReadStatus::InvalidOffset);
        CHECK(!res.record.has_value());
    }

    SUBCASE("TruncatedHeader: Header de metadata truncado"){
        std::stringstream ss("MUS2");
        auto res= lab2::read_record_at(ss, 0);
        CHECK(res.status== lab2::ReadStatus::TruncatedHeader);
        CHECK(!res.record.has_value());
    }

    SUBCASE("BadMagic: Error Magic Invalido"){
        std::string bad_h= "MUS2\x01\x00\x00\x00\x00\x00";
        std::stringstream ss(bad_h);
        auto res= lab2::read_record_at(ss, 0);
        CHECK(res.status== lab2::ReadStatus::BadMagic);
        CHECK(!res.record.has_value());
    }

    SUBCASE("UnsupportedVersion: Version != 1"){
        std::string bad_ver= "MUS2\x02\x00\x00\x00\x00\x00";
        std::stringstream ss(bad_ver);
        auto res= lab2::read_record_at(ss, 0);
        CHECK(res.status== lab2::ReadStatus::UnsupportedVersion);
        CHECK(!res.record.has_value());
    }

}


TEST_CASE("Prueba para TODO 2: build_primary_index") {
    SUBCASE("FlujoExitoso: Construye y ordena el indice primario correctamente") {
        // Stream simulado con registros válidos
        std::stringstream ss(
            "LABEL: ALB-002\nTITLE: Album B\n\n"
            "LABEL: ALB-001\nTITLE: Album A\n\n"
        );

        auto result = lab2::build_primary_index(ss);

        CHECK(result.status == lab2::BuildStatus::Ok);
        CHECK(result.entries.size() == 2);
        
        // Verifica que se hayan ordenado por label_id
        CHECK(result.entries[0].label_id == "ALB-001");
        CHECK(result.entries[1].label_id == "ALB-002");
    }

    SUBCASE("ClavesDuplicadas: Retorna DuplicateKey si hay dos registros con el mismo label_id") {
        std::stringstream ss(
            "LABEL: ALB-001\nTITLE: Album A\n\n"
            "LABEL: ALB-001\nTITLE: Album A Repetido\n\n"
        );

        auto result = lab2::build_primary_index(ss);

        CHECK(result.status == lab2::BuildStatus::DuplicateKey);
        CHECK(result.entries.empty());
    }

    SUBCASE("ArchivoVacio: Retorna Ok con lista de entradas vacia") {
        std::stringstream ss("");

        auto result = lab2::build_primary_index(ss);

        CHECK(result.status == lab2::BuildStatus::Ok);
        CHECK(result.entries.empty());
    }

    SUBCASE("ErrorDeLectura: Retorna ReadError si el formato del archivo es invalido") {
        std::stringstream ss("FORMATO_INVALIDO_SIN_ETIQUETAS");

        auto result = lab2::build_primary_index(ss);

        CHECK(result.status == lab2::BuildStatus::ReadError);
        CHECK(result.entries.empty());
    }
}



TEST_CASE("Prueba para TODO 3: find_offset"){
    std::vector<lab2::PrimaryEntry> primary_mock = {
        {"ALB-001",   0},
        { "ALB-002",  128},
        { "ALB-003",  256},
        { "ALB-005",  512}
    };

    SUBCASE("BusquedaExitosa: Encuentra el offset del label_id existente") {
        auto offset_opt = lab2::find_offset(primary_mock, "ALB-002");

       
        REQUIRE(offset_opt.has_value());
        CHECK(offset_opt.value() == 128);
    }

    SUBCASE("BusquedaBordes: Encuentra el primer y ultimo elemento del indice") {
        auto first_opt = lab2::find_offset(primary_mock, "ALB-001");
        REQUIRE(first_opt.has_value());
        CHECK(first_opt.value() == 0);

        auto last_opt = lab2::find_offset(primary_mock, "ALB-005");
        REQUIRE(last_opt.has_value());
        CHECK(last_opt.value() == 512);
    }

    SUBCASE("ClaveNoExistente: Retorna nullopt si la clave no esta en el indice") {
        auto offset_opt = lab2::find_offset(primary_mock, "ALB-004");

        CHECK_FALSE(offset_opt.has_value());
        CHECK(offset_opt == std::nullopt);
    }

    SUBCASE("IndiceVacio: Retorna nullopt si el indice esta vacio") {
        std::vector<lab2::PrimaryEntry> empty_index;
        auto offset_opt = lab2::find_offset(empty_index, "ALB-001");

        CHECK_FALSE(offset_opt.has_value());
        CHECK(offset_opt == std::nullopt);
    }

}




TEST_CASE("Preuba para TODO 4: build_composer_index"){
    SUBCASE("IndicePrimarioVacio: Retorna Ok y lista de entradas vacia"){
        std::stringstream ss("");
        std::vector<lab2::PrimaryEntry> primary_empty;

        auto res = lab2::build_composer_index(ss, primary_empty);

        CHECK(res.entries.empty());
        CHECK(res.skipped.empty());
    }

    SUBCASE("ErrorEnLectura: Falla si un offset del indice primario es invalido"){
        std::stringstream ss ("data corta");
        std::vector<lab2::PrimaryEntry> primary_invalid = {
            {"ALB-001", 500}
        };

        auto res = lab2::build_composer_index(ss, primary_invalid);

        CHECK(res.entries.empty());
        CHECK(res.skipped.size()==1);
        CHECK(res.skipped[0].offset==500);
        CHECK(res.skipped[0].status = lab2::ReadStatus::InvalidOffset);
    }
}



TEST_CASE("Prueba para TODO 5: find_by_composer"){

    lab2::ComposerIndex index = {
        {"Bach",  {"ALB-001", "ALB-005"}},
        {"Beethoven", {"ALB-002"}},
        {"Mozart", {"ALB-003", "ALB-004"}}
    };

    SUBCASE("BusquedaExitosa: Retorna los label_ids si el compositor existe"){
        auto result = lab2::find_by_composer(index, "Beethoven");

        CHECK(result.size()==1);
        CHECK(result[0] == "ALB-002");

    }

    SUBCASE("BusquedaExitosaMultiplesLabels: Retorna todos los label_ids del compositor"){
        auto result = lab2::find_by_composer(index, "Bach");

        CHECK(result.size() == 2);
        CHECK(result[0] == "ALB-001");
        CHECK(result[1] == "ALB-005");

    }

    SUBCASE("CompositorNoExistente: Retorna un span vacio"){
        auto result = lab2::find_by_composer(index, "Chopin");

        CHECK(result.empty());

    }

    SUBCASE("IndiceVacio: Retorna un span vacio si el indice no contiene elementos"){
        lab2::ComposerIndex empty_index;
        auto result = lab2::find_by_composer(empty_index, "Bach");

        CHECK(result.empty());

    }

}



TEST_CASE("Prueba para TODO 6: verify_primary_index"){
    SUBCASE("IndiceVacio: Reporta contadores en cero e indice consistente"){
        std::stringstream ss("");
        std::vector<lab2::PrimaryEntry> empty_index;

        auto report = lab2::verify_primary_index(ss, empty_index);

        CHECK(report.entries_checked==0);
        CHECK(report.readable_matching_entries ==0);
        CHECK(report.consistent());
    }

    SUBCASE("EstructuralDeteccion: Detecta llaves o offsets duplicados"){
        std::stringstream ss;
        std::vector<lab2::PrimaryEntry> duc_index = {
            {"ALB_001", 0},
            {"ALB_00,", 100}
        };
        auto report = lab2::verify_primary_index(ss, duc_index);

        CHECK(report.entries_checked==2);
        CHECK(report.consistent());
    }

    SUBCASE("EstructuralOrden: Detecta indice desordenado"){
        std::stringstream ss;
        std::vector<lab2::PrimaryEntry> unsorted_index = {
            {"ALB-002",  0},
            {"ALB-001",  100} // Desordenado
        };

        auto report = lab2::verify_primary_index(ss, unsorted_index);

        CHECK_FALSE(report.consistent());
    }
}