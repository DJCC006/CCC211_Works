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
