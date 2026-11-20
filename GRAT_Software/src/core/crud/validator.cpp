#include "validator.h"

#include "core/crud/validation/field_validation_rules.h"
#include "core/crud/validation/record_validation_rules.h"
#include "core/crud/validation/table_gateway_validation_lookup.h"

#include <utility>

ValidationRuleSet Validator::defaultRules()
{
    return {makeFieldValidationRules(), makeRecordValidationRules()};
}

Validator::Validator(const TableGateway &gateway, ValidationRuleSet rules)
    : m_ownedLookup(std::make_unique<TableGatewayValidationLookup>(gateway))
    , m_lookup(*m_ownedLookup)
    , m_rules(std::move(rules))
{
}

Validator::Validator(const IValidationLookup &lookup, ValidationRuleSet rules)
    : m_lookup(lookup)
    , m_rules(std::move(rules))
{
}

QList<ValidationError> Validator::validate(const TableSpec &spec, const QVariantMap &values, bool isInsert,
                                          const QVariantMap &currentRecord) const
{
    QList<ValidationError> errors;
    const QString pkCol = QString::fromLatin1(spec.pkColumn);
    const ValidationContext context{spec, values, isInsert, currentRecord, m_lookup};

    for (const auto &c : spec.columns) {
        const QString col = QString::fromLatin1(c.dbColumn);

        // System-managed columns are not validated from operator input.
        if (c.autoManaged)   // created_by_id, created_at, updated_at, last_login
            continue;
        if (c.isState)       // is_active is managed via state commands
            continue;
        if (spec.identityPk && col == pkCol) // identity PK is auto-generated
            continue;

        const auto it = values.constFind(col);
        const QVariant *value = it == values.cend() ? nullptr : &it.value();
        const ValidationField field{c, col, value, value ? value->toString() : QString()};
        auto fieldContext = context;
        fieldContext.field = &field;
        for (const auto &rule : m_rules.fieldRules)
            rule->validate(fieldContext, errors);
    }

    for (const auto &rule : m_rules.recordRules)
        rule->validate(context, errors);

    return errors;
}
