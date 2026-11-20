#pragma once

#include <optional>

#include "core/crud/repositories/i_record_repository.h"
#include "core/crud/table_spec.h"

class TableGateway;

// Bound collection adapter. The gateway owns SQL/transactions; this class maps
// application operations to the proper standalone, child or authorized RBAC path.
class SqlRecordRepository final : public IRecordRepository
{
public:
    SqlRecordRepository(TableGateway &gateway, const TableSpec &spec);
    SqlRecordRepository(TableGateway &gateway, const TableSpec &spec, const ChildTableSpec &child);

    RecordListResult load(const QVariant &parentKey = {}) const override;
    RecordWriteResult add(const QVariantMap &values, const QVariant &parentKey = {}) const override;
    RecordWriteResult change(const QVariant &pk, const QVariantMap &values,
                             const QVariant &parentKey = {}) const override;
    RecordWriteResult setState(const QVariant &pk, const QString &state,
                               const QVariant &parentKey = {}) const override;
    RecordWriteResult remove(const QVariant &pk, const QVariant &parentKey = {}) const override;
    RecordWriteResult grantPermission(const AuthSession &session, const QVariant &roleKey,
                                      const QVariant &permissionKey) const override;
    RecordWriteResult revokePermission(const AuthSession &session, const QVariant &roleKey,
                                       const QVariant &linkKey) const override;

private:
    bool isRolePermissions() const;

    TableGateway &m_gateway;
    TableSpec m_spec;
    std::optional<ChildTableSpec> m_child;
};
