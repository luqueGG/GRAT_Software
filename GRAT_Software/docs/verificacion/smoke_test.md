# Smoke test final

- **Fecha:** 2026-11-18
- **Commit probado:** `f6216fd` (`git rev-parse --short HEAD`)
- **Build:** limpio, **0 warnings y 0 errores** con `-Wall -Wextra`
  (`build_despues.log`, g++ 15.2.0)
- **Entorno:** WSL2 + WSLg (Qt 6 Widgets/Sql, driver `QSQLITE`), CMake 3.16+ con Ninja
- **Base de datos:** `db/grat.db` relativa al directorio de trabajo

> **Alcance de esta ronda:** verificaciones automáticas y prueba de conexión
> ejecutadas. Las filas con `⬜` no se ejecutaron (recorrido CRUD, validación,
> persistencia desde la GUI y restauración de la conexión).

## 1. Arranque

| Caso | Esperado | Obtenido | Estado | Evidencia |
|---|---|---|---|---|
| Primera ejecución sobre carpeta vacía | Crea `db/grat.db` y aplica el esquema + seed | 20 tablas, 43 permisos, 3 roles, 98 asignaciones rol→permiso, 3 usuarios | ✅ | Directorio temporal nuevo; salida de `sqlite3`/Python |
| Segunda ejecución | No vuelve a sembrar; conserva los datos | Mismos conteos, 3 usuarios, registro de prueba `ZZ` intacto, `last_login` sin cambios | ✅ | Segunda ejecución headless sobre la misma BD |
| Primer arranque visible (GUI) | Se abre el diálogo de login | ⬜ | ⬜ | — |

Evidencia (directorio temporal, sin tocar la BD del proyecto):

```text
# primera ejecución
tablas: 20
permisos: 43
grupos: 3
usuarios: 3
group_permissions: 98
probe insertado: [('ZZ', 'Smoke Probe', 'A')]

# segunda ejecución (6 s headless)
tablas: 20
permisos: 43
usuarios: 3
probe tras 2a ejecucion: [('ZZ', 'Smoke Probe', 'A')]
last_login (sin login): [('admin', None), ('supervisor', None), ('operator', None)]
integrity: ok
```

## 2. Recorrido como Administrator

Iniciar sesión con `admin` / `Admin123!` y completar el ciclo en una tabla por
módulo. Anotar en cada celda si la acción funciona y adjuntar captura.

| Módulo | Tabla de ejemplo | Add | Modify | Inactivate | Reactivate | Delete | Grid refresca | Captura |
|---|---|---|---|---|---|---|---|---|
| Location | Countries | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Catalog | Constituents | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| User | Users | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Waste | Waste | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Transfer | Transfers | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Admin | Roles | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

> En Admin → Roles no aplica el ciclo de estado: se verifica conceder/revocar un
> permiso y que la rejilla hija se refresque.

## 3. Validación

| Caso | Esperado | Obtenido | Estado | Captura |
|---|---|---|---|---|
| Campo requerido vacío | Mensaje por campo y sin escritura en BD | ⬜ | ⬜ | ⬜ |
| Longitud máxima excedida | Rechazo con el límite indicado | ⬜ | ⬜ | ⬜ |
| Clave primaria duplicada | Rechazo por duplicado | ⬜ | ⬜ | ⬜ |
| Decimal negativo | Rechazo por valor negativo | ⬜ | ⬜ | ⬜ |
| Varios errores a la vez | Se reportan todos juntos | ⬜ | ⬜ | ⬜ |

## 4. RBAC (re-login como Operator)

| Caso | Esperado | Obtenido | Estado | Captura |
|---|---|---|---|---|
| Cerrar sesión e iniciar como `operator` | Solo 4 módulos y 11 tablas; sin User ni Admin | Verificado en la Fase 1: 4 módulos y 11 tablas, sin User ni Admin | ✅ | `login_roles.md` · `capturas/03-operator-botones.png` |
| Solo lectura en Countries | Botones de escritura deshabilitados | Verificado en la Fase 1: todos los botones deshabilitados | ✅ | `capturas/03-operator-botones.png` |
| `Add` activo en Waste y Transfers | Acciones permitidas según `add_<model>` | ⬜ | ⬜ | — |
| Acción no permitida | Botón deshabilitado; no se puede ejecutar | ⬜ | ⬜ | — |

## 5. Persistencia

| Caso | Esperado | Obtenido | Estado | Evidencia |
|---|---|---|---|---|
| Datos propios sobreviven a un reinicio (nivel BD) | El registro insertado sigue tras cerrar y reabrir | Registro `ZZ` presente tras la segunda ejecución; `integrity_check = ok` | ✅ | Directorio temporal |
| Registro creado desde la GUI sobrevive al cierre | Reaparece al reabrir la app | ⬜ | ⬜ | — |

## 6. Conexión (base de datos no disponible)

| Caso | Esperado | Obtenido | Estado | Evidencia |
|---|---|---|---|---|
| Arranque sin poder crear/abrir `db/` | No crea la BD, informa del fallo y no continúa | `db` como archivo regular: error `database directory: Could not create database directory: db`, sin archivo de BD creado | ✅ | Ejecución headless; el diálogo modal queda abierto (proceso detenido por `timeout`) |
| Mantenimiento bloqueado con la BD caída | Acciones deshabilitadas y estado visible | Barra en rojo `● DB unavailable — maintenance disabled`; rejilla, formulario y botones atenuados y deshabilitados | ✅ | `capturas/smoke/bd-no disponible.png` |
| Restauración de la conexión | Se rehabilitan las acciones | ⬜ | ⬜ | — |

## 7. Observaciones / bugs encontrados

- En las verificaciones automáticas **no se detectaron bugs**.
- La base de datos se abre relativa al directorio de trabajo: ejecutar desde
  `GRAT_Software/` deja el archivo en `GRAT_Software/db/grat.db` (ignorado por
  Git). Desde la raíz se crearía `<raíz>/db/grat.db`, que no está ignorado.
- El árbol de trabajo conserva 34 archivos con cambios solo de fin de línea
  (LF→CRLF) ajenos a esta rama; no se han modificado ni commiteado y no afectan
  al build ni al comportamiento.
- La prueba de conexión se ejecutó con la app abierta como `admin` y la base de
  datos no disponible: la captura evidencia el estado rojo y el mantenimiento
  deshabilitado. No se capturó la restauración de la conexión.
- Quedan sin evidencia en esta ronda: recorrido CRUD por módulo (sección 2),
  casos de validación (sección 3), `Add` habilitado como Operator y acciones no
  permitidas (sección 4), persistencia desde la GUI (sección 5) y restauración de
  la conexión (sección 6).
