#!/usr/bin/env python3
"""Prepara una base de demostracion aislada para GRAT_Software (P5).

Este script NO toca GRAT_Software/db/grat.db. Solo escribe en el directorio
que el operador indica explicitamente con --dir.

Fases:
  prepare   crea <dir>/db/grat.db ejecutando el esquema original del proyecto.
  finalize  inserta los datos ficticios despues de que la aplicacion haya
            creado los tres usuarios de demostracion en el primer arranque.

No calcula contraseñas ni hashes: los usuarios los crea AuthService dentro de
la aplicacion. Este script solo consulta el id del usuario 'admin'.
"""

from __future__ import annotations

import argparse
import sqlite3
import sys
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve()
REPO_ROOT = SCRIPT_PATH.parents[2]
SCHEMA_PATH = REPO_ROOT / "GRAT_Software" / "resources" / "grat_schema.sqlite.sql"
REAL_DB_PATH = REPO_ROOT / "GRAT_Software" / "db" / "grat.db"
DB_RELATIVE = Path("db") / "grat.db"


def fail(message: str) -> int:
    print(f"ERROR: {message}", file=sys.stderr)
    return 2


def validate_target(raw_dir: str, force: bool, phase: str) -> Path | None:
    target = Path(raw_dir).expanduser().resolve()
    if target == REPO_ROOT or REPO_ROOT in target.parents:
        print(
            "ERROR: el directorio de demostracion debe estar fuera del repositorio.",
            file=sys.stderr,
        )
        return None
    if target == REAL_DB_PATH.parent or target in REAL_DB_PATH.parents:
        print("ERROR: se prohibe escribir en la carpeta db del proyecto.", file=sys.stderr)
        return None

    db_path = target / DB_RELATIVE
    if phase == "prepare" and db_path.exists() and not force:
        print(
            f"ERROR: ya existe {db_path}. Usa --force para reemplazarlo de forma controlada.",
            file=sys.stderr,
        )
        return None
    return target


def connect(db_path: Path) -> sqlite3.Connection:
    connection = sqlite3.connect(db_path)
    connection.execute("PRAGMA foreign_keys = ON")
    return connection


def schema_is_present(connection: sqlite3.Connection) -> bool:
    row = connection.execute(
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'location_country'"
    ).fetchone()
    return row is not None


def do_prepare(target: Path) -> int:
    if not SCHEMA_PATH.is_file():
        return fail(f"No se encontro el esquema original: {SCHEMA_PATH}")

    db_path = target / DB_RELATIVE
    db_path.parent.mkdir(parents=True, exist_ok=True)
    if db_path.exists():
        db_path.unlink()

    schema_sql = SCHEMA_PATH.read_text(encoding="utf-8")
    with connect(db_path) as connection:
        connection.executescript(schema_sql)
        if not schema_is_present(connection):
            return fail("El esquema original no se aplico correctamente.")

    print(f"BASE_CREADA={db_path}")
    print("SIGUIENTE: ejecuta la aplicacion desde esta carpeta y cierra el login una vez.")
    print("Luego ejecuta: prepare_demo_db.py finalize --dir <esta carpeta>")
    return 0


def insert_if_missing(
    connection: sqlite3.Connection,
    sql: str,
    params: tuple,
    check_sql: str,
    check_params: tuple,
) -> None:
    if connection.execute(check_sql, check_params).fetchone() is None:
        connection.execute(sql, params)


