// validator.h — metadata-driven field validation (design §10, Req 7).
//
// Validates the values the operator entered for a table against its TableSpec.
// All invalid fields are collected and reported together (Req 7.4); the validator
// performs only read-only checks (it never writes to the DB — Req 7.5).

#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

#include <memory>

#include "core/crud/table_spec.h"
#include "core/crud/validation/i_validation_lookup.h"

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
    // Owns an adapter that borrows gateway; gateway must outlive this Validator.
    explicit Validator(const TableGateway &gateway);
    Validator(const TableGateway &&) = delete;

    // Borrows an existing lookup, which must outlive this Validator.
    explicit Validator(const IValidationLookup &lookup);
    Validator(const IValidationLookup &&) = delete;

    // Keep the lookup binding fixed, including when it refers to the owned adapter.
    Validator(const Validator &) = delete;
    Validator &operator=(const Validator &) = delete;
    Validator(Validator &&) = delete;
    Validator &operator=(Validator &&) = delete;

    // Validate the provided values for a table.
    // isInsert=true  → required fields must be present; natural-PK uniqueness checked.
    // isInsert=false → only the columns present in values are validated (others unchanged).
    // currentRecord supplies unchanged fields and the ID to exclude during child edits.
    QList<ValidationError> validate(const TableSpec &spec, const QVariantMap &values, bool isInsert,
                                   const QVariantMap &currentRecord = {}) const;

private:
    std::unique_ptr<const IValidationLookup> m_ownedLookup;
    const IValidationLookup &m_lookup;
};
