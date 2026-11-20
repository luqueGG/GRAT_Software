// test_doubles.h — in-memory substitutes for the injectable contracts (P3).
//
// FakeValidationLookup replaces IValidationLookup (P2) and FakeRecordRepository
// replaces IRecordRepository (P1), so validator and controller tests run without
// SQLite and can assert exactly which queries/writes were requested.

#pragma once

#include <QList>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include "core/crud/repositories/i_record_repository.h"
#include "core/crud/table_spec.h"
#include "core/crud/validation/i_validation_lookup.h"

namespace grat_test {

// A child spec seen as a table with identity 'id', as the UI and controllers use it.
inline TableSpec childAsTable(const ChildTableSpec &child)
{
    TableSpec spec;
    spec.table = child.table;
    spec.pkColumn = "id";
    spec.appLabel = child.appLabel;
    spec.model = child.model;
    spec.identityPk = true;
    spec.columns.push_back(ColumnSpec{.dbColumn = "id", .label = "ID",
                                      .type = ColumnType::Integer, .editable = false});
    spec.columns.insert(spec.columns.end(), child.columns.begin(), child.columns.end());
    return spec;
}

inline const ChildTableSpec *findChildSpec(const TableSpec &parent, const char *table)
{
    for (const auto &child : parent.children) {
        if (QLatin1String(child.table) == QLatin1String(table))
            return &child;
    }
    return nullptr;
}

class FakeValidationLookup final : public IValidationLookup
{
public:
    struct PairRow
    {
        QString waste;
        QString constituent;
        QString id;
    };

    // Natural keys that already exist, as "<table>|<pk>".
    QSet<QString> existingKeys;
    // Existing waste–constituent links in any lifecycle state.
    QList<PairRow> pairs;
    bool failQueries = false;

    mutable int existsCalls = 0;
    mutable int pairCalls = 0;
    mutable QVariant lastExcluded;

    ValidationLookupResult exists(const TableSpec &spec, const QVariant &pk) const override
    {
        ++existsCalls;
        if (failQueries)
            return ValidationLookupResult::QueryError;
        return existingKeys.contains(QString::fromLatin1(spec.table) + QLatin1Char('|') + pk.toString())
            ? ValidationLookupResult::Found
            : ValidationLookupResult::NotFound;
    }

    ValidationLookupResult constituentExists(const QVariant &wasteKey, const QVariant &constituentKey,
                                             const QVariant &excludePk) const override
    {
        ++pairCalls;
        lastExcluded = excludePk;
        if (failQueries)
            return ValidationLookupResult::QueryError;
        for (const PairRow &row : pairs) {
            if (row.waste == wasteKey.toString() && row.constituent == constituentKey.toString()
                && (excludePk.isNull() || row.id != excludePk.toString()))
                return ValidationLookupResult::Found;
        }
        return ValidationLookupResult::NotFound;
    }
};

class FakeRecordRepository final : public IRecordRepository
{
public:
    struct Call
    {
        QString operation;   // load/add/change/setState/remove/grant/revoke
        QVariant pk;
        QVariantMap values;
        QVariant parentKey;
        QString state;
    };

    QList<QVariantMap> rows;
    bool failLoad = false;
    bool failWrites = false;
    QString failMessage = QStringLiteral("No se pudo completar la operacion.");
    QVariant nextPk = 100;

    mutable QList<Call> calls;

    int writeCount() const
    {
        int n = 0;
        for (const Call &call : calls) {
            if (call.operation != QLatin1String("load"))
                ++n;
        }
        return n;
    }

    RecordListResult load(const QVariant &parentKey = {}) const override
    {
        calls.append({QStringLiteral("load"), {}, {}, parentKey, {}});
        if (failLoad)
            return {false, failMessage, {}};
        return {true, {}, rows};
    }

    RecordWriteResult add(const QVariantMap &values, const QVariant &parentKey = {}) const override
    {
        calls.append({QStringLiteral("add"), {}, values, parentKey, {}});
        return write(nextPk);
    }

    RecordWriteResult change(const QVariant &pk, const QVariantMap &values,
                             const QVariant &parentKey = {}) const override
    {
        calls.append({QStringLiteral("change"), pk, values, parentKey, {}});
        return write(pk);
    }

    RecordWriteResult setState(const QVariant &pk, const QString &state,
                               const QVariant &parentKey = {}) const override
    {
        calls.append({QStringLiteral("setState"), pk, {}, parentKey, state});
        return write(pk);
    }

    RecordWriteResult remove(const QVariant &pk, const QVariant &parentKey = {}) const override
    {
        calls.append({QStringLiteral("remove"), pk, {}, parentKey, {}});
        return write(pk);
    }

    RecordWriteResult grantPermission(const AuthSession &, const QVariant &roleKey,
                                      const QVariant &permissionKey) const override
    {
        calls.append({QStringLiteral("grant"), permissionKey, {}, roleKey, {}});
        return write(permissionKey);
    }

    RecordWriteResult revokePermission(const AuthSession &, const QVariant &roleKey,
                                       const QVariant &linkKey) const override
    {
        calls.append({QStringLiteral("revoke"), linkKey, {}, roleKey, {}});
        return write(linkKey);
    }

private:
    RecordWriteResult write(const QVariant &pk) const
    {
        if (failWrites)
            return {false, failMessage, {}};
        return {true, {}, pk};
    }
};

} // namespace grat_test
