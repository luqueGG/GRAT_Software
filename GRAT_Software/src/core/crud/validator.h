// validator.h — metadata-driven field validation (design §10, Req 7).
//
// Validates the values the operator entered for a table against its TableSpec.
// All invalid fields are collected and reported together (Req 7.4); the validator
// performs only read-only checks (it never writes to the DB — Req 7.5).

#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

#include "core/crud/table_spec.h"

class TableGateway;

// A single validation problem on a single column.
struct ValidationError
{
    QString column;  // SQL column name
    QString reason;  // user-facing message
};

class Validator
{
public:
    explicit Validator(const TableGateway &gateway);

    // Validate the provided values for a table.
    // isInsert=true  → required fields must be present; natural-PK uniqueness checked.
    // isInsert=false → only the columns present in values are validated (others unchanged).
    // currentRecord supplies unchanged fields and the ID to exclude during child edits.
    QList<ValidationError> validate(const TableSpec &spec, const QVariantMap &values, bool isInsert,
                                   const QVariantMap &currentRecord = {}) const;

private:
    const TableGateway &m_gateway;
};
