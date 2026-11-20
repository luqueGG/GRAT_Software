# Aceptacion final - GRAT_Software (P5)

Estado del documento: **plan de aceptacion preparado; resultados pendientes de
ejecucion**. Este archivo se actualizara en el tercer commit con resultados
reales. Ningun caso se marcara como `Aprobado` sin ejecucion.

## 1. Version bajo prueba

| Dato | Valor |
|---|---|
| Commit base de integracion | `16abf09c44a8f767de3119978d70a749d62d8e18` (`origin/main`) |
| Rama P5 | `docs/rubrica-demo-evidencias` |
| Rama destino solicitada | `improvements/final` (**no existe en `origin` al preparar esta entrega**) |
| Base real utilizada | `origin/main`, que ya contiene P1, P2, P3, P4 y P6 documentados |
| Entorno previsto | Windows, Qt 6.12.0 MinGW 64-bit, CMake 3.x, Ninja, SQLite/QSQLITE |
| Base de datos | Copia aislada creada por `tools/demo/prepare_demo_db.py` |
| Base habitual | `GRAT_Software/db/grat.db` - prohibido modificar |

Dependencia registrada: `improvements/final` no aparece en
`git branch -a` tras `git fetch origin --prune`. No se creo ni modifico esa
rama; se trabajo desde `origin/main` y se documenta la desviacion.

## 2. Convenciones

Estados permitidos: **Aprobado**, **Fallido**, **Bloqueado**, **Pendiente**.

- `Aprobado`: ejecutado en esta sesion con resultado conforme.
- `Fallido`: ejecutado con resultado no conforme.
- `Bloqueado`: existe una dependencia externa que impide ejecutarlo.
- `Pendiente`: aun no ejecutado; debe ejecutarse segun los pasos indicados.

## 3. Casos de aceptacion

### AC-01 - Inicio de sesion Administrator

- **Objetivo:** validar acceso del rol con 43 permisos.
- **Precondiciones:** base aislada creada; app iniciada desde esa carpeta.
- **Pasos:** escribir `admin` / `Admin123!`; pulsar `OK`.
- **Resultado esperado:** ventana principal; barra de estado con `admin` y `Administrador`; seis modulos visibles.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** Windows / Qt 6 / build-rubrica-app.
- **Comando:** `.\grat_software.exe` desde la carpeta aislada.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/01_login_admin.png`, `evidencias/02_menu_modulos_admin.png`.

### AC-02 - Inicio de sesion Supervisor

- **Objetivo:** validar acceso del rol con 36 permisos.
- **Precondiciones:** base aislada creada.
- **Pasos:** escribir `supervisor` / `Supervisor123!`; pulsar `OK`.
- **Resultado esperado:** ventana principal sin modulo `Administracion`; barra de estado con `Supervisor`.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** igual a AC-01.
- **Commit probado:** `16abf09`.
- **Evidencia:** a definir durante la ejecucion.

### AC-03 - Inicio de sesion Operator

- **Objetivo:** validar acceso del rol con 19 permisos.
- **Precondiciones:** base aislada creada.
- **Pasos:** escribir `operator` / `Operator123!`; pulsar `OK`.
- **Resultado esperado:** sin modulos `Usuarios` ni `Administracion`; tablas de solo lectura sin botones de escritura.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** igual a AC-01.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/06_operador_permisos.png`.

### AC-04 - Accion permitida y denegada por rol

- **Objetivo:** comparar una operacion permitida con una denegada.
- **Precondiciones:** sesiones indicadas.
- **Pasos:** `admin` abre `Administracion > Roles`; `operator` intenta ver `Usuarios` y pulsar `Agregar` en `Paises`.
- **Resultado esperado:** `admin` puede abrir Roles; `operator` no ve esos modulos ni botones de alta.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** interaccion GUI.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/06_operador_permisos.png`, `evidencias/07_roles_revocar.png`.

### AC-05 - Modificacion autorizada de permisos y reflejo en sesion

- **Objetivo:** verificar otorgamiento/revocacion y el momento de actualizacion.
- **Precondiciones:** sesion `admin`; rol `Supervisor` seleccionado.
- **Pasos:** revocar `add_constituent` a `Supervisor`; cerrar sesion; iniciar como `supervisor`; abrir `Catalogos > Componentes`.
- **Resultado esperado:** el boton `Agregar` ya no aparece en la nueva sesion; la sesion abierta antes del cambio no se actualiza automaticamente (comportamiento documentado por P1).
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** interaccion GUI.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/07_roles_revocar.png`.

### AC-06 - Validacion de campo obligatorio

