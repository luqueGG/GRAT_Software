# Matriz de rubrica y evidencias - P5

Estado: **matriz construida con fuentes verificables del repositorio**. El
archivo `rubrica/distribucion_mejoras_rubrica.md` no existe en el repositorio
ni en el historial disponible, por lo que no se puede reproducir literalmente
la distribucion oficial ni atribuir responsables no confirmados. Las etiquetas
`P1` a `P6` se usan solo como referencia operativa del enunciado de la tarea.

Fuentes consultadas:

- `git log` de `origin/main` y de las ramas `feature/*` y `reportes-operativos`.
- `git ls-tree` de esas ramas para confirmar documentos y pruebas.
- Commits P5 locales de `docs/rubrica-demo-evidencias`.
- Resultados reales registrados en `aceptacion_final.md`.

## 1. Matriz de criterios

| Criterio / etiqueta operativa | Aporte localizado en Git | Evidencia documental | Verificacion P5 | Estado P5 |
|---|---|---|---|---|
| Arquitectura MVC, repositorio y observer (P1) | `origin/feature/rubrica-mvc-repository`; commits `240d6b0`, `e1862ff`, `39eeefb`, `06e0cbb`, `6f4229c` | `GRAT_Software/docs/arquitectura/mvc_repository.md`; capturas de navegacion por rol en `GRAT_Software/docs/verificacion/capturas/` | AC-01 confirma navegacion del rol Administrador; AC-08 confirma relacion maestro-detalle; AC-10 confirma reporte con permiso | Aportes localizados; verificacion funcional parcial |
| Diseno POO y estrategias de validacion (P2) | `origin/feature/stragy-polimorfismo`; commits `3ef32b7`, `a336885`, `e1a5f20`, `77a9ee1`; merge en `main` `feb53db` | `GRAT_Software/docs/arquitectura/strategy_poo.md`; archivos `src/core/crud/validation/i_validation_rule.*`, `field_validation_rules.*`, `record_validation_rules.*` | No verificada: este cambio se fusiono en `origin/main` despues de que se ejecutara la aceptacion sobre `16abf09` | Pendiente; no se afirma verificacion de `feb53db` |
| Calidad, manejo de errores y resiliencia (P3) | `origin/feature/error-handling`; commits `7eeca6c`, `14550ec`, `a5638a7`, `dad2703` | `GRAT_Software/docs/verificacion/smoke_test.md`, `login_roles.md`, `build_warnings.md` | AC-12 confirma compilacion y prueba automatizada; no se ejecutaron los escenarios de fallo de base de datos | Aportes localizados; escenarios de fallo pendientes en P5 |
| Innovacion funcional y reportes operativos (P4) | `origin/reportes-operativos`; commits `e7015da`, `b2b53ce`, `140f148`, `8c5c779` | `GRAT_Software/docs/innovacion/reportes_impacto.md`; `GRAT_Software/tests/reportes/test_reportes.cpp` | AC-10 aprobado con filtro `Activo (A)`, `Registros: 1` y `Cantidad total: 12.50`; AC-11 verificado manualmente por el usuario el 19-11-2026 | Aprobado segun los casos ejecutados y la confirmacion manual recibida |
| Demostracion, datos y aceptacion (P5) | Rama `docs/rubrica-demo-evidencias`; commits `222c45e`, `e4361a9`, `e48bcb1`, `f4a1c2d` | `docs/presentacion/guion_demo.md`, `datos_demo.md`, `aceptacion_final.md`, `matriz_rubrica.md`, `evidencias/` | Guion y datos reproducibles completos; aceptacion ejecutada de forma parcial con resultados reales | Entregable completado documentalmente; casos AC-02 a AC-07 y AC-09 pendientes |
| Herramientas, build y verificacion (P6) | `origin/feature/build-docs`; commits `c05bddd`, `8469c1a`, `4c32ab1`, `f6216fd`, `9f2678c`, `37e43ec`, `0e8ca8b`, `da145a9` | `GRAT_Software/docs/verificacion/build_antes.log`, `build_despues.log`, `build_warnings.md`; `README.md` de build y usuarios | AC-12 aprobado: configuracion y build correctos; CTest `1/1 passed`; compilacion de app `28/28` pasos | Aprobado. No existe flujo de GitHub Actions en las ramas revisadas |

## 2. Relacion de evidencias por caso

| Caso | Evidencia P5 | Origen |
|---|---|---|
| AC-01 | `evidencias/01_login_admin.png`, `evidencias/02_menu_modulos_admin.png`, `evidencias/16_admin_salida.txt` | Ejecucion real con la base aislada |
| AC-08 | `evidencias/03_residuos_maestro_detalle.png`, `evidencias/15_residuos_salida.txt` | Ejecucion real; solo consulta de la relacion |
| AC-10 | `evidencias/09_reportes_conteo.png`, `evidencias/14_reportes_salida.txt` | Ejecucion real del filtro |
| AC-11 | Registro en `aceptacion_final.md` | Verificacion manual del usuario, 19-11-2026; sin archivo generado por P5 |
| AC-12 | `evidencias/11_ctest_salida.txt`, `evidencias/12_build_salida.txt`, `evidencias/17_verificacion_integral.txt` | Ejecucion real de CMake, Ninja y CTest |
| AC-02 a AC-07, AC-09 | Sin evidencia generada | No ejecutados; no se fabricaron capturas ni resultados |

## 3. Trazabilidad de P5

| Commit P5 | Contenido | Relacion con la matriz |
|---|---|---|
| `222c45e` | Guion y escenarios de demostracion | Casos AC-01 a AC-12 y tiempos de la demo |
| `e4361a9` | Datos ficticios y script de preparacion | Aislamiento de la base y reproducibilidad de AC-01, AC-08 y AC-10 |
| Commit 3 (`22178a1`) | Resultados de aceptacion y registro de incidencias | Estados reales, incluidos pendientes y verificacion manual del usuario |
| Commit 4 (hash por confirmar) | Matriz de rubrica e indice de evidencias | Cierre documental de la entrega |

El hash del commit 3 es `22178a1`. El hash del commit 4 se confirma con
`git log` despues de crearlo. No se crean commits vacios.

## 4. Fuera del alcance de P5

- No se modifico codigo productivo, esquema SQLite, los 43 permisos,
  contraseñas o hashes de usuarios.
- No se modifico la interfaz ni las traducciones. El PR #6
  (`feature/redisenio-ui-espanol`) queda como trabajo separado y no fue
  fusionado por esta entrega.
- No se modification `GRAT_Software/db/grat.db`; su hash se mantuvo en
  `E984D59668BC24823D286B1AFEA710ADD7165D8331FF516EA7CB925F596B4A9D`.
- No se abrio PR ni se realizo merge. La rama destino `improvements/final` no
  existe en `origin`, por lo que la publicacion queda condicionada.
