#include "validator.h"

#include "core/crud/validation/table_gateway_validation_lookup.h"

#include <QRegularExpression>
#include <QVariant>

Validator::Validator(const TableGateway &gateway)
    : m_ownedLookup(std::make_unique<TableGatewayValidationLookup>(gateway))
    , m_lookup(*m_ownedLookup)
{
}

Validator::Validator(const IValidationLookup &lookup)
    : m_lookup(lookup)
{
}

QList<ValidationError> Validator::validate(const TableSpec &spec, const QVariantMap &values, bool isInsert,
                                          const QVariantMap &currentRecord) const
{
    QList<ValidationError> errors;
    const QString pkCol = QString::fromLatin1(spec.pkColumn);

    for (const auto &c : spec.columns) {
        const QString col = QString::fromLatin1(c.dbColumn);
        const QString label = QString::fromLatin1(c.label);

        // System-managed columns are not validated from operator input.
        if (c.autoManaged)   // created_by_id, created_at, updated_at, last_login
            continue;
        if (c.isState)       // is_active is managed via state commands
            continue;
        if (spec.identityPk && col == pkCol) // identity PK is auto-generated
            continue;

        const bool present = values.contains(col);
        const QString valStr = present ? values.value(col).toString() : QString();
        const bool empty = !present || valStr.isEmpty();

        // --- Required (Req 7.1) ------------------------------------------------
        // Password is required only on insert; on update an empty password means
        // "keep current" and is therefore allowed.
        const bool requiredNow = c.required && !(c.type == ColumnType::Password && !isInsert);
        if (requiredNow) {
            if (isInsert && !present) {
                errors.append({col, QStringLiteral("%1 es obligatorio.").arg(label)});
            } else if (present && valStr.isEmpty()) {
                errors.append({col, QStringLiteral("%1 es obligatorio.").arg(label)});
            }
        }

        if (empty)
            continue; // nothing else to check for an empty field

        // --- Max length (Req 7.3) — text-like columns --------------------------
        if (c.maxLength > 0 && (c.type == ColumnType::Text || c.type == ColumnType::Password)) {
            if (valStr.length() > c.maxLength) {
                errors.append({col, QStringLiteral("%1 supera la longitud maxima de %2 caracteres.")
                                    .arg(label).arg(c.maxLength)});
            }
        }

        // --- Decimal >= 0 (CHECK constraint) ----------------------------------
        if (c.type == ColumnType::Decimal) {
            bool ok = false;
            const double d = values.value(col).toDouble(&ok);
            if (ok && d < 0.0)
                errors.append({col, QStringLiteral("%1 no debe ser negativo.").arg(label)});
        }

        // --- Email format (user_user.email) -----------------------------------
        if (col == QStringLiteral("email")) {
            static const QRegularExpression re(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
            if (!re.match(valStr).hasMatch())
                errors.append({col, QStringLiteral("%1 no es una direccion de correo valida.").arg(label)});
        }
    }

    // --- Natural-PK uniqueness (Req 7.2) — insert only ------------------------
    if (isInsert && !spec.identityPk) {
        const QVariant pkVal = values.value(pkCol);
        if (!pkVal.isNull() && !pkVal.toString().isEmpty()) {
            const auto result = m_lookup.exists(spec, pkVal);
            if (result == ValidationLookupResult::Found)
                errors.append({pkCol, QStringLiteral("Ya existe un registro con este identificador.")});
            else if (result == ValidationLookupResult::QueryError)
                errors.append({pkCol, QStringLiteral("No se pudo verificar el identificador.")});
        }
    }

    if (QLatin1String(spec.table) == QLatin1String("waste_wasteconstituent")) {
        // Updates can contain only changed fields. Recover the rest from the row
        // being edited, and never mistake that same row for a duplicate.
        QVariantMap effectiveValues = isInsert ? QVariantMap() : currentRecord;
        for (auto it = values.cbegin(); it != values.cend(); ++it)
            effectiveValues.insert(it.key(), it.value());
        const QVariant waste = effectiveValues.value(QStringLiteral("id_waste_id"));
        const QVariant constituent = effectiveValues.value(QStringLiteral("id_constituent_id"));
        const QVariant excluded = isInsert ? QVariant() : currentRecord.value(pkCol);
        if (!waste.isNull() && !waste.toString().isEmpty()
            && !constituent.isNull() && !constituent.toString().isEmpty()) {
            const auto result = m_lookup.constituentExists(waste, constituent, excluded);
            if (result == ValidationLookupResult::Found) {
                errors.append({QStringLiteral("id_constituent_id"),
                               QStringLiteral("Este componente ya esta vinculado a este residuo. "
                                              "Modifique o reactive la fila existente.")});
            } else if (result == ValidationLookupResult::QueryError) {
                errors.append({QStringLiteral("id_constituent_id"),
                               QStringLiteral("No se pudo verificar el vinculo del componente.")});
            }
        }
    }

    return errors;
}
