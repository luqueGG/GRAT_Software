#include "sql_record_repository.h"

#include "core/crud/table_gateway.h"

namespace {
RecordWriteResult fromDb(const DbResult &result)
{
    return {result.success, result.error, result.pk};
}

RecordWriteResult denied()
{
    return {false, QStringLiteral("Esta operacion no esta permitida para esta coleccion."), {}};
}
} // namespace

SqlRecordRepository::SqlRecordRepository(TableGateway &gateway, const TableSpec &spec)
    : m_gateway(gateway), m_spec(spec)
{
}

SqlRecordRepository::SqlRecordRepository(TableGateway &gateway, const TableSpec &spec,
                                         const ChildTableSpec &child)
    : m_gateway(gateway), m_spec(spec), m_child(child)
{
}

bool SqlRecordRepository::isRolePermissions() const
{
    return QLatin1String(m_spec.table) == QLatin1String("auth_group_permissions");
}

RecordListResult SqlRecordRepository::load(const QVariant &parentKey) const
{
    RecordListResult result;
    result.records = m_child ? m_gateway.selectChildren(*m_child, parentKey, &result.error)
                             : m_gateway.selectAll(m_spec, &result.error);
    result.success = result.error.isEmpty();
    return result;
}

RecordWriteResult SqlRecordRepository::add(const QVariantMap &values, const QVariant &parentKey) const
{
    if (isRolePermissions())
        return denied();
    return fromDb(m_child ? m_gateway.insertChild(*m_child, parentKey, values)
                          : m_gateway.insert(m_spec, values));
}

RecordWriteResult SqlRecordRepository::change(const QVariant &pk, const QVariantMap &values,
                                              const QVariant &parentKey) const
{
    if (isRolePermissions())
        return denied();
    return fromDb(m_child ? m_gateway.updateChild(*m_child, parentKey, pk, values)
                          : m_gateway.update(m_spec, pk, values));
}

RecordWriteResult SqlRecordRepository::setState(const QVariant &pk, const QString &state,
                                                const QVariant &parentKey) const
{
    if (isRolePermissions())
        return denied();
    return fromDb(m_child ? m_gateway.setChildState(*m_child, parentKey, pk, state)
                          : m_gateway.setState(m_spec, pk, state));
}

RecordWriteResult SqlRecordRepository::remove(const QVariant &pk, const QVariant &parentKey) const
{
    // Standalone records retain their lifecycle; only child links can be removed.
    if (!m_child || isRolePermissions())
        return denied();
    return fromDb(m_gateway.removeChild(*m_child, parentKey, pk));
}

RecordWriteResult SqlRecordRepository::grantPermission(const AuthSession &session,
                                                       const QVariant &roleKey,
                                                       const QVariant &permissionKey) const
{
    if (!m_child || !isRolePermissions())
        return denied();
    return fromDb(m_gateway.grantRolePermission(session, roleKey, permissionKey));
}

RecordWriteResult SqlRecordRepository::revokePermission(const AuthSession &session,
                                                        const QVariant &roleKey,
                                                        const QVariant &linkKey) const
{
    if (!m_child || !isRolePermissions())
        return denied();
    return fromDb(m_gateway.revokeRolePermission(session, roleKey, linkKey));
}
