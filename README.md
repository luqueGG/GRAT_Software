# GRAT_Software

Aplicación de escritorio para la **Gestión Responsable de Residuos de Transporte (GRAT)**. Está pensada para mantener catálogos y registros de los módulos Location, Catalog, User, Waste, Transfer y Admin a través de pantallas CRUD generadas a partir de metadatos, con autenticación de usuarios y control de acceso basado en permisos (RBAC).

El proyecto nace como un ejercicio académico de arquitectura de software en C++ moderno con Qt 6, donde se aplican patrones como Repository, Strategy y Observer, y se mantiene una suite de pruebas automatizadas con Qt Test y CTest.

## Integrantes

- Jhon Cuyo
- Wilber Guerra
- Gerson Luque
- Diego Santa
- Jaime Igreda
- Cristhian Bravo

## Stack tecnológico

- **C++20**
- **Qt 6** — módulos Widgets, Sql y Svg
- **SQLite** a través del driver `QSQLITE`
- **CMake ≥ 3.16** (con Ninja como generador recomendado)

## Requisitos para compilar

### Obligatorios

- Compilador con soporte C++20 (probado con `g++ 15.2.0`).
- Qt 6 con los módulos **Widgets**, **Sql**, **Svg** y el driver **QSQLITE**.
- CMake 3.16 o superior.

### Opcionales

- **Ninja**: el proyecto también compila con el generador por defecto de CMake, pero Ninja es más rápido y es el que usamos para verificar.
- **Qt Test**: necesario únicamente si compilas con `BUILD_TESTING=ON`. En Ubuntu/Debian viene incluido en `qt6-base-dev`.

### Instalación en Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build qt6-base-dev libqt6sql6-sqlite qt6-svg-dev
```

Si Qt está instalado en una ruta no estándar, añade `-DCMAKE_PREFIX_PATH=<ruta a Qt>` al configurar.

## Estructura del repositorio

El archivo `CMakeLists.txt` principal está dentro de `GRAT_Software/`, no en la raíz. Desde la raíz del repositorio se configura el build apuntando a esa carpeta.

```text
.
├── .github/workflows/build-test.yml    # CI con GitHub Actions (Ubuntu 24.04)
├── .gitignore                          # Archivos y carpetas ignorados por Git
├── .vscode/settings.json               # Configuración opcional de VS Code
├── README.md                           # Este archivo
├── GRAT_Software/                      # Código fuente, build system y recursos
│   ├── CMakeLists.txt                  # Definición de targets: grat_core y grat_software
│   ├── cmake/
│   │   └── features/
│   │       ├── arquitectura.cmake      # Fuentes de controladores y repositorios (P1)
│   │       ├── reportes.cmake          # Fuentes del módulo de reportes (P4)
│   │       └── validacion.cmake        # Fuentes de reglas de validación (P2)
│   ├── db/                             # Base de datos local (no se versiona)
│   ├── docs/
│   │   ├── arquitectura/
│   │   │   ├── mvc_repository.md       # Documentación de MVC y Repository (P1)
│   │   │   └── strategy_poo.md         # Documentación de Strategy y polimorfismo (P2)
│   │   ├── herramientas/
│   │   │   ├── build_ci.md             # Guía de build reproducible y CI (P6)
│   │   │   └── logs/                   # Logs de configure, build y ctest
│   │   ├── innovacion/
│   │   │   └── reportes_impacto.md     # Documentación de reportes CSV (P4)
│   │   ├── pruebas/
│   │   │   └── suite_core.md           # Descripción de la suite de pruebas (P3)
│   │   └── verificacion/
│   │       ├── build_antes.log         # Log de build previo a mejoras
│   │       ├── build_despues.log       # Log de build posterior a mejoras
│   │       ├── build_warnings.md       # Clasificación de advertencias
│   │       ├── login_roles.md          # Verificación de login y permisos
│   │       ├── smoke_test.md           # Smoke test de la aplicación
│   │       └── capturas/               # Capturas de pantalla de pruebas manuales
│   ├── resources/
│   │   ├── grat_schema.qrc             # Recurso Qt con el esquema SQL
│   │   ├── grat_schema.sqlite.sql      # Esquema completo de SQLite
│   │   ├── grat_resources.qrc          # Recurso Qt con tema e iconos
│   │   ├── grat_theme.qss              # Hoja de estilos de la aplicación
│   │   └── icons/                      # Iconos SVG de la interfaz
│   ├── src/
│   │   ├── main.cpp                    # Punto de entrada
│   │   ├── app/session.h               # Sesión de usuario autenticado
│   │   ├── core/
│   │   │   ├── auth/                   # Autenticación y comprobación de permisos
│   │   │   ├── crud/                   # TableGateway, especificaciones, validación
│   │   │   │   ├── controllers/        # CrudController y ChildCrudController
│   │   │   │   ├── repositories/       # IRecordRepository y SqlRecordRepository
│   │   │   │   └── validation/         # Reglas de validación con Strategy
│   │   │   ├── db/                     # Conexión SQLite y ciclo de vida
│   │   │   ├── errors/                 # Manejador central de errores
│   │   │   └── reports/                # Servicio de reportes y exportación CSV
│   │   └── ui/                         # Pantallas Qt: login, CRUD, roles, reportes
│   └── tests/
│       ├── CMakeLists.txt              # Entrada de CTest
│       ├── core/                       # Pruebas unitarias e integración (P3)
│       └── reportes/                   # Pruebas del módulo de reportes (P4)
├── docs/
│   └── presentacion/                   # Guión, datos y evidencias de la demo (P5)
├── rubrica/                            # Distribución de mejoras y rúbrica
├── sample_images/                      # Imágenes de ejemplo
└── tools/demo/
    └── prepare_demo_db.py              # Script para preparar base de demo aislada
