#include "field_validation_rules.h"

#include <QRegularExpression>

#include "core/crud/table_spec.h"

namespace {

class RequiredRule final : public IValidationRule
{
public:
    void validate(const FieldValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        // An empty password on update means "keep current".
        if (!context.column.required
            || (context.column.type == ColumnType::Password && !context.isInsert))
            return;

        if (context.text.isEmpty() && (context.isInsert || context.value)) {
            errors.append({context.columnName,
                           QStringLiteral("%1 es obligatorio.")
                               .arg(QString::fromLatin1(context.column.label))});
        }
    }
};

class MaxLengthRule final : public IValidationRule
{
public:
    void validate(const FieldValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        const auto &column = context.column;
        if (column.maxLength > 0 && (column.type == ColumnType::Text || column.type == ColumnType::Password)
            && context.text.length() > column.maxLength) {
            errors.append({context.columnName,
                           QStringLiteral("%1 supera la longitud maxima de %2 caracteres.")
                               .arg(QString::fromLatin1(column.label)).arg(column.maxLength)});
        }
    }
};

class NonNegativeDecimalRule final : public IValidationRule
{
public:
    void validate(const FieldValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        if (context.column.type != ColumnType::Decimal || !context.value || context.text.isEmpty())
            return;

        bool ok = false;
        const double value = context.value->toDouble(&ok);
        if (ok && value < 0.0) {
            errors.append({context.columnName,
                           QStringLiteral("%1 no debe ser negativo.")
                               .arg(QString::fromLatin1(context.column.label))});
        }
    }
};

class EmailRule final : public IValidationRule
{
public:
    void validate(const FieldValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        if (context.columnName != QStringLiteral("email") || context.text.isEmpty())
            return;

        static const QRegularExpression re(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
        if (!re.match(context.text).hasMatch()) {
            errors.append({context.columnName,
                           QStringLiteral("%1 no es una direccion de correo valida.")
                               .arg(QString::fromLatin1(context.column.label))});
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
