#include "field_validation_rules.h"

#include <QRegularExpression>

#include "core/crud/table_spec.h"

namespace {

class RequiredRule final : public IValidationRule
{
public:
    void validate(const ValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        if (!context.field)
            return;
        const auto &field = *context.field;
        // An empty password on update means "keep current".
        if (!field.column.required
            || (field.column.type == ColumnType::Password && !context.isInsert))
            return;

        if (field.text.isEmpty() && (context.isInsert || field.value)) {
            errors.append({field.columnName,
                           QStringLiteral("%1 es obligatorio.")
                               .arg(QString::fromLatin1(field.column.label))});
        }
    }
};

class MaxLengthRule final : public IValidationRule
{
public:
    void validate(const ValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        if (!context.field)
            return;
        const auto &field = *context.field;
        const auto &column = field.column;
        if (column.maxLength > 0 && (column.type == ColumnType::Text || column.type == ColumnType::Password)
            && field.text.length() > column.maxLength) {
            errors.append({field.columnName,
                           QStringLiteral("%1 supera la longitud maxima de %2 caracteres.")
                               .arg(QString::fromLatin1(column.label)).arg(column.maxLength)});
        }
    }
};

class NonNegativeDecimalRule final : public IValidationRule
{
public:
    void validate(const ValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        if (!context.field)
            return;
        const auto &field = *context.field;
        if (field.column.type != ColumnType::Decimal || !field.value || field.text.isEmpty())
            return;

        bool ok = false;
        const double value = field.value->toDouble(&ok);
        if (ok && value < 0.0) {
            errors.append({field.columnName,
                           QStringLiteral("%1 no debe ser negativo.")
                               .arg(QString::fromLatin1(field.column.label))});
        }
    }
};

class EmailRule final : public IValidationRule
{
public:
    void validate(const ValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        if (!context.field)
            return;
        const auto &field = *context.field;
        if (field.columnName != QStringLiteral("email") || field.text.isEmpty())
            return;

        static const QRegularExpression re(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
        if (!re.match(field.text).hasMatch()) {
            errors.append({field.columnName,
                           QStringLiteral("%1 no es una direccion de correo valida.")
                               .arg(QString::fromLatin1(field.column.label))});
        }
    }
};

} // namespace

std::vector<std::unique_ptr<IValidationRule>> makeFieldValidationRules()
{
    std::vector<std::unique_ptr<IValidationRule>> rules;
    rules.reserve(4);
    rules.push_back(std::make_unique<RequiredRule>());
    rules.push_back(std::make_unique<MaxLengthRule>());
    rules.push_back(std::make_unique<NonNegativeDecimalRule>());
    rules.push_back(std::make_unique<EmailRule>());
    return rules;
}
