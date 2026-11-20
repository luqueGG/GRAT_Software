# Suite de pruebas del núcleo (P3)

Suite Qt Test versionada en [`tests/core/`](../../tests/core/), registrada en CTest y ejecutable desde un checkout limpio. Cubre validación, permisos, autenticación, controladores y persistencia sin abrir ni modificar la base de desarrollo `GRAT_Software/db/grat.db`.

## Tipos de prueba

| Tipo | Qué significa aquí | Dependencias |
|---|---|---|
| **Unitaria** | Ejercita una clase a través de su contrato público, con dobles en memoria en lugar de SQLite. | `FakeValidationLookup` (sustituye a `IValidationLookup`, P2) y `FakeRecordRepository` (sustituye a `IRecordRepository`, P1). |
| **Integración** | Usa el código productivo contra un archivo SQLite temporal creado con el esquema y el seed reales (`resources/grat_schema.sqlite.sql`). | `TestDatabase` en `QTemporaryDir`; `PRAGMA foreign_keys = ON`. |
| **Aceptación manual** | No forma parte de esta suite. La recorrida por la interfaz con los tres roles la registra P5 sobre la versión integrada. | — |

## Ejecutables y casos

Los totales de Qt Test incluyen `initTestCase` y `cleanupTestCase` (dos por ejecutable) y cuentan cada fila de una prueba con datos.

| Ejecutable | Tipo | Qué verifica | Totales |
|---|---|---|---|
| [`test_core_fixtures`](../../tests/core/test_core_fixtures.cpp) | Integración | La base temporal queda fuera de `db/`, carga el esquema y el seed reales (3 roles, 43 permisos; 43/36/19 por rol), activa las claves foráneas, crea los usuarios y datos ficticios, y cada fixture empieza vacío. | 7 |
| [`test_validator`](../../tests/core/test_validator.cpp) | Unitaria + integración | Obligatorios (todos reportados juntos), longitud exacta y excedida, decimal cero y negativo, formato de correo (6 casos), errores simultáneos de distinto tipo, edición parcial, contraseña opcional al editar, campos de sistema omitidos. Unicidad con el doble: clave natural duplicada, solo en altas, tablas con identidad sin consulta, fallo de consulta no tratado como “único”, pareja residuo–constituyente duplicada, el mismo constituyente admitido en otro residuo, exclusión del propio ID al editar y uso de los valores originales en edición parcial. Strategy: una regla inyectada se ejecuta a través de `IValidationRule` y un conjunto vacío no aplica reglas. Integración: el lookup real detecta la clave existente y la pareja en estado `*`. | 30 |
| [`test_permissions`](../../tests/core/test_permissions.cpp) | Unitaria + integración | `PermissionChecker`: mapeo de acciones a codenames, permisos presentes y ausentes, `assign_role_user` y administración de roles con Administrator, superusuario, Supervisor, Operator y sesión inválida. `AuthService` sobre el seed: 43/36/19 permisos y su rol, matriz del operador, contraseña incorrecta sin revelar detalles internos, usuario inexistente, usuario `I` o `*` sin acceso, actualización de `last_login`, unión con permisos directos del usuario sin duplicados y revocación aplicada en el siguiente login. | 22 |
| [`test_controllers`](../../tests/core/test_controllers.cpp) | Unitaria | `CrudController`: leer exige `view_`; sin permiso no se escribe, aunque se llame directamente a `save`/`changeState`; sin operación pendiente no se escribe; la validación fallida no escribe y conserva la operación; alta con `created_by_id` de la sesión e ignorando estado y fechas enviados; modificación sin cambiar la clave y con `updated_at`; hash de contraseña y “contraseña vacía conserva la actual”; propagación del error de persistencia; cancelación; reglas de estado. `ChildCrudController`: sin padre no lee ni escribe; escrituras ligadas al padre seleccionado aunque el formulario envíe otro; filas de otro padre rechazadas; baja lógica en constituyentes y física en tramos; roles de usuario exigen `assign_role_user`; permisos de rol solo para administración, después de leer y como otorgar/revocar. | 20 |
| [`test_gateway`](../../tests/core/test_gateway.cpp) | Integración | `TableGateway`: alta, lectura y cambio; lectura de todos los estados; transiciones `A`/`I`/`*`; `update` no cambia clave ni estado; registro inexistente; clave duplicada con rollback y mensaje sin detalles SQL; violación de FK; `CHECK` de cantidad negativa; las tablas gestionadas no se borran físicamente. Hijos: aislamiento por padre, imposibilidad de reasignar un hijo al editar, escritura con padre equivocado sin efecto, baja lógica y física, duplicado con rollback, cascada directa y transitiva. Permisos de rol solo por administración y catálogo de 43 permisos intacto. `SqlRecordRepository`: colección vacía distinta de error, colecciones hijas por padre, operaciones no permitidas rechazadas. | 22 |

