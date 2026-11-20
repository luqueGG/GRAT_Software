# Datos ficticios reproducibles - GRAT_Software (P5)

Este escenario permite demostrar el sistema con datos inventados, sin tocar la
base habitual `GRAT_Software/db/grat.db`.

## 1. Fuente real consultada

Antes de disenar los datos se inspecciono el esquema original
`GRAT_Software/resources/grat_schema.sqlite.sql` de `origin/main` (`16abf09`).

Entidades y relaciones confirmadas:

| Entidad | Campos usados en la demo | Restricciones relevantes |
|---|---|---|
| `location_country` | `alpha_2`, `alpha_3`, `numerical_code`, `name` | PK `alpha_2`; `alpha_3` unico |
| `location_subdivision` | `subdiv_code`, `country_alpha_2_id`, `type_subdivision`, `name` | PK `subdiv_code`; FK a pais con `ON DELETE CASCADE` |
| `catalog_constituent` | `id`, `name` | PK autoincremental |
| `catalog_containertype` | `id`, `name`, `description` | PK autoincremental |
| `catalog_toxicitylevel` | `id`, `name`, `description` | PK autoincremental |
| `catalog_transporttype` | `id`, `name`, `description` | PK autoincremental |
| `waste_generatorcompany` | `id_gen_company`, `name_company`, `city_id`, `activity` | PK texto; FK a subdivision |
| `waste_waste` | `id_waste`, `id_gen_company_id`, `amount_waste`, `id_toxicity_id`, `created_by_id`, `is_active` | PK texto; FK a generadora, toxicidad y usuario; `amount_waste >= 0` |
| `waste_wasteconstituent` | `id_waste_id`, `id_constituent_id`, `amount_constituent`, `is_active` | FK a residuo y componente; `amount_constituent >= 0` |
| `transfer_destination` | `id`, `name`, `city_id` | PK autoincremental; FK a subdivision |
| `transfer_travelcompany` | `id_travel_company`, `name`, `city_id` | PK texto; FK a subdivision |
| `transfer_transfer` | `id_transfer`, `id_travel_company_id`, `id_destination_id`, `id_waste_id`, `id_container_type_id`, `amount_waste`, `delivery_date`, `created_by_id`, `is_active` | PK texto; FK a transportista, destino, residuo, contenedor y usuario; `amount_waste >= 0` |
| `transfer_transfertravelcompany` | `id_transfer_id`, `id_travel_company_id`, `id_type_transport_id`, `cost`, `kms` | FK a traslado, transportista y tipo de transporte; `cost >= 0`, `kms >= 0` |

No se inventaron campos ni relaciones. Todos los valores usados existen en las
restricciones del esquema.

Estados logicos confirmados en README y validador: `A` activo, `I` inactivo,
`*` eliminado. Los borrados son logicos mediante `is_active`.

## 2. Usuarios de demostracion

Los usuarios los crea la propia aplicacion mediante `AuthService` en el primer
arranque de una base vacia. El script P5 **no calcula hashes** y solo consulta
el `id` de `admin` para completar `created_by_id`.

| Usuario | Contraseña documentada | Rol | Permisos |
|---|---|---|---|
| `admin` | `Admin123!` | Administrator | 43 / 43 |
| `supervisor` | `Supervisor123!` | Supervisor | 36 (antes de la revocacion de la demo) |
| `operator` | `Operator123!` | Operator | 19 |

Estas credenciales son las publicadas en `README.md` para desarrollo; no son
credenciales personales.

## 3. Escenario ficticio

### 3.1 Ubicaciones y catalogos

| Tabla | Registro |
|---|---|
| `location_country` | `PE` / `PER` / `604` / `Peru` |
| `location_subdivision` | `LIMA` / `PE` / `Departamento` / `Lima` |
| `catalog_constituent` | 1 = `Plastico`; 2 = `Papel` |
| `catalog_containertype` | 1 = `Bidon de 200 L`; 2 = `Saco de 25 kg` |
| `catalog_toxicitylevel` | 1 = `Baja`; 2 = `Media` |
| `catalog_transporttype` | 1 = `Terrestre`; 2 = `Maritimo` |

### 3.2 Residuos y constituyentes

| Residuo | Generadora | Cantidad | Toxicidad | Creado por | Estado | Constituyentes |
|---|---|---:|---|---|---|---|
| `RS-DEMO-01` | `GC-DEMO-01` | 12.50 | 1 (`Baja`) | `admin` | `A` | 1 `Plastico` 7.50; 2 `Papel` 5.00 |
| `RS-DEMO-02` | `GC-DEMO-01` | 3.00 | 2 (`Media`) | `admin` | `I` | 1 `Plastico` 3.00 |

`GC-DEMO-01` = `Empresa Generadora Demo SAC`, ubicada en `LIMA`, actividad
`Mantenimiento industrial ficticio`.

El segundo residuo permite comprobar que el panel de constituyentes muestra
solo los hijos del padre seleccionado y que el filtro de reportes `A` excluye
el registro inactivo.

### 3.3 Traslado y tramo

| Traslado | Residuo | Transportista | Destino | Contenedor | Cantidad | Entrega | Estado |
|---|---|---|---|---|---:|---|---|
| `TR-DEMO-01` | `RS-DEMO-01` | `TT-DEMO-01` | 1 `Planta de tratamiento demo` | 1 `Bidon de 200 L` | 12.50 | 2026-11-20 | `A` |

