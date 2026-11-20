#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

struct ColumnSpec;
struct TableSpec;
class IValidationLookup;

// A single validation problem on a single column.
struct ValidationError
{
    QString column;  // SQL column name
    QString reason;  // user-facing message
};

// value is null when the field was not submitted.
struct ValidationField
{
    const ColumnSpec &column;
    QString columnName;
    const QVariant *value;
    QString text;
};

// Read-only inputs valid only during the call. field is null for record rules.
struct ValidationContext
{
    const TableSpec &spec;
    const QVariantMap &values;
    bool isInsert;
    const QVariantMap &currentRecord;
    const IValidationLookup &lookup;
    const ValidationField *field = nullptr;
};

class IValidationRule
{
public:
    virtual ~IValidationRule() = default;

    // Append errors without changing the inputs or writing data.
    virtual void validate(const ValidationContext &context,
                          QList<ValidationError> &errors) const = 0;
};
