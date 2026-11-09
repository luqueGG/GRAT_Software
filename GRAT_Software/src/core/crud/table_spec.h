// table_spec.h — schema metadata model for the metadata-driven CRUD engine
// (design §6). Each managed table is described by a TableSpec consumed by the
// generic CrudWidget / RegisterForm / Validator / TableGateway.
//
// The metadata encodes the validation rules (Req 7) and the FK→ComboBox rendering
// (Req 10), so all screens share one implementation.

#pragma once

#include <QString>
#include <vector>

// Field rendering/data type for a column.
enum class ColumnType {
    Text,       // VARCHAR → QLineEdit, length-validated
    Integer,    // integer → read-only for identity PKs
    Decimal,    // DECIMAL → numeric input, >= 0, 2-decimal display
    Date,       // DATE → QDateEdit (ISO YYYY-MM-DD)
    DateTime,   // TIMESTAMP → QDateTimeEdit (ISO-8601 UTC)
    Boolean,    // BOOLEAN flag → QCheckBox
    State,      // is_active A/I/* → read-only badge, changed via commands
    Fk,         // foreign key → QComboBox (Req 10)
    Password,   // hashed password → QLineEdit echo mode Password
};

// Description of a single column of a managed table.
struct ColumnSpec
{
    const char *dbColumn = nullptr;   // SQL column name
    const char *label = nullptr;      // English display label
    ColumnType type = ColumnType::Text;
    int maxLength = -1;               // VARCHAR limit for app-side validation (Req 7.3); -1 = unlimited
    bool required = true;             // NOT NULL → must provide (Req 7.1)
    bool editable = true;             // identity PK / auto fields → false
    bool autoManaged = false;          // created_at/updated_at/created_by_id/last_login (read-only/auto)
    bool isState = false;              // marks the is_active column
    // FK only:
    const char *refTable = nullptr;   // referenced table
    const char *refPk = nullptr;      // referenced PK column
    const char *refDisplay = nullptr; // column shown in ComboBox; null → show PK only
};

// A child (link/association) table rendered as master-detail under a parent.
struct ChildTableSpec
{
    const char *table = nullptr;       // child SQL table name
    const char *fkToParent = nullptr;  // child column referencing the parent's PK (auto-filled)
    const char *appLabel = nullptr;    // RBAC app_label
    const char *model = nullptr;       // RBAC model
    bool hasState = false;             // child owns an is_active lifecycle
    std::vector<ColumnSpec> columns;
};

// Description of a managed table.
struct TableSpec
{
    const char *table = nullptr;    // SQL table name
    const char *module = nullptr;   // location/catalog/user/waste/transfer/admin
    const char *pkColumn = nullptr; // primary key column
    const char *appLabel = nullptr;  // RBAC app_label (e.g. "waste")
    const char *model = nullptr;     // RBAC model (e.g. "waste")
    bool identityPk = false;         // true → PK auto-generated, read-only in form
    std::vector<ColumnSpec> columns;
    std::vector<ChildTableSpec> children;
};

// Registry of all managed tables (12 standalone + the admin 'auth_group' role
// screen per design §5.3). Returns a reference to a static collection.
const std::vector<TableSpec> &allTableSpecs();

// Look up a table's spec by SQL table name; nullptr if not found.
const TableSpec *findTableSpec(const QString &tableName);