Tramo hijo: `TT-DEMO-01`, tipo de transporte 1 (`Terrestre`), costo 150.00,
42.00 km.

`TT-DEMO-01` = `Transportes Demo SAC`, ubicada en `LIMA`.

### 3.4 Casos de permisos

| Caso | Usuario | Resultado esperado |
|---|---|---|
| Abrir `Administracion > Roles` | `admin` | Permitido |
| Abrir `Administracion > Roles` | `operator` | Modulo no visible |
| Boton `Agregar` en `Paises` | `operator` | No visible ( Rolle solo `view_country`) |
| Revocar `add_constituent` a `Supervisor` | `admin` | Operacion permitida |
| Boton `Agregar` en `Componentes` tras revocar | `supervisor` (nueva sesion) | No visible |
| Abrir reporte de residuos | `admin` | Permitido (`view_waste`) |

Los cambios de permisos se reflejan al iniciar una nueva sesion; una sesion ya
abierta conserva la copia de permisos cargada al hacer login, segun lo
documentado por P1.

### 3.5 Casos de validacion

| Caso | Pantalla | Accion | Resultado esperado |
|---|---|---|---|
| Obligatorio vacio | `Paises` | `Agregar` sin `Codigo (Alfa-2)` | `Validation failed`, sin insercion |
| Duplicado | `Paises` | Repetir `PE` | Rechazo por duplicado, fila original intacta |
| Decimal negativo | `Residuos` | Cantidad `-1` | Rechazo por valor no negativo |
| Longitud excedida | `Paises` | Codigo de 3 caracteres | Rechazo por maxima de 2 |

## 4. Procedimiento reproducible

Los comandos se ejecutan desde la raiz del repositorio. Se usa un directorio
aislado fuera del repo.

### 4.1 Inicio

```bash
python tools/demo/prepare_demo_db.py prepare --dir "C:/Users/USER/AppData/Local/Temp/opencode/grat_demo" --force
```

Salida real verificada: `BASE_CREADA=.../grat_demo/db/grat.db`.

A continuacion se inicia **una sola vez** la aplicacion desde esa carpeta y se
cierra el dialogo de login con `Cancel`. Eso hace que `AuthService` cree los
tres usuarios, sin que P5 implemente ningun hash.

```powershell
Start-Process -FilePath "C:\Users\USER\GRAT_Software\build-rubrica-app\grat_software.exe" `
  -WorkingDirectory "C:\Users\USER\AppData\Local\Temp\opencode\grat_demo"
```

Despues:

```bash
python tools/demo/prepare_demo_db.py finalize --dir "C:\Users\USER\AppData\Local\Temp\opencode/grat_demo"
```

Salida real verificada:

```text
CONTEOS=location_country:1, location_subdivision:1, catalog_constituent:2,
catalog_containertype:2, catalog_toxicitylevel:2, catalog_transporttype:2,
waste_generatorcompany:1, waste_waste:2, waste_wasteconstituent:3,
transfer_destination:1, transfer_travelcompany:1, transfer_transfer:1,
transfer_transfertravelcompany:1
La base habitual no fue modificada.
```

`finalize` es idempotente: se ejecuto dos veces y los conteos no cambiaron.
`PRAGMA foreign_key_check` devolvio una lista vacia.

### 4.2 Ejecucion de la demo

Iniciar el mismo ejecutable con `WorkingDirectory` apuntando a la carpeta
aislada. `Database::open()` usa la ruta relativa `db/grat.db`, por lo que la
aplicacion lee y escribe solo la copia.

### 4.3 Limpieza y repeticion

Para repetir desde cero:

```bash
python tools/demo/prepare_demo_db.py prepare --dir "C:/Users/USER/AppData/Local/Temp/opencode/grat_demo" --force
```

`--force` solo se aplica a la fase `prepare` y solo dentro del directorio
indicado. El script rechaza rutas dentro del repositorio y la carpeta real
`db/`. No se versiona ni la base ni ningun archivo temporal.

Para limpiar, eliminar el directorio aislado; no eliminar nunca
`GRAT_Software/db/grat.db`.

## 5. Verificacion de aislamiento

| Comprobacion | Resultado real |
|---|---|
| SHA256 de `GRAT_Software/db/grat.db` antes de preparar | `E984D59668BC24823D286B1AFEA710ADD7165D8331FF516EA7CB925F596B4A9D` |
| SHA256 despues de `prepare`, primer arranque, `finalize` y repeticion | `E984D59668BC24823D286B1AFEA710ADD7165D8331FF516EA7CB925F596B4A9D` |
| Intento de `prepare --dir .../GRAT_Software/db` | Rechazado con `ERROR: el directorio de demostracion debe estar fuera del repositorio.` |
| Base versionada | No; `db/` y `*.db` permanecen ignorados |

## 6. Seguridad del script

- Exige `--dir` explicito.
- Usa el esquema original del proyecto; no redefine tablas ni restricciones.
- No escribe contraseñas ni hashes.
- No usa credenciales personales.
- Valida que el directorio este fuera del repositorio.
- Requiere `--force` para reemplazar una base aislada existente.
- Informa errores con codigo de salida distinto de cero.
- Permite repetir `finalize` sin duplicar registros.
