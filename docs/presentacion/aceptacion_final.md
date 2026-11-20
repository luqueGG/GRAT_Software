# Aceptacion final - GRAT_Software (P5)

Estado del documento: **aceptacion parcial con resultados reales registrados el
19 de noviembre de 2026**. Los casos no ejecutados permanecen como `Pendiente`;
no se han marcado como aprobados casos diferentes a los verificados.

> **Nota de integracion (19-11-2026):** la base de integracion y el destino del
> Pull Request cambiaron a `origin/main` (`feb53db`). El merge local es
> `71fda80`. Las secciones 1 a 8 se conservan como registro historico de la
> aceptacion ejecutada sobre `16abf09`. La verificacion automatizada integral se
> repitio una sola vez sobre la version integrada y se documenta en la seccion
> 9. No se abrio de nuevo el dialogo de exportacion CSV y los casos pendientes
> no se marcaron como aprobados.

## 1. Version bajo prueba

| Dato | Valor |
|---|---|
| Commit base y probado | `16abf09c44a8f767de3119978d70a749d62d8e18` (`origin/main` al crear la rama) |
| Rama P5 | `docs/rubrica-demo-evidencias` |
| Rama destino solicitada | `improvements/final` (**no existe en `origin` al cierre de esta entrega**) |
| Base real utilizada | `origin/main` en el commit indicado arriba |
| Commit local P5 | `e4361a9` al momento de ejecutar las verificaciones GUI y de compilacion |
| Entorno | Windows, Qt 6.12.0 MinGW 64-bit, CMake 3.x, Ninja, SQLite/QSQLITE |
| Base de datos de demo | `C:\Users\USER\AppData\Local\Temp\opencode\grat_demo\db\grat.db` |
| Base habitual | `GRAT_Software/db/grat.db` - prohibido modificar |
| Usuarios de la demo | `admin/Admin123!`, `supervisor/Supervisor123!`, `operator/Operator123!` |

Observaciones sobre la base:

- `git fetch origin --prune` confirmo que `improvements/final` no existe. Por
  eso no se creo ni modifico esa rama; la entrega P5 se construyo desde
  `origin/main`.
