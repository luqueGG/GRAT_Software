#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

struct ColumnSpec;
struct TableSpec;

// A single validation problem on a single column.
struct ValidationError
{
    QString column;  // SQL column name
    QString reason;  // user-facing message
};

// Read-only inputs for one field. value is null when the field was not submitted.
// The context and its references are only valid during the validation call.
struct FieldValidationContext
{
    const TableSpec &spec;
    const ColumnSpec &column;
    const QVariantMap &values;
    bool isInsert;
    const QVariantMap &currentRecord;
    QString columnName;
    const QVariant *value;
    QString text;
};

class IValidationRule
{
public:
    virtual ~IValidationRule() = default;

    // Append errors without changing the inputs or writing data.
    virtual void validate(const FieldValidationContext &context,
                          QList<ValidationError> &errors) const = 0;
};
