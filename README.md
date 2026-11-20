# GRAT_Software

Aplicación de escritorio para la gestión responsable de residuos de transporte
(GRAT). Permite mantener los catálogos y registros de los módulos Location,
Catalog, User, Waste, Transfer y Admin mediante pantallas CRUD generadas a partir
de metadatos, con autenticación y control de acceso basado en permisos (RBAC).

## Integrantes

- Jhon Cuyo
- Wilber Guerra
- Gerson Luque
- Diego Santa
- Jaime Igreda
- Cristhian Bravo

## Stack

- **C++20**
- **Qt 6** — Widgets + Sql + Svg
- **SQLite** a través del driver `QSQLITE`
- **CMake ≥ 3.16**

## Requisitos

### Obligatorios

- Compilador con soporte de C++20 (probado con `g++ 15.2.0`).
- Qt 6 con los módulos **Widgets**, **Sql**, **Svg** y el driver **QSQLITE**.
- CMake 3.16 o superior.

### Opcionales

- **Ninja** (recomendado; el proyecto también compila con el generador por
defecto de CMake).
- **Qt Test** cuando se habilitan las pruebas con `BUILD_TESTING=ON` (incluido
en `qt6-base-dev` en Ubuntu/Debian).

En Ubuntu/Debian, por ejemplo:

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev libqt6sql6-sqlite qt6-svg-dev
```

Si Qt está en una ruta no estándar, añade `-DCMAKE_PREFIX_PATH=<ruta a Qt>` al
configurar.

## Estructura del repositorio

```text
.
├── README.md
└── GRAT_Software/
    ├── CMakeLists.txt
    ├── db/
    ├── docs/
    ├── resources/
    ├── src/
    └── tests/
```

El archivo `CMakeLists.txt` se encuentra dentro de `GRAT_Software/`, no en la raíz
del repositorio.

## Cómo compilar

Desde la raíz del repositorio:

### Solo la aplicación (sin pruebas)

```bash
cmake -S GRAT_Software -B build -G Ninja -DBUILD_TESTING=OFF
cmake --build build
```

### Aplicación y pruebas

```bash
cmake -S GRAT_Software -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build
```

Para listar los tests registrados:

```bash
ctest --test-dir build -N
```

Para ejecutarlos:

```bash
ctest --test-dir build --output-on-failure
```

El ejecutable queda en `build/grat_software`.

## Cómo ejecutar

La aplicación abre la base de datos `db/grat.db` de forma relativa al directorio
desde el que se lanza. Por tanto, se debe ejecutar desde dentro de
`GRAT_Software/`:

```bash
cd GRAT_Software
../build/grat_software
```

Con el comando anterior, la base queda en `GRAT_Software/db/grat.db`.

## Base de datos

- En la **primera ejecución** se crea la carpeta `db/` y se aplica
  `resources/grat_schema.sqlite.sql`: tablas, restricciones, roles, los 43
  permisos y los usuarios de desarrollo.
- En ejecuciones posteriores **no se vuelve a sembrar**; se conservan los datos.
- Si el archivo `db/grat.db` se elimina, se recrea en el siguiente arranque.
- Cada conexión activa `PRAGMA foreign_keys = ON`.
- Si la base de datos no está disponible, la aplicación lo indica y bloquea el
  mantenimiento hasta que la conexión se restablece.
- `db/` y los archivos `*.db` no se versionan.

## Usuarios de prueba

> **Solo desarrollo.** Estas cuentas se crean automáticamente al inicializar una
> base de datos vacía.

| Usuario | Contraseña | Rol | Permisos |
|---|---|---|---|
| `admin` | `Admin123!` | Administrator (superusuario) | 43 / 43 |
| `supervisor` | `Supervisor123!` | Supervisor | 36 |
| `operator` | `Operator123!` | Operator | 19 |

Las contraseñas se almacenan con **SHA-256 + salt por registro**; el texto plano
nunca se guarda.

## Funcionalidades

- **Login** contra `user_user`, solo para usuarios con `is_active = 'A'`, con
  actualización de `last_login`.
- **RBAC** por permisos `view_<model>`, `add_<model>`, `change_<model>` y
  `assign_role_user`. Los módulos y botones se muestran u ocultan según el rol.
- **Borrado lógico** mediante `is_active`: `A` activo, `I` inactivo y `*`
  eliminado. La rejilla muestra todos los estados.
- **Validación** previa a escribir en la base de datos: campos obligatorios,
  longitud máxima, clave primaria duplicada, decimales no negativos y formato de
  correo.
- **Claves foráneas como desplegables**, preseleccionando el valor actual.
- **Pantallas maestro-detalle** para las tablas hijas (constituyentes de un
  residuo, empresas de transporte de un traslado y grupos/permisos de un usuario).
- **Admin → Roles** para conceder o revocar permisos a un rol (solo
  Administrator).
- **Manejo central de errores**: los mensajes al usuario no exponen SQL ni
  detalles internos del esquema.
- **Protección ante caída de la base de datos**: se deshabilitan las acciones de
  mantenimiento y se rehabilitan al recuperar la conexión.

## Flujo de trabajo del equipo

- Una rama por persona: `feature/`, `build/`, `test/` o `docs/` según la
  actividad.
- No se versionan `build/`, `db/`, los archivos `*.db` ni `.kiro/`.

## Verificación

Los resultados de verificación están en
[`GRAT_Software/docs/verificacion/`](GRAT_Software/docs/verificacion/):

- `login_roles.md` — conteo de permisos por rol y pruebas manuales de login.
- `build_antes.log` / `build_despues.log` — compilación con `-Wall -Wextra`.
- `build_warnings.md` — clasificación de warnings.
- `capturas/` — capturas de las pruebas de interfaz.
