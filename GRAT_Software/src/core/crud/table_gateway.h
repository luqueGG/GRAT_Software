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
#include "core/crud/validation/i_validation_lookup.h"
#include "core/errors/error_handler.h"

struct AuthSession;

// Outcome of a write operation (insert/update/setState).
struct DbResult
{
    bool success = false;
    QString error;   // user-facing message (no internal details — Req 9.2)
    QVariant pk;     // resulting PK on insert (identity → lastInsertId, natural → provided)
    ErrorHandler::Category category = ErrorHandler::Category::Unexpected;
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
    QList<QVariantMap> selectAll(const TableSpec &spec, QString *errorMessage = nullptr) const;
    // The fixed permission definitions, with assignment status for the selected role.
    QList<QVariantMap> rolePermissionCatalog(const QVariant &roleKey,
                                           QString *errorMessage = nullptr) const;
    // Rows of one saved parent, including every lifecycle state, ordered by id.
    QList<QVariantMap> selectChildren(const ChildTableSpec &child, const QVariant &parentKey,
                                     QString *errorMessage = nullptr) const;
    // All {pk, label} pairs of a referenced table (all states — Req 10.2) for
    // ComboBoxes. When the list cannot be loaded, errorMessage receives a safe
    // Reference/Connection message and the caller keeps the ComboBox disabled.
    QList<RefValue> loadRefValues(const QString &refTable, const QString &refPk,
                                   const QString &refDisplay,
                                   QString *errorMessage = nullptr) const;
    // Look up a row by PK for duplicate-PK validation (Req 7.2).
    ValidationLookupResult exists(const TableSpec &spec, const QVariant &pk) const;
    // A constituent may occur only once per waste, including inactive/deleted rows.
    // Exclude the edited row so changing its amount does not count as a duplicate.
    ValidationLookupResult constituentExists(const QVariant &wasteKey, const QVariant &constituentKey,
                                              const QVariant &excludePk = {}) const;

    // Write operations ------------------------------------------------------
    // Explicitly authorized role-permission writes; definitions are never modified.
    DbResult grantRolePermission(const AuthSession &session, const QVariant &roleKey,
                                 const QVariant &permissionKey) const;
    DbResult revokeRolePermission(const AuthSession &session, const QVariant &roleKey,
                                  const QVariant &linkKey) const;
    // Insert the provided column values (identity PK auto-generated; is_active/timestamp
    // defaults applied by the DB). Runs in a transaction.
    DbResult insert(const TableSpec &spec, const QVariantMap &values) const;
    // Update the non-PK, non-state columns of the row identified by pk. Runs in a transaction.
    DbResult update(const TableSpec &spec, const QVariant &pk, const QVariantMap &values) const;
    // Logical state transition: Delete('*'), Inactivate('I'), Reactivate('A') (Req 4).
    DbResult setState(const TableSpec &spec, const QVariant &pk, const QString &newState) const;
    // Physical delete, only for link tables without an is_active lifecycle
    // (design §4 / §7.4). Managed tables are never physically deleted.
    DbResult remove(const TableSpec &spec, const QVariant &pk) const;

    // Child writes always use the selected parent, never a parent supplied by the form.
    DbResult insertChild(const ChildTableSpec &child, const QVariant &parentKey,
                         const QVariantMap &values) const;
    DbResult updateChild(const ChildTableSpec &child, const QVariant &parentKey,
                         const QVariant &pk, const QVariantMap &values) const;
    DbResult removeChild(const ChildTableSpec &child, const QVariant &parentKey,
                         const QVariant &pk) const;
    DbResult setChildState(const ChildTableSpec &child, const QVariant &parentKey,
                           const QVariant &pk, const QString &newState) const;

private:
    DbResult writeRolePermission(const AuthSession &session, const QVariant &roleKey,
                                 const QVariant &key, bool grant) const;
    static QString genericError();
    // An empty assignment list means physical deletion; otherwise update these values.
    DbResult writeChild(const ChildTableSpec &child, const QVariant &parentKey,
                        const QVariant &pk, const QVariantMap &assignments) const;

    // The handle is shared/shallow; marking mutable lets const methods drive
    // transactions without changing the gateway's own state.
    mutable QSqlDatabase m_db;
};