```

## Cómo compilar

Todos los comandos se ejecutan desde la raíz del repositorio.

### Solo la aplicación

```bash
cmake -S GRAT_Software -B build -G Ninja -DBUILD_TESTING=OFF
cmake --build build
```

### Aplicación y pruebas

```bash
cmake -S GRAT_Software -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build
```

El ejecutable queda en `build/grat_software`.

### Listar y ejecutar pruebas

```bash
# Ver qué tests están registrados
ctest --test-dir build -N

# Ejecutar todos los tests
ctest --test-dir build --output-on-failure
```

Con `BUILD_TESTING=ON`, CTest descubre automáticamente las suites de `tests/core/` y `tests/reportes/`.

## Cómo ejecutar la aplicación

La aplicación abre la base de datos `db/grat.db` de forma relativa al directorio desde el que se lanza. Por eso hay que ejecutarla desde dentro de `GRAT_Software/`:

```bash
cd GRAT_Software
../build/grat_software
```

La primera vez crea la carpeta `db/`, aplica el esquema e inserta los usuarios de desarrollo. Las siguientes ejecuciones conservan los datos existentes.

## Base de datos

- La primera ejecución crea `db/grat.db` aplicando `resources/grat_schema.sqlite.sql`: tablas, restricciones, roles, los 43 permisos y los usuarios de desarrollo.
- En ejecuciones posteriores **no se vuelve a sembrar**; se conservan los datos.
- Si se elimina `db/grat.db`, se recrea en el siguiente arranque.
- Cada conexión activa `PRAGMA foreign_keys = ON`.
- Si la base de datos no está disponible, la aplicación lo indica y bloquea el mantenimiento hasta que se restablezca la conexión.
- `db/` y los archivos `*.db` no se versionan.

## Usuarios de prueba

> **Solo para desarrollo.** Estas cuentas se crean automáticamente al inicializar una base vacía.

| Usuario | Contraseña | Rol | Permisos |
|---|---|---|---|
| `admin` | `Admin123!` | Administrator (superusuario) | 43 / 43 |
| `supervisor` | `Supervisor123!` | Supervisor | 36 |
| `operator` | `Operator123!` | Operator | 19 |

Las contraseñas se almacenan con **SHA-256 + salt por registro**; nunca se guarda el texto plano.

## Funcionalidades principales

- **Login** contra `user_user`, solo para usuarios activos (`is_active = 'A'`), con actualización de `last_login`.
- **RBAC** basado en permisos como `view_<model>`, `add_<model>`, `change_<model>` y `assign_role_user`. La interfaz muestra u oculta módulos y botones según el rol.
- **Borrado lógico** mediante `is_active`: `A` activo, `I` inactivo, `*` eliminado. La rejilla muestra registros en todos los estados.
- **Validación** antes de escribir: campos obligatorios, longitud máxima, clave natural duplicada, decimales no negativos y formato de correo.
- **Claves foráneas como desplegables**, preseleccionando el valor actual.
- **Pantallas maestro-detalle** para tablas hijas: constituyentes de un residuo, empresas de transporte de un traslado, y grupos/permisos de un usuario.
- **Administración de roles** para conceder o revocar permisos (solo Administrator).
- **Reportes operativos** de residuos y traslados con filtros por estado y exportación a CSV.
- **Manejo central de errores** que evita exponer SQL o detalles internos del esquema al usuario final.
- **Recuperación ante desconexión** de la base de datos: se deshabilitan las acciones de mantenimiento y se rehabilitan al recuperar la conexión.

## Arquitectura y diseño

La separación entre interfaz y lógica se apoya en:

- **MVC + Repository**: los widgets (`CrudWidget`, `MasterDetailWidget`) delegan operaciones en `CrudController` y `ChildCrudController`, que usan `IRecordRepository`/`SqlRecordRepository` para persistencia. Ver `docs/arquitectura/mvc_repository.md`.
- **Strategy**: las reglas de validación implementan `IValidationRule` y se componen en `Validator`. Ver `docs/arquitectura/strategy_poo.md`.
- **Observer**: `Database` notifica cambios de conexión mediante señales Qt, y `GuardedApplication::notify()` captura excepciones inesperadas en el bucle de eventos.

El target `grat_core` agrupa el código no visual (datos, validación, autenticación, reportes) y `grat_software` es el ejecutable con la interfaz.

## Flujo de trabajo del equipo

- Cada integrante trabaja en su propia rama (`feature/`, `build/`, `test/` o `docs/`).
- La integración se hace mediante pull requests hacia la rama base acordada.
- No se versionan directorios de build (`build/`, `build-rubrica/`, `build-rubrica-app/`), la carpeta `db/`, archivos `*.db` ni la carpeta `.kiro/`.

## Verificación y documentación

La documentación del proyecto está organizada por responsabilidades:

- `docs/arquitectura/` — decisiones de arquitectura (MVC, Repository, Strategy).
- `docs/innovacion/` — reportes operativos y medición de impacto.
- `docs/pruebas/` — suite de pruebas unitarias e integración.
- `docs/herramientas/` — build reproducible, CI y logs.
- `docs/verificacion/` — pruebas manuales, conteo de permisos, smoke test y capturas.
- `docs/presentacion/` — guión, datos y evidencias de la demo final.

Puedes consultar cada documento para entender qué se verificó, con qué comandos y qué resultados se obtuvieron.
