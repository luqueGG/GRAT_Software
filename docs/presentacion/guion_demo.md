# Guion de demostracion - GRAT_Software (P5)

Este guion organiza una exposicion de **11 minutos 30 segundos**, ajustable a
10-12 minutos. El responsable de la sesion es **P5 (Jaime Igreda)**, que
presenta, encadena las pantallas y cierra. Cada integrante P1-P6 puede explicar
su bloque usando la columna *Responsable* y la evidencia asociada.

> Los nombres de los demas integrantes no se asignan a P1-P4/P6 porque el
> documento `rubrica/distribucion_mejoras_rubrica.md` no esta presente en el
> repositorio ni en el historial disponible. El README si enumera a Jhon Cuyo,
> Wilber Guerra, Gerson Luque, Diego Santa, Jaime Igreda y Cristhian Bravo,
> pero no los mapea con las etiquetas P1-P6. Para no atribuir trabajos
> retrospectiveamente se usa la etiqueta funcional.

## 0. Reglas de la demo

- Ejecutar siempre con la base aislada descrita en [`datos_demo.md`](datos_demo.md).
- No usar `GRAT_Software/db/grat.db`.
- Mostrar una accion, esperar a que se vea el resultado y pasar a la siguiente.
- Si falla el entorno grafico, usar el procedimiento de
  [`evidencias/README.md`](evidencias/README.md).

---

## 5.1 Introduccion (1 min 00 s)

| Campo | Contenido |
|---|---|
| Responsable | P5 |
| Explicacion | GRAT_Software es una aplicacion de escritorio para la gestion responsable de residuos de transporte. Resuelve la dispersion de catalogos, registros y traslados en una sola herramienta con acceso por permisos. |
| Pantalla | Dialogo de inicio de sesion |
| Accion | Abrir la aplicacion desde el directorio aislado y mostrar el titulo `GRAT_Software - Iniciar sesion`. |
| Resultado esperado | Se ve el formulario de acceso con `Usuario`, `Contrasena` y el subtitulo `Gestion Responsable de Residuos de Transporte`. |
| Evidencia | `evidencias/01_login_admin.png` |

Guion sugerido: "La aplicacion centraliza catalogos, residuos, constituyentes,
traslados y administracion de roles. Hoy veremos el ciclo completo con datos
ficticios, desde el acceso hasta la exportacion de un reporte."

---

## 5.2 Arquitectura (1 min 20 s)

| Campo | Contenido |
|---|---|
| Responsable | P5 introduce; P1 y P2 amplian |
| Explicacion | Cliente Qt Widgets + SQLite. La vista esta en `src/ui/`, los metadatos de tablas en `src/core/crud/table_specs.cpp`, la persistencia en `TableGateway`/`SqlRecordRepository`, la autorizacion en `PermissionChecker` y la sesion en `app/session.h`. P1 incorporo MVC con `CrudController`/`ChildCrudController` y Repository con `IRecordRepository`/`SqlRecordRepository`. P2 aporta polimorfismo y ciclo de vida de dependencias. El Observer de `Database::connectionChanged` ya existia y se conserva. |
| Pantalla | Ventana principal, menu `Modulos` y barra de estado |
| Accion | Entrar como `admin`, recorrer el arbol de navegacion y senalar la barra de estado `Base de datos conectada`, `Usuario` y `Rol(es)`. |
| Resultado esperado | La navegacion muestra los seis modulos permitidos y la barra de estado refleja la sesion real. |
| Evidencia | `evidencias/02_menu_modulos_admin.png`; referencia: `docs/arquitectura/mvc_repository.md` |

No afirmar que existe una unidad de trabajo completa: el documento de P1 define
el limite del repositorio generico apoyado en `QVariantMap`.

---

## 5.3 Gestion de residuos (2 min 30 s)

| Campo | Contenido |
|---|---|
| Responsable | P5, con apoio de P1/P2 |
| Explicacion | El registro principal `waste_waste` usa una rejilla y un formulario inferior; sus constituyentes se gestionan como detalle vinculado al residuo seleccionado. El traslado usa el mismo patron con sus tramos de transporte. |
| Pantalla | `Residuos > Residuos` y `Transporte > Traslados` |
| Accion | 1) Abrir `Residuos`. 2) Senalar el registro `RS-DEMO-01`. 3) Cambiar a la pestaña `Constituents` y mostrar los dos hijos. 4) Abrir `Traslados` y mostrar `TR-DEMO-01` con su tramo. 5) Usar `Desactivar` y `Reactivar` en el registro de demostracion. |
| Resultado esperado | El hijo queda filtrado por el padre, la relacion se ve en la rejilla y el estado cambia de `A` a `I` y vuelve a `A`. |
| Evidencia | `evidencias/03_residuos_maestro_detalle.png`, `evidencias/04_traslados_detalle.png`, `evidencias/05_estados.png` |

El cambio de estado es logico (`is_active`); no se borra la fila.

---

## 5.4 Autenticacion y permisos (2 min 00 s)

