# Verificación de login y RBAC (nivel de base de datos)

- **Fecha:** 2026-11-17
- **Responsable:** Jhon
- **Rama:** `feature/build-docs`
- **Commit base:** `74a5098` (merge de `feature/error-handling`, igual a `origin/main`)
- **Alcance:** Tareas 7 y 8 de `tasks.md`, verificación sin interfaz gráfica.

## Método

1. La base de datos se creó desde el esquema versionado:
   `python3` + `executescript(GRAT_Software/resources/grat_schema.sqlite.sql)`
   (equivalente a `sqlite3 db/grat.db < resources/grat_schema.sqlite.sql`, ya que
   el CLI `sqlite3` no está instalado en este equipo).
2. El script siembra roles (`auth_group`), los 43 permisos (`auth_permission`) y
   las matrices rol→permiso (`auth_group_permissions`). **No** crea usuarios.
3. Los tres usuarios de desarrollo los crea la propia aplicación en el primer
   arranque (`AuthService::ensureDefaultUsers`), con SHA-256 + salt por registro.
   Se ejecutó el binario en modo headless (`QT_QPA_PLATFORM=offscreen`) para que
   realizara ese seed real.
4. Los conteos se obtuvieron con consultas SQL sobre `db/grat.db`, replicando la
   consulta exacta de `AuthService::loadPermissions` (unión de permisos por grupo
   y permisos directos del usuario).

> **Nota de ruta:** la aplicación abre `db/grat.db` de forma relativa al
> directorio de trabajo (`Database::open(dbDir = "db", ...)`). Ejecutando desde
> `GRAT_Software/`, la base de datos queda en `GRAT_Software/db/grat.db`, que es
> la ruta indicada en `tasks.md` y está ignorada por Git.

## Credenciales de prueba (solo desarrollo)

| Usuario | Contraseña | Rol | Permisos esperados |
|---|---|---|---|
| `admin` | `Admin123!` | Administrator (superuser) | 43 / 43 |
| `supervisor` | `Supervisor123!` | Supervisor | 36 |
| `operator` | `Operator123!` | Operator | 19 |

## Consultas ejecutadas

```sql
-- Conteo total del catálogo de permisos
SELECT COUNT(*) FROM auth_permission;

-- Permisos efectivos de un usuario (idéntico a AuthService::loadPermissions)
SELECT COUNT(*) FROM (
    SELECT p.codename
      FROM auth_permission p
      JOIN auth_group_permissions gp ON gp.permission_id = p.id
      JOIN user_user_groups ug ON ug.group_id = gp.group_id
     WHERE ug.user_id = :uid
    UNION
    SELECT p.codename
      FROM auth_permission p
      JOIN user_user_user_permissions up ON up.permission_id = p.id
     WHERE up.user_id = :uid
);

-- Estado de los usuarios sembrados
SELECT id, username, is_active, is_superuser FROM user_user ORDER BY id;

-- Integridad referencial
PRAGMA foreign_keys;
PRAGMA foreign_key_check;
```

## Resultados

| Caso | Esperado | Obtenido | Estado | Evidencia |
|---|---|---|---|---|
| Catálogo de permisos (`auth_permission`) | 43 | 43 | ✅ | `SELECT COUNT(*)` = 43 |
| Permisos efectivos de `admin` (Administrator) | 43 | 43 | ✅ | id=1, `is_active='A'`, grupo Administrator con 43 |
| Permisos efectivos de `supervisor` (Supervisor) | 36 | 36 | ✅ | id=2, `is_active='A'`, grupo Supervisor con 36, sin permisos directos |
| Permisos efectivos de `operator` (Operator) | 19 | 19 | ✅ | id=3, `is_active='A'`, grupo Operator con 19, sin permisos directos |
| Los 3 usuarios sembrados están activos | `A` en los 3 | `A`, `A`, `A` | ✅ | `user_user.is_active` |
| Integridad referencial | Sin violaciones | `PRAGMA foreign_key_check` vacío | ✅ | `integrity_check` = `ok` |
| `PRAGMA foreign_keys` | ON en conexiones de la app | 0 en sesión SQL cruda; 1 tras `PRAGMA foreign_keys = ON` | ✅ | La app lo activa por conexión; SQLite lo desactiva por defecto en una conexión nueva |

### Evidencia de ejecución