- Despues de ejecutar las verificaciones, `origin/main` avanzo a `feb53db`
  (merge de `feature/stragy-polimorfismo`, PR #10). Ese cambio **no** fue
  incluido en el binario ni en las capturas aqui registradas; por ello no se
  afirma que P5 haya verificado el codigo agregado en `feb53db`.

## 2. Convenciones

Estados permitidos: **Aprobado**, **Aprobado (verificacion manual del
usuario)**, **Fallido**, **Bloqueado**, **Pendiente**.

- `Aprobado`: ejecutado en esta sesion con resultado conforme.
- `Aprobado (verificacion manual del usuario)`: confirmado por el usuario;
  no se afirma ejecucion automatizada adicional.
- `Fallido`: ejecutado con resultado no conforme.
- `Bloqueado`: existe una dependencia externa que impide ejecutarlo.
- `Pendiente`: no ejecutado; conserva su procedimiento para ejecucion futura.

## 3. Resumen de resultados

| ID | Caso | Estado | Evidencia principal |
|---|---|---|---|
| AC-01 | Inicio de sesion `Administrator` | Aprobado | `01_login_admin.png`, `02_menu_modulos_admin.png`, `16_admin_salida.txt` |
| AC-02 | Inicio de sesion `Supervisor` | Pendiente | Sin ejecutar |
| AC-03 | Inicio de sesion `Operator` | Pendiente | Sin ejecutar |
| AC-04 | Accion permitida y denegada por rol | Pendiente | Sin ejecutar |
| AC-05 | Modificacion de permisos y reflejo en sesion | Pendiente | Sin ejecutar |
| AC-06 | Validacion de campo obligatorio | Pendiente | Sin ejecutar |
| AC-07 | Validacion de duplicado | Pendiente | Sin ejecutar |
| AC-08 | Consulta maestro-detalle de residuos | Aprobado (consulta) | `03_residuos_maestro_detalle.png`, `15_residuos_salida.txt` |
| AC-09 | Maestro-detalle de traslados y estados | Pendiente | Sin ejecutar |
| AC-10 | Reporte autorizado, filtro y conteo | Aprobado | `09_reportes_conteo.png`, `14_reportes_salida.txt` |
| AC-11 | Exportacion CSV | Aprobado (verificacion manual del usuario) | Registro de este documento |
| AC-12 | Pruebas automatizadas y build | Aprobado | `11_ctest_salida.txt`, `12_build_salida.txt` |

## 4. Casos de aceptacion

### AC-01 - Inicio de sesion `Administrator`

- **Objetivo:** validar acceso del rol con 43 permisos.
- **Precondiciones:** base aislada creada; app iniciada desde esa carpeta.
- **Comando:** `.\grat_software.exe` desde la carpeta aislada.
- **Commit probado:** `16abf09`.
- **Resultado observado:** inicio de sesion correcto con `admin`; la barra de
  estado mostro `Rol(es): Administrador`; el arbol de modulos incluyo
  `Ubicaciones`, `Paises`, `Subdivisiones`, `Catalogos`, `Componentes`,
  `Tipos de contenedores`, `Niveles de toxicidad`, `Tipos de transporte`,
  `Usuarios`, `Residuos`, `Empresas generadoras`, `Transporte`, `Destinos`,
  `Empresas transportistas` y `Traslados`.
- **Observacion:** el boton del cuadro de acceso se rotulo `Iniciar sesion`,
  no `OK` como se escribio en el guion inicial.
- **Estado:** Aprobado.
- **Evidencia:** `evidencias/01_login_admin.png`,
  `evidencias/02_menu_modulos_admin.png`, `evidencias/16_admin_salida.txt`.

### AC-02 - Inicio de sesion `Supervisor`

- **Objetivo:** validar acceso del rol con 36 permisos.
- **Pasos previstos:** escribir `supervisor` / `Supervisor123!`; pulsar
  `Iniciar sesion`; verificar ausencia de `Administracion` y texto
  `Supervisor` en la barra de estado.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Evidencia:** no generada; no se fabrico captura.

### AC-03 - Inicio de sesion `Operator`

- **Objetivo:** validar acceso del rol con 19 permisos.
- **Pasos previstos:** escribir `operator` / `Operator123!`; verificar ausencia
  de `Usuarios` y `Administracion`, y tablas sin botones de escritura.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Evidencia:** `evidencias/06_operador_permisos.png` queda como evidencia
  prevista, no generada.

### AC-04 - Accion permitida y denegada por rol

- **Objetivo:** comparar una operacion permitida con una denegada.
- **Pasos previstos:** `admin` abre `Administracion > Roles`; `operator`
  intenta ver `Usuarios` y pulsar `Agregar` en `Paises`.
- **Resultado observado:** sin ejecutar. No se abrio `Administracion > Roles`
  ni se inicio sesion como `operator`.
- **Estado:** Pendiente.
- **Evidencia:** `evidencias/06_operador_permisos.png` y
  `evidencias/07_roles_revocar.png` quedan como evidencias previstas, no
  generadas.

### AC-05 - Modificacion autorizada de permisos y reflejo en sesion

- **Objetivo:** verificar otorgamiento/revocacion y el momento de actualizacion.
- **Pasos previstos:** revocar `add_constituent` a `Supervisor`; cerrar sesion;
  iniciar como `supervisor`; abrir `Catalogos > Componentes`.
- **Resultado observado:** sin ejecutar. No se modificaron permisos.
- **Estado:** Pendiente.
- **Evidencia:** `evidencias/07_roles_revocar.png` queda como evidencia
  prevista, no generada.

### AC-06 - Validacion de campo obligatorio

- **Objetivo:** comprobar rechazo sin escritura.
- **Pasos previstos:** en `Paises`, `Agregar`, dejar `Codigo (Alfa-2)` vacio,
  completar los demas campos obligatorios y pulsar `Actualizar`.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Evidencia:** `evidencias/08_validacion.png` queda como evidencia prevista,
  no generada.

### AC-07 - Validacion de duplicado

- **Objetivo:** comprobar rechazo de clave primaria duplicada.
- **Pasos previstos:** en `Paises`, repetir `PE` ya presente en la base de
  demo y verificar que no se modifique la fila existente.
- **Resultado observado:** sin ejecutar.
- **Estado:** Pendiente.
- **Evidencia:** no generada.

### AC-08 - Consulta maestro-detalle de residuos

- **Objetivo:** verificar la asociacion padre-hijo con los datos precargados de
  la demo. La creacion de registros por interfaz queda fuera de este caso
  verificado.
- **Pasos ejecutados:** iniciar como `admin`; abrir `Residuos`; seleccionar
  `RS-DEMO-01`; abrir la pestaña de componentes.
- **Commit probado:** `16abf09`.
- **Resultado observado:** el registro maestro `RS-DEMO-01` mostro dos hijos:
  `1 - Plastico / 7.50 / Activo` y `2 - Papel / 5.00 / Activo`. El registro
  `RS-DEMO-02` aparece como `Inactivo` y no se mezclo con los hijos del primero.
- **Estado:** Aprobado (consulta). La creacion/edicion por interfaz sigue sin
  ejecutarse y no se reporta como verificada.
- **Evidencia:** `evidencias/03_residuos_maestro_detalle.png`,
  `evidencias/15_residuos_salida.txt`.

### AC-09 - Maestro-detalle de traslados y estados

- **Objetivo:** verificar tramo hijo y ciclo de estado del traslado.
- **Pasos previstos:** en `Traslados`, abrir `TR-DEMO-01`; mostrar la pestaña
  de tramos; `Desactivar`; verificar `I`; `Reactivar`; verificar `A`.
- **Resultado observado:** sin ejecutar. No se abrio el modulo `Traslados` ni
  se cambio estado.
- **Estado:** Pendiente.
- **Evidencia:** `evidencias/04_traslados_detalle.png` y
  `evidencias/05_estados.png` quedan como evidencias previstas, no generadas.

### AC-10 - Reporte autorizado, filtro y conteo

- **Objetivo:** verificar acceso, filtro y conteo de reporte de residuos.
- **Pasos ejecutados:** sesion `admin`; `Reportes > Abrir reportes`; elegir
  `Residuos registrados`; filtrar `Activo (A)`.
- **Commit probado:** `16abf09`.
- **Resultado observado:** `Registros: 1`, `Cantidad total: 12.50` y una sola
  fila `RS-DEMO-01,Empresa Generadora Demo SAC,12.50,Baja,admin,2026-11-20T02:41:00Z,A`.
- **Estado:** Aprobado.
- **Evidencia:** `evidencias/09_reportes_conteo.png`,
  `evidencias/14_reportes_salida.txt`.

### AC-11 - Exportacion CSV

- **Objetivo:** comprobar la exportacion CSV del reporte filtrado.
- **Origen de la verificacion:** **verificacion manual realizada por el
  usuario el 19 de noviembre de 2026**. El usuario informo que la probo y
 	functiona correctamente.
- **Alcance de esta entrega:** no se volvio a ejecutar la prueba manual, no se
  abrio nuevamente el cuadro `Guardar CSV` y no se genero ni fabrico un archivo
  CSV en el repositorio. No se añaden resultados adicionales a los informados
  por el usuario.
- **Estado:** Aprobado (verificacion manual del usuario).
- **Evidencia:** este registro documental. No hay evidencia de captura ni
  archivo CSV generado por la automatizacion P5.

### AC-12 - Pruebas automatizadas y build

- **Objetivo:** verificar compilacion y suite de pruebas.
- **Commit probado:** `16abf09`.
- **Comandos ejecutados:** los de la seccion 5, con
  `C:\Qt\6.12.0\mingw_64\bin` antepuesto a `PATH` para CTest.
- **Resultado observado:**

  | Operacion | Resultado |
  |---|---|
  | Configurar build de pruebas | Correcto |
  | Compilar build de pruebas | Correcto, 36/36 pasos |
  | `ctest -N` | 1 prueba descubierta: `test_reportes` |
  | `ctest --output-on-failure` | `1/1 Test #1: test_reportes .... Passed`, `100% tests passed, 0 tests failed out of 1` |
  | Configurar build de aplicacion | Correcto |
  | Compilar aplicacion (`BUILD_TESTING=OFF`) | Correcto, 28/28 pasos |

- **Primer intento:** CTest termino con `0xc0000135` porque no se encontraban
  DLL de Qt. Se corrigio anteponiendo `C:\Qt\6.12.0\mingw_64\bin` a `PATH`; el
  resultado final fue el indicado en la tabla.
- **Verificacion integral final:** se ejecuto una sola vez al cierre; CMake
  reconfiguro ambos builds, Ninja informo `no work to do` porque ya estaban
  compilados, `ctest -N`± displayed `test_reportes` y CTest termino
  `1/1 passed` en `0.03 s`. Ademas verifico los seis entregables P5, la rama
  actual y el hash de la base original.
- **Estado:** Aprobado.
- **Evidencia:** `evidencias/11_ctest_salida.txt`,
  `evidencias/12_build_salida.txt`, `evidencias/17_verificacion_integral.txt`.

## 5. Comandos de verificacion

```bash
git status --short --branch
git branch --show-current
git log --oneline -n 8

cmake -S GRAT_Software -B build-rubrica -G Ninja -DBUILD_TESTING=ON
cmake --build build-rubrica
ctest --test-dir build-rubrica -N
ctest --test-dir build-rubrica --output-on-failure

cmake -S GRAT_Software -B build-rubrica-app -G Ninja -DBUILD_TESTING=OFF
cmake --build build-rubrica-app
```

No se ejecuto `git diff improvements/final...HEAD` porque la rama destino no
existe; la ausencia se registro como INC-01.

## 6. Aislamiento de datos

- La base de la demo se creo en
  `C:\Users\USER\AppData\Local\Temp\opencode\grat_demo\db\grat.db` con
  `tools/demo/prepare_demo_db.py`.
- El intento de usar `C:\Users\USER\GRAT_Software\db` fue rechazado por el
  script con: `ERROR: el directorio de demostracion debe estar fuera del
  repositorio.`
- Hash SHA-256 de `GRAT_Software/db/grat.db` antes y despues de la demostracion:
  `E984D59668BC24823D286B1AFEA710ADD7165D8331FF516EA7CB925F596B4A9D`.
- `PRAGMA foreign_key_check` sobre la base aislada devolvio un resultado vacio.

## 7. Registro de incidencias

| ID | Incidencia | Estado | Accion |
|---|---|---|---|
| INC-01 | `improvements/final` no existe en `origin` | Abierta | Se uso `origin/main` como base real; no se creo ni modifico otra rama. |
| INC-02 | `rubrica/distribucion_mejoras_rubrica.md` no existe en el repositorio ni en el historial disponible | Abierta | Se sigue el prompt P5 como fuente; no se atribuyen autores no confirmados. |
| INC-03 | `origin/main` avanzo a `feb53db` despues de la aceptacion | Registrada | El cambio de `feature/stragy-polimorfismo` no fue incluido en el binario probado; no se afirma su verificacion. |
| INC-04 | El primer CTest fallo por DLL de Qt no localizada | Resuelta | Se anadio `C:\Qt\6.12.0\mingw_64\bin` a `PATH`; CTest finalizo `1/1 passed`. |
| INC-05 | Exportacion CSV no se ejecuto por automatizacion | Cerrada por el usuario | Verificada manualmente por el usuario el 19 de noviembre de 2026; no se abrio de nuevo el dialogo. |
| INC-06 | Escenarios de roles, validacion y estados no ejecutados | Pendiente | Se conservan como `Pendiente`; no se generaron capturas ni resultados artificiales. |

## 8. Condiciones para publicar

Antes de hacer push o abrir el PR deben cumplirse:

1. Los cuatro commits sugeridos para P5 estan presentes y no vacios.
2. La verificacion automatizada integral termina correctamente.
3. No existen cambios no intencionados en codigo productivo, esquema SQLite,
   permisos, contraseñas, UI, traduccion ni bases originales.
4. La rama destino `improvements/final` existe en `origin`.

Mientras la condicion 4 no se cumpla, los commits se conservan localmente y no
se crea el PR. No se realiza merge.

## 9. Integracion con `origin/main` y verificacion final

Tras la instruccion del usuario de usar `origin/main` como base y destino del
PR, se hicieron `git fetch origin` y una revision de los cinco commits nuevos:

| Commit | Descripcion |
|---|---|
| `3ef32b7` | Desacoplar las consultas de existencia del validador |
| `a336885` | Extraer reglas de campos con una interfaz comun |
| `e1a5f20` | Componer estrategias para unicidad y relaciones hijas |
| `77a9ee1` | Documentar el polimorfismo y la propiedad de las reglas |
| `feb53db` | Merge del PR #10 (`feature/stragy-polimorfismo`) |

El cambio se incorporo con `git merge --no-ff origin/main`, generando el merge
local `71fda80`. No hubo conflictos y no se reescribio el historial ajeno. El
merge toco unicamente `GRAT_Software/CMakeLists.txt`,
`GRAT_Software/cmake/features/validacion.cmake`,
`GRAT_Software/docs/arquitectura/strategy_poo.md` y archivos bajo
`GRAT_Software/src/core/crud/`; no toco `docs/presentacion/` ni
`tools/demo/`.

Los seis entregables P5 se compararon con `git hash-object` antes y despues del
merge y permanecieron **byte a byte intactos**:

| Archivo | Hash |
|---|---|
| `docs/presentacion/guion_demo.md` | `f0536fb595c04be9c73ae0942969df8e81edb2c3` |
| `docs/presentacion/datos_demo.md` | `868b641b6c1f059cf27cca32f48c0f698c6f626f` |
| `docs/presentacion/aceptacion_final.md` | `88ba60d2cfae3d4133ae6557647c6c1af998e65d` |
| `docs/presentacion/matriz_rubrica.md` | `824adf89ac9d92f5bb5fc78b6f671ea56df7a763` |
| `docs/presentacion/evidencias/README.md` | `66686d5f9dce959ddaee7d26fe944dacaa3fadb7` |
| `tools/demo/prepare_demo_db.py` | `b0a078e98f6216c9e22c55d0496d52864430ae05` |

Nota: los hashes de los tres documentos actualizados en esta seccion
corresponden al estado anterior al addendo; el contenido anterior se conserva y
se amplia.

### 9.1 Verificacion automatizada integral sobre la version integrada

Se ejecuto **una sola vez** tras el merge, con `C:\Qt\6.12.0\mingw_64\bin` en
`PATH`:

| Operacion | Resultado |
|---|---|
| Configurar build de pruebas | Correcto |
| Compilar build de pruebas con las nuevas reglas de validacion | Correcto, 16/16 pasos |
| `ctest -N` | 1 prueba descubierta: `test_reportes` |
| `ctest --output-on-failure` | `1/1 Test #1: test_reportes .... Passed`, `100% tests passed, 0 tests failed out of 1` |
| Configurar build de aplicacion | Correcto |
| Compilar aplicacion (`BUILD_TESTING=OFF`) | Correcto, 16/16 pasos |
| Comprobacion de los seis entregables P5 | Todos presentes |
| Hash de `GRAT_Software/db/grat.db` | `E984D59668BC24823D286B1AFEA710ADD7165D8331FF516EA7CB925F596B4A9D` (sin cambios) |

Evidencia: `evidencias/19_integracion_ctest.txt`,
`evidencias/20_integracion_build.txt`,
`evidencias/21_integracion_git.txt`,
`evidencias/22_integracion_entregables.txt` y
`evidencias/23_integracion_resumen.txt`.

### 9.2 Estado de casos tras la integracion

- No se ejecutaron nuevas pruebas graficas. No se abrio el cuadro
  `Guardar CSV`.
- AC-11 conserva su estado de **Aprobado (verificacion manual del usuario)**.
- AC-02, AC-03, AC-04, AC-05, AC-06, AC-07 y AC-09 permanecen **Pendiente**.
- AC-12 se considera verificado sobre la version integrada por la seccion 9.1.
- INC-01 queda resuelta: el destino es `origin/main`.
- INC-03 queda resuelta: los cinco commits nuevos ya estan integrados y
  compilados.
- INC-02 permanece abierta: no existe el archivo de rubrica oficial.

La rama local quedo con cinco commits propios (los cuatro de P5 mas el merge)
y lista para push y PR hacia `main`. No se realizo merge del Pull Request.