`test_reportes` pertenece a P4 y se ejecuta en la misma corrida de CTest.

## Aislamiento

- Cada caso crea su propio `TestDatabase` (o lo crea en `init()` y lo destruye en `cleanup()`), así que no depende del orden de ejecución.
- La base se crea en un directorio temporal del sistema y se elimina al terminar. `test_core_fixtures` comprueba que la ruta no está bajo `GRAT_Software/db/`.
- Se usa el esquema real, no uno simplificado, para que las claves foráneas, `UNIQUE`, `CHECK` y las cascadas sean las de producción.
- CTest ejecuta las suites con `QT_QPA_PLATFORM=offscreen` y con su propio directorio de trabajo dentro del build.

## Compilación

[`tests/core/CMakeLists.txt`](../../tests/core/CMakeLists.txt) usa el target `grat_core` de P6 cuando exista. Mientras tanto compila una biblioteca de objetos `grat_test_core` con las mismas fuentes no visuales, así que no hace falta modificar el `CMakeLists.txt` principal. Cuando se integre `grat_core`, la suite lo toma automáticamente.

## Comandos y resultado obtenido

Ejecutados desde `C:\Users\<usuario>\GRAT`, con el repositorio clonado en `GRAT_Software/` y el build fuera del repositorio:

```sh
cmake -S GRAT_Software/GRAT_Software -B build-p3-clean -G Ninja -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64
cmake --build build-p3-clean
ctest --test-dir build-p3-clean -N
ctest --test-dir build-p3-clean --output-on-failure
```

| Dato | Valor |
|---|---|
| Commit probado | `552663c` (rama `test/rubrica-core`, sobre `main` en `feb53db`) |
| Entorno | Windows 11, Qt 6.11.2 MinGW 64-bit, GCC 13.1.0 (MinGW-Builds), CMake 3.30.5, Ninja |
| Build | Build limpio sin errores, 0 advertencias en el log de compilación |
| `ctest -N` | 6 pruebas: `test_core_fixtures`, `test_validator`, `test_permissions`, `test_controllers`, `test_gateway`, `test_reportes` |
| `ctest` | `100% tests passed, 0 tests failed out of 6` (etiqueta `core`: 5 pruebas) |
| Base de desarrollo | No se creó `GRAT_Software/db/` durante la ejecución |

En Linux con CI se espera el mismo resultado, pero aún no se ha verificado: el workflow corresponde a P6.

## Limitaciones y pendientes

- La suite no prueba widgets ni la apariencia. Las pantallas se cubren con la aceptación manual de P5.
- La recuperación de la conexión (`Database::reconnect`) se cubre solo en parte: se verifica que un repositorio sobre una conexión cerrada informe el error sin detalles internos.
- Si P6 cambia el nombre del target o la forma de enlazar los recursos Qt, hay que ajustar `tests/core/CMakeLists.txt` (archivo de P3).
- No se encontraron defectos en el código productivo durante esta entrega.