```text
script ejecutado OK
tablas: 20
permisos: 43
grupos: 3
usuarios: 0          # el esquema no crea usuarios
group_permissions: 98
  Administrator: 43
  Operator: 19
  Supervisor: 36

# tras el primer arranque headless de la app
usuarios: 3
(1, 'admin', 'A', 1, 1, 97)
(2, 'supervisor', 'A', 0, 1, 97)
(3, 'operator', 'A', 0, 0, 97)

admin        id=1 superuser=1 permisos_efectivos=43
supervisor   id=2 superuser=0 permisos_efectivos=36
operator     id=3 superuser=0 permisos_efectivos=19
permisos directos: 0
PRAGMA foreign_key_check: sin violaciones
integrity_check: ok
```

## Pruebas manuales (interfaz gráfica)

Ejecutadas el **2026-11-18** sobre `db/grat.db` (build `build/grat_software`,
WSLg). Capturas en `docs/verificacion/capturas/`.

| Caso | Esperado | Obtenido | Estado | Evidencia |
|---|---|---|---|---|
| Login correcto de `admin` + árbol de navegación y barra de estado | Acceso total, 43 permisos | Ventana `GRAT_Software — admin`; 6 módulos, incluido Admin → Roles; barra: `● DB connected`, `User: admin`, `Role(s): Administrator` | ✅ | `capturas/01-admin-nav.png` |
| Login correcto de `supervisor` | 36 permisos, módulos permitidos | Ventana `GRAT_Software — supervisor`; 5 módulos (sin Admin), 12 tablas con User → Users; `User: supervisor`, `Role(s): Supervisor` | ✅ | `capturas/02-supervisor-nav.png` |
| Login correcto de `operator` | 19 permisos, módulos permitidos | Ventana `GRAT_Software — operator`; 4 módulos (sin User ni Admin), 11 tablas; `User: operator`, `Role(s): Operator` | ✅ | `capturas/03-operator-botones.png` |
| Contraseña incorrecta | Rechazo con mensaje genérico | `admin` con clave errónea: `Invalid username or password.` en rojo; el diálogo permanece abierto y se limpia el campo de contraseña | ✅ | `capturas/04-contraseñaIncorrecta.png` |
| Usuario inexistente | Rechazo con el mismo mensaje genérico | Usuario `pepito`: mismo texto `Invalid username or password.` (no distingue de clave incorrecta) | ✅ | `capturas/05-usuario-inexistente.png` |
| Campos vacíos | Rechazo sin consultar la BD | Usuario y contraseña vacíos: `Enter your username and password.` | ✅ | `capturas/06-campos-vacios.png` |
| Usuario inactivo (`is_active='I'`) | Rechazo con mensaje genérico | Con `operator` en `is_active='I'`: `Invalid username or password.`, idéntico al de usuario inexistente; la BD se restauró después desde la copia de seguridad | ✅ | `capturas/07-usuario-inactivo.png` |
| Mensajes de error sin SQL ni nombres de tablas | Sin detalles internos | Clave incorrecta, usuario inexistente, campos vacíos y usuario inactivo muestran solo mensajes genéricos: no aparecen SQL, `user_user`, hashes ni rutas de la BD | ✅ | `capturas/04-contraseñaIncorrecta.png` a `capturas/07-usuario-inactivo.png` |
| Botones deshabilitados según el rol | Solo acciones permitidas | `operator` en Location → Countries: `Add`, `Modify`, `Delete`, `Inactivate`, `Reactivate` y `Update` deshabilitados (solo tiene `view_country`) | ✅ | `capturas/03-operator-botones.png` |

## Observaciones

- El rol Administrator obtiene sus 43 permisos por la matriz
  `auth_group_permissions`, no por un atajo de `is_superuser` en
  `AuthService::loadPermissions`; el conteo coincide igualmente con lo esperado.
- `auth_group_permissions` contiene 98 filas (43 + 36 + 19).
- Los tres logins manuales actualizaron `user_user.last_login` en `db/grat.db`,
  lo que confirma que la autenticación escribe en la base de datos real.
- Para la prueba de usuario inactivo se marcó `operator` como `is_active='I'` y,
  al terminar, se restauró `db/grat.db` desde `db/grat.db.bak`; los tres usuarios
  de desarrollo quedaron activos de nuevo.
