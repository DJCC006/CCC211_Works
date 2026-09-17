# Starter — Laboratorio 2: catálogo indexado y confiable

Este proyecto contiene la infraestructura y las pruebas visibles del Laboratorio 2 de Estructura de Datos II.

Consulte la especificación completa en:

```text
../Lab2_Catalogo_Indexado_Confiable_EstructuraDeDatosII-Q32026.md
```

## Trabajo del estudiante

Modifique `src/catalog.cpp` y complete:

1. `read_record_at`
2. `build_primary_index`
3. `find_offset`
4. `build_composer_index`
5. `find_by_composer`
6. `verify_primary_index`
7. Opcional: `intersect_sorted`

Puede crear funciones auxiliares privadas dentro de ese archivo. Agregue sus pruebas en `tests/student_tests.cpp` y actualice este README. No modifique las interfaces públicas, `tests/tests.cpp`, `CMakeLists.txt` ni los demás archivos provistos.

## Compilar

```bash
cmake -S . -B build
cmake --build build
```

## Ejecutar pruebas

```bash
ctest --test-dir build --output-on-failure
```

O directamente:

```bash
./build/catalog_tests
```

Para ejecutar un ejercicio específico:

```bash
./build/catalog_tests --test-case="E04*"
```

El starter compila desde el inicio, pero las pruebas fallan hasta completar los TODO.

## Generar datos de ejemplo

```bash
./build/catalog_generate data/catalog.psv data/catalog.bin
```

## Usar la aplicación

```bash
./build/lab2_catalog build data/catalog.bin data/catalog.idx
./build/lab2_catalog find data/catalog.bin data/catalog.idx DG18807
./build/lab2_catalog composer data/catalog.bin data/catalog.idx BEETHOVEN
./build/lab2_catalog verify data/catalog.bin data/catalog.idx
```

## Simular corrupción

```bash
./build/catalog_corrupt flip data/catalog.bin data/catalog.corrupt 20
./build/lab2_catalog verify data/catalog.corrupt data/catalog.idx

./build/catalog_corrupt truncate data/catalog.bin data/catalog.truncated 3
./build/lab2_catalog verify data/catalog.truncated data/catalog.idx
```

## Archivos que ya están completos

- `src/binary_io.cpp`: I/O little-endian y utilidades de stream.
- `src/crc32.cpp`: CRC-32/ISO-HDLC.
- `src/catalog_codec.cpp`: codificación y decodificación del payload.
- `src/index_io.cpp`: persistencia del índice primario.
- `src/main.cpp`: interfaz de línea de comandos.
- `tools/`: generación y corrupción controlada de datos.

## Antes de entregar

Actualice este README con:

- Nombre del estudiante.
- Complejidad de las operaciones principales.
- Diferencia entre integridad física y consistencia lógica.
- Descripción de las pruebas adicionales realizadas en `tests/student_tests.cpp`.

No entregue `build/`, ejecutables ni archivos generados `.bin`, `.idx` o `.corrupt`.
