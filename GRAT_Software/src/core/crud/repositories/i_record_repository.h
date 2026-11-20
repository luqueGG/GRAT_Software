#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

#include "app/session.h"

// Application results: no SQL handles, queries or driver errors cross this contract.
struct RecordListResult
{
    bool success = false;
    QString error;
    QList<QVariantMap> records;
};

struct RecordWriteResult
{
    bool success = false;
    QString error;
    QVariant pk;
};

// A collection of records bound to one table/association by its implementation.
// parentKey scopes a child collection; it is unused for a standalone collection.
// Permission links are deliberately changed through the authorized methods only.
class IRecordRepository
{
public:
    virtual ~IRecordRepository() = default;

    virtual RecordListResult load(const QVariant &parentKey = {}) const = 0;
    virtual RecordWriteResult add(const QVariantMap &values, const QVariant &parentKey = {}) const = 0;
    virtual RecordWriteResult change(const QVariant &pk, const QVariantMap &values,
                                     const QVariant &parentKey = {}) const = 0;
    virtual RecordWriteResult setState(const QVariant &pk, const QString &state,
                                       const QVariant &parentKey = {}) const = 0;
    virtual RecordWriteResult remove(const QVariant &pk, const QVariant &parentKey = {}) const = 0;
    virtual RecordWriteResult grantPermission(const AuthSession &session, const QVariant &roleKey,
                                              const QVariant &permissionKey) const = 0;
    virtual RecordWriteResult revokePermission(const AuthSession &session, const QVariant &roleKey,
                                               const QVariant &linkKey) const = 0;
};