- **Objetivo:** comprobar rechazo sin escritura.
- **Precondiciones:** sesion `admin`; tabla `Paises`.
- **Pasos:** `Agregar`; dejar `Codigo (Alfa-2)` vacio; completar los demas campos obligatorios; `Actualizar`.
- **Resultado esperado:** mensaje `Validation failed`; no se inserta fila; conteo previo conservado.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** interaccion GUI + consulta SQLite de conteo.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/08_validacion.png`.

### AC-07 - Validacion de duplicado

- **Objetivo:** comprobar rechazo de clave primaria duplicada.
- **Precondiciones:** base aislada con `PE` ya presente.
- **Pasos:** `Agregar` en `Paises`; repetir `PE`; completar el resto; `Actualizar`.
- **Resultado esperado:** error por duplicado; no se modifica la fila existente.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** interaccion GUI.
- **Commit probado:** `16abf09`.
- **Evidencia:** a definir durante la ejecucion.

### AC-08 - Maestro-detalle de residuos

- **Objetivo:** crear principal, crear hijo, asociar y consultar.
- **Precondiciones:** sesion `admin`; catalogos de la demo presentes.
- **Pasos:** en `Residuos`, `Agregar` `RS-DEMO-01`; guardar; en la pestaña `Constituents`, agregar dos filas; seleccionar el padre y verificar que solo aparezcan sus hijos.
- **Resultado esperado:** relacion por `id_waste_id` correcta; los hijos de otro padre no aparecen.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** interaccion GUI + consulta SQLite.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/03_residuos_maestro_detalle.png`.

### AC-09 - Maestro-detalle de traslados y estados

- **Objetivo:** verificar tramo hijo y ciclo de estado del traslado.
- **Precondiciones:** residuo de la demo presente.
- **Pasos:** en `Traslados`, abrir `TR-DEMO-01`; mostrar la pestaña de tramos; `Desactivar`, verificar `I`, `Reactivar`, verificar `A`.
- **Resultado esperado:** tramo asociado al traslado; estado logico reversible sin borrado fisico.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** interaccion GUI + consulta SQLite.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/04_traslados_detalle.png`, `evidencias/05_estados.png`.

### AC-10 - Reporte autorizado, filtro y conteo

- **Objetivo:** verificar acceso, filtro y conteo de reporte de residuos.
- **Precondiciones:** sesion `admin` (tiene `view_waste`).
- **Pasos:** `Reportes > Abrir reportes`; elegir `Residuos registrados`; filtrar `Activo (A)`; leer `Registros` y `Cantidad total`.
- **Resultado esperado:** solo filas activas; conteo y suma coherentes con la base aislada.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** interaccion GUI.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/09_reportes_conteo.png`.

### AC-11 - Exportacion CSV y correspondencia con la tabla

- **Objetivo:** comprobar que el CSV contiene exactamente el conjunto filtrado.
- **Precondiciones:** reporte con filas visibles.
- **Pasos:** `Exportar CSV`; guardar en la carpeta de evidencias; contar filas del archivo; comparar con `Registros`.
- **Resultado esperado:** filas CSV = conteo mostrado; UTF-8 con BOM; CRLF.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** igual a AC-01.
- **Comando:** interaccion GUI + inspeccion del CSV.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/10_exportacion_csv.png`, `evidencias/reportes_residuos_activos.csv`.

### AC-12 - Pruebas automatizadas y build

- **Objetivo:** verificar compilacion y suite de pruebas.
- **Precondiciones:** dependencias Qt6, CMake y Ninja disponibles.
- **Pasos:** ejecutar los cuatro comandos de la seccion 4.
- **Resultado esperado:** build de pruebas y build de app correctos; CTest ejecuta las pruebas descubiertas.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Entorno:** Windows / Qt 6.12.0 MinGW 64-bit.
- **Comando:** ver seccion 4.
- **Commit probado:** `16abf09`.
- **Evidencia:** `evidencias/11_ctest_salida.txt`, `evidencias/12_build_salida.txt`.

## 4. Comandos de verificacion

```bash
git status --short --branch
git branch --show-current
git log --oneline -n 8
git diff improvements/final...HEAD --stat   # falla si la rama no existe; documentado

cmake -S GRAT_Software -B build-rubrica -G Ninja -DBUILD_TESTING=ON
cmake --build build-rubrica
ctest --test-dir build-rubrica -N
ctest --test-dir build-rubrica --output-on-failure

cmake -S GRAT_Software -B build-rubrica-app -G Ninja -DBUILD_TESTING=OFF
cmake --build build-rubrica-app
```

Si `ctest -N` muestra cero pruebas, ese resultado **no** se interpreta como
suite aprobada y se documenta como limitacion.

## 5. Registro de incidencias

| ID | Incidencia | Estado | Accion |
|---|---|---|---|
| INC-01 | `improvements/final` no existe en `origin` | Abierta | Se uso `origin/main` como base real; no se creo ni modifico otra rama. |
| INC-02 | `rubrica/distribucion_mejoras_rubrica.md` no existe en el repositorio ni en el historial disponible | Abierta | Se sigue el prompt P5 como fuente; no se atribuyen autores no confirmados. |
