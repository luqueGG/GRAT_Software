// table_gateway.h — generic data-access layer for the metadata-driven CRUD engine
// (design §8). One class parameterised by TableSpec provides all read/write
// operations shared by every screen. Writes run inside a transaction and surface a
// generic, non-leaking error (Req 9.2–9.3).

#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "core/crud/table_spec.h"

// Outcome of a write operation (insert/update/setState).
struct DbResult
{
    bool success = false;
    QString error;   // user-facing message (no internal details — Req 9.2)
    QVariant pk;     // resulting PK on insert (identity → lastInsertId, natural → provided)
};

// A selectable FK reference value for ComboBox population (Req 10).
struct RefValue
{
    QVariant pk;     // value stored as the FK
    QString text;    // ComboBox display text ("id - label" or just id)
};

class TableGateway
{
public:
    explicit TableGateway(QSqlDatabase db);

    // Read operations -------------------------------------------------------
    // All rows for the grid, regardless of is_active state (Req 2.1), ordered by PK.
    QList<QVariantMap> selectAll(const TableSpec &spec) const;
    // All {pk, label} pairs of a referenced table (all states — Req 10.2) for ComboBoxes.
    QList<RefValue> loadRefValues(const QString &refTable, const QString &refPk,
                                   const QString &refDisplay) const;
    // True if a row with the given PK exists (duplicate-PK validation — Req 7.2).
    bool exists(const TableSpec &spec, const QVariant &pk) const;

    // Write operations ------------------------------------------------------
    // Insert the provided column values (identity PK auto-generated; is_active/timestamp
    // defaults applied by the DB). Runs in a transaction.
    DbResult insert(const TableSpec &spec, const QVariantMap &values) const;
    // Update the non-PK, non-state columns of the row identified by pk. Runs in a transaction.
    DbResult update(const TableSpec &spec, const QVariant &pk, const QVariantMap &values) const;
    // Logical state transition: Delete('*'), Inactivate('I'), Reactivate('A') (Req 4).
    DbResult setState(const TableSpec &spec, const QVariant &pk, const QString &newState) const;

private:
    static QString genericError();

    // The handle is shared/shallow; marking mutable lets const methods drive
    // transactions without changing the gateway's own state.
    mutable QSqlDatabase m_db;
};
