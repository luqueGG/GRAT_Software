#include "record_validation_rules.h"

#include "core/crud/table_spec.h"
#include "i_validation_lookup.h"

namespace {

class NaturalKeyUniquenessRule final : public IValidationRule
{
public:
    void validate(const ValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        if (context.field || !context.isInsert || context.spec.identityPk)
            return;

        const QString pkColumn = QString::fromLatin1(context.spec.pkColumn);
        const QVariant pk = context.values.value(pkColumn);
        if (pk.isNull() || pk.toString().isEmpty())
            return;

        const auto result = context.lookup.exists(context.spec, pk);
        if (result == ValidationLookupResult::Found)
            errors.append({pkColumn, QStringLiteral("Ya existe un registro con este identificador.")});
        else if (result == ValidationLookupResult::QueryError)
            errors.append({pkColumn, QStringLiteral("No se pudo verificar el identificador.")});
    }
};

class ConstituentUniquenessRule final : public IValidationRule
{
public:
    void validate(const ValidationContext &context,
                  QList<ValidationError> &errors) const override
    {
        if (context.field || QLatin1String(context.spec.table) != QLatin1String("waste_wasteconstituent"))
            return;

        // On partial edits use the original only when a field was not submitted.
        const auto effectiveValue = [&context](const QString &column) {
            const auto it = context.values.constFind(column);
            if (it != context.values.cend())
                return it.value();
            return context.isInsert ? QVariant() : context.currentRecord.value(column);
        };
        const QString constituentColumn = QStringLiteral("id_constituent_id");
        const QVariant waste = effectiveValue(QStringLiteral("id_waste_id"));
        const QVariant constituent = effectiveValue(constituentColumn);
        if (waste.isNull() || waste.toString().isEmpty()
            || constituent.isNull() || constituent.toString().isEmpty())
            return;

        const QVariant excluded = context.isInsert
            ? QVariant() : context.currentRecord.value(QString::fromLatin1(context.spec.pkColumn));
        const auto result = context.lookup.constituentExists(waste, constituent, excluded);
        if (result == ValidationLookupResult::Found) {
            errors.append({constituentColumn,
                           QStringLiteral("Este componente ya esta vinculado a este residuo. "
                                          "Modifique o reactive la fila existente.")});
        } else if (result == ValidationLookupResult::QueryError) {
            errors.append({constituentColumn,
                           QStringLiteral("No se pudo verificar el vinculo del componente.")});
        }
    }
};

} // namespace

std::vector<std::unique_ptr<IValidationRule>> makeRecordValidationRules()
{
    std::vector<std::unique_ptr<IValidationRule>> rules;
    rules.reserve(2);
    rules.push_back(std::make_unique<NaturalKeyUniquenessRule>());
    rules.push_back(std::make_unique<ConstituentUniquenessRule>());
    return rules;
}
