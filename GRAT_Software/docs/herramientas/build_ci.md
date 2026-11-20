# Build reproducible, CI y herramientas (P6)

Responsable: **Jhon Cuyo** — rama `build/rubrica-ci-reproducible`.

## Propósito

Establecer una configuración de CMake reproducible para GRAT_Software, separando el
código reutilizable (`grat_core`) del ejecutable con interfaz (`grat_software`),
habilitando CTest para las suites de P3/P4 y registrando un workflow de GitHub
Actions que compile y ejecute pruebas automáticamente.

## Estructura de targets

| Target | Tipo | Contenido | Módulos Qt |
|---|---|---|---|
| `grat_core` | `STATIC` | Código no visual: acceso a datos, validación, autenticación, reportes | `Qt6::Core`, `Qt6::Sql` |
| `grat_software` | `EXECUTABLE` | Fuentes visuales, `main.cpp` y recursos `.qrc` | `Qt6::Widgets`, `Qt6::Sql`, `Qt6::Svg` (más `grat_core`) |
| `test_reportes` | `EXECUTABLE` | Prueba de P4 (registrada cuando `BUILD_TESTING=ON`) | `Qt6::Test` |

Convención acordada con el equipo:

- Fuentes sin interfaz → `grat_core`.
- Fuentes visuales → `grat_software`.
- Las extensiones en `cmake/features/{arquitectura,validacion,reportes}.cmake` se
  incluyen de forma opcional y pueden añadir fuentes a cualquiera de los dos
  targets sin modificar el CMake principal.

## Comandos

Desde la raíz del repositorio:

```bash
# Solo la aplicación
cmake -S GRAT_Software -B build-rubrica-app -G Ninja -DBUILD_TESTING=OFF
cmake --build build-rubrica-app

# Aplicación + pruebas
cmake -S GRAT_Software -B build-rubrica -G Ninja -DBUILD_TESTING=ON
cmake --build build-rubrica
ctest --test-dir build-rubrica -N
ctest --test-dir build-rubrica --output-on-failure
```

## Versiones de herramientas verificadas

| Herramienta | Versión |
|---|---|
| CMake | 4.2.3 |
| Ninja | 1.13.2 |
| g++ | 15.2.0 (Ubuntu 15.2.0-16ubuntu1) |
| Qt6 | 6.10.2 |
| Paquete Qt Svg dev | `qt6-svg-dev` 6.10.2-2 |
| Paquetes GL/VK/XKB | `libgl1-mesa-dev`, `libvulkan-dev`, `libxkbcommon-dev` |

## Commit probado

```text
7f0d37610798f466fbd884d0fd9517bfb4796dff
```

Mensaje: `Automatizar compilacion y pruebas con GitHub Actions`.

## Resultados reales

### Configuración

```text
-- The CXX compiler identification is GNU 15.2.0
-- Found Threads: TRUE
-- Found WrapAtomic: TRUE
-- Found OpenGL: /usr/lib/x86_64-linux-gnu/libOpenGL.so
-- Found WrapOpenGL: TRUE
-- Found WrapVulkanHeaders: /usr/include
-- Configuring done (28.4s)
-- Generating done (0.0s)
-- Build files have been written to: /home/jhon/build-grat-warnings
```

### Build

```text
[42/42] Linking CXX executable grat_software
```

### `ctest -N`

```text
Test project /home/jhon/build-grat-warnings
  Test #1: test_reportes

Total Tests: 1
```

### `ctest --output-on-failure`

```text
100% tests passed, 0 tests failed out of 1
```

## Advertencias

| Escenario | `-Wall -Wextra` (GCC) | Total |
|---|---|---|
| Línea base (Paso 0, sin flags) | — | 0 |
| Con flags activados en `grat_core` y `grat_software` | `-Wall -Wextra` | 0 |

No se detectaron advertencias nuevas tras activar los flags. No se modificó
`src/` para silenciar advertencias.

## Archivos entregados

- `GRAT_Software/CMakeLists.txt` — targets `grat_core`/`grat_software`, extensiones
  opcionales, `BUILD_TESTING` e integración con CTest.
- `GRAT_Software/tests/CMakeLists.txt` — entrada de pruebas (ya existía, compatible
  con la infraestructura).
- `.github/workflows/build-test.yml` — CI para compilar y ejecutar CTest en
  Ubuntu 24.04.
- `README.md` — dependencias reales, comandos de build y directorio de trabajo
  para ejecutar la aplicación.
- `.gitignore` — exclusiones para `build-rubrica/` y `build-rubrica-app/`.
- `GRAT_Software/docs/herramientas/build_ci.md` — este documento.
- `GRAT_Software/docs/herramientas/logs/` — logs breves de configure, build y
  ctest.

## Limitaciones y pendientes

- **CI en GitHub Actions:** el workflow se escribió y validó sintácticamente con
  `python -c "import yaml"`, pero **no se ejecutó en GitHub** porque no se dispone
  de acceso al servicio ni se autorizó push/PR.
- **MSVC y otros sistemas operativos:** solo se probó en Linux/WSL con g++ 15.2.0.
  Los flags para MSVC (`/W4`) están configurados mediante expresiones de generador,
  pero no se compiló en Windows.
- **Tests:** actualmente se descubre y ejecuta `test_reportes` de P4. La suite de
  P3 (`tests/core/`) no existe aún; cuando se integre, CTest la descubrirá
  automáticamente.
- **Rama destino:** el workflow apunta a `improvements/final`. Esa rama no existe
  en `origin` al momento de esta entrega; el equipo debe crearla o acordar un
  nombre alternativo de forma uniforme.
