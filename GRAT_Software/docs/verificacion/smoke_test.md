# Smoke test final

- Fecha: 2026-11-18
- Commit probado: `f6216fd` (`git rev-parse --short HEAD`)
- Build: limpio, 0 warnings y 0 errores con `-Wall -Wextra`
  (`build_despues.log`, g++ 15.2.0)
- Entorno: WSL2 + WSLg (Qt 6 Widgets/Sql, driver `QSQLITE`), CMake 3.16+ con Ninja
- Base de datos: `db/grat.db` relativa al directorio de trabajo

Las pruebas se ejecutaron el 2026-11-18 sobre la base de datos del proyecto.

## 1. Arranque

| Caso | Esperado | Obtenido | Resultado | Evidencia |
|---|---|---|---|---|
| Primera ejecución sobre carpeta vacía | Crea `db/grat.db` y aplica el esquema con el seed | 20 tablas, 43 permisos, 3 roles, 98 asignaciones rol-permiso y 3 usuarios | Conforme | Directorio temporal; consultas sobre la base creada |
| Segunda ejecución | No vuelve a sembrar y conserva los datos | Mismos conteos, 3 usuarios, registro de prueba `ZZ` intacto y `last_login` sin cambios | Conforme | Segunda ejecución sobre la misma base |
| Primer arranque visible | Se abre el diálogo de inicio de sesión | Diálogo de inicio visible y funcional | Conforme | `capturas/01-admin-nav.png` |

Evidencia de las ejecuciones en el directorio temporal:

```text
# primera ejecución
tablas: 20
permisos: 43
grupos: 3
usuarios: 3
group_permissions: 98
probe insertado: [('ZZ', 'Smoke Probe', 'A')]

# segunda ejecución
tablas: 20
permisos: 43
usuarios: 3
probe tras 2a ejecucion: [('ZZ', 'Smoke Probe', 'A')]
last_login (sin login): [('admin', None), ('supervisor', None), ('operator', None)]
integrity: ok
```

## 2. Recorrido como Administrator

Se inició sesión con `admin` / `Admin123!` y se completó el ciclo en una tabla
por módulo: alta, modificación, inactivación, reactivación y borrado lógico,
comprobando el refresco de la rejilla y el cambio de estado.

| Módulo | Tabla de ejemplo | Add | Modify | Inactivate | Reactivate | Delete | Grid refresca | Captura |
|---|---|---|---|---|---|---|---|---|
| Location | Countries | Correcto | Correcto | Correcto | Correcto | Correcto | Sí | `capturas/01-admin-nav.png` |
| Catalog | Constituents | Correcto | Correcto | Correcto | Correcto | Correcto | Sí | — |
| User | Users | Correcto | Correcto | Correcto | Correcto | Correcto | Sí | — |
| Waste | Waste | Correcto | Correcto | Correcto | Correcto | Correcto | Sí | — |
| Transfer | Transfers | Correcto | Correcto | Correcto | Correcto | Correcto | Sí | — |
| Admin | Roles | No aplica | Correcto | No aplica | No aplica | No aplica | Sí | — |

En Admin, Roles no tiene ciclo de estado: se concedió y revocó un permiso y se
comprobó que la rejilla hija se actualiza.

## 3. Validación

| Caso | Esperado | Obtenido | Resultado | Captura |
|---|---|---|---|---|
| Campo requerido vacío | Mensaje por campo y sin escritura en la base | Error en el campo y ninguna fila insertada | Conforme | — |
| Longitud máxima excedida | Rechazo con el límite indicado | Error de longitud y sin escritura | Conforme | — |
| Clave primaria duplicada | Rechazo por duplicado | Error de duplicado y sin escritura | Conforme | — |
| Decimal negativo | Rechazo por valor negativo | Error de valor negativo y sin escritura | Conforme | — |
| Varios errores a la vez | Se reportan todos juntos | Todos los errores mostrados en una sola pasada | Conforme | — |

## 4. RBAC

| Caso | Esperado | Obtenido | Resultado | Captura |
|---|---|---|---|---|
| Inicio de sesión como `operator` | Solo los módulos y tablas permitidos | 4 módulos y 11 tablas, sin User ni Admin | Conforme | `capturas/03-operator-botones.png` |
| Tablas de solo lectura | Botones de escritura deshabilitados | En Countries y Destinations los botones de escritura aparecen deshabilitados | Conforme | `capturas/03-operator-botones.png` |
| Tablas con permiso de alta y cambio | Add y Modify habilitados | En Waste y Transfers las acciones permitidas están habilitadas | Conforme | — |
| Acción no permitida | No se puede ejecutar | La acción permanece deshabilitada | Conforme | — |

## 5. Persistencia

| Caso | Esperado | Obtenido | Resultado | Evidencia |
|---|---|---|---|---|
| Datos propios tras reiniciar la aplicación | El registro creado sigue disponible | El registro se mantiene tras cerrar y volver a abrir la aplicación | Conforme | Consulta sobre `db/grat.db` |
| Integridad de la base | Sin corrupción | `integrity_check` devuelve `ok` | Conforme | Consulta sobre `db/grat.db` |

## 6. Conexión

| Caso | Esperado | Obtenido | Resultado | Evidencia |
|---|---|---|---|---|
| Arranque sin poder abrir `db/` | No crea la base e informa del fallo | Mensaje de error y sin archivo de base creado | Conforme | Ejecución en directorio temporal |
| Mantenimiento bloqueado con la base caída | Acciones deshabilitadas y estado visible | Estado en rojo y área de mantenimiento deshabilitada | Conforme | `capturas/smoke/bd-no disponible.png` |
| Restauración de la conexión | Se rehabilitan las acciones | El estado vuelve a conectado y las acciones se habilitan | Conforme | Verificado en la sesión |

## 7. Observaciones

- No se detectaron bugs durante las pruebas.
- La base de datos se abre relativa al directorio de trabajo: si se ejecuta desde
  `GRAT_Software/`, el archivo queda en `GRAT_Software/db/grat.db`, que está
  ignorado por Git. Desde la raíz se crearía `<raíz>/db/grat.db`, que no está
  ignorado.
- El árbol de trabajo conserva 34 archivos con cambios solo de fin de línea
  (LF a CRLF) ajenos a esta rama. No se modificaron ni se incluyeron en los
  commits, y no afectan al build ni al comportamiento.
- La base de datos del proyecto se respaldó antes de las pruebas en
  `db/grat.db.bak`.
