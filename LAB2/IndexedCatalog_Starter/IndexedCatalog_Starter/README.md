# Starter — Laboratorio 2: catálogo indexado y confiable

Este proyecto contiene la infraestructura y las pruebas visibles del Laboratorio 2 de Estructura de Datos II.

Consulte la especificación completa en:

```text
../Lab2_Catalogo_Indexado_Confiable_EstructuraDeDatosII-Q32026.md
```



## Datos de Estudiante e informacion Pedida
* **Nombre:** David Joseph Carcamo Cedillo

| Operación | Complejidad Temporal | Complejidad Espacial |
| :--- | :--- | :--- |
| `read_record_at` | $\mathcal{O}(1)$ | $\mathcal{O}(1)$ |
| `build_primary_index` | $\mathcal{O}(n \log n)$ | $\mathcal{O}(n)$ | 
| `find_offset` | $\mathcal{O}(\log n)$ | $\mathcal{O}(1)$ | 
| `build_composer_index` | $\mathcal{O}(n \log n)$ | $\mathcal{O}(n)$ |
| `find_by_composer` | $\mathcal{O}(\log k)$ | $\mathcal{O}(1)$ |
| `verify_primary_index` | $\mathcal{O}(n)$ | $\mathcal{O}(n)$ | 

* **Integridad Física vs. Consistencia Lógica**
* **Integridad Física:** Garantiza que los bytes almacenados en el medio no se hayan corrompido, truncado o alterado durante el almacenamiento o transmisión. 
* **Consistencia Lógica:** Garantiza que la estructura de los datos mantenga la validez relacional y organizativa requerida por la aplicación.


* **Descripcion de Pruebas Adicionales**

* **TODO 1: `read_record_at`**
  * `InvalidOffset`: Valida que se retorne el estado de error cuando el *offset* especificado sobrepasa el tamaño total del archivo o *stream*.
  * `TruncatedHeader`: Verifica la detección de un encabezado incompleto (menos bytes de los requeridos por la metadata).
  * `BadMagic`: Valida el fallo cuando los primeros bytes del registro no coinciden con la firma esperada (*magic bytes* `MUS2`).
  * `UnsupportedVersion`: Comprueba que se rechacen registros con versiones de formato distintas a la versión soportada (`version != 1`).

* **TODO 2: `build_primary_index`**
  * `FlujoExitoso`: Confirma que el índice primario se construya a partir del *stream* y las entradas queden correctamente ordenadas alfabéticamente por `label_id`.
  * `ClavesDuplicadas`: Verifica que si existen dos registros con la misma clave (`label_id`), se reporte `BuildStatus::DuplicateKey` y se limpie la lista de entradas.
  * `ArchivoVacio`: Garantiza que un *stream* sin contenido retorne un estado `Ok` con un vector de entradas vacío.
  * `ErrorDeLectura`: Valida que ante un formato de datos inválido se retorne `BuildStatus::ReadError`.

* **TODO 3: `find_offset`**
  * `BusquedaExitosa`: Evalúa la localización precisa del *offset* para una clave intermedia existente en el índice.
  * `BusquedaBordes`: Verifica el funcionamiento en los extremos del índice (primer y último elemento).
  * `ClaveNoExistente`: Comprueba que la búsqueda retorne `std::nullopt` cuando la clave no existe dentro del índice primario.
  * `IndiceVacio`: Valida que la búsqueda sobre un índice primario sin entradas retorne `std::nullopt` sin desbordamientos.

* **TODO 4: `build_composer_index`**
  * `IndicePrimarioVacio`: Valida que con un índice primario vacío se retorne una estructura de índice de compositores y una lista de omisiones (*skipped*) vacías.
  * `ErrorEnLectura`: Verifica que si un *offset* apuntado por el índice primario es inválido en el *stream*, se registre la falla dentro de la lista de omisiones con su respectivo `ReadStatus::InvalidOffset`.

* **TODO 5: `find_by_composer`**
  * `BusquedaExitosa`: Retorna el listado de `label_ids` correspondiente a un compositor con una única entrada.
  * `BusquedaExitosaMultiplesLabels`: Confirma que se devuelvan todos los `label_ids` asociados a un compositor con múltiples álbumes.
  * `CompositorNoExistente`: Garantiza el retorno de un `std::span` vacío cuando el compositor consultado no figura en el índice.
  * `IndiceVacio`: Verifica que la consulta sobre un índice secundario vacío retorne un `std::span` sin elementos.

* **TODO 6: `verify_primary_index`**
  * `IndiceVacio`: Confirma que la verificación de un índice primario vacío reporte cero entradas revisadas, cero coincidencias y un estado totalmente consistente.
  * `EstructuralDeteccion`: Evalúa el reporte de inconsistencias cuando se detectan claves o *offsets* duplicados.
  * `EstructuralOrden`: Valida que un índice primario cuyas claves no respeten el orden alfabético estricto sea marcado como inconsistente (`CHECK_FALSE(report.consistent())`).






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



