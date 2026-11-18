# Clasificación de warnings de compilación

- **Fecha:** 2026-11-18
- **Responsable:** Jhon
- **Rama:** `feature/build-docs`
- **Commit base:** `8469c1a`
- **Compilador:** `g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0`
- **Configuración:** `cmake -G Ninja -B build -S GRAT_Software -DCMAKE_CXX_FLAGS="-Wall -Wextra"`
- **Comando de build:** `cmake --build build --clean-first`
- **Log analizado:** `build_antes.log`

## Verificación de que las flags se aplicaron

```text
$ grep -m2 "FLAGS = " build/build.ninja
  FLAGS = -Wall -Wextra -std=c++20
  FLAGS = -Wall -Wextra -std=c++20
```

Compilación directa con `/usr/bin/c++`, sin `ccache` ni launcher intermedio, por
lo que los diagnósticos no pueden venir de una caché.

## Resultado del análisis

```text
$ grep -c "warning:" build_antes.log
0
$ grep -c "error:" build_antes.log
0
```

| Archivo | Línea | Tipo | Dueño | Acción |
|---|---|---|---|---|
| — | — | Sin warnings | — | Ninguna |

**No hay warnings que clasificar ni corregir.**

## Notas

- El árbol de trabajo contiene además cambios de fin de línea (LF→CRLF) en 34
  archivos ajenos a esta rama que **no se han modificado ni commiteado**. No
  afectan a los diagnósticos del compilador.
- El único warning previsible con `-Wextra` en este código era
  `missing initializer for member 'TableSpec::children'` en `table_specs.cpp`.
  Ya estaba corregido en el árbol de trabajo y se incorporó en el commit
  `8469c1a` (9 líneas `.children = {}`), por lo que no aparece en
  `build_antes.log`.
- **Pendientes de otras ramas:** ninguno.