| Campo | Contenido |
|---|---|
| Responsable | P5, con apoio de P1 |
| Explicacion | Los permisos efectivos son union de los permisos del grupo y los directos del usuario. `Administrator` tiene 43, `Supervisor` 36 y `Operator` 19. La navegacion y los botones se ocultan segun `PermissionChecker`; la verificacion tambien se ejecuta en controladores y servicios. |
| Pantalla | Login, ventana principal, `Administracion > Roles`, sesion `operator` |
| Accion | 1) Entrar como `operator` y mostrar que no aparecen `Usuarios` ni `Administracion` y que en `Paises` no hay botones de escritura. 2) Entrar como `admin`, abrir `Administracion > Roles`, seleccionar `Supervisor` y revocar `add_constituent`. 3) Cerrar sesion y reingresar como `supervisor` para mostrar que el alta ya no aparece. |
| Resultado esperado | Operacion permitida: `admin` modifica permisos. Operacion denegada: `operator` no ve administracion ni botones de alta. El cambio de permisos se refleja **al iniciar una nueva sesion**; no se actualiza automaticamente una sesion ya abierta. |
| Evidencia | `evidencias/06_operador_permisos.png`, `evidencias/07_roles_revocar.png` |

---

## 5.5 Validaciones (1 min 10 s)

| Campo | Contenido |
|---|---|
| Responsable | P5, con apoio de P3 |
| Explicacion | El validador se ejecuta antes de escribir. Los errores se asocian a columnas y se muestran juntos; la operacion queda pendiente para corregir. |
| Pantalla | `Ubicaciones > Paises` |
| Accion | Como `admin`, pulsar `Agregar`, dejar `Codigo (Alfa-2)` vacio, completar solo el nombre y pulsar `Actualizar`. |
| Resultado esperado | Aparece `Validation failed` con el mensaje del campo requerido; el conteo de paises no aumenta. |
| Evidencia | `evidencias/08_validacion.png` |

---

## 5.6 Reportes (1 min 40 s)

| Campo | Contenido |
|---|---|
| Responsable | P5, con apoio de P4 |
| Explicacion | Reportes de solo lectura sobre residuos y traslados, protegidos por `view_waste`/`view_transfer`. Permiten filtro por estado, conteo, suma de cantidades y exportacion CSV del conjunto filtrado. |
| Pantalla | `Reportes > Abrir reportes` |
| Accion | 1) Elegir `Residuos registrados`. 2) Filtrar `Activo (A)`. 3) Leer `Registros` y `Cantidad total`. 4) Exportar a `reportes_residuos_activos.csv` en la carpeta de evidencias. 5) Comparar el numero de filas del CSV con el conteo mostrado. |
| Resultado esperado | El conteo y las filas exportadas coinciden. El CSV usa UTF-8 con BOM y CRLF. |
| Evidencia | `evidencias/09_reportes_conteo.png`, `evidencias/10_exportacion_csv.png`, `evidencias/reportes_residuos_activos.csv`; impacto documentado en `docs/innovacion/reportes_impacto.md` |

La metrica determinista es la coincidencia filas filtradas = filas exportadas.
No se presenta una estimacion de tiempo como medicion estadistica.

---

## 5.7 Calidad (1 min 00 s)

| Campo | Contenido |
|---|---|
| Responsable | P5 muestra; P3 explica |
| Explicacion | Las pruebas automaticas quedan en `tests/`. Se ejecutan con CTest sobre el build de pruebas; el build de la aplicacion es independiente. |
| Pantalla | Terminal |
| Accion | Mostrar `ctest --test-dir build-rubrica -N` y luego `ctest --test-dir build-rubrica --output-on-failure`. |
| Resultado esperado | Se listan las pruebas descubiertas y el resultado real de la ejecucion se registra en `aceptacion_final.md`. |
| Evidencia | `evidencias/11_ctest_salida.txt`, `evidencias/12_build_salida.txt` |

---

## 5.8 Herramientas (0 min 50 s)

| Campo | Contenido |
|---|---|
| Responsable | P5, con apoio de P6 |
| Explicacion | Git/GitHub para ramas y PR; CMake + Ninja para compilar; Qt Test y CTest para pruebas. No hay flujo de GitHub Actions en las ramas revisadas, por lo que **no se menciona como funcionando**. |
| Pantalla | Terminal y `git log` |
| Accion | Mostrar la rama `docs/rubrica-demo-evidencias`, los commits P5 y los comandos de compilacion. |
| Resultado esperado | Se distingue claramente entre commit local, push y PR. |
| Evidencia | `evidencias/13_git_estado.txt`; seccion `Verificacion` de `aceptacion_final.md` |

---

## 5.9 Conclusiones (0 min 50 s)

| Campo | Contenido |
|---|---|
| Responsable | P5 cierra |
| Explicacion | P1 arquitectura MVC/Repository, P2 POO avanzada, P3 calidad, P4 innovacion y funcionalidad, P5 demostracion y aceptacion, P6 herramientas de compilacion y pruebas. Cada bloque se vincula en `matriz_rubrica.md` con rama, commits y evidencia. |
| Pantalla | Documento `matriz_rubrica.md` |
| Accion | Recorrer la matriz de seis criterios y senalar que la entrega P5 no modifica codigo productivo. |
| Resultado esperado | Cierre en menos de un minuto, sin afirmar integraciones que no existen. |
| Evidencia | [`matriz_rubrica.md`](matriz_rubrica.md), [`aceptacion_final.md`](aceptacion_final.md) |

---

## Ajuste de duracion

- **Si piden 10 minutos:** recortar 20 s de 5.2, 20 s de 5.7 y 10 s de 5.9.
- **Si piden 12 minutos:** ampliar 30 s la narracion de 5.3 y 30 s la de 5.4.

## Dependencias de esta demo

1. Base aislada preparada con `tools/demo/prepare_demo_db.py`.
2. Binario compilado desde `origin/main` (`16abf09`) porque la rama
   `improvements/final` no existe en el remoto al preparar esta entrega.
3. Reportes integrados desde P4 y pruebas CTest desde P6 ya presentes en `main`.
