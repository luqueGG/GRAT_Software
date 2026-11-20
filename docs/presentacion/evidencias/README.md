# Indice de evidencias P5

Este directorio contiene evidencias **reales** de la demostracion y la
verificacion. No se han fabricado capturas ni resultados. Los archivos que no
se generaron se listan como previstos/pendientes al final de este indice.

## 1. Capturas generadas

| Archivo | Escenario | Estado |
|---|---|---|
| `01_login_admin.png` | Inicio de sesion con `admin` | Generada el 19-11-2026 |
| `02_menu_modulos_admin.png` | Modulos visibles para `Administrador` | Generada el 19-11-2026 |
| `03_residuos_maestro_detalle.png` | Residuo `RS-DEMO-01` con sus dos hijos | Generada el 19-11-2026 |
| `09_reportes_conteo.png` | Reporte filtrado `Activo (A)` | Generada el 19-11-2026 |

Las imagenes `dump_main.png`, `inspect_reportes.png` e `inspect_residuos.png`
son capturas diagnosticas de la inspeccion de interfaz. Se conservan por
trazabilidad, pero **no** deben usarse como evidencia principal de la
presentacion.

## 2. Registros de texto

| Archivo | Contenido |
|---|---|
| `11_ctest_salida.txt` | Salida de CTest con `1/1 passed` |
| `12_build_salida.txt` | Configuracion y compilacion de pruebas y aplicacion |
| `13_git_estado.txt` | Rama, commits y archivos modificados al ejecutar la verificacion integral, antes de los commits finales |
| `14_reportes_salida.txt` | Lectura real del filtro de reportes |
| `15_residuos_salida.txt` | Lectura real del maestro-detalle de residuos |
| `16_admin_salida.txt` | Estado del rol y arbol de modulos del administrador |
| `17_verificacion_integral.txt` | Salida de la verificacion automatizada integral final |
| `18_aislamiento.txt` | Comprobacion de rutas y hash de la base original |
| `19_integracion_ctest.txt` | CTest sobre la version integrada con `origin/main` |
| `20_integracion_build.txt` | CMake/Ninja sobre la version integrada |
| `21_integracion_git.txt` | Rama, commits y hash de la base tras el merge |
| `22_integracion_entregables.txt` | Comprobacion de los seis entregables P5 |
| `23_integracion_resumen.txt` | Resumen de la verificacion integral post-merge |

Los archivos `11` a `18` corresponden a la aceptacion sobre `16abf09`; los
archivos `19` a `23` corresponden a la verificacion unica realizada despues de
integrar `origin/main`.

Los archivos `11_ctest_salida.txt`, `12_build_salida.txt` y
`17_verificacion_integral.txt` pueden contener parte del mismo resultado: el
primero corresponde a CTest, el segundo a CMake/Ninja y el tercero a la
ejecucion unica de verificacion integral solicitada al cierre.

## 3. Evidencias previstas pero no generadas

| Archivo previsto | Motivo |
|---|---|
| `04_traslados_detalle.png` | Escenario AC-09 no ejecutado |
| `05_estados.png` | Ciclo Desactivar/Reactivar no ejecutado |
| `06_operador_permisos.png` | Sesion `operator` no ejecutada |
| `07_roles_revocar.png` | Modificacion de permisos no ejecutada |
| `08_validacion.png` | Escenarios AC-06 y AC-07 no ejecutados |
| `10_exportacion_csv.png` | No se abrio nuevamente el dialogo `Guardar CSV`, segun instruccion del usuario |
| `reportes_residuos_activos.csv` | No se genero archivo por automatizacion; la exportacion fue verificada manualmente por el usuario el 19-11-2026 |

## 4. Regla de verificacion

Todo resultado marcado como `Aprobado` en `../aceptacion_final.md` cuenta con
ejecucion real o con la verificacion manual explicitamente atribuida al
usuario. Los casos sin evidencia permanecen como `Pendiente`.