def do_finalize(target: Path) -> int:
    db_path = target / DB_RELATIVE
    if not db_path.is_file():
        return fail(f"No existe la base aislada: {db_path}. Ejecuta primero 'prepare'.")

    with connect(db_path) as connection:
        if not schema_is_present(connection):
            return fail("La base indicada no contiene el esquema de GRAT_Software.")

        admin = connection.execute(
            "SELECT id FROM user_user WHERE username = 'admin'"
        ).fetchone()
        if admin is None:
            return fail(
                "Aun no existen usuarios. Inicia la aplicacion una vez desde la carpeta "
                "aislada para que AuthService cree las cuentas, y vuelve a ejecutar finalize."
            )
        admin_id = int(admin[0])

        # Catalogos basicos (IDs 1..n; el esquema los deja vacios).
        insert_if_missing(
            connection,
            "INSERT INTO location_country (alpha_2, alpha_3, numerical_code, name) "
            "VALUES ('PE', 'PER', '604', 'Peru')",
            (),
            "SELECT 1 FROM location_country WHERE alpha_2 = 'PE'",
            (),
        )
        insert_if_missing(
            connection,
            "INSERT INTO location_subdivision "
            "(subdiv_code, country_alpha_2_id, type_subdivision, name) "
            "VALUES ('LIMA', 'PE', 'Departamento', 'Lima')",
            (),
            "SELECT 1 FROM location_subdivision WHERE subdiv_code = 'LIMA'",
            (),
        )

        catalog_rows = [
            ("catalog_constituent", "Plastico"),
            ("catalog_constituent", "Papel"),
            ("catalog_containertype", "Bidon de 200 L"),
            ("catalog_containertype", "Saco de 25 kg"),
            ("catalog_toxicitylevel", "Baja"),
            ("catalog_toxicitylevel", "Media"),
            ("catalog_transporttype", "Terrestre"),
            ("catalog_transporttype", "Maritimo"),
        ]
        for table, name in catalog_rows:
            insert_if_missing(
                connection,
                f"INSERT INTO {table} (name, description) VALUES (?, '')"
                if table != "catalog_constituent"
                else f"INSERT INTO {table} (name) VALUES (?)",
                (name,),
                f"SELECT 1 FROM {table} WHERE name = ?",
                (name,),
            )

        insert_if_missing(
            connection,
            "INSERT INTO waste_generatorcompany "
            "(id_gen_company, name_company, city_id, activity) "
            "VALUES ('GC-DEMO-01', 'Empresa Generadora Demo SAC', 'LIMA', "
            "'Mantenimiento industrial ficticio')",
            (),
            "SELECT 1 FROM waste_generatorcompany WHERE id_gen_company = 'GC-DEMO-01'",
            (),
        )
        insert_if_missing(
            connection,
            "INSERT INTO transfer_destination (name, city_id) "
            "VALUES ('Planta de tratamiento demo', 'LIMA')",
            (),
            "SELECT 1 FROM transfer_destination WHERE name = 'Planta de tratamiento demo'",
            (),
        )
        insert_if_missing(
            connection,
            "INSERT INTO transfer_travelcompany "
            "(id_travel_company, name, city_id) "
            "VALUES ('TT-DEMO-01', 'Transportes Demo SAC', 'LIMA')",
            (),
            "SELECT 1 FROM transfer_travelcompany WHERE id_travel_company = 'TT-DEMO-01'",
            (),
        )

        wastes = [
            ("RS-DEMO-01", 12.5, 1, "A"),
            ("RS-DEMO-02", 3.0, 2, "I"),
        ]
        for waste_id, amount, toxicity, state in wastes:
            insert_if_missing(
                connection,
                "INSERT INTO waste_waste "
                "(id_waste, id_gen_company_id, amount_waste, id_toxicity_id, "
                "created_by_id, is_active) "
                "VALUES (?, 'GC-DEMO-01', ?, ?, ?, ?)",
                (waste_id, amount, toxicity, admin_id, state),
                "SELECT 1 FROM waste_waste WHERE id_waste = ?",
                (waste_id,),
            )

        constituents = [
            ("RS-DEMO-01", 1, 7.5),
            ("RS-DEMO-01", 2, 5.0),
            ("RS-DEMO-02", 1, 3.0),
        ]
        for waste_id, constituent_id, amount in constituents:
            insert_if_missing(
                connection,
                "INSERT INTO waste_wasteconstituent "
                "(id_waste_id, id_constituent_id, amount_constituent) "
                "VALUES (?, ?, ?)",
                (waste_id, constituent_id, amount),
                "SELECT 1 FROM waste_wasteconstituent "
                "WHERE id_waste_id = ? AND id_constituent_id = ?",
                (waste_id, constituent_id),
            )

        insert_if_missing(
            connection,
            "INSERT INTO transfer_transfer "
            "(id_transfer, id_travel_company_id, id_destination_id, id_waste_id, "
            "id_container_type_id, amount_waste, delivery_date, created_by_id) "
            "VALUES ('TR-DEMO-01', 'TT-DEMO-01', 1, 'RS-DEMO-01', 1, 12.5, "
            "'2026-11-20', ?)",
            (admin_id,),
            "SELECT 1 FROM transfer_transfer WHERE id_transfer = 'TR-DEMO-01'",
            (),
        )
        insert_if_missing(
            connection,
            "INSERT INTO transfer_transfertravelcompany "
            "(id_transfer_id, id_travel_company_id, id_type_transport_id, cost, kms) "
            "VALUES ('TR-DEMO-01', 'TT-DEMO-01', 1, 150.0, 42.0)",
            (),
            "SELECT 1 FROM transfer_transfertravelcompany "
            "WHERE id_transfer_id = 'TR-DEMO-01'",
            (),
        )

        counts = {}
        for table in (
            "location_country",
            "location_subdivision",
            "catalog_constituent",
            "catalog_containertype",
            "catalog_toxicitylevel",
            "catalog_transporttype",
            "waste_generatorcompany",
            "waste_waste",
            "waste_wasteconstituent",
            "transfer_destination",
            "transfer_travelcompany",
            "transfer_transfer",
            "transfer_transfertravelcompany",
        ):
            counts[table] = connection.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0]

    print(f"DATOS_LISTOS={db_path}")
    print("CONTEOS=" + ", ".join(f"{k}:{v}" for k, v in counts.items()))
    print("La base habitual no fue modificada.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("prepare", "finalize"))
    parser.add_argument("--dir", required=True, help="Directorio aislado de la demo")
    parser.add_argument(
        "--force",
        action="store_true",
        help="Permite reemplazar una base aislada existente en prepare",
    )
    args = parser.parse_args()

    target = validate_target(args.dir, args.force, args.phase)
    if target is None:
        return 2
    return do_prepare(target) if args.phase == "prepare" else do_finalize(target)


if __name__ == "__main__":
    raise SystemExit(main())
